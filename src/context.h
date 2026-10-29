#pragma once

#include "ats.h"
#include "button.hpp"
#include "env/vm_io.hpp"
#include "fault.hpp"
#include "r2d.h"
#include "signal/accel_mps2.hpp"
#include "signal/adc_volts.hpp"
#include "signal/brake_pedal_pct.hpp"
#include "signal/brake_press.hpp"
#include "signal/brake_target.hpp"
#include "signal/conn_stability.hpp"
#include "signal/moving_average.hpp"
#include "signal/processor.h"
#include "signal/processor_chain.hpp"
#include "signal/range.hpp"
#include "signal/rpm.hpp"
#include "signal/sdc_volts.hpp"
#include "signal/splitter.hpp"
#include "signal/threshold.hpp"
#include "signal/throttle_pct.hpp"
#include "signal/throttle_torque_map.hpp"
#include "signal/variation.hpp"
#include "time.hpp"
#include "vehicle.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <queue>
#include <vector>

#define ADC_SDC_DASH 0
#define ADC_SDC_BSPD 1
#define ADC_SDC_MASTER_PANEL 2
#define ADC_SDC_BMS 3
#define ADC_SDC_VCU 4
#define ADC_BRAKE_PRESSURE 5
#define ADC_BRAKE_PEDAL 6
#define ADC_THROTTLE 7
#define ADC_WHEEL_SPEED_FL 8
#define ADC_WHEEL_SPEED_FR 9
#define ADC_WHEEL_SPEED_RL 10
#define ADC_WHEEL_SPEED_RR 11
#define ADC_STEERING_ANGLE 12
#define ADC_ACCEL_X 13
#define ADC_ACCEL_Y 14

constexpr double pi = 3.14159265358979323846;

// using adc::Adc;
using Spc = SignalProcessorChain;

struct HardwareContext {
  // Using pointers rather than references here
  // so HardwareContext variables can be reassigned
  const Sensors *s;
  Actuators *act;
};

class Context {
private:
  HardwareContext hw;

  bool _vcu_sdc_relay_closed{false};

public:
  inline Context(HardwareContext hw) : hw(hw) {
    hw.act->sdc_vcu_relay = _vcu_sdc_relay_closed;
  }

  ~Context() {}

  //
  //
  // ===============================
  // ----------- STATES ------------
  // ===============================
  r2d::R2D r2d_state{};
  ats::ATS ats_state{};

  //
  //
  // ===============================
  // ----------- SENSORS -----------
  // ===============================

  inline void read_sensors(HardwareContext new_cycle) {
    hw = new_cycle;

    _sdc_dash_closed.update(hw.s->sdc_after_dash);
    _sdc_bspd_closed.update(hw.s->sdc_after_bspd);
    _sdc_master_panel_closed.update(hw.s->sdc_after_master_panel);
    _sdc_bms_closed.update(hw.s->sdc_after_bms);
    _sdc_vcu_closed.update(hw.s->sdc_after_vcu);

    _wheel_fl_rpm.update(hw.s->wheel_speed[FL]);
    _wheel_fr_rpm.update(hw.s->wheel_speed[FR]);
    _wheel_rl_rpm.update(hw.s->wheel_speed[RL]);
    _wheel_rr_rpm.update(hw.s->wheel_speed[RR]);

    _accel_wheel_mps2.update(speed_mps());

    _brake_pedal_pct.update(hw.s->brake_pedal_position);
    _brake_pressure_bar.update(hw.s->brake_pressure);

    _throttle_pct.update(hw.s->accelerator_position);

    _steering_volts.update(hw.s->steering_angle);

    _accel_x_mps2.update(hw.s->accel_x);
    _accel_y_mps2.update(hw.s->accel_y);

    // _sdc_ma.update(hw.s->sdc_after_vcu);

    r2d_button.update(hw.s->R2D_btn);
    ats_button.update(hw.s->ATS_btn);
  }

  // ---------- Buttons ------------
  Button r2d_button{};
  Button ats_button{};

