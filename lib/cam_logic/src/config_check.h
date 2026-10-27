#pragma once

// ESP32のツールチェーンがC++11なので、constexprは単一のreturnで書く。

constexpr bool isNonEmpty(const char* s) { return s != nullptr && s[0] != '\0'; }

namespace config_check_detail {
constexpr bool startsWith(const char* s, const char* prefix) {
  return *prefix == '\0' ? true : (*s == *prefix && startsWith(s + 1, prefix + 1));
}
}  // namespace config_check_detail

constexpr bool isHttpsUrl(const char* url) {
  return url != nullptr && config_check_detail::startsWith(url, "https://") &&
         url[8] != '\0' && url[8] != '/' && url[8] != ':' && url[8] != '?' && url[8] != '#';
}

// 区切りを毎時0分に揃えるため、1時間を割り切る値に限る。
constexpr bool isValidInterval(long sec) { return sec >= 60 && sec <= 3600 && 3600 % sec == 0; }

constexpr bool isValidWindow(int startMin, int endMin) {
  return startMin >= 0 && startMin < endMin && endMin <= 1439;
}

constexpr bool isValidExposure(long v) { return v >= -2 && v <= 2; }
