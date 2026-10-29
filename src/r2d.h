#pragma once

#include "vehicle.h"
#include <chrono>
#include <cstdint>
#include <optional>

// forward declaration
class Context;

namespace r2d {

// maximum allowed time to complete ready-to-drive activation
constexpr long ACTIVATION_TIMEOUT_MS = 10000;

enum class SessionStage : uint8_t {
  Inactive,
  Deactivated,
  ActivationFailed,
  WaitingForPedals,
  Active
};

enum class ActivationError : uint8_t {
  None,
  TimedOut,
  User,
  SdcNotClosed,
  AtsNotActive,
};

enum class DeactivationReason : uint8_t {
  None,
  User,
  SdcNotClosed,
  AtsNotActive,
};

/*
 * Represents an R2D Session, reset on each activation start.
 * Hold temporary states that with their default values and makes R2D restarts
 * safer by ensuring a "clean slate" for every initialization.
 */
struct Session {
  SessionStage stage{SessionStage::Inactive};

  std::chrono::steady_clock::time_point activating_since{};

  ActivationError activation_error{ActivationError::None};
  DeactivationReason deactivation_reason{DeactivationReason::None};
};

/*
 * Ready to Drive (R2D) state handler.
 * Does state tracking and handles transition requests for a R2D state.
 */
class R2D {
private:
  r2d::Session current_session{};

  void set_stage(SessionStage);

  r2d::ActivationError do_startup_checks(Context &ctx);

  /*
   * Called by `tick()` when current session stage is `WaitingForPedals`.
   * Checks that the throttle is idle and that brakes are actuated and
   * pressurized. Moves to next activation stage if conditions are met.
   */
  void tick_waiting_for_pedals(Context &ctx);

  /*
   * Called by `tick()` as long as R2D is in activation.
   * Checks vehicle state requirements are met for R2D activation.
   * If not, cancels activation process.
   */
  void tick_activation_checks(Context &ctx);

  /*
   * Called by `tick()` when current session stage is `Active`.
   * Ensures all conditions are met for R2D to remain active and
   * deactivates R2D with the appropriate `deactivation_reason` if not.
   */
  void tick_active(Context &ctx);

  /*
   * Called unconditionally by `tick()`.
   * Checks for activation timeout and fails activation
   * if timeout occurs.
   */
  void tick_check_activation_timeout(Context &ctx);

public:
  /*
   * Ran directly from program loop().
   * Handles R2D state logic by selectively calling `tick_` methods (declared
   * above)
   */
  void tick(Context &ctx);

  void begin_activation(Context &ctx);
  void cancel_activation(Context &ctx);
  void deactivate(DeactivationReason reason, Context &ctx);

  inline bool is_active() {
    return current_session.stage == SessionStage::Active;
  };
  inline bool is_idle() {
    return current_session.stage == SessionStage::Deactivated ||
           current_session.stage == SessionStage::Inactive ||
           current_session.stage == SessionStage::ActivationFailed;
  }
  inline bool is_activating() { return !is_active() && !is_idle(); }

  inline SessionStage stage() { return current_session.stage; }
  inline ActivationError activation_error() {
    return current_session.activation_error;
  };
  inline DeactivationReason deactivation_reason() {
    return current_session.deactivation_reason;
  }
  inline bool reset() {
    if (is_idle()) {
      current_session = Session{};
      return true;
    } else {
      return false;
    }
  }
};

} // namespace r2d