  // ----------- Brakes ------------
  Spc _brake_pressure_bar{
      {new AdcVolts(),
       new AdcRangeVerifier(ADC_BRAKE_PRESSURE, 0.325, 2.975, 0.1, &faults),
       new BrakePressure(), new Splitter(&_brake_pressure_light_threshold)}};
  Spc _brake_pressure_light_threshold{{new Threshold(10, 5)}};

  Spc _brake_pedal_pct{
      {new AdcVolts(),
       new AdcRangeVerifier(ADC_BRAKE_PEDAL, 0.325, 2.975, 0.1, &faults),
       new BrakePedalPct(), new Splitter(&_brake_pressure_target_bar)}};
  Spc _brake_pressure_target_bar{{new BrakePressureTarget()}};

  inline double brake_bar() { return _brake_pressure_bar.out(); }
  inline double brake_target_bar() { return _brake_pressure_target_bar.out(); }
  inline double brake_pedal_pct() { return _brake_pedal_pct.out(); }

  // ----------- Motor -------------
  Spc _torque_request_nm{{new ThrottleTorqueMap()}};
  Spc _throttle_pct{
      {new AdcVolts(),
       new AdcRangeVerifier(ADC_THROTTLE, 0.325, 2.975, 0.1, &faults),
       new ThrottlePct(), new Splitter(&_torque_request_nm)}};

  inline double throttle_pct() { return _throttle_pct.out(); }
  inline bool is_throttle_idle() { return throttle_pct() < 5; }
  inline double torque_target_nm() {
    if (r2d_state.is_active()) {
      double torque = _torque_request_nm.out();
      if (anti_bspd_limitting) {
        torque = std::min(torque, BSPD_MAX_TORQUE_NM);
      }
      return torque;
    } else {
      return 0;
    }
  }

  // ----------- Wheels ------------
  Spc wheel_spc() { return Spc{{new Rpm(), new MovingAverage(0.5)}}; }
  Spc _wheel_fl_rpm{wheel_spc()};
  Spc _wheel_fr_rpm{wheel_spc()};
  Spc _wheel_rl_rpm{wheel_spc()};
  Spc _wheel_rr_rpm{wheel_spc()};

  Spc _accel_wheel_mps2{{new Variation(), new MovingAverage(0.5)}};
  //
  inline double wheel_rpm_avg() {
    return (_wheel_fl_rpm.out() + _wheel_fr_rpm.out() + _wheel_rl_rpm.out() +
            _wheel_rr_rpm.out()) /
           4.0;
  }
  inline double speed_mps() {
    return (wheel_rpm_avg() / 60.0) * (2 * pi * 0.25);
  }

  // ----------- Accel -------------
  Spc _accel_x_mps2{
      {new AdcVolts(),
       new AdcRangeVerifier(ADC_ACCEL_X, 0.325, 2.975, 0.1, &faults),
       new AccelMps2()}};
  Spc _accel_y_mps2{
      {new AdcVolts(),
       new AdcRangeVerifier(ADC_ACCEL_Y, 0.325, 2.975, 0.1, &faults),
       new AccelMps2()}};

  inline double accel_y_mps2() { return _accel_y_mps2.out(); }
  inline double accel_x_mps2() { return _accel_x_mps2.out(); }

  // ---------- SDC Line -----------
  SignalProcessorChain sdc_spc(uint8_t id, Spc *ma = nullptr) {
    std::vector<SignalProcessor *> chain{{
        new SdcVolts(),
        new SignalStabilityVerifier(id, 20, 1000, &faults),
        new MovingAverage(1),
    }};

    if (ma != nullptr) {
      chain.push_back(ma);
    }

    chain.push_back(new Threshold(2, 10));

    return SignalProcessorChain(chain);
  }
  Spc _sdc_dash_closed{sdc_spc(ADC_SDC_DASH)};
  Spc _sdc_bspd_closed{sdc_spc(ADC_SDC_BSPD)};
  Spc _sdc_master_panel_closed{sdc_spc(ADC_SDC_MASTER_PANEL)};
  Spc _sdc_bms_closed{sdc_spc(ADC_SDC_BMS)};
  Spc _sdc_ma{{}};
  Spc _sdc_vcu_closed{sdc_spc(ADC_SDC_VCU, &_sdc_ma)};

