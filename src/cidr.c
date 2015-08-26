#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "repsheet.h"
#include "cidr.h"

/**
 * @file cidr.c
 * @author Aaron Bedra
 * @date 10/09/2014
 */

int _string_to_cidr(CIDR *cidr, char *block);

int block_to_range( char *block, range *range )
{
  if (block == NULL) {
    return NIL;
  }

  CIDR cidr;
  int result = _string_to_cidr(&cidr, block);
  if (result == BAD_ADDRESS || result == BAD_CIDR_BLOCK) {
    return result;
  }

  range->lower = cidr.address;
  range->upper = range->lower + (pow(2, (32 - cidr.mask)) -1);

  if (cidr.address == BAD_ADDRESS) {
    return BAD_ADDRESS;
  }
  return 1;
}

/**
 * Test an IP to see if it is contained in the CIDR block
 *
 * @param block the CIDR block string
 * @param address the IP address string
 *
 * @returns 1 if in the block, 0 if not
 */
int cidr_contains(char *block, int ip)
{
  range range;
  if ( ip == BAD_ADDRESS )
  {
    return BAD_ADDRESS;
  }
  int rc =  block_to_range( block, &range );
  if ( rc <= 0 ) {
    return rc;
  }
  return address_in_range( &range, ip );
}

int address_in_range( range *r, int ip )
{
  return ((r->lower <= ip) && (ip <= r->upper));
}

int _string_to_cidr(CIDR *cidr, char *block)
{
  char dup[strlen(block) + 1];
  char *value;
  memcpy(dup, block, strlen(block) + 1);

  cidr->address_string = NULL;
  value = strtok(dup,"/");
  if (value != NULL) {
    cidr->address_string = value;
    if (strlen(cidr->address_string) < 7 || strlen(cidr->address_string) > 16) {
      return BAD_CIDR_BLOCK;
    }
  }

  value = strtok(NULL,"/");
  if (value == NULL) {
    return BAD_CIDR_BLOCK;
  } else {
    cidr->mask = strtol(value, 0, 10);
    if (cidr->mask < 0 || cidr->mask > 32) {
      return BAD_CIDR_BLOCK;
    }
  }

  if (cidr->address_string == NULL) {
    return BAD_ADDRESS;
  } else {
    cidr->address = ip_address_to_integer(cidr->address_string);
    if (cidr->address == BAD_ADDRESS) {
      return BAD_ADDRESS;
    }
  }

  return LIBREPSHEET_OK;
}

int ip_address_to_integer(const char *address)
{
  char dup[strlen(address) + 1];
  char *value;
  memcpy(dup, address, strlen(address) + 1);

  int first, second, third, fourth;

  value = strtok(dup, ".");
  if (value != NULL) {
    first = strtol(value, 0, 10);
  } else {
    return BAD_ADDRESS;
  }

  value = strtok(NULL, ".");
  if (value != NULL) {
    second = strtol(value, 0, 10);
  } else {
    return BAD_ADDRESS;
  }

  value = strtok(NULL, ".");
  if (value != NULL) {
    third = strtol(value, 0, 10);
  } else {
    return BAD_ADDRESS;
  }

  value = strtok(NULL, ".");
  if (value != NULL) {
    fourth = strtol(value, 0, 10);
  } else {
    return BAD_ADDRESS;
  }

  int i;
  int octets[] = {first, second, third, fourth};

  for (i = 0; i < 4; i++) {
    if (octets[i] < 0 || octets[i] > 256) {
      return BAD_ADDRESS;
    }
  }

  int ip_integer = ((first << 24) & 0xFF000000)
    | ((second << 16) & 0xFF0000)
    | ((third << 8)   & 0xFF00)
    |  (fourth        & 0xFF);

  return ip_integer;
}

