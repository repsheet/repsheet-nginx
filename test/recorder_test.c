#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/recorder.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void recorder_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void recorder_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(record_test)
{
  record(context, "4/23/2014", "airpair", "GET", "/airpair", NULL, 5, 10000, "1.1.1.1");

  reply = redisCommand(context, "EXISTS 1.1.1.1:requests");
  ck_assert_int_eq(reply->integer, 1);

  reply = redisCommand(context, "TTL 1.1.1.1:requests");
  ck_assert_int_eq(reply->integer, 10000);

  reply = redisCommand(context, "LPOP 1.1.1.1:requests");
  ck_assert_str_eq(reply->str, "4/23/2014, airpair, GET, /airpair, -");
}
END_TEST

START_TEST(record_handles_null_values)
{
  record(context, NULL, NULL, NULL, NULL, NULL, 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "-, -, -, -, -");
}
END_TEST

START_TEST(record_properly_records_uri)
{
  record(context, NULL, NULL, NULL, "/", NULL, 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "-, -, -, /, -");
}
END_TEST

START_TEST(record_properly_records_arguments)
{
  record(context, NULL, NULL, NULL, NULL, "foo=bar", 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "-, -, -, -, foo=bar");
}
END_TEST

START_TEST(record_properly_records_timestamp)
{
  record(context, "12345", NULL, NULL, NULL, NULL, 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "12345, -, -, -, -");
}
END_TEST

START_TEST(record_properly_records_user_agent)
{
  record(context, NULL, "A User Agent", NULL, NULL, NULL, 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "-, A User Agent, -, -, -");
}
END_TEST

START_TEST(record_properly_records_http_method)
{
  record(context, NULL, NULL, "GET", NULL, NULL, 5, 200, "1.1.1.1");

  reply = redisCommand(context, "LRANGE 1.1.1.1:requests 0 0");
  ck_assert_str_eq(reply->element[0]->str, "-, -, GET, -, -");
}
END_TEST

Suite *make_recorder_suite(void) {
  Suite *suite = suite_create("Recorder Suite");

  TCase *tc_recorder = tcase_create("Recorder");
  tcase_add_checked_fixture(tc_recorder, recorder_setup, recorder_teardown);
  tcase_add_test(tc_recorder, record_test);
  tcase_add_test(tc_recorder, record_handles_null_values);
  tcase_add_test(tc_recorder, record_properly_records_uri);
  tcase_add_test(tc_recorder, record_properly_records_arguments);
  tcase_add_test(tc_recorder, record_properly_records_timestamp);
  tcase_add_test(tc_recorder, record_properly_records_user_agent);
  tcase_add_test(tc_recorder, record_properly_records_http_method);
  suite_add_tcase(suite, tc_recorder);

  return suite;
}
