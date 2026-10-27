#include "ingest_response.h"

#include <string.h>

namespace {
struct Cursor {
  const char* p;
  const char* end;
  void skipWs() {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
  }
  bool eat(char c) {
    skipWs();
    if (p < end && *p == c) {
      p++;
      return true;
    }
    return false;
  }
  bool eatStr(const char* s) {
    size_t n = strlen(s);
    if ((size_t)(end - p) < n || strncmp(p, s, n) != 0) return false;
    p += n;
    return true;
  }
};

// {"exposure": <整数>} だけを読む。他のキーや整数以外の値は「値なし」に倒す。
bool readExposure(const char* body, size_t len, int* out) {
  if (body == nullptr) return false;
  Cursor c = {body, body + len};
  if (!c.eat('{')) return false;
  c.skipWs();
  if (!c.eatStr("\"exposure\"") || !c.eat(':')) return false;
  c.skipWs();
  bool neg = false;
  if (c.p < c.end && *c.p == '-') {
    neg = true;
    c.p++;
  }
  int v = 0, n = 0;
  while (c.p < c.end && *c.p >= '0' && *c.p <= '9') {
    if (++n > 3) return false;
    v = v * 10 + (*c.p++ - '0');
  }
  if (n == 0) return false;
  if (neg) v = -v;
  if (v < -2 || v > 2 || !c.eat('}')) return false;
  c.skipWs();
  if (c.p != c.end) return false;
  *out = v;
  return true;
}

bool contains(const char* body, size_t len, const char* needle) {
  size_t n = strlen(needle);
  if (body == nullptr || len < n) return false;
  for (size_t i = 0; i + n <= len; i++) {
    if (strncmp(body + i, needle, n) == 0) return true;
  }
  return false;
}
}  // namespace

IngestResult parseIngestResponse(int status, const char* body, size_t len) {
  IngestResult r = {IngestOutcome::UnexpectedStatus, false, 0};
  if (status == 200) {
    r.outcome = IngestOutcome::Ok;
    r.hasExposure = readExposure(body, len, &r.exposure);
  } else if (status == 400) {
    r.outcome = contains(body, len, "\"invalid_captured_at\"") ? IngestOutcome::InvalidCapturedAt
                                                                : IngestOutcome::InvalidExposure;
  } else if (status == 401) {
    r.outcome = IngestOutcome::Unauthorized;
  } else if (status == 413) {
    r.outcome = IngestOutcome::TooLarge;
  } else if (status == 415) {
    r.outcome = IngestOutcome::NotJpeg;
  } else if (status >= 500 && status <= 599) {
    r.outcome = IngestOutcome::ServerError;
  }
  return r;
}