  inline bool sdc_dash_closed() { return _sdc_dash_closed.out(); }
  inline bool sdc_bspd_closed() { return _sdc_bspd_closed.out(); }
  inline bool sdc_master_panel_closed() {
    return _sdc_master_panel_closed.out();
  }
  inline bool sdc_bms_closed() { return _sdc_bms_closed.out(); }
  inline bool sdc_vcu_closed() { return _sdc_vcu_closed.out(); }

  inline bool sdc_closed() { return sdc_vcu_closed(); }

  inline int sdc_after_vcu_raw() { return _sdc_vcu_closed.in(); }

  //------------ Others ------------
  Spc _steering_volts{{
      new AdcVolts(),
      new AdcRangeVerifier(ADC_STEERING_ANGLE, 0.325, 2.975, 0.1, &faults),
  }};

  //
  //
  // ===============================
  // ---------- ACTUATORS ----------
  // ===============================

  // ------- SDC VCU Relay ---------
  inline void set_vcu_sdc_relay(bool closed) {
    hw.act->sdc_vcu_relay = closed;
    _vcu_sdc_relay_closed = closed;
  }
  inline bool vcu_sdc_relay_closed() { return _vcu_sdc_relay_closed; }

  inline void apply_motor() { hw.act->motor_torque = torque_target_nm(); }
  inline void apply_leds() {
    hw.act->R2D_LED = r2d_state.is_active();
    hw.act->TS_activated_led = sdc_closed();
    hw.act->brake_light = _brake_pressure_light_threshold.out();
  }

  //
  //
  // ===============================
  // ----------- FAULTS ------------
  // ===============================

  std::queue<Fault> faults{};

  void update_faults() {
    update_brake_deviation();
    update_sdc_implausibility();
  }

  // ------- Brake Deviation -------
  FaultHelper brake_deviation_fault{&faults};
  inline void update_brake_deviation() {
    double target = brake_target_bar();
    double actual = brake_bar();

    double deviation_pct = std::abs(((actual - target) / actual) * 100);

    bool implausibility = deviation_pct > 5 && (actual > 5); // <-- relatorio

    if (implausibility) {
      brake_deviation_fault.set(
          {fault::Type::BRAKE_DEVIATION, fault::Severity::WARNING, 0});
    }

    brake_deviation_fault.tick();
  }

  // ----- SDC Implausibility ------
  FaultHelper sdc_implausibility_fault{&faults};
  inline bool is_sdc_implausibility() {
    bool sdc[5]{sdc_dash_closed(), sdc_bspd_closed(), sdc_master_panel_closed(),
                sdc_bms_closed(), sdc_vcu_closed()};
    bool open_found = false;
    for (auto sdc_reading : sdc) {
      if (!sdc_reading) {
        open_found = true;
      } else if (open_found) {
        return true;
      }
    }
    return false;
  }
  inline void update_sdc_implausibility() {
    if (is_sdc_implausibility()) {
      sdc_implausibility_fault.set(
          {fault::Type::SDC_IMPLAUSIBILITY, fault::Severity::ERROR, 0});
    }

    sdc_implausibility_fault.tick();
  }

  static constexpr double BSPD_MAX_TORQUE_NM = 0;
  static constexpr double BSPD_MAX_BRAKE_BAR = 20.0;

  //
  //
  // ===============================
  // ---------- ANTI-BSPD ----------
  // ===============================

  bool anti_bspd_limitting = false;
  bool prev_hard_braking = false;
  std::optional<steady_clock::time_point> hard_braking_since{std::nullopt};
  inline void update_anti_bspd() {
    bool hard_braking = brake_bar() > BSPD_MAX_BRAKE_BAR;
    anti_bspd_limitting = hard_braking;
  }

