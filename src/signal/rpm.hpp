#pragma once

#include "../time.hpp"
#include "processor.h"
#include <algorithm>
#include <chrono>
#include <limits.h>
#include <optional>

using std::chrono::steady_clock;

class Rpm : public SignalProcessor {
private:
  double _raw;
  double _rpm;

  bool _prev_over_sensor{false};
  std::optional<steady_clock::time_point> _last_met_sensor{std::nullopt};

public:
  inline Rpm() {}

  inline void update(double raw, uint8_t) override {
    _raw = raw;
    bool over_sensor = raw < (double)UINT16_MAX / 2;

    bool meeting_sensor_now = over_sensor && !_prev_over_sensor;

    if (_last_met_sensor.has_value()) {
      uint64_t elapsed = elapsed_ms(_last_met_sensor.value());
      if (elapsed == 0) {
        return;
      }
      double rpm_if_meeting_sensor_now =
          (double)0.25 / ((double)elapsed / 60000);

      if (meeting_sensor_now) {
        _rpm = rpm_if_meeting_sensor_now;
      } else {
        _rpm = std::min(_rpm, rpm_if_meeting_sensor_now);
      }
    }

    if (meeting_sensor_now) {
      _last_met_sensor = steady_clock::now();
    }
    _prev_over_sensor = over_sensor;
  }

  inline double in() const override { return _raw; }
  inline double out() const override { return _rpm; }
};
