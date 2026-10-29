#pragma once

#include "time.hpp"
#include <chrono>
#include <cstdint>
#include <optional>
#include <queue>

namespace fault {

constexpr uint64_t MIN_INTERVAL_MS = 1000;

enum class Type : uint8_t {
  UNSTABLE_SIGNAL = 0,
  NEAR_OUT_OF_RANGE = 1,
  OUT_OF_RANGE = 2,
  BRAKE_DEVIATION = 3,
  SDC_IMPLAUSIBILITY = 4
};
enum class Severity : uint8_t { WARNING = 0, ERROR = 1 };

struct FaultBuilder {
  // bool active{false};
  Type type{0};
  Severity severity{0};
  uint32_t arg{0};

  bool operator==(const FaultBuilder &other) const {
    return type == other.type && severity == other.severity && arg == other.arg;
  }

  bool operator!=(const FaultBuilder &other) const { return !(*this == other); }
};

struct Fault {
  Type type{0};
  Severity severity{0};
  uint32_t arg{0};
  std::chrono::system_clock::time_point detected_at{};
};

class FaultHelper {
private:
  std::queue<Fault> *queue;
  FaultBuilder _prev_fault;

  FaultBuilder _builder{};

  bool _active{false};
  bool _prev_active{false};

  uint64_t min_interval_ms;

  std::optional<std::chrono::system_clock::time_point> last_sent;

  bool _build_inhibit() {
    return last_sent.has_value() &&
           elapsed_ms(last_sent.value()) < min_interval_ms;
  }

  bool _skipped = false;

public:
  inline FaultHelper(std::queue<Fault> *queue,
                     uint64_t min_interval_ms = MIN_INTERVAL_MS * 10)
      : queue(queue), min_interval_ms(min_interval_ms) {}

  inline bool set(FaultBuilder builder) {
    if (_build_inhibit()) {
      _skipped = true;
      return false;
    } else {
      _active = true;
      _builder = builder;
      return true;
    }
  }

  inline void tick() {
    if (!_skipped) {
      if (_active && (_prev_fault != _builder || _active != _prev_active)) {
        queue->push(Fault{_builder.type, _builder.severity, _builder.arg,
                          std::chrono::system_clock::now()});
        last_sent = std::chrono::system_clock::now();
      }
      _prev_fault = _builder;
      _prev_active = _active;
      _active = false;
    }
    _skipped = false;
  }
};
} // namespace fault