  //
  //
  // ===============================
  // ----------- CAN BUS -----------
  // ===============================

  uint fault_n = 0;
  std::optional<steady_clock::time_point> fault_last_sent{std::nullopt};

  bool first_can_message_sent = false;

  inline void update_can(json &can_out) {
    // ------------- R2D -------------
    can_out["r2d_active"] = r2d_state.is_active();
    can_out["r2d_stage"] = r2d_state.stage();
    can_out["r2d_activation_error"] = r2d_state.activation_error();
    can_out["r2d_deactivation_reason"] = r2d_state.deactivation_reason();
    can_out["r2d_button"] = hw.s->R2D_btn;

    // ------------- ATS -------------
    can_out["ats_active"] = ats::ATS::is_active(ats_state.stage());
    can_out["ats_stage"] = ats_state.stage();
    can_out["ats_activation_error"] = ats_state.activation_error();
    can_out["ats_deactivation_reason"] = ats_state.deactivation_reason();

    // ------------- SDC -------------
    can_out["sdc_dash"] = sdc_dash_closed();
    can_out["sdc_bspd"] = sdc_bspd_closed();
    can_out["sdc_master_panel"] = sdc_master_panel_closed();
    can_out["sdc_bms"] = sdc_bms_closed();
    can_out["sdc_vcu"] = sdc_vcu_closed();

    // Separated out raw and MA for VCU sensor because it
    // influences veredict on ATS (and, consequently, R2D)
    // activating/staying active
    can_out["sdc_after_vcu_raw"] = sdc_volts(sdc_after_vcu_raw());
    can_out["sdc_after_vcu_filtered_ma"] = _sdc_ma.out();

    // ------------- Brakes -------------
    can_out["brake_pedal_pct"] = brake_pedal_pct();
    can_out["brake_pressure_bar_raw"] = _brake_pressure_bar.in();
    can_out["brake_pressure_bar"] = brake_bar();
    can_out["brake_pressure_bar_target"] = brake_target_bar();
    can_out["brake_pressure_light_threshold"] =
        _brake_pressure_light_threshold.out();

    // ------------- Motor -------------
    can_out["throttle_pct"] = throttle_pct();
    can_out["torque_target_nm"] = torque_target_nm();

    // ------------- Wheels -------------
    can_out["speed_mps"] = speed_mps();
    can_out["wheel_rpm"] = wheel_rpm_avg();
    can_out["accel_wheel_mps2"] = _accel_wheel_mps2.out();

    // ------------- Accel --------------
    can_out["accel_x_mps2"] = accel_x_mps2();
    can_out["accel_y_mps2"] = accel_y_mps2();

    // ==================================
    //               FAULTS
    // ==================================
    bool fault_inhibit =
        fault_last_sent.has_value() &&
        elapsed_ms(fault_last_sent.value()) < fault::MIN_INTERVAL_MS;

    // Initialize everything on first message
    // so schema doesn't change and Foxglove
    // freaks out
    if (!first_can_message_sent) {
      can_out["fault_n"] = 0;

      can_out["fault_type"] = 0;
      can_out["fault_severity"] = 0;
      can_out["fault_arg"] = 0;
      can_out["fault_detected_at"] = 0;
    } else if (!faults.empty() && !fault_inhibit) {
      const auto fault = faults.front();
      json fault_json{};

      // milliseconds since epoch
      uint64_t detected_at =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              fault.detected_at.time_since_epoch())
              .count();

      // change `fault_n` to signal new fault
      can_out["fault_n"] = ++fault_n;

      can_out["fault_type"] = fault.type;
      can_out["fault_severity"] = fault.severity;
      can_out["fault_arg"] = fault.arg;
      can_out["fault_detected_at"] = detected_at;

      faults.pop();

      fault_last_sent = steady_clock::now();
    }

    first_can_message_sent = true;
  }
};
