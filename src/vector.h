#ifndef VECTOR_H
#define VECTOR_H

#include "cidr.h"

struct expanding_vector 
{
  int size;
  int alloced_size;
  range* data;
};

int init_expanding_vector( struct expanding_vector * , int element_size , int initial_size );
void clear_expanding_vector( struct expanding_vector *ev );
void push_item( struct expanding_vector *ev , range *push_range );

#endif
