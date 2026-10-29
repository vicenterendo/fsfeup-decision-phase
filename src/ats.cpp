#include "ats.h"
#include "context.h"

namespace ats {

void ATS::transition_stage(SessionStage stage, Context &ctx) {
  if (is_idle(stage)) {
    ctx.set_vcu_sdc_relay(false);
  }
  current_session.stage = stage;
}

void ATS::tick_sdc_other_relay_checks(Context &ctx) {
  bool ok = false;
  if (!ctx.sdc_dash_closed()) {
    current_session.activation_error = ActivationError::SdcDashNotClosed;
  } else if (!ctx.sdc_bspd_closed()) {
    current_session.activation_error = ActivationError::SdcBspdNotClosed;
  } else if (!ctx.sdc_master_panel_closed()) {
    current_session.activation_error = ActivationError::SdcMasterPanelNotClosed;
  } else if (!ctx.sdc_bms_closed()) {
    current_session.activation_error = ActivationError::SdcBmsNotClosed;
  } else {
    ok = true;
  }

  if (ok) {
    transition_stage(SessionStage::SdcVcuRelayClose, ctx);
  } else {
    transition_stage(SessionStage::ActivationFailed, ctx);
  }
}

void ATS::tick_sdc_vcu_relay_close(Context &ctx) {
  ctx.set_vcu_sdc_relay(true);
  transition_stage(SessionStage::WaitingForSdcClosed, ctx);
}

void ATS::tick_waiting_for_sdc_closed(Context &ctx) {
  if (ctx.sdc_closed()) {
    transition_stage(SessionStage::Active, ctx);
  }
}

void ATS::tick_active(Context &ctx) {
  if (!ctx.sdc_closed()) {
    deactivate(DeactivationReason::SdcNotClosed, ctx);
  }
}

void ATS::tick(Context &ctx) {
  tick_check_activation_timeout(ctx);

  if (current_session.stage == SessionStage::SdcOtherRelayChecks) {
    tick_sdc_other_relay_checks(ctx);
  }
  if (current_session.stage == SessionStage::SdcVcuRelayClose) {
    tick_sdc_vcu_relay_close(ctx);
  }
  if (current_session.stage == SessionStage::WaitingForSdcClosed) {
    tick_waiting_for_sdc_closed(ctx);
  }

  if (current_session.stage == SessionStage::Active) {
    tick_active(ctx);
  }
}

void ATS::tick_check_activation_timeout(Context &ctx) {
  if (is_activating(current_session.stage)) {
    long elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - current_session.activating_since)
            .count();
    if (elapsed_ms > ACTIVATION_TIMEOUT_MS) {
      current_session.activation_error = ActivationError::TimedOut;
      transition_stage(SessionStage::ActivationFailed, ctx);
    }
  }
}

void ATS::begin_activation(Context &ctx) {
  current_session = Session{};
  current_session.activating_since = std::chrono::steady_clock::now();
  transition_stage(SessionStage::SdcOtherRelayChecks, ctx);
}
void ATS::cancel_activation(Context &ctx) {
  current_session.activation_error = ActivationError::Canceled;
  transition_stage(SessionStage::ActivationFailed, ctx);
}

void ATS::deactivate(DeactivationReason reason, Context &ctx) {
  current_session.deactivation_reason = reason;
  transition_stage(SessionStage::Deactivated, ctx);
}

}; // namespace ats
