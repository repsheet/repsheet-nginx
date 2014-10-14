#ifndef __COMMON_H
#define __COMMON_H

void populate_reason(redisReply *reply, char *reason);

int expire(redisContext *context, const char *actor, char *label, int expiry);
int blacklist_and_expire(redisContext *context, int type, const char *actor, int expiry, char *reason);

int increment_rule_count(redisContext *context, const char *actor, char *rule);

int set(redisContext *context, const char *prefix, char *keyspace, const char *value);
int set_list(redisContext *context, const char *prefix, char *keyspace, char *list, const char *value);

#endif
