#pragma once

#include <stdint.h>

// "Wed, 21 Oct 2026 07:28:00 GMT"(IMF-fixdate)をUnix秒に。読めなければfalse
bool parseHttpDate(const char* s, int64_t* out);
