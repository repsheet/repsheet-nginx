#ifndef __BLACKLIST_H
#define __BLACKLIST_H

int blacklist_actor(redisContext *context, const char *actor, int type, const char *reason);
int is_ip_blacklisted(redisContext *context, const char *actor, char *reason);
int is_user_blacklisted(redisContext *context, const char *actor, char *reason);
int is_country_blacklisted(redisContext *context, const char *country_code);
int is_historical_offender(redisContext *context, int type, const char *actor);

#endif
