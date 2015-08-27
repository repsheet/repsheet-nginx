#ifndef __CIDR_H
#define __CIDR_H

#define BAD_ADDRESS -3
#define BAD_CIDR_BLOCK -4

typedef struct {
  int address;
  int mask;
  char *address_string;
} CIDR;

typedef struct {
  int lower;
  int upper; 
} range;

int cidr_contains(char *block, int address);
int address_in_range( range *r, int ip );
int ip_address_to_integer(const char *address);
int block_to_range( char *block, range *range );

#endif
