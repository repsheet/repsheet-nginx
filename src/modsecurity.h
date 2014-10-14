#ifndef __MODSECURITY_H
#define __MODSECURITY_H

int matches(const char *waf_events);
void process_mod_security_headers(const char *waf_events, char *events[]);
int modsecurity_total(const char *waf_score);

#endif
