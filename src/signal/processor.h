#pragma once

#include <cstdint>
class SignalProcessor {
public:
  virtual ~SignalProcessor() = default;

  virtual void update(double in, uint8_t channel = 0) = 0;

  virtual double in() const = 0;
  virtual double out() const = 0;
};
