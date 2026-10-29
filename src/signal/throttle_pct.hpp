#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class ThrottlePct : public SignalProcessor {
private:
  double _volts;
  double _pct;

public:
  inline ThrottlePct() {}

  inline void update(double volts, uint8_t) override {
    _volts = volts;
    _pct = lin_map(volts, SENSOR_V_MIN, SENSOR_V_MAX, 0.0, 100.0);
  }

  inline double in() const override { return _volts; }
  inline double out() const override { return _pct; }
};
