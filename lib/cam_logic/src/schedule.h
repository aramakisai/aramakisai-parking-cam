#pragma once

#include <stdint.h>

struct Window {
  int startMin;
  int endMin;
};
constexpr long JST_OFFSET_SEC = 9 * 3600;

int64_t nextSlot(int64_t from, long intervalSec, Window w);
bool inWindow(int64_t t, Window w);
