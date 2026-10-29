#pragma once

#include "../fault.hpp"
#include "processor.h"
#include <chrono>
#include <optional>

using std::chrono::steady_clock;
using namespace fault;

/*
 * Triggers an UNSTABLE_SIGNAL fault if the input drops
 * below `threshold` for less that `min_cutoff_ms`
 */
class SignalStabilityVerifier : public SignalProcessor {
private:
  uint8_t sensor_id;

  double _threshold;
  uint64_t _min_cutoff_ms;

  std::optional<steady_clock::time_point> _last_drop_at;

  FaultHelper fault;

  double _last_value{0};

public:
  inline SignalStabilityVerifier(uint8_t sensor_id, double threshold,
                                 uint64_t min_cutoff_ms,
                                 std::queue<Fault> *fault_queue)
      : sensor_id(sensor_id), _threshold(threshold),
        _min_cutoff_ms(min_cutoff_ms), fault(fault_queue, 60000) {}

  inline void update(double value, uint8_t) override {
    bool _last_on = _last_value > _threshold;
    bool now_on = value > _threshold;

    if (_last_on && !now_on) {
      _last_drop_at = steady_clock::now();
    } else if (!_last_on && now_on && _last_drop_at.has_value()) {
      uint64_t elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              steady_clock::now() - _last_drop_at.value())
              .count();
      if (elapsed_ms < _min_cutoff_ms) {
        uint32_t arg = 0 | sensor_id;
        fault.set(
            {fault::Type::UNSTABLE_SIGNAL, fault::Severity::WARNING, arg});
      }
    }

    fault.tick();

    _last_value = value;
  };

  inline double in() const override { return _last_value; }
  inline double out() const override { return _last_value; };
};
