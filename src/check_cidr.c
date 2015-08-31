#include <string.h>
#include <time.h>
#include "repsheet.h"
#include "common.h"
#include "cidr.h"
#include "check_cidr.h"
#include "vector.h"


int CACHED_FOR_SECONDS = 60;

int check_and_update_cache(redisContext *context, const char *actor, char *reason, char *list, expanding_vector *ev, time_t *last_update_time)
{
  long current_seconds = time(NULL);

  if (*last_update_time + CACHED_FOR_SECONDS < current_seconds) {
    clear_expanding_vector(ev);
    redisReply *listed = redisCommand(context, "SMEMBERS repsheet:cidr:%s", list);
    if (listed) {
      if (listed->type == REDIS_REPLY_ARRAY) {
        int i;

        char *block;
        for(i = 0; i < listed->elements; i++) {
          block = strtok(listed->element[i]->str, ":");
          range range;
          int rc = block_to_range(block, &range);
          if (rc >= 0) {
            strncpy(range.block, block, MAX_BLOCK_SIZE);
            push_item(ev, &range);
          }
        }
      }
      freeReplyObject(listed);
    } else {
      return DISCONNECTED;
    }
    //loaded ok, update the cache time
    *last_update_time = current_seconds;
  }
  return 0;
}


int checkCIDR(redisContext *context, const char *actor, char *reason, char *list, expanding_vector *ev, time_t *last_update_time)
{
  int ip = ip_address_to_integer(actor);
  if (ip == BAD_ADDRESS)
    return BAD_ADDRESS;

  int rc = check_and_update_cache(context, actor, reason, list, ev, last_update_time);
  if (rc != 0) {
    return rc;
  }

  for(int i = 0; i < ev->size; ++ i) {
    range *range = &(ev->data[i]);
    if (address_in_range(range, ip) > 0) {
      redisReply* value = redisCommand(context, "GET %s:repsheet:cidr:%s", range->block, list);
      if (value) {
        populate_reason(value, reason);
        freeReplyObject(value);
      }
      return TRUE;
    }
  }
  return FALSE;
}
