#include "../src/librepsheet.h"
#include "test_suite.h"

START_TEST(connection_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  ck_assert(context);
  ck_assert_int_eq(context->err, 0);
}
END_TEST

Suite *make_librepsheet_suite(void) {
  Suite *suite = suite_create("librepsheet");

  TCase *tc_librepsheet = tcase_create("wiring");
  tcase_add_test(tc_librepsheet, connection_test);
  suite_add_tcase(suite, tc_librepsheet);

  return suite;
}
