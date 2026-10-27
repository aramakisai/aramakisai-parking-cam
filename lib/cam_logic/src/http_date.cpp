#include "http_date.h"

#include <string.h>

namespace {
const char* const MONTHS[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool digits(const char*& p, int n, int* out) {
  int v = 0;
  for (int i = 0; i < n; i++, p++) {
    if (*p < '0' || *p > '9') return false;
    v = v * 10 + (*p - '0');
  }
  *out = v;
  return true;
}

bool lit(const char*& p, const char* t) {
  size_t n = strlen(t);
  if (strncmp(p, t, n) != 0) return false;
  p += n;
  return true;
}

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
}  // namespace

bool parseHttpDate(const char* s, int64_t* out) {
  if (s == nullptr || out == nullptr || strlen(s) < 29) return false;
  const char* p = s + 3;  // 曜日は検証しない
  int d, y, hh, mm, ss, mon = -1;
  if (!lit(p, ", ") || !digits(p, 2, &d) || !lit(p, " ")) return false;
  for (int i = 0; i < 12; i++) {
    if (strncmp(p, MONTHS[i], 3) == 0) mon = i;
  }
  if (mon < 0) return false;
  p += 3;
  if (!lit(p, " ") || !digits(p, 4, &y) || !lit(p, " ") || !digits(p, 2, &hh) || !lit(p, ":") ||
      !digits(p, 2, &mm) || !lit(p, ":") || !digits(p, 2, &ss) || !lit(p, " GMT") || *p != '\0') {
    return false;
  }
  static const int DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int dim = DAYS[mon] + (mon == 1 && isLeap(y) ? 1 : 0);
  if (y < 1970 || d < 1 || d > dim || hh > 23 || mm > 59 || ss > 60) return false;

  // days-from-civil (Howard Hinnant)。3月起点の年に直して閏日を末尾に置く
  int yy = mon < 2 ? y - 1 : y;
  int era = yy / 400;
  int yoe = yy - era * 400;
  int doy = (153 * (mon + (mon > 1 ? -2 : 10)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + doe - 719468;
  *out = days * 86400 + hh * 3600 + mm * 60 + ss;
  return true;
}
