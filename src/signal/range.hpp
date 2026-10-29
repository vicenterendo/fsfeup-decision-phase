#pragma once

#include "../fault.hpp"
#include "processor.h"
#include <cmath>
#include <cstdlib>

using namespace fault;

class AdcRangeVerifier : public SignalProcessor {
private:
  uint8_t sensor_id;

  double min;
  double max;
  double safezone;

  FaultHelper near_out_of_range_fault;
  FaultHelper out_of_range_fault;

  double _volts{0};

public:
  inline AdcRangeVerifier(uint8_t sensor_id, double min, double max,
                          double safezone, std::queue<Fault> *faults)
      : sensor_id(sensor_id), min(min), max(max), safezone(safezone),
        near_out_of_range_fault(faults), out_of_range_fault(faults) {}

  inline void update(double volts, uint8_t) override {
    _volts = volts;

    double deviation = std::abs(
        volts < min ? (volts - min) : (max < volts ? (volts - max) : 0));

    bool faulting_near = deviation > 0 && deviation <= safezone;
    bool faulting_far = deviation > 0 && deviation > safezone;

    if ((faulting_far || faulting_near)) {
      uint32_t arg = 0;

      uint16_t millivolts = std::round(volts * 1000.0);

      arg |= millivolts;
      arg <<= 8;

      arg |= sensor_id;

      if (faulting_far) {
        out_of_range_fault.set(
            {fault::Type::OUT_OF_RANGE, fault::Severity::ERROR, arg});
        // printf("OOR\n");
      } else if (faulting_near) {
        near_out_of_range_fault.set(
            {fault::Type::NEAR_OUT_OF_RANGE, fault::Severity::WARNING, arg});
      }
    }

    out_of_range_fault.tick();
    near_out_of_range_fault.tick();
  };

  inline double in() const override { return _volts; }
  inline double out() const override { return _volts; };
};
