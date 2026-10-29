#pragma once

#include "../env/vm_io.hpp"
#include "../vehicle.h"
#include "processor.h"
#include <algorithm>

class BrakePressureTarget : public SignalProcessor {
private:
  double _pedal_pct;
  double _bar;

public:
  inline BrakePressureTarget() {}

  inline void update(double pedal_pct, uint8_t) override {
    _pedal_pct = pedal_pct;
    _bar = std::clamp(
        lin_map(pedal_pct, 0, 100, 0, VEHICLE_MAX_BRAKE_PRESSURE_BAR), 0.0,
        VEHICLE_MAX_BRAKE_PRESSURE_BAR);
  }

  inline double in() const override { return _pedal_pct; }
  inline double out() const override { return _bar; }
};
