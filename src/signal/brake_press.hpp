#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class BrakePressure : public SignalProcessor {
private:
  double _volts;
  double _bar;

public:
  inline BrakePressure() {}

  inline void update(double volts, uint8_t) override {
    _volts = volts;
    _bar = lin_map(volts, SENSOR_V_MIN, SENSOR_V_MAX, 0.0, 80.0);
  }

  inline double in() const override { return _volts; }
  inline double out() const override { return _bar; }
};
