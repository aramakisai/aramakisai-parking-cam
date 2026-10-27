#include "clock_sync.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "cms_root_ca.h"
#include "config.h"
#include "http_date.h"
#include "schedule.h"
#include "url_origin.h"

namespace {

constexpr uint32_t SNTP_WAIT_MS = 30000;
constexpr uint32_t SNTP_INTERVAL_MS = 3600000;

volatile bool synced = false;
ClockSource source = ClockSource::None;

void onSntpSync(struct timeval*) {
  source = ClockSource::Sntp;
  synced = true;
}

void startSntp() {
  sntp_set_time_sync_notification_cb(onSntpSync);
  sntp_set_sync_interval(SNTP_INTERVAL_MS);
  configTime(0, 0, "ntp.nict.jp", "time.cloudflare.com", "pool.ntp.org");
}

void logSynced() {
  time_t now = time(nullptr) + JST_OFFSET_SEC;
  struct tm t;
  gmtime_r(&now, &t);
  char buf[32];
  strftime(buf, sizeof buf, "%FT%T", &t);
  Serial.printf("[time] synced source=%s now=%s+09:00\n", source == ClockSource::Sntp ? "sntp" : "date", buf);
}

bool syncFromHttpDate() {
  size_t n = httpsOriginLen(CMS_INGEST_URL);
  if (n == 0) return false;
  String url = String(CMS_INGEST_URL).substring(0, n) + "/";

  WiFiClientSecure client;
  client.setCACert(CMS_ROOT_CA_PEM);
  client.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(20000);
  http.setReuse(false);
  const char* keys[] = {"Date"};
  http.collectHeaders(keys, 1);
  if (!http.begin(client, url)) return false;
  // 応答コードは問わない(/ は404でも Date は付く)。負値は通信失敗
  int status = http.GET();
  String date = status > 0 ? http.header("Date") : String();
  http.end();

  int64_t sec;
  if (date.length() == 0 || !parseHttpDate(date.c_str(), &sec)) return false;
  struct timeval tv = {(time_t)sec, 0};
  settimeofday(&tv, nullptr);
  source = ClockSource::HttpDate;
  synced = true;
  return true;
}

}  // namespace

void clockBegin() { startSntp(); }

bool clockEnsure() {
  if (synced) return true;
  uint32_t start = millis();
  while (!synced && millis() - start < SNTP_WAIT_MS) delay(200);
  if (!synced) syncFromHttpDate();
  if (synced) {
    logSynced();
    return true;
  }
  Serial.println("[time] not synced");
  return false;
}

void clockInvalidate() {
  synced = false;
  source = ClockSource::None;
  esp_sntp_stop();
  startSntp();
}

ClockSource clockSource() { return source; }
