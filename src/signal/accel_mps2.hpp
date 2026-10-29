#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class AccelMps2 : public SignalProcessor {
private:
  double _volts;
  double _mps2;

public:
  inline AccelMps2() {}

  inline void update(double volts, uint8_t) override {
    _volts = volts;
    _mps2 = lin_map(volts, SENSOR_V_MIN, SENSOR_V_MAX, -39.24, 39.24);
  }

  inline double in() const override { return _volts; }
  inline double out() const override { return _mps2; }
};
