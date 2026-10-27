#pragma once

constexpr int MAX_CONSECUTIVE_FAILURES = 6;

struct FailurePolicy {
  int consecutive = 0;
  void onSuccess() { consecutive = 0; }
  bool onFailureShouldRestart() { return ++consecutive >= MAX_CONSECUTIVE_FAILURES; }
};
