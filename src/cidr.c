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

int _string_to_integer(const char *address);
int _string_to_cidr(CIDR *cidr, char *block);

/**
 * Test an IP to see if it is contained in the CIDR block
 *
 * @param block the CIDR block string
 * @param address the IP address string
 *
 * @returns 1 if in the block, 0 if not
 */
int cidr_contains(char *block, const char *address)
{
  if (block == NULL || address == NULL) {
    return NIL;
  }

  CIDR *cidr = malloc(sizeof(*cidr));
  int result = _string_to_cidr(cidr, block);
  if (result == BAD_ADDRESS || result == BAD_CIDR_BLOCK) {
    free(cidr);
    return result;
  }

  int lower = cidr->address;
  int upper = lower + (pow(2, (32 - cidr->mask)) -1);
  int ip = _string_to_integer(address);

  if (cidr->address == BAD_ADDRESS || ip == BAD_ADDRESS) {
    free(cidr);
    return BAD_ADDRESS;
  }

  free(cidr);

  return ((lower <= ip) && (ip <= upper));
}

int _string_to_cidr(CIDR *cidr, char *block)
{
  char dup[strlen(block) + 1];
  memcpy(dup, block, strlen(block) + 1);

  cidr->address_string = strtok(dup,"/");
  if (strlen(cidr->address_string) < 8 || strlen(cidr->address_string) > 16) {
    return BAD_CIDR_BLOCK;
  }

  cidr->mask = strtol(strtok(NULL,"/"), 0, 10);
  if (cidr->mask < 0 || cidr->mask > 32) {
    return BAD_CIDR_BLOCK;
  }

  cidr->address = _string_to_integer(cidr->address_string);
  if (cidr->address == BAD_ADDRESS) {
    return BAD_ADDRESS;
  }

  return LIBREPSHEET_OK;
}

int _string_to_integer(const char *address)
{
  char dup[strlen(address) + 1];
  memcpy(dup, address, strlen(address) + 1);

  int first, second, third, fourth;

  first = strtol(strtok(dup, "."), 0, 10);
  second = strtol(strtok(NULL, "."), 0, 10);
  third = strtol(strtok(NULL, "."), 0, 10);
  fourth = strtol(strtok(NULL, "."), 0, 10);

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
