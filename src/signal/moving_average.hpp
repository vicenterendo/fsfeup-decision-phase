#pragma once

#include "processor.h"
#include <chrono>
#include <cmath>
#include <optional>

class MovingAverage : public SignalProcessor {
private:
  using Clock = std::chrono::steady_clock;

  double last_in{0};
  double filtered{0.0};

  std::optional<Clock::time_point> last_update;
  double tau;

public:
  inline MovingAverage(double tau_seconds = 5.0) : tau(tau_seconds) {}

  inline void update(double in, uint8_t) override {
    last_in = in;

    auto now = Clock::now();

    if (!last_update) {
      filtered = in;
      last_update = now;
      return;
    }

    double dt = std::chrono::duration<double>(now - *last_update).count();

    double alpha = 1.0 - std::exp(-dt / tau);
    filtered += alpha * (in - filtered);

    last_update = now;
  }

  inline double in() const override { return last_in; }
  inline double out() const override { return filtered; }
};
