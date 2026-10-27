#include "uploader.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "cms_root_ca.h"
#include "config.h"

namespace {

constexpr size_t BODY_MAX = 256;

const char* outcomeName(IngestOutcome o) {
  switch (o) {
    case IngestOutcome::Ok: return "ok";
    case IngestOutcome::InvalidCapturedAt: return "invalid_captured_at";
    case IngestOutcome::InvalidExposure: return "invalid_exposure";
    case IngestOutcome::Unauthorized: return "unauthorized";
    case IngestOutcome::TooLarge: return "too_large";
    case IngestOutcome::NotJpeg: return "not_jpeg";
    case IngestOutcome::ServerError: return "server_error";
    default: return "unexpected";
  }
}

}  // namespace

UploadResult upload(const uint8_t* jpeg, size_t len, int exposure, int64_t capturedAt,
                    const DeviceStatus& status) {
  UploadResult r = {false, 0, {IngestOutcome::UnexpectedStatus, false, 0}};

  if (!isHttpsUrl(CMS_INGEST_URL)) {
    Serial.println("[upload] refused non-https url");
    return r;
  }

  uint32_t start = millis();
  WiFiClientSecure client;
  client.setCACert(CMS_ROOT_CA_PEM);
  client.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(20000);
  http.setReuse(false);
  if (!http.begin(client, CMS_INGEST_URL)) {
    Serial.printf("[upload] transport error rssi=%d\n", status.rssi);
    return r;
  }
  http.addHeader("Authorization", String("Bearer ") + DEVICE_TOKEN);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-Exposure", String(exposure));
  http.addHeader("X-Captured-At", String((long long)capturedAt));
  http.addHeader("X-Device-Rssi", String(status.rssi));
  http.addHeader("X-Device-Uptime", String((unsigned long)status.uptimeSec));
  http.addHeader("X-Device-Reset-Reason", status.resetReason);
  http.addHeader("X-Device-Failures", String(status.failures));
  http.addHeader("X-Device-Firmware", status.firmware);

  // POST は本文を書き換えない(uint8_t* 引数がconstでないだけ)
  int code = http.POST(const_cast<uint8_t*>(jpeg), len);
  if (code <= 0) {
    http.end();
    Serial.printf("[upload] transport error rssi=%d ms=%lu\n", status.rssi, (unsigned long)(millis() - start));
    return r;
  }

  // chunked 応答でも復号された本文が要るので getString を使い、解釈に渡す長さだけ切る
  String body = http.getString();
  http.end();
  r.transportOk = true;
  r.status = code;
  r.ingest = parseIngestResponse(code, body.c_str(), body.length() < BODY_MAX ? body.length() : BODY_MAX);

  unsigned long ms = millis() - start;
  if (r.ingest.outcome == IngestOutcome::Ok) {
    if (r.ingest.hasExposure)
      Serial.printf("[upload] ok status=%d exposure=%d rssi=%d ms=%lu\n", code, r.ingest.exposure, status.rssi, ms);
    else
      Serial.printf("[upload] ok status=%d rssi=%d ms=%lu\n", code, status.rssi, ms);
  } else {
    Serial.printf("[upload] status=%d %s rssi=%d\n", code, outcomeName(r.ingest.outcome), status.rssi);
  }
  return r;
}
