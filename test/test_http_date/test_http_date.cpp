#include <unity.h>

#include "http_date.h"

void setUp() {}
void tearDown() {}

static void expectOk(const char* s, int64_t want) {
  int64_t out = 0;
  TEST_ASSERT_TRUE_MESSAGE(parseHttpDate(s, &out), s);
  TEST_ASSERT_TRUE_MESSAGE(out == want, s);
}

static void expectFail(const char* s) {
  int64_t out = 0;
  TEST_ASSERT_FALSE_MESSAGE(parseHttpDate(s, &out), s);
}

void test_valid() {
  expectOk("Thu, 01 Jan 1970 00:00:00 GMT", 0);
  expectOk("Wed, 21 Oct 2026 07:28:00 GMT", 1792567680);
  expectOk("Sat, 31 Dec 2022 23:59:59 GMT", 1672531199);
}

void test_leapDay() {
  expectOk("Thu, 29 Feb 2024 12:00:00 GMT", 1709208000);
  expectOk("Fri, 01 Mar 2024 00:00:00 GMT", 1709251200);
}

void test_invalid() {
  expectFail("");
  expectFail("garbage");
  expectFail("Wed, 21 Oct 2026 07:28:00");
  expectFail("Wed, 21 Oct 2026 07:28:00 JST");
  expectFail("Wed, 21 Foo 2026 07:28:00 GMT");
  expectFail("Wed, 29 Feb 2025 07:28:00 GMT");
  expectFail("Wed, 32 Oct 2026 07:28:00 GMT");
  expectFail("Wed, 21 Oct 2026 24:28:00 GMT");
  expectFail("Wed, 21 Oct 2026 07:60:00 GMT");
  expectFail("Wed, 21 Oct 2026 07:28:00 GMTx");
  int64_t out = 0;
  TEST_ASSERT_FALSE(parseHttpDate(nullptr, &out));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid);
  RUN_TEST(test_leapDay);
  RUN_TEST(test_invalid);
  return UNITY_END();
}
