#ifndef __COMMON_H
#define __COMMON_H

void populate_reason(redisReply *reply, char *reason);
int set_list(redisContext *context, const char *prefix, char *keyspace, char *list, const char *value);
int set_block(redisContext *context, const char *prefix, char *list, const char *value);

#endif
