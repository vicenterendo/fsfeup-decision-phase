#pragma once

#include "../time.hpp"
#include "processor.h"
#include <chrono>
#include <limits.h>
#include <optional>

using std::chrono::steady_clock;

class Variation : public SignalProcessor {
private:
  double _variation{0};

  double _prev{0};
  std::optional<steady_clock::time_point> _last_update{std::nullopt};

public:
  inline Variation() {}

  inline void update(double value, uint8_t) override {
    if (_last_update.has_value()) {
      auto elapsed_s = (double)elapsed_ms(_last_update.value()) / 1000.0;
      if (elapsed_s == 0) {
        return;
      }
      _variation = (value - _prev) / (elapsed_s);
    }

    _last_update = steady_clock::now();
    _prev = value;
  }

  inline double in() const override { return _prev; }
  inline double out() const override { return _variation; }
};
