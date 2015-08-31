#ifndef VECTOR_H
#define VECTOR_H

#include "cidr.h"

typedef struct  
{
  int size;
  int alloced_size;
  range* data;
} expanding_vector;

expanding_vector *create_expanding_vector(int initial_size);
void clear_expanding_vector(expanding_vector *ev);
void push_item(expanding_vector *ev, range *push_range);

#endif
