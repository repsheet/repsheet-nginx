#ifndef __CIDR_H
#define __CIDR_H

#define BAD_ADDRESS -3
#define BAD_CIDR_BLOCK -4

typedef struct {
  int address;
  int mask;
  char *address_string;
} CIDR;

int cidr_contains(char *block, const char *address);

#endif
