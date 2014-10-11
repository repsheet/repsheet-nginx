#ifndef __COMMON_H
#define __COMMON_H

int set(redisContext *context, const char *prefix, char *keyspace, const char *value);
int set_list(redisContext *context, const char *prefix, char *keyspace, char *list, const char *value);

#endif
