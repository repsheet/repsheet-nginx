#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/marked.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void marked_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void marked_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
  }
  redisFree(context);
}

START_TEST(mark_ip_test)
{
  mark_actor(context, "1.1.1.1", IP, "IP Mark Actor Test");

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip");
  ck_assert_str_eq(reply->str, "IP Mark Actor Test");
}
END_TEST

START_TEST(mark_user_test)
{
  mark_actor(context, "repsheet", USER, "User Mark Actor Test");

  reply = redisCommand(context, "GET repsheet:repsheet:users");
  ck_assert_str_eq(reply->str, "User Mark Actor Test");
}
END_TEST

START_TEST(is_ip_marked_test)
{
  char value[MAX_REASON_LENGTH];

  mark_actor(context, "1.1.1.1", IP, "Is IP Marked Test");
  int response = is_ip_marked(context, "1.1.1.1", value);
  ck_assert_str_eq(value, "Is IP Marked Test");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_ip_marked_false_test)
{
  char value[MAX_REASON_LENGTH];


  int response = is_ip_marked(context, "1.1.1.1", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_user_marked_test)
{
  char value[MAX_REASON_LENGTH];

  mark_actor(context, "repsheet", USER, "Is User Marked Test");
  int response = is_user_marked(context, "repsheet", value);
  ck_assert_str_eq(value, "Is User Marked Test");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_user_marked_false_test)
{
  char value[MAX_REASON_LENGTH];

  int response = is_user_marked(context, "repsheet", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_country_marked_true_test)
{
  redisCommand(context, "SADD repsheet:countries:marked KP");
  ck_assert_int_eq(is_country_marked(context, "KP"), TRUE);
}
END_TEST

START_TEST(is_country_marked_false_test)
{
  ck_assert_int_eq(is_country_marked(context, "KP"), FALSE);
}
END_TEST

Suite *make_marked_suite(void) {
  Suite *suite = suite_create("Marked Suite");

  TCase *tc_ip_marked = tcase_create("IP list");
  tcase_add_checked_fixture(tc_ip_marked, marked_setup, marked_teardown);
  tcase_add_test(tc_ip_marked, mark_ip_test);
  tcase_add_test(tc_ip_marked, is_ip_marked_test);
  tcase_add_test(tc_ip_marked, is_ip_marked_false_test);
  suite_add_tcase(suite, tc_ip_marked);

  TCase *tc_user_marked= tcase_create("User list");
  tcase_add_checked_fixture(tc_user_marked, marked_setup, marked_teardown);
  tcase_add_test(tc_user_marked, mark_user_test);
  tcase_add_test(tc_user_marked, is_user_marked_test);
  tcase_add_test(tc_user_marked, is_user_marked_false_test);
  suite_add_tcase(suite, tc_user_marked);

  TCase *tc_cidr_marked = tcase_create("CIDR list");
  tcase_add_checked_fixture(tc_cidr_marked, marked_setup, marked_teardown);

  suite_add_tcase(suite, tc_cidr_marked);

  TCase *tc_country_marked = tcase_create("Country list");
  tcase_add_checked_fixture(tc_country_marked, marked_setup, marked_teardown);
  tcase_add_test(tc_country_marked, is_country_marked_true_test);
  tcase_add_test(tc_country_marked, is_country_marked_false_test);
  suite_add_tcase(suite, tc_country_marked);

  return suite;
}
