#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#include "repsheet.h"
#include "modsecurity.h"

/**
 * Finds the number of ModSecurity rules in X-WAF-Events header. This
 * is used before the actual processor so that the events array can be
 * properly allocated.
 *
 * @param waf_events the contents of the X-WAF-Events header
 */
int matches(const char *waf_events)
{
  int match, error_offset;
  int offset = 0;
  int matches = 0;
  int ovector[100];

  const char *error;

  pcre *regex = pcre_compile("(?<!\\d)\\d{6}(?!\\d)", PCRE_MULTILINE, &error, &error_offset, 0);

  while (offset < strlen(waf_events) && (match = pcre_exec(regex, 0, waf_events, strlen(waf_events), offset, 0, ovector, sizeof(ovector))) >= 0) {
    matches++;
    offset = ovector[1];
  }

  return matches;
}

/**
 * Populates the events array with ModSecurity rule ids that were
 * present in the X-WAF-Events header
 *
 * @param waf_events the contents of the X-WAF-Events header
 * @param events the pre-allocated events array to place results
 */
void process_mod_security_headers(const char *waf_events, char *events[])
{
  int i = 0;
  int matches = 0;
  int offset = 0;
  int count = 0;
  int match, error_offset;
  int ovector[100];

  char *prev_event = NULL;

  const char *event;
  const char *error;

  pcre *regex;

  regex = pcre_compile("(?<!\\d)\\d{6}(?!\\d)", PCRE_MULTILINE, &error, &error_offset, 0);

  while (offset < strlen(waf_events) && (match = pcre_exec(regex, 0, waf_events, strlen(waf_events), offset, 0, ovector, sizeof(ovector))) >= 0) {
    for(i = 0; i < match; i++) {
      pcre_get_substring(waf_events, ovector, match, i, &(event));
      if (event != prev_event) {
        strcpy(events[count], event);
        prev_event = (char*)event;
      }
    }
    count++;
    offset = ovector[1];
  }
}

/**
 * Processes the X-WAF-Score header to determine the total ModSecurity
 * anomaly score
 *
 * @param waf_score the X-WAF-Score header
 */
int modsecurity_total(const char *waf_score)
{
  int offset = 0;
  int match, error_offset;
  int ovector[100];

  const char *event;
  const char *error;

  pcre *regex;

  regex = pcre_compile("\\d+;", PCRE_MULTILINE, &error, &error_offset, 0);

  match = pcre_exec(regex, 0, waf_score, strlen(waf_score), offset, 0, ovector, sizeof(ovector));

  if (match && match > 0) {
    pcre_get_substring(waf_score, ovector, match, 0, &(event));
    return strtol(event, 0, 10);
  }

  return 0;
}

