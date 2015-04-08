#include <arpa/inet.h>
#include "../src/xff.h"
#include "../src/repsheet.h"
#include "check.h"

START_TEST(returns_null_when_headers_are_null)
{
  fail_unless(remote_address(NULL, NULL, NULL) == NIL);
}
END_TEST

START_TEST(processes_a_single_address) {
  char address[INET6_ADDRSTRLEN];
  int result = remote_address("192.168.1.100", NULL, address);
  ck_assert_int_eq(result, LIBREPSHEET_OK);
  ck_assert_str_eq(address, "192.168.1.100");
}
END_TEST

START_TEST(extract_only_the_first_ip_address)
{
  char address[INET6_ADDRSTRLEN];
  int result = remote_address("1.1.1.1", "8.8.8.8 12.34.56.78, 212.23.230.15", address);
  ck_assert_int_eq(result, LIBREPSHEET_OK);
  ck_assert_str_eq(address, "8.8.8.8");
}
END_TEST

START_TEST(ignores_user_generated_noise)
{
  char address[INET6_ADDRSTRLEN];
  int result;

  result = remote_address("1.1.1.1", "\\x5000 8.8.8.8, 12.23.45.67", address);
  ck_assert_int_eq(result, BLACKLISTED);

  result = remote_address("1.1.1.1", "This is not an IP address 8.8.8.8, 12.23.45.67", address);
  ck_assert_int_eq(result, BLACKLISTED);

  result = remote_address("1.1.1.1", "999.999.999.999, 8.8.8.8, 12.23.45.67", address);
  ck_assert_int_eq(result, BLACKLISTED);
}
END_TEST

Suite *make_xff_suite(void) {
  Suite *suite = suite_create("XFF Suite");

  TCase *tc_xff = tcase_create("Parsing");
  tcase_add_test(tc_xff, returns_null_when_headers_are_null);
  tcase_add_test(tc_xff, processes_a_single_address);
  tcase_add_test(tc_xff, extract_only_the_first_ip_address);
  tcase_add_test(tc_xff, ignores_user_generated_noise);
  suite_add_tcase(suite, tc_xff);

  return suite;
}
