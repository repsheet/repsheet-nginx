#ifndef __MARKED_H
#define __MARKED_H

int mark_actor(redisContext *context, const char *actor, int type, const char *reason);
int is_ip_marked(redisContext *context, const char *actor, char *reason);
int is_user_marked(redisContext *context, const char *actor, char *reason);
int is_country_marked(redisContext *context, const char *country_code);

#endif
