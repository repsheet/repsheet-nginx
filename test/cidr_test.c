#include "../src/cidr.h"
#include "check.h"

START_TEST(class_c)
{
  char *block = "10.0.0.0/24";
  char *in = "10.0.0.50";
  char *lower = "10.0.0.0";
  char *upper = "10.0.0.256";

  ck_assert_int_eq(1, cidr_contains(block, in));
  ck_assert_int_eq(1, cidr_contains(block, lower));
  ck_assert_int_eq(1, cidr_contains(block, upper));
}
END_TEST

START_TEST(class_c_excludes)
{
  char *block = "10.0.0.0/24";
  char *out = "10.0.1.50";
  //  char *invalid = "10.0.0.257";

  ck_assert_int_eq(0, cidr_contains(block, out));
  //ck_assert_int_eq(0, cidr_contains(block, invalid));
}
END_TEST

Suite *make_cidr_suite(void) {
  Suite *suite = suite_create("CIDR suite");

  TCase *tc_cidr = tcase_create("CIDR contains");
  tcase_add_test(tc_cidr, class_c);
  suite_add_tcase(suite, tc_cidr);

  TCase *tc_cidr_excludes = tcase_create("CIDR excludes");
  tcase_add_test(tc_cidr_excludes, class_c_excludes);
  suite_add_tcase(suite, tc_cidr_excludes);

  return suite;
}
