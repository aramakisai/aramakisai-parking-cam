#include <string.h>
#include <unity.h>

#include "ingest_response.h"

void setUp() {}
void tearDown() {}

static IngestResult parse(int status, const char* body) {
  return parseIngestResponse(status, body, body ? strlen(body) : 0);
}

static void expectExposure(const char* body, int want) {
  IngestResult r = parse(200, body);
  TEST_ASSERT_TRUE(r.outcome == IngestOutcome::Ok);
  TEST_ASSERT_TRUE_MESSAGE(r.hasExposure, body);
  TEST_ASSERT_EQUAL_INT(want, r.exposure);
}

static void expectNoExposure(const char* body) {
  IngestResult r = parse(200, body);
  TEST_ASSERT_TRUE(r.outcome == IngestOutcome::Ok);
  TEST_ASSERT_FALSE_MESSAGE(r.hasExposure, body);
}

void test_exposure_present() {
  expectExposure("{\"exposure\":1}", 1);
  expectExposure("{ \"exposure\" : -2 }", -2);
  expectExposure("{\"exposure\":0}", 0);
  expectExposure("  {\"exposure\":2}\n", 2);
}

void test_exposure_absent() {
  expectNoExposure("{}");
  expectNoExposure("{\"exposure\":3}");
  expectNoExposure("{\"exposure\":-3}");
  expectNoExposure("{\"exposure\":1.5}");
  expectNoExposure("{\"exposure\":null}");
  expectNoExposure("{\"exposure\":\"1\"}");
  expectNoExposure("{\"exposure\":");
  expectNoExposure("{\"exposure\":99999999999999999999}");
  expectNoExposure("not json");
  expectNoExposure("");
}

void test_nullBody() {
  IngestResult r = parseIngestResponse(200, nullptr, 0);
  TEST_ASSERT_TRUE(r.outcome == IngestOutcome::Ok);
  TEST_ASSERT_FALSE(r.hasExposure);
}

void test_400() {
  TEST_ASSERT_TRUE(parse(400, "{\"error\":\"invalid_captured_at\"}").outcome == IngestOutcome::InvalidCapturedAt);
  TEST_ASSERT_TRUE(parse(400, "{\"error\":\"invalid_exposure\"}").outcome == IngestOutcome::InvalidExposure);
  TEST_ASSERT_TRUE(parse(400, "").outcome == IngestOutcome::InvalidExposure);
}

void test_status() {
  TEST_ASSERT_TRUE(parse(401, "").outcome == IngestOutcome::Unauthorized);
  TEST_ASSERT_TRUE(parse(413, "").outcome == IngestOutcome::TooLarge);
  TEST_ASSERT_TRUE(parse(415, "").outcome == IngestOutcome::NotJpeg);
  TEST_ASSERT_TRUE(parse(500, "").outcome == IngestOutcome::ServerError);
  TEST_ASSERT_TRUE(parse(503, "").outcome == IngestOutcome::ServerError);
  TEST_ASSERT_TRUE(parse(302, "").outcome == IngestOutcome::UnexpectedStatus);
  TEST_ASSERT_TRUE(parse(404, "").outcome == IngestOutcome::UnexpectedStatus);
  TEST_ASSERT_TRUE(parse(-1, "").outcome == IngestOutcome::UnexpectedStatus);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_exposure_present);
  RUN_TEST(test_exposure_absent);
  RUN_TEST(test_nullBody);
  RUN_TEST(test_400);
  RUN_TEST(test_status);
  return UNITY_END();
}
