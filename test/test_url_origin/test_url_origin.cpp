#include <unity.h>

#include "url_origin.h"

void setUp() {}
void tearDown() {}

void test_origin() {
  TEST_ASSERT_EQUAL(23, httpsOriginLen("https://cms.example.com/api/x"));
  TEST_ASSERT_EQUAL(23, httpsOriginLen("https://cms.example.com"));
  TEST_ASSERT_EQUAL(28, httpsOriginLen("https://cms.example.com:8443/a"));
  TEST_ASSERT_EQUAL(23, httpsOriginLen("https://cms.example.com?q=1"));
  TEST_ASSERT_EQUAL(23, httpsOriginLen("https://cms.example.com#f"));
}

void test_invalid() {
  TEST_ASSERT_EQUAL(0, httpsOriginLen("http://cms.example.com/"));
  TEST_ASSERT_EQUAL(0, httpsOriginLen("https:///x"));
  TEST_ASSERT_EQUAL(0, httpsOriginLen(nullptr));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_origin);
  RUN_TEST(test_invalid);
  return UNITY_END();
}
