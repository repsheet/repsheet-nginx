#include "../src/librepsheet.h"
#include "test_suite.h"

START_TEST(wiring_works)
{
  ck_assert_int_eq(wiring(), 1);
}
END_TEST

Suite *make_librepsheet_suite(void) {
  Suite *suite = suite_create("librepsheet");

  TCase *tc_librepsheet = tcase_create("wiring");
  tcase_add_test(tc_librepsheet, wiring_works);
  suite_add_tcase(suite, tc_librepsheet);

  return suite;
}
