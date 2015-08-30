#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"

expanding_vector *create_expanding_vector(int initial_size)
{
  expanding_vector *ev = malloc(sizeof(expanding_vector));
  assert(ev != NULL);
  ev->size = 0;
  ev->alloced_size = initial_size;
  ev->data = malloc(sizeof(range) * initial_size);
  assert(ev->data != NULL);
  return ev;
}

void clear_expanding_vector(expanding_vector *ev)
{
  ev->size = 0;
}

void push_item(expanding_vector *ev, range *push_range) 
{
  if (ev->size == ev->alloced_size) {
    ev->alloced_size *= 2;
    ev->data = realloc(ev->data, ev->alloced_size * sizeof(range));
    assert(ev->data != NULL);
  }
  memcpy(ev->data + ev->size, push_range, sizeof(range));
  ++ev->size;
}



  

 
