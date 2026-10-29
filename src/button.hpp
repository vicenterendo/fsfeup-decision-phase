#include <chrono>
#include <cstdint>
#include <optional>

using std::chrono::steady_clock;

class Button {
private:
  bool _last_state{false};
  std::optional<steady_clock::time_point> _last_changed_at{std::nullopt};
  bool _hold_counted;

  unsigned int down_events;
  unsigned int up_events;
  unsigned int click_events;
  unsigned int hold_events;

public:
  static constexpr uint16_t BTN_HOLD_THRESHOLD_MS = 1000;

  inline Button() {};
  inline void update(bool current_state) {
    if (!_last_state && current_state) {
      down_events++;
    } else if (_last_state && current_state) {
      if (_last_changed_at.has_value() && !_hold_counted) {
        steady_clock::time_point last_changed_at = _last_changed_at.value();

        long elapsed_since_last_change_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_changed_at)
                .count();

        if (elapsed_since_last_change_ms > BTN_HOLD_THRESHOLD_MS) {
          hold_events++;
          _hold_counted = true;
        }
      }
    } else if (_last_state && !current_state) {
      up_events++;
      if (!_hold_counted) {
        click_events++;
      }
    }

    if (current_state != _last_state) {
      _last_changed_at = steady_clock::now();
      if (!current_state) {
        _hold_counted = false;
      }
    }
    _last_state = current_state;
  }

  inline void flush() {
    down_events = 0;
    up_events = 0;
    click_events = 0;
    hold_events = 0;
  }

  inline bool on_down() {
    if (down_events > 0) {
      down_events--;
      return true;
    } else {
      return false;
    }
  }

  inline bool on_up() {
    if (up_events > 0) {
      up_events--;
      return true;
    } else {
      return false;
    }
  }

  inline bool on_click() {
    if (click_events > 0) {
      click_events--;
      return true;
    } else {
      return false;
    }
  }

  inline bool on_hold() {
    if (hold_events > 0) {
      hold_events--;
      return true;
    } else {
      return false;
    }
  }
};
