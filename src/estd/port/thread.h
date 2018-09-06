#pragma once

#include "identify_platform.h"

#ifdef ESTD_FREERTOS
#include "freertos/thread.h"
#elif defined(ESTD_ARDUINO)
#include "arduino/thread.h"
#elif defined(ESTD_POSIX)
#include "posix/thread.h"
#endif
