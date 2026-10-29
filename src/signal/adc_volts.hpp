#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class AdcVolts : public SignalProcessor {
private:
  double _raw;
  double _volts;

public:
  inline AdcVolts() {}

  inline void update(double raw, uint8_t) override {
    _raw = raw;
    _volts = adc_volts((int)raw);
  }

  inline double in() const override { return _raw; }
  inline double out() const override { return _volts; }
};
