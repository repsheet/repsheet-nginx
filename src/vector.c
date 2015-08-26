#include <stdlib.h>
#include "vector.h"

int init_expanding_vector( struct expanding_vector *ev, int element_size, int initial_size )
{
  ev->size = 0;
  ev->alloced_size = initial_size;
  ev->element_size = element_size;
  ev->data = malloc( element_size * initial_size );
  return( ev->data != NULL ); 
}

void clear_expanding_vector( struct expanding_vector *ev )
{
  ev->size = 0;
}

void *get_next_item( struct expanding_vector *ev ) 
{
  ++ev->size;
  if ( ev->size >= ev->alloced_size ) {
    ev->alloced_size *= 2;
    ev->data = realloc( ev->data , ev->element_size * ev->alloced_size );
  }
  return ev->data + ( ev->size - 1 ) * ev->element_size;
}
  

 
