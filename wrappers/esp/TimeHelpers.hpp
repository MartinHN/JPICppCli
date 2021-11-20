#pragma once
#include "time.h"

// helpers around posix time function
namespace Helpers {
// epoch  time getters
static bool getLocalTimeUTC(time_t *epoch) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    PRINT("no time found stuck at :");
    PRINTLN(asctime(&timeinfo));
    return false;
  }
  time(epoch);

  return true;
}

static void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    PRINTLN("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

} // namespace Helpers
