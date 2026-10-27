#pragma once

#include <stddef.h>

enum class IngestOutcome {
  Ok,
  InvalidCapturedAt,
  InvalidExposure,
  Unauthorized,
  TooLarge,
  NotJpeg,
  ServerError,
  UnexpectedStatus
};

struct IngestResult {
  IngestOutcome outcome;
  bool hasExposure;  // Okのときだけ意味を持つ
  int exposure;      // -2〜2
};

IngestResult parseIngestResponse(int status, const char* body, size_t len);
