#include "context.h"
#include "env/vm_io.hpp"
#include "lib/json.hpp"
#include "r2d.h"

using json = nlohmann::json;

inline Context *ctx = nullptr;

inline void setup() {
  // Can't do much here since there is no access to hardware
}

inline void loop(const Sensors &s, Actuators &act, json &can_out) {
  HardwareContext hw{&s, &act};
  if (ctx == nullptr) {
    ctx = new Context{hw};
  }
  ctx->read_sensors(hw);

  ctx->update_faults();
  ctx->update_anti_bspd();

  while (ctx->r2d_button.on_hold()) {
    if (ctx->r2d_state.is_idle()) {
      ctx->r2d_state.begin_activation(*ctx);
    } else if (ctx->r2d_state.is_active()) {
      ctx->r2d_state.deactivate(r2d::DeactivationReason::User, *ctx);
    }
  }
  while (ctx->r2d_button.on_click()) {
    if (ctx->r2d_state.is_activating()) {
      ctx->r2d_state.cancel_activation(*ctx);
    } else if (ctx->r2d_state.is_idle()) {
      ctx->r2d_state.reset();
    }
  }

  while (ctx->ats_button.on_hold()) {
    auto ats_stage = ctx->ats_state.stage();
    if (ats::ATS::is_idle(ats_stage)) {
      ctx->ats_state.begin_activation(*ctx);
    } else if (ats::ATS::is_active(ats_stage)) {
      ctx->ats_state.deactivate(ats::DeactivationReason::User, *ctx);
    }
  }
  while (ctx->ats_button.on_click()) {
    auto ats_stage = ctx->ats_state.stage();
    if (ats::ATS::is_activating(ats_stage)) {
      ctx->ats_state.cancel_activation(*ctx);
    } else if (ats::ATS::is_idle(ats_stage)) {
      ctx->ats_state.reset();
    }
  }

  ctx->r2d_state.tick(*ctx);
  ctx->ats_state.tick(*ctx);

  ctx->apply_motor();
  ctx->apply_leds();

  ctx->update_can(can_out);
}
