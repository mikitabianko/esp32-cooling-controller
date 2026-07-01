#pragma once

#include <Arduino.h>

#if defined(DEBUG) || defined(COOLING_DEBUG_LOG)
#define DEBUG_LOG_ENABLED 1
#else
#define DEBUG_LOG_ENABLED 0
#endif

#if DEBUG_LOG_ENABLED
#define DEBUG_SERIAL_BEGIN(baud) Serial.begin(baud)
#define DEBUG_PRINT(value) Serial.print(value)
#define DEBUG_PRINT2(value, format) Serial.print(value, format)
#define DEBUG_PRINTLN(value) Serial.println(value)
#define DEBUG_PRINTLN2(value, format) Serial.println(value, format)
#else
#define DEBUG_SERIAL_BEGIN(baud) ((void)0)
#define DEBUG_PRINT(value) ((void)0)
#define DEBUG_PRINT2(value, format) ((void)0)
#define DEBUG_PRINTLN(value) ((void)0)
#define DEBUG_PRINTLN2(value, format) ((void)0)
#endif
