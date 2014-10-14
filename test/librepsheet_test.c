#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "test_suite.h"
#include "assert.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
  }
  redisFree(context);
}

START_TEST(get_redis_context_failure_test)
{
  context = get_redis_context("localhost", 12345, 0);
  ck_assert(context == NULL);
}
END_TEST

START_TEST(check_connection_test)
{
  context = get_redis_context("localhost", 6379, 0);
  ck_assert_int_eq(LIBREPSHEET_OK, check_connection(context));
}
END_TEST

START_TEST(check_connection_failure_test)
{
  context = get_redis_context("localhost", 12345, 0);
  ck_assert_int_eq(DISCONNECTED, check_connection(context));
}
END_TEST

START_TEST(increment_rule_count_test)
{
  increment_rule_count(context, "1.1.1.1", "950001");
  reply = redisCommand(context, "ZRANGE 1.1.1.1:detected 0 -1");

  ck_assert_int_eq(reply->elements, 1);
  ck_assert_str_eq(reply->element[0]->str, "950001");

}
END_TEST

START_TEST(mark_actor_test)
{
  mark_actor(context, "1.1.1.1", IP, "IP Mark Actor Test");
  mark_actor(context, "repsheet", USER, "User Mark Actor Test");

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip");
  ck_assert_str_eq(reply->str, "IP Mark Actor Test");

  reply = redisCommand(context, "GET repsheet:repsheet:users");
  ck_assert_str_eq(reply->str, "User Mark Actor Test");
}
END_TEST



