#pragma once

#include "processor.h"
#include <vector>

class SignalProcessorChain : public SignalProcessor {
private:
  double _in{0};
  double _out{0};

  const std::vector<SignalProcessor *> filters;

public:
  inline SignalProcessorChain(std::vector<SignalProcessor *> filters)
      : filters(filters) {}

  inline void update(double value, uint8_t channel = 0) override {
    _in = value;
    for (unsigned int i = 0; i < filters.size(); i++) {
      filters[i]->update(value, channel);
      value = filters[i]->out();
    }
    _out = value;
  }

  inline double in() const override { return _in; }
  inline double out() const override { return _out; }
};
