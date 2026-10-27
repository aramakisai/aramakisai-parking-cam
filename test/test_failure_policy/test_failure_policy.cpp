#include <unity.h>

#include "failure_policy.h"

void setUp() {}
void tearDown() {}

void test_restartAtSixth() {
  FailurePolicy p;
  for (int i = 0; i < 5; i++) TEST_ASSERT_FALSE(p.onFailureShouldRestart());
  TEST_ASSERT_TRUE(p.onFailureShouldRestart());
}

void test_successResets() {
  FailurePolicy p;
  for (int i = 0; i < 5; i++) p.onFailureShouldRestart();
  p.onSuccess();
  TEST_ASSERT_EQUAL_INT(0, p.consecutive);
  for (int i = 0; i < 5; i++) TEST_ASSERT_FALSE(p.onFailureShouldRestart());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_restartAtSixth);
  RUN_TEST(test_successResets);
  return UNITY_END();
}
