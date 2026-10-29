#pragma once

#include "processor.h"
#include <cstdint>

class Splitter : public SignalProcessor {
private:
  double _value;

  SignalProcessor *_processor;
  uint8_t channel;

public:
  inline Splitter(SignalProcessor *_split_processor, uint8_t channel = 0)
      : _processor(_split_processor), channel(channel) {}

  inline void update(double value, uint8_t) override {
    _value = value;
    _processor->update(value, channel);
  }

  inline double in() const override { return _value; }
  inline double out() const override { return _value; }
};
