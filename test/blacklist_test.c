#include <hiredis/hiredis.h>
#include "../src/repsheet.h"
#include "../src/blacklist.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void blacklist_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void blacklist_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(is_ip_blacklisted_true_test)
{
  char value[MAX_REASON_LENGTH];
  blacklist(context, "1.1.1.1", IP, "Is IP Blacklisted Test");
  int response = is_ip_blacklisted(context, "1.1.1.1", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Is IP Blacklisted Test");
}
END_TEST

START_TEST(is_ip_blacklisted_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_ip_blacklisted(context, "1.1.1.1", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_user_blacklisted_true_test)
{
  char value[MAX_REASON_LENGTH];
  blacklist(context, "repsheet", USER, "Is User Blacklisted Test");
  int response = is_user_blacklisted(context, "repsheet", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Is User Blacklisted Test");
}
END_TEST

START_TEST(is_user_blacklisted_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_user_blacklisted(context, "repsheet", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_ip_blacklisted_in_cidr_true_test)
{
  char value[MAX_REASON_LENGTH];
  blacklist(context, "10.0.0.0/24", BLOCK, "CIDR 24 Test");
  int response = is_ip_blacklisted(context, "10.0.0.15", value);

  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 24 Test");
}
END_TEST

START_TEST(is_ip_blacklisted_in_cidr_false_test)
{
  char value[MAX_REASON_LENGTH];
  int response = is_ip_blacklisted(context, "10.0.0.15", value);

  ck_assert_int_eq(response, FALSE);
}
END_TEST

Suite *make_blacklist_suite(void) {
  Suite *suite = suite_create("Blacklist Suite");

  TCase *tc_ip_blacklist = tcase_create("IP list");
  tcase_add_checked_fixture(tc_ip_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_ip_blacklist, is_ip_blacklisted_true_test);
  tcase_add_test(tc_ip_blacklist, is_ip_blacklisted_false_test);
  suite_add_tcase(suite, tc_ip_blacklist);

  TCase *tc_user_blacklist = tcase_create("User list");
  tcase_add_checked_fixture(tc_user_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_user_blacklist, is_user_blacklisted_true_test);
  tcase_add_test(tc_user_blacklist, is_user_blacklisted_false_test);
  suite_add_tcase(suite, tc_user_blacklist);

  TCase *tc_cidr_blacklist = tcase_create("CIDR list");
  tcase_add_checked_fixture(tc_cidr_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_cidr_blacklist, is_ip_blacklisted_in_cidr_true_test);
  tcase_add_test(tc_cidr_blacklist, is_ip_blacklisted_in_cidr_false_test);
  suite_add_tcase(suite, tc_cidr_blacklist);

  return suite;
}
