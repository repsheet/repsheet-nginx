#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cidr.h"

int  _string_to_integer(char *address);
void _string_to_cidr(CIDR *cidr, char *block);

int cidr_contains(char *block, char *address)
{
  if (block == NULL || address == NULL) {
    return -1;
  }

  CIDR *cidr = malloc(sizeof(*cidr));
  _string_to_cidr(cidr, block);

  int lower = cidr->address;
  int upper = lower + (pow(2, (32 - cidr->mask)) -1);
  int ip = _string_to_integer(address);

  free(cidr);

  return ((lower <= ip) && (ip <= upper));
}

void _string_to_cidr(CIDR *cidr, char *block)
{
  char dup[strlen(block)];
  memcpy(dup, block, strlen(block));

  //TODO: validate that the string is at least 8 and no more than 16 characters
  cidr->address_string = strtok(dup,"/");
  //TODO: validate that the mask is a positive number less than 33
  cidr->mask = strtol(strtok(NULL,"/"), 0, 10);
  //TODO: validate that the conversion didn't fail (BAD_ADDRESS)
  cidr->address = _string_to_integer(cidr->address_string);
}

int _string_to_integer(char *address)
{
  char dup[strlen(address)];
  memcpy(dup, address, strlen(address));

  //TODO: validate each octet to ensure they are within range and
  //return BAD_ADDRESS if invalid
  int ip_integer = ((strtol(strtok(dup, "."), 0, 10) << 24) & 0xFF000000)
    | ((strtol(strtok(NULL, "."), 0, 10) << 16) & 0xFF0000)
    | ((strtol(strtok(NULL, "."), 0, 10) << 8)  & 0xFF00)
    |  (strtol(strtok(NULL, "."), 0, 10)        & 0xFF);

  return ip_integer;
}
