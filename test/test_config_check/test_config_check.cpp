#include <unity.h>

#include "config_check.h"

static_assert(isHttpsUrl("https://example.com/x"), "constexpr で評価できること");

void setUp() {}
void tearDown() {}

void test_isNonEmpty() {
  TEST_ASSERT_TRUE(isNonEmpty("a"));
  TEST_ASSERT_FALSE(isNonEmpty(""));
  TEST_ASSERT_FALSE(isNonEmpty(nullptr));
}

void test_isHttpsUrl() {
  TEST_ASSERT_TRUE(isHttpsUrl("https://cms.example.com/api/parking-ingest"));
  TEST_ASSERT_TRUE(isHttpsUrl("https://h"));
  TEST_ASSERT_TRUE(isHttpsUrl("https://h:8443/p"));
  TEST_ASSERT_FALSE(isHttpsUrl("http://cms.example.com/x"));
  TEST_ASSERT_FALSE(isHttpsUrl("HTTPS://cms.example.com/x"));
  TEST_ASSERT_FALSE(isHttpsUrl("https:/cms.example.com"));
  TEST_ASSERT_FALSE(isHttpsUrl("https://"));
  TEST_ASSERT_FALSE(isHttpsUrl("https:///path"));
  TEST_ASSERT_FALSE(isHttpsUrl("https://:443/p"));
  TEST_ASSERT_FALSE(isHttpsUrl(""));
  TEST_ASSERT_FALSE(isHttpsUrl("https"));
  TEST_ASSERT_FALSE(isHttpsUrl(nullptr));
}

void test_isValidInterval() {
  TEST_ASSERT_TRUE(isValidInterval(60));
  TEST_ASSERT_TRUE(isValidInterval(300));
  TEST_ASSERT_TRUE(isValidInterval(600));
  TEST_ASSERT_TRUE(isValidInterval(3600));
  TEST_ASSERT_FALSE(isValidInterval(59));
  TEST_ASSERT_FALSE(isValidInterval(7));
  TEST_ASSERT_FALSE(isValidInterval(0));
  TEST_ASSERT_FALSE(isValidInterval(-300));
  TEST_ASSERT_FALSE(isValidInterval(61));
  TEST_ASSERT_FALSE(isValidInterval(3601));
  TEST_ASSERT_FALSE(isValidInterval(7200));
}

void test_isValidWindow() {
  TEST_ASSERT_TRUE(isValidWindow(540, 1050));
  TEST_ASSERT_TRUE(isValidWindow(0, 1));
  TEST_ASSERT_TRUE(isValidWindow(0, 1439));
  TEST_ASSERT_FALSE(isValidWindow(540, 540));
  TEST_ASSERT_FALSE(isValidWindow(1050, 540));
  TEST_ASSERT_FALSE(isValidWindow(-1, 540));
  TEST_ASSERT_FALSE(isValidWindow(0, 1440));
}

void test_isValidExposure() {
  TEST_ASSERT_TRUE(isValidExposure(-2));
  TEST_ASSERT_TRUE(isValidExposure(0));
  TEST_ASSERT_TRUE(isValidExposure(2));
  TEST_ASSERT_FALSE(isValidExposure(-3));
  TEST_ASSERT_FALSE(isValidExposure(3));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_isNonEmpty);
  RUN_TEST(test_isHttpsUrl);
  RUN_TEST(test_isValidInterval);
  RUN_TEST(test_isValidWindow);
  RUN_TEST(test_isValidExposure);
  return UNITY_END();
}
