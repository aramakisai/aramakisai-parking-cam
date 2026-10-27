#pragma once

#include <stddef.h>

#include "config_check.h"

// "https://host[:port]" の長さ。https:// でないかホスト名が無ければ 0
inline size_t httpsOriginLen(const char* url) {
  if (!isHttpsUrl(url)) return 0;
  size_t n = 8;
  while (url[n] && url[n] != '/' && url[n] != '?' && url[n] != '#') n++;
  return n;
}
