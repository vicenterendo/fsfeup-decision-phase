#pragma once

#include "../env/vm_io.hpp"
#include "../vehicle.h"
#include "processor.h"
#include <algorithm>

class ThrottleTorqueMap : public SignalProcessor {
private:
  double _throttle_pct;
  double _torque;

public:
  inline ThrottleTorqueMap() {}

  inline void update(double throttle_pct, uint8_t) override {
    _throttle_pct = throttle_pct;
    _torque =
        std::clamp(lin_map(throttle_pct, 0, 100, 0, VEHICLE_MAX_TORQUE_NM), 0.0,
                   VEHICLE_MAX_TORQUE_NM);
  }

  inline double in() const override { return _throttle_pct; }
  inline double out() const override { return _torque; }
};
