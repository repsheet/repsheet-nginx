#include "../src/cidr.h"
#include "check.h"

START_TEST(class_c)
{
  char *block = "10.0.0.0/24";
  int in = ip_address_to_integer("10.0.0.50");
  int lower = ip_address_to_integer("10.0.0.0");
  int upper = ip_address_to_integer("10.0.0.255");

  ck_assert_int_eq(1, cidr_contains(block, in));
  ck_assert_int_eq(1, cidr_contains(block, lower));
  ck_assert_int_eq(1, cidr_contains(block, upper));
}
END_TEST

START_TEST(class_c_excludes)
{
  char *block = "10.0.0.0/24";
  char *bad_value = "foo";
  char *short_block = "0.0.0/24";
  char *bad_mask = "10.0.0.0/33";
  char *no_mask = "10.0.0.0";

  int in = ip_address_to_integer("10.0.0.50");
  int out = ip_address_to_integer("10.0.1.50");
  int too_large = ip_address_to_integer("10.0.0.257");
  int too_long = ip_address_to_integer("10.0.0.1024");

  ck_assert_int_eq(BAD_CIDR_BLOCK, cidr_contains(bad_value, in));
  ck_assert_int_eq(BAD_CIDR_BLOCK, cidr_contains(short_block, in));
  ck_assert_int_eq(BAD_CIDR_BLOCK, cidr_contains(bad_mask, in));
  ck_assert_int_eq(BAD_CIDR_BLOCK, cidr_contains(no_mask, in));

  ck_assert_int_eq(0, cidr_contains(block, out));
  ck_assert_int_eq(BAD_ADDRESS, cidr_contains(block, too_large));
  ck_assert_int_eq(BAD_ADDRESS, cidr_contains(block, too_long));
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
