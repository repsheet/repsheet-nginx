#include <string.h>
#include "repsheet.h"
#include "common.h"
#include "cidr.h"
#include "check_cidr.h"
#include "vector.h"

#define COMMAND_MAX_LENGTH 200  
int checkCIDR(redisContext *context, const char *actor, char *reason, char *list)
{
  char command[COMMAND_MAX_LENGTH];
  snprintf(command ,COMMAND_MAX_LENGTH, "SMEMBERS repsheet:cidr:%s", list);
  redisReply *listed = redisCommand(context, command);
  int ip = ip_address_to_integer(actor);
  if ( ip == BAD_ADDRESS )
    return BAD_ADDRESS;
  if (listed) {
    if (listed->type == REDIS_REPLY_ARRAY) {
      int i;
      redisReply *value;
      char *block;
      for(i = 0; i < listed->elements; i++) {
        block = strtok(listed->element[i]->str, ":");
        range range;
        int rc = block_to_range( block , &range );
        if ( rc >= 0 ) {   
          if (address_in_range(&range, ip) > 0) {
            snprintf(command, COMMAND_MAX_LENGTH, "GET %s:repsheet:cidr:%s", block ,list);
            value = redisCommand(context, command);
            if (value) {
              populate_reason(value, reason);
              freeReplyObject(value);
            }
            freeReplyObject(listed);
            return TRUE;
          }
        }
      }
    } else{
      freeReplyObject(listed);
    }
  } else {
    return DISCONNECTED;
  }

  return FALSE;
  }

