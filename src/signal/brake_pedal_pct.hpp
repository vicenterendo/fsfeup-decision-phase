#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class BrakePedalPct : public SignalProcessor {
private:
  double _volts;
  double _bar;

public:
  inline BrakePedalPct() {}

  inline void update(double volts, uint8_t) override {
    _volts = volts;
    _bar = lin_map(volts, SENSOR_V_MIN, SENSOR_V_MAX, 0.0, 100.0);
  }

  inline double in() const override { return _volts; }
  inline double out() const override { return _bar; }
};
