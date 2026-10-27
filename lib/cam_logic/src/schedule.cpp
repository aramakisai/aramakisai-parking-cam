#include "schedule.h"

namespace {
const int64_t DAY = 86400;

int64_t ceilTo(int64_t v, int64_t unit) { return (v + unit - 1) / unit * unit; }
}  // namespace

bool inWindow(int64_t t, Window w) {
  int64_t s = (t + JST_OFFSET_SEC) % DAY;
  return s >= w.startMin * 60 && s <= w.endMin * 60;
}

int64_t nextSlot(int64_t from, long intervalSec, Window w) {
  int64_t local = ceilTo(from + JST_OFFSET_SEC, intervalSec);
  int64_t day = local - local % DAY;
  int64_t startOfWindow = ceilTo(day + w.startMin * 60, intervalSec);
  if (local < startOfWindow) return startOfWindow - JST_OFFSET_SEC;
  if (local <= day + w.endMin * 60) return local - JST_OFFSET_SEC;
  // 開始が区切りに揃っていない設定でも、翌日の最初の区切りに丸める
  return ceilTo(day + DAY + w.startMin * 60, intervalSec) - JST_OFFSET_SEC;
}
