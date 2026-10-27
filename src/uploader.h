#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ingest_response.h"

struct DeviceStatus {
  int rssi;
  uint32_t uptimeSec;
  const char* resetReason;
  int failures;
  const char* firmware;
};

struct UploadResult {
  bool transportOk;
  int status;
  IngestResult ingest;
};

UploadResult upload(const uint8_t* jpeg, size_t len, int exposure, int64_t capturedAt,
                    const DeviceStatus& status);
