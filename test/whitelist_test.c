#include <hiredis/hiredis.h>
#include "../src/repsheet.h"
#include "../src/whitelist.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void whitelist_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void whitelist_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(is_ip_whitelisted_true_test)
{
  char value[MAX_REASON_LENGTH];
  whitelist(context, "1.1.1.1", IP, "Is IP Whitelisted Test");
  int response = is_ip_whitelisted(context, "1.1.1.1", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Is IP Whitelisted Test");
}
END_TEST

START_TEST(is_ip_whitelisted_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_ip_whitelisted(context, "1.1.1.1", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_user_whitelisted_true_test)
{
  char value[MAX_REASON_LENGTH];
  whitelist(context, "repsheet", USER, "Is User Whitelisted Test");
  int response = is_user_whitelisted(context, "repsheet", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Is User Whitelisted Test");
}
END_TEST

START_TEST(is_user_whitelisted_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_user_whitelisted(context, "repsheet", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_ip_whitelisted_in_cidr_true_test)
{
  char value[MAX_REASON_LENGTH];
  whitelist(context, "10.0.0.0/24", BLOCK, "CIDR 24 Test");
  int response = is_ip_whitelisted(context, "10.0.0.15", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 24 Test");
}
END_TEST

START_TEST(is_ip_whitelisted_in_cidr_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_ip_whitelisted(context, "10.0.0.15", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

Suite *make_whitelist_suite(void) {
  Suite *suite = suite_create("Whitelist Suite");

  TCase *tc_ip_whitelist = tcase_create("IP list");
  tcase_add_checked_fixture(tc_ip_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_ip_whitelist, is_ip_whitelisted_true_test);
  tcase_add_test(tc_ip_whitelist, is_ip_whitelisted_false_test);
  suite_add_tcase(suite, tc_ip_whitelist);

  TCase *tc_user_whitelist = tcase_create("User list");
  tcase_add_checked_fixture(tc_user_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_user_whitelist, is_user_whitelisted_true_test);
  tcase_add_test(tc_user_whitelist, is_user_whitelisted_false_test);
  suite_add_tcase(suite, tc_user_whitelist);

  TCase *tc_cidr_whitelist = tcase_create("CIDR list");
  tcase_add_checked_fixture(tc_cidr_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_cidr_whitelist, is_ip_whitelisted_in_cidr_true_test);
  tcase_add_test(tc_cidr_whitelist, is_ip_whitelisted_in_cidr_false_test);
  suite_add_tcase(suite, tc_cidr_whitelist);

  return suite;
}
