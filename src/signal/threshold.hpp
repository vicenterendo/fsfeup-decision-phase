#pragma once

#include "processor.h"

/*
 * Behavior when value is below `open_below` and above `close_above`
 * 1: value decreases => threshold opens; value increases => threshold closes;
 * 0: threshold unchanged inside the range
 */
#define THRESHOLD_SENSITIVE 1

class Threshold : public SignalProcessor {
private:
  bool has_updated_once{false};
  double last{0};

  bool closed{false};

  double close_above;
  double open_below;

public:
  inline Threshold(double close_above, double open_below)
      : close_above(close_above), open_below(open_below) {}

  inline void update(double in, uint8_t) override {
    double delta = in - last;

#if THRESHOLD_SENSITIVE
    if ((delta > 0 || !has_updated_once) && in > close_above && !closed) {
      closed = true;
    } else if ((delta < 0 || !has_updated_once) && in < open_below && closed) {
      closed = false;
    }
#else
    if (last <= low || last >= high) {
      if ((!has_updated_once || last <= low) && in > low && !closed) {
        closed = true;
      } else if ((!has_updated_once || last >= high) && in < high && closed) {
        closed = false;
      }
    }
#endif

    last = in;
    has_updated_once = true;
  }

  inline double in() const override { return last; }
  inline double out() const override { return closed ? 1.0 : 0.0; }
};
