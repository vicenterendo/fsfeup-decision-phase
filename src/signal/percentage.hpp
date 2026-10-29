#pragma once

#include "../env/vm_io.hpp"
#include "processor.h"

class Percentage : public SignalProcessor {
private:
  double _raw;
  double _pct;

  double _sensor_min;
  double _sensor_max;

public:
  inline Percentage(double _sensor_min, double _sensor_max)
      : _sensor_min(_sensor_min), _sensor_max(_sensor_max) {}

  inline void update(double raw, uint8_t) override {
    _raw = raw;
    _pct = lin_map(raw, _sensor_min, _sensor_max, 0, 100);
  }

  inline double in() const override { return _raw; }
  inline double out() const override { return _pct; }
};
