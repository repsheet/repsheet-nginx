#ifndef __WHITELIST_H
#define __WHITELIST_H

int whitelist_actor(redisContext *context, const char *actor, int type, const char *reason);
int is_ip_whitelisted(redisContext *context, const char *actor, char *reason);
int is_user_whitelisted(redisContext *context, const char *actor, char *reason);
int is_country_whitelisted(redisContext *context, const char *country_code);

#endif
