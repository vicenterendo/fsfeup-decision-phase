#include "r2d.h"
#include "ats.h"
#include "context.h"

namespace r2d {

void R2D::set_stage(SessionStage stage) { current_session.stage = stage; }

ActivationError R2D::do_startup_checks(Context &ctx) {
  if (!ats::ATS::is_active(ctx.ats_state.stage())) {
    return ActivationError::AtsNotActive;
  }
  return ActivationError::None;
}

void R2D::tick_waiting_for_pedals(Context &ctx) {
  if (ctx.is_throttle_idle() && ctx.brake_pedal_pct() > 80 &&
      ctx.brake_bar() > 60) {
    set_stage(SessionStage::Active);
  }
}

void R2D::tick_activation_checks(Context &ctx) {
  ActivationError startup_status = do_startup_checks(ctx);
  if (startup_status != ActivationError::None) {
    set_stage(SessionStage::ActivationFailed);
    current_session.activation_error = startup_status;
  }
}

void R2D::tick_active(Context &ctx) {
  // if (!ctx.sdc_closed()) {
  //   deactivate(DeactivationReason::SdcNotClosed, ctx);
  // } else
  if (!ats::ATS::is_active(ctx.ats_state.stage())) {
    deactivate(DeactivationReason::AtsNotActive, ctx);
  }
}

void R2D::tick_check_activation_timeout(Context &) {
  if (is_activating()) {
    long elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - current_session.activating_since)
            .count();
    if (elapsed_ms > ACTIVATION_TIMEOUT_MS) {
      current_session.activation_error = ActivationError::TimedOut;
      set_stage(SessionStage::ActivationFailed);
    }
  }
}

void R2D::tick(Context &ctx) {
  tick_check_activation_timeout(ctx);

  SessionStage initial_stage;
  do {
    if (is_activating()) {
      tick_activation_checks(ctx);
    }
    initial_stage = current_session.stage;
    switch (initial_stage) {
    case SessionStage::Inactive:
      break;
    case SessionStage::Deactivated:
      break;
    case SessionStage::ActivationFailed:
      break;
    case SessionStage::WaitingForPedals:
      tick_waiting_for_pedals(ctx);
      break;
    case SessionStage::Active:
      tick_active(ctx);
      break;
    }
  } while (current_session.stage != initial_stage);
}

void R2D::begin_activation(Context &) {
  current_session = r2d::Session{};
  current_session.activating_since = std::chrono::steady_clock::now();
  set_stage(SessionStage::WaitingForPedals);
}
void R2D::cancel_activation(Context &) {
  set_stage(SessionStage::ActivationFailed);
  current_session.activation_error = ActivationError::User;
}

void R2D::deactivate(DeactivationReason reason, Context &) {
  current_session.deactivation_reason = reason;
  set_stage(SessionStage::Deactivated);
}

} // namespace r2d
