#ifndef __CIDR_H
#define __CIDR_H

typedef struct {
  int address;
  int mask;
  char *address_string;
} CIDR;

int cidr_contains(char *block, char *address);

#endif
