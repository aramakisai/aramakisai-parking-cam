#include <Arduino.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <time.h>

#include "camera.h"
#include "clock_sync.h"
#include "config.h"
#include "failure_policy.h"
#include "schedule.h"
#include "uploader.h"
#include "wifi_link.h"

namespace {

constexpr uint32_t WDT_TIMEOUT_SEC = 90;
constexpr uint32_t RETRY_INTERVAL_MS = 60000;
constexpr uint32_t WIFI_TIMEOUT_MS = 80000;
constexpr Window WINDOW = {MONITOR_START_MIN, MONITOR_END_MIN};

int exposure = EXPOSURE_DEFAULT;
const char* resetReason = "UNKNOWN";
FailurePolicy policy;
bool slotInitialized = false;
int64_t nextSlotAt = 0;
bool sntpStarted = false;
uint32_t lastWifiTryMs = 0;
uint32_t lastClockTryMs = 0;
bool wifiTried = false;
bool clockTried = false;
bool wifiWasUp = false;
bool probePending = false;

// 受信側が ^[A-Z0-9_]{1,32}$ で検証するため、esp_reset_reason_t を英字に直す
const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON: return "POWERON";
    case ESP_RST_EXT: return "EXT";
    case ESP_RST_SW: return "SW";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "INT_WDT";
    case ESP_RST_TASK_WDT: return "TASK_WDT";
    case ESP_RST_WDT: return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    default: return "UNKNOWN";
  }
}

bool due(uint32_t last, bool tried) { return !tried || millis() - last >= RETRY_INTERVAL_MS; }

void logNext(int64_t now) {
  int64_t t = nextSlotAt + JST_OFFSET_SEC;
  struct tm tmv;
  time_t tt = (time_t)t;
  gmtime_r(&tt, &tmv);
  char buf[32];
  strftime(buf, sizeof buf, "%FT%T", &tmv);
  Serial.printf("[cycle] %snext=%s+09:00\n", inWindow(now, WINDOW) ? "" : "outside window ", buf);
}

void failCycle() {
  if (policy.onFailureShouldRestart()) {
    Serial.println("[recover] consecutive_failures=6 restarting");
    Serial.flush();
    esp_restart();
  }
}

void runCycle() {
  esp_task_wdt_reset();
  if (!wifiIsConnected()) {
    Serial.println("[wifi] not connected, skip cycle");
    failCycle();
    return;
  }
  camera_fb_t* fb = cameraCapture();
  esp_task_wdt_reset();
  if (!fb) {
    Serial.println("[capture] failed");
    failCycle();
    return;
  }
  int64_t capturedAt = time(nullptr);
  Serial.printf("[capture] ok bytes=%u exposure=%d captured_at=%lld\n", (unsigned)fb->len, exposure,
                (long long)capturedAt);
  DeviceStatus st = {WiFi.RSSI(), (uint32_t)(millis() / 1000), resetReason, policy.consecutive,
                     FIRMWARE_VERSION};
  UploadResult r = upload(fb->buf, fb->len, exposure, capturedAt, st);
  esp_camera_fb_return(fb);
  esp_task_wdt_reset();

  if (r.transportOk && r.ingest.outcome == IngestOutcome::Ok) {
    policy.onSuccess();
    if (r.ingest.hasExposure) {
      exposure = r.ingest.exposure;
      cameraSetExposure(exposure);
    }
  } else {
    if (r.transportOk && r.ingest.outcome == IngestOutcome::InvalidCapturedAt) clockInvalidate();
    failCycle();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  resetReason = resetReasonName(esp_reset_reason());
  Serial.printf("[boot] reset_reason=%s firmware=%s\n", resetReason, FIRMWARE_VERSION);

  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(nullptr);

  if (!cameraInit(exposure)) {
    Serial.println("[camera] restarting in 10s");
    for (int i = 0; i < 10; i++) {
      esp_task_wdt_reset();
      delay(1000);
    }
    esp_restart();
  }
}

void loop() {
  esp_task_wdt_reset();

  if (!wifiIsConnected() && due(lastWifiTryMs, wifiTried)) {
    wifiTried = true;
    wifiConnect(WIFI_TIMEOUT_MS);
    lastWifiTryMs = millis();
    esp_task_wdt_reset();
  }

  // 監視時間外に復帰しても疎通を確かめられるよう、接続の立ち上がりごとに1回送る
  bool wifiUp = wifiIsConnected();
  if (wifiUp && !wifiWasUp) probePending = true;
  wifiWasUp = wifiUp;

  if (wifiUp) {
    if (!sntpStarted) {
      clockBegin();
      sntpStarted = true;
    }
    // 同期済みなら clockEnsure は即戻る。未同期のときだけ60秒あけて試す
    if (clockSource() != ClockSource::None || due(lastClockTryMs, clockTried)) {
      clockTried = true;
      clockEnsure();
      lastClockTryMs = millis();
      esp_task_wdt_reset();
    }
  }

  if (clockSource() != ClockSource::None) {
    int64_t now = time(nullptr);
    if (!slotInitialized) {
      slotInitialized = true;
      // 設置時の確認のため、監視時間内なら区切りを待たず1枚目を撮る
      nextSlotAt = inWindow(now, WINDOW) ? now : nextSlot(now, CAPTURE_INTERVAL_SEC, WINDOW);
      if (nextSlotAt != now) logNext(now);
    }
    bool slotDue = now >= nextSlotAt && inWindow(now, WINDOW);
    if (slotDue || probePending) {
      if (!slotDue) Serial.println("[probe] send on reconnect");
      probePending = false;  // 区切りの1枚が確認送信を兼ねる
      runCycle();
      now = time(nullptr);
      nextSlotAt = nextSlot(now + 1, CAPTURE_INTERVAL_SEC, WINDOW);
      logNext(now);
    } else if (now >= nextSlotAt) {
      nextSlotAt = nextSlot(now, CAPTURE_INTERVAL_SEC, WINDOW);
      logNext(now);
    }
  }

  delay(1000);
}
