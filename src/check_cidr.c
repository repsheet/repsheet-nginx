#include <string.h>
#include "repsheet.h"
#include "common.h"
#include "cidr.h"
#include "check_cidr.h"
#include "vector.h"

long msecs()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

#define COMMAND_MAX_LENGTH 200  
#define CACHE_MILISECONDS 1000 * 60 // sixty seconds 

int checkCIDR(redisContext *context, const char *actor, char *reason, char *list, expanding_vector *ev, long* last_update_time)
{
  int ip = ip_address_to_integer(actor);
  if ( ip == BAD_ADDRESS )
    return BAD_ADDRESS;

  long current_miliseconds = msecs();

  char command[COMMAND_MAX_LENGTH];

  if ( *last_update_time + CACHE_MILISECONDS < current_miliseconds ) {
    clear_expanding_vector( ev );
    
    snprintf(command ,COMMAND_MAX_LENGTH, "SMEMBERS repsheet:cidr:%s", list);
    redisReply *listed = redisCommand(context, command);
    
    if (listed) {
      if (listed->type == REDIS_REPLY_ARRAY) {
        int i;
      
        char *block;
        for(i = 0; i < listed->elements; i++) {
          block = strtok(listed->element[i]->str, ":");
          range range;
          int rc = block_to_range( block , &range );
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
    *last_update_time = current_miliseconds;
  }

  for( int i = 0 ; i < ev->size ; ++ i) {
    range *range = &(ev->data[i]);
    if (address_in_range(range, ip) > 0) {
      snprintf(command, COMMAND_MAX_LENGTH, "GET %s:repsheet:cidr:%s", range->block ,list);
      redisReply *value;
      value = redisCommand(context, command);
      if (value) {
        populate_reason(value, reason);
        freeReplyObject(value);
      }
      return TRUE;
    }
  }
  return FALSE;
}

