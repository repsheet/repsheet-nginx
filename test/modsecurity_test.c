#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/modsecurity.h"
#include "check.h"

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

Suite *make_modsecurity_suite(void) {
  Suite *suite = suite_create("ModSecurity Suite");

  TCase *tc_mod_security = tcase_create("ModSecurity Headers");
  tcase_add_test(tc_mod_security, handles_a_single_event);
  tcase_add_test(tc_mod_security, handles_multiple_events);
  tcase_add_test(tc_mod_security, correctly_parses_anonmaly_total);
  tcase_add_test(tc_mod_security, returns_0_when_no_values_exist);
  suite_add_tcase(suite, tc_mod_security);

  return suite;
}