START_TEST(expire_test)
{
  mark_actor(context, "1.1.1.1", IP, "Expire Test");
  expire(context, "1.1.1.1", "repsheet:ip", 200);
  reply = redisCommand(context, "TTL 1.1.1.1:repsheet:ip");

  ck_assert_int_eq(reply->integer, 200);
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

START_TEST(actor_status_test)
{
  char value[MAX_REASON_LENGTH];

  whitelist_actor(context, "1.1.1.1", IP, "IP Whitelist Actor Status");
  whitelist_actor(context, "whitelist", USER, "User Whitelist Actor Status");
  blacklist_actor(context, "1.1.1.2", IP, "IP Blacklist Actor Status");
  blacklist_actor(context, "blacklist", USER, "User Blacklist Actor Status");
  mark_actor(context, "1.1.1.3", IP, "IP Marked Actor Status");
  mark_actor(context, "marked", USER, "User Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "1.1.1.1", IP, value), WHITELISTED);
  ck_assert_str_eq(value, "IP Whitelist Actor Status");
  ck_assert_int_eq(actor_status(context, "1.1.1.2", IP, value), BLACKLISTED);
  ck_assert_str_eq(value, "IP Blacklist Actor Status");
  ck_assert_int_eq(actor_status(context, "1.1.1.3", IP, value), MARKED);
  ck_assert_str_eq(value, "IP Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "whitelist", USER, value), WHITELISTED);
  ck_assert_str_eq(value, "User Whitelist Actor Status");
  ck_assert_int_eq(actor_status(context, "blacklist", USER, value), BLACKLISTED);
  ck_assert_str_eq(value, "User Blacklist Actor Status");
  ck_assert_int_eq(actor_status(context, "marked", USER, value), MARKED);
  ck_assert_str_eq(value, "User Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "good", UNSUPPORTED, value), UNSUPPORTED);
}
END_TEST

START_TEST(blacklist_and_expire_ip_test)
{
  blacklist_and_expire(context, IP, "1.1.1.1", 200, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "TTL 1.1.1.1:repsheet:ip:blacklist");
  ck_assert_int_eq(reply->integer, 200);

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip:blacklist");
  ck_assert_str_eq(reply->str, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "SISMEMBER repsheet:ip:blacklist:history 1.1.1.1");
  ck_assert_int_eq(reply->integer, 1);
}
END_TEST

START_TEST(blacklist_and_expire_user_test)
{
  blacklist_and_expire(context, USER, "test", 200, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "TTL test:repsheet:users:blacklist");
  ck_assert_int_eq(reply->integer, 200);

  reply = redisCommand(context, "GET test:repsheet:users:blacklist");
  ck_assert_str_eq(reply->str, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "SISMEMBER repsheet:users:blacklist:history test");
  ck_assert_int_eq(reply->integer, 1);
}
END_TEST

START_TEST(blacklist_reason_ip_found_test)
{
  char value[MAX_REASON_LENGTH];
  int response;

  blacklist_and_expire(context, IP, "1.1.1.1", 200, "Blacklist Reason IP Found Test");
  response = is_ip_blacklisted(context, "1.1.1.1", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Blacklist Reason IP Found Test");
}
END_TEST

START_TEST(blacklist_reason_ip_not_found_test)
{
  char value[MAX_REASON_LENGTH];
  int response;

  blacklist_and_expire(context, IP, "1.1.1.1", 200, "Blacklist Reason IP Not Found Test");
  response = is_ip_blacklisted(context, "1.1.1.2", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

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

START_TEST(returns_null_when_headers_are_null)
{
  fail_unless(remote_address(NULL, NULL) == NULL);
}
END_TEST

START_TEST(processes_a_single_address) {
  ck_assert_str_eq(remote_address("192.168.1.100", NULL), "192.168.1.100");
}
END_TEST

START_TEST(extract_only_the_first_ip_address)
{
  ck_assert_str_eq(remote_address("1.1.1.1", "8.8.8.8 12.34.56.78, 212.23.230.15"), "8.8.8.8");
}
END_TEST

START_TEST(ignores_user_generated_noise)
{
  ck_assert_str_eq(remote_address("1.1.1.1", "\\x5000 8.8.8.8, 12.23.45.67"), "8.8.8.8");
  ck_assert_str_eq(remote_address("1.1.1.1", "This is not an IP address 8.8.8.8, 12.23.45.67"), "8.8.8.8");
  ck_assert_str_eq(remote_address("1.1.1.1", "999.999.999.999, 8.8.8.8, 12.23.45.67"), "8.8.8.8");
}
END_TEST

START_TEST(handles_a_single_event)
{
  char *waf_events = "X-WAF-Events: TX: / 999935-Detects common comment types-WEB_ATTACK/INJECTION-ARGS:test";
  int i, m = matches(waf_events);

  char **events;

  events = malloc(m * sizeof(char*));
  for(i = 0; i < m; i++) {
    events[i] = malloc(i * sizeof(char));
  }

  process_mod_security_headers(waf_events, events);

  ck_assert_str_eq(events[0], "999935");
}
END_TEST

START_TEST(handles_multiple_events)
{
  char *waf_events = "X-WAF-Events: TX: / 999935-Detects common comment types-WEB_ATTACK/INJECTION-ARGS:test, TX:999923-Detects JavaScript location/document property access and window access obfuscation-WEB_ATTACK/INJECTION-REQUEST_URI_RAW, TX:950001- WEB_ATTACK/SQL_INJECTION-ARGS:test";

  int i, m = matches(waf_events);

  char **events;

  events = malloc(m * sizeof(char*));
  for(i = 0; i  < m; i++) {
    events[i] = malloc(i * sizeof(char));
  }

  process_mod_security_headers(waf_events, events);

  ck_assert_str_eq("999935", events[0]);
  ck_assert_str_eq("999923", events[1]);
  ck_assert_str_eq("950001", events[2]);
}
END_TEST

START_TEST(correctly_parses_anonmaly_total) {
  char *input = "Total=5;SQLi=;XSS=;";
  int score = modsecurity_total(input);

  ck_assert_int_eq(5, score);
}
END_TEST

START_TEST(returns_0_when_no_values_exist) {
  char *input = "Total=;SQLi=;XSS=;";
  int score = modsecurity_total(input);

  ck_assert_int_eq(0, score);
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

START_TEST(country_status_marked_test)
{
  redisCommand(context, "SADD repsheet:countries:marked KP");
  ck_assert_int_eq(country_status(context, "KP"), MARKED);
}
END_TEST

START_TEST(country_status_good_test)
{
  ck_assert_int_eq(country_status(context, "US"), LIBREPSHEET_OK);
}
END_TEST

Suite *make_librepsheet_connection_suite(void) {
  Suite *suite = suite_create("librepsheet connection");

  TCase *tc_redis_connection = tcase_create("redis connection");
  tcase_add_test(tc_redis_connection, get_redis_context_failure_test);
  tcase_add_test(tc_redis_connection, check_connection_test);
  tcase_add_test(tc_redis_connection, check_connection_failure_test);
  suite_add_tcase(suite, tc_redis_connection);

  TCase *tc_connection_operations = tcase_create("connection operations");
  tcase_add_checked_fixture(tc_connection_operations, setup, teardown);

  tcase_add_test(tc_connection_operations, increment_rule_count_test);

  tcase_add_test(tc_connection_operations, mark_actor_test);

  tcase_add_test(tc_connection_operations, is_ip_marked_test);
  tcase_add_test(tc_connection_operations, is_ip_marked_false_test);
  tcase_add_test(tc_connection_operations, is_user_marked_test);
  tcase_add_test(tc_connection_operations, is_user_marked_false_test);
  tcase_add_test(tc_connection_operations, actor_status_test);

  tcase_add_test(tc_connection_operations, expire_test);
  tcase_add_test(tc_connection_operations, blacklist_and_expire_ip_test);
  tcase_add_test(tc_connection_operations, blacklist_and_expire_user_test);
  tcase_add_test(tc_connection_operations, blacklist_reason_ip_found_test);
  tcase_add_test(tc_connection_operations, blacklist_reason_ip_not_found_test);

  tcase_add_test(tc_connection_operations, record_test);
  tcase_add_test(tc_connection_operations, record_handles_null_values);
  tcase_add_test(tc_connection_operations, record_properly_records_uri);
  tcase_add_test(tc_connection_operations, record_properly_records_arguments);
  tcase_add_test(tc_connection_operations, record_properly_records_timestamp);
  tcase_add_test(tc_connection_operations, record_properly_records_user_agent);
  tcase_add_test(tc_connection_operations, record_properly_records_http_method);

  tcase_add_test(tc_connection_operations, is_country_marked_true_test);
  tcase_add_test(tc_connection_operations, is_country_marked_false_test);
  tcase_add_test(tc_connection_operations, country_status_marked_test);
  tcase_add_test(tc_connection_operations, country_status_good_test);

  suite_add_tcase(suite, tc_connection_operations);

  TCase *tc_proxy = tcase_create("Standard");
  tcase_add_test(tc_proxy, returns_null_when_headers_are_null);
  tcase_add_test(tc_proxy, processes_a_single_address);
  tcase_add_test(tc_proxy, extract_only_the_first_ip_address);
  suite_add_tcase(suite, tc_proxy);

  TCase *tc_proxy_malicious = tcase_create("Malicious");
  tcase_add_test(tc_proxy_malicious, ignores_user_generated_noise);
  suite_add_tcase(suite, tc_proxy_malicious);

  TCase *tc_mod_security = tcase_create("ModSecurity Headers");
  tcase_add_test(tc_mod_security, handles_a_single_event);
  tcase_add_test(tc_mod_security, handles_multiple_events);
  tcase_add_test(tc_mod_security, correctly_parses_anonmaly_total);
  tcase_add_test(tc_mod_security, returns_0_when_no_values_exist);
  suite_add_tcase(suite, tc_mod_security);

  return suite;
}
