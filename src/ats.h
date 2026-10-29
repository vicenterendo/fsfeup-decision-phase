#pragma once

#include <chrono>
#include <cstdint>

// forward declaration
class Context;

namespace ats {

// maximum allowed time to complete activation
constexpr long ACTIVATION_TIMEOUT_MS = 10000;

enum class SessionStage : uint8_t {
  Inactive,
  Deactivated,
  ActivationFailed,
  SdcOtherRelayChecks,
  SdcVcuRelayClose,
  WaitingForSdcClosed,
  Active
};

enum class ActivationError : uint8_t {
  None,
  TimedOut,
  Canceled,
  SdcDashNotClosed,
  SdcBspdNotClosed,
  SdcMasterPanelNotClosed,
  SdcBmsNotClosed,
};

enum class DeactivationReason : uint8_t {
  None,
  User,
  SdcNotClosed,
  SdcDashNotClosed,
  SdcBspdNotClosed,
  SdcMasterPanelNotClosed,
  SdcBmsNotClosed,
  SdcVcuNotClosed,
};

/*
 * Represents an ATS Session, reset on each activation start.
 * Hold temporary states that with their default values and makes restarts
 * safer by ensuring a "clean slate" for every initialization.
 */
struct Session {
  SessionStage stage{SessionStage::Inactive};

  std::chrono::steady_clock::time_point activating_since{};

  ActivationError activation_error{ActivationError::None};
  DeactivationReason deactivation_reason{DeactivationReason::None};
};

/*
 * ATS (Activate Tractive System) state handler.
 * Does state tracking and handles transition requests for an ATS state.
 */
class ATS {
private:
  Session current_session{};

  void transition_stage(SessionStage stage, Context &ctx);

  /*
   * Called by `tick()` when current session stage is `Active`.
   * Ensures all conditions are met for ATS to remain active and
   * deactivates ATS with the appropriate `DeactivationReason` if not.
   */
  void tick_active(Context &ctx);

  /*
   * Called by `tick()` when current session stage is `SdcOtherRelayChecks`.
   * If all other SDU line ADCs show closed, moves to next stage; Otherwise,
   * fails activation.
   */
  void tick_sdc_other_relay_checks(Context &ctx);

  /*
   * Called by `tick()` when current session stage is `SdcVcuRelayClose`.
   * Closes the VCU shutdown circuit relay and moves to next stage.
   */
  void tick_sdc_vcu_relay_close(Context &ctx);

  /*
   * Called by `tick()` when current session stage is `WaitingForSdcClosed`.
   * If VCU shutdown circuit ADC detects voltage, moves to next stage.
   */
  void tick_waiting_for_sdc_closed(Context &ctx);

  /*
   * Called unconditionally by `tick()`.
   * Checks for activation timeout and fails activation
   * if timeout occurs.
   */
  void tick_check_activation_timeout(Context &ctx);

public:
  /*
   * Ran directly from program loop().
   * Handles ATS state logic by selectively calling other `tick_*()` methods
   * (declared above)
   */
  void tick(Context &ctx);

  void begin_activation(Context &ctx);
  void cancel_activation(Context &ctx);
  void deactivate(DeactivationReason reason, Context &ctx);

  static inline bool is_active(SessionStage stage) {
    return stage == SessionStage::Active;
  };
  static inline bool is_idle(SessionStage stage) {
    return stage == SessionStage::Deactivated ||
           stage == SessionStage::Inactive ||
           stage == SessionStage::ActivationFailed;
  }
  static inline bool is_activating(SessionStage stage) {
    return !is_active(stage) && !is_idle(stage);
  }

  inline SessionStage stage() { return current_session.stage; }
  inline ActivationError activation_error() {
    return current_session.activation_error;
  };
  inline DeactivationReason deactivation_reason() {
    return current_session.deactivation_reason;
  }
  inline bool reset() {
    if (ats::ATS::is_idle(current_session.stage)) {
      current_session = Session{};
      return true;
    } else {
      return false;
    }
  }
};

} // namespace ats
