#include <unity.h>

#include "schedule.h"

void setUp() {}
void tearDown() {}

static const Window W = {9 * 60, 17 * 60 + 30};

// 日本時間 -> Unix秒。2026-10-21 を基準日にする。
static int64_t jst(int day, int h, int m, int s) {
  // 2026-10-01 00:00:00 JST = 2026-09-30 15:00:00 UTC
  const int64_t oct1Utc = 1790780400;
  return oct1Utc + (int64_t)(day - 1) * 86400 + h * 3600 + m * 60 + s;
}

void test_nextSlot_basic() {
  TEST_ASSERT_TRUE(jst(21, 9, 0, 0) == nextSlot(jst(21, 8, 59, 59), 300, W));
  TEST_ASSERT_TRUE(jst(21, 9, 0, 0) == nextSlot(jst(21, 9, 0, 0), 300, W));
  TEST_ASSERT_TRUE(jst(21, 9, 5, 0) == nextSlot(jst(21, 9, 2, 0), 300, W));
  TEST_ASSERT_TRUE(jst(21, 17, 30, 0) == nextSlot(jst(21, 17, 30, 0), 300, W));
  TEST_ASSERT_TRUE(jst(22, 9, 0, 0) == nextSlot(jst(21, 17, 30, 1), 300, W));
}

void test_nextSlot_dateBoundary() {
  TEST_ASSERT_TRUE(jst(31, 9, 0, 0) == nextSlot(jst(30, 20, 0, 0), 300, W));
  // 月末: 10/31 -> 11/1 (= 10月の32日目)
  TEST_ASSERT_TRUE(jst(32, 9, 0, 0) == nextSlot(jst(31, 18, 0, 0), 300, W));
  TEST_ASSERT_TRUE(jst(21, 9, 0, 0) == nextSlot(jst(21, 0, 0, 0), 300, W));
}

void test_nextSlot_intervals() {
  TEST_ASSERT_TRUE(jst(21, 9, 10, 0) == nextSlot(jst(21, 9, 1, 0), 600, W));
  TEST_ASSERT_TRUE(jst(21, 10, 0, 0) == nextSlot(jst(21, 9, 0, 1), 3600, W));
  TEST_ASSERT_TRUE(jst(21, 17, 0, 0) == nextSlot(jst(21, 16, 30, 0), 3600, W));
  // 17:00の次は翌日9:00(18:00は監視時間外)
  TEST_ASSERT_TRUE(jst(22, 9, 0, 0) == nextSlot(jst(21, 17, 0, 1), 3600, W));
}

void test_nextSlot_unalignedStart() {
  Window w = {9 * 60 + 2, 17 * 60};
  TEST_ASSERT_TRUE(jst(21, 9, 5, 0) == nextSlot(jst(21, 8, 0, 0), 300, w));
}

void test_inWindow() {
  TEST_ASSERT_FALSE(inWindow(jst(21, 8, 59, 59), W));
  TEST_ASSERT_TRUE(inWindow(jst(21, 9, 0, 0), W));
  TEST_ASSERT_TRUE(inWindow(jst(21, 17, 30, 0), W));
  TEST_ASSERT_FALSE(inWindow(jst(21, 17, 30, 1), W));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nextSlot_basic);
  RUN_TEST(test_nextSlot_dateBoundary);
  RUN_TEST(test_nextSlot_intervals);
  RUN_TEST(test_nextSlot_unalignedStart);
  RUN_TEST(test_inWindow);
  return UNITY_END();
}
