#pragma once
struct Timeout {
  Timeout(int _interval) : interval(_interval) {}
  bool update(unsigned long long newTime) {

    if (running && (newTime - lastValidTime > interval)) {
      lastValidTime = newTime;
      return true;
    }
    return false;
  }

  bool updateOneShot(unsigned long long time) {
    if (update(time)) {
      running = false;
      return true;
    }
    return false;
  }

  bool running = true;

private:
  unsigned long long lastValidTime = 0;
  int interval;
};
