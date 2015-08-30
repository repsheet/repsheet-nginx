#ifndef CHECK_CIDR
#define CHECK_CIDR

extern int CACHED_FOR_SECONDS; //TODO: supply this as a command-line argument

#include "vector.h"
int checkCIDR(redisContext *context, const char *actor, char *reason, char *list, expanding_vector *ev, long *cache_update_time);

#endif
