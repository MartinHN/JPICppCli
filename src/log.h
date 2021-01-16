#pragma once

#if ARDUINO
#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)
#else
#include <iostream>
#define PRINT(x) std::cout << x
#define PRINTLN(x) std::cout << x << std::endl
#endif
