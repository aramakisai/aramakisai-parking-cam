#include "wifi_link.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <esp_wpa2.h>

#include "config.h"

namespace {

// 802.1X は TLS ハンドシェイクだけで 20 秒超かかる(GunetEAP の実機)ため 1 回 40 秒待つ
constexpr uint32_t ATTEMPT_MS = 40000;

bool initialized = false;

const char* authName() {
#if WIFI_AUTH == WIFI_AUTH_PSK
  return "psk";
#elif WIFI_AUTH == WIFI_AUTH_EAP_PEAP
  return "peap";
#else
  return "ttls";
#endif
}

void beginAttempt() {
#if WIFI_AUTH == WIFI_AUTH_PSK
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#else
  // WiFi.begin は前回のCA証明書を消さないので毎回消してから設定し直す
  esp_wifi_sta_wpa2_ent_disable();
  esp_wifi_sta_wpa2_ent_clear_ca_cert();
  esp_wifi_sta_wpa2_ent_clear_identity();
  esp_wifi_sta_wpa2_ent_clear_username();
  esp_wifi_sta_wpa2_ent_clear_password();
#ifdef EAP_CA_PEM
  // WiFi.begin の ca_pem 経由だと終端NULを含まない長さになり、どのCAでも接続に失敗する
  esp_wifi_sta_wpa2_ent_set_ca_cert((const unsigned char*)EAP_CA_PEM, strlen(EAP_CA_PEM) + 1);
#else
  Serial.println("[warn] eap ca not set");
#endif
#if WIFI_AUTH == WIFI_AUTH_EAP_TTLS
  esp_wifi_sta_wpa2_ent_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_MSCHAPV2);
  const wpa2_auth_method_t method = WPA2_AUTH_TTLS;
#else
  const wpa2_auth_method_t method = WPA2_AUTH_PEAP;
#endif
  const char* identity = strlen(EAP_IDENTITY) ? EAP_IDENTITY : EAP_USERNAME;
  WiFi.begin(WIFI_SSID, method, identity, EAP_USERNAME, EAP_PASSWORD, NULL);
#endif
}

bool waitConnected(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    if (WiFi.status() == WL_CONNECTED) return true;
    esp_task_wdt_reset();
    delay(200);
  }
  return false;
}

}  // namespace

bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

WifiResult wifiConnect(uint32_t timeoutMs) {
  if (wifiIsConnected()) return WifiResult::Connected;

  if (!initialized) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    initialized = true;
#if (WIFI_AUTH != WIFI_AUTH_PSK) && !defined(EAP_CA_PEM)
    Serial.println("[warn] eap ca not set");
#endif
  }

  Serial.printf("[wifi] connecting ssid=%s auth=%s\n", WIFI_SSID, authName());
  uint32_t perAttempt = timeoutMs / 2 < ATTEMPT_MS ? timeoutMs / 2 : ATTEMPT_MS;
  for (int i = 0; i < 2; i++) {
    beginAttempt();
    if (waitConnected(perAttempt)) {
      Serial.printf("[wifi] connected ip=%s rssi=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      return WifiResult::Connected;
    }
    WiFi.disconnect();
  }
  Serial.println("[wifi] connect timeout");
  return WifiResult::Timeout;
}
