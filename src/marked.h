#ifndef __MARKED_H
#define __MARKED_H

int mark(redisContext *context, const char *actor, int type, const char *reason);
int is_ip_marked(redisContext *context, const char *actor, char *reason);
int is_user_marked(redisContext *context, const char *actor, char *reason);

#endif
