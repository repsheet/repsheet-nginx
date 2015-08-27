#include <stdlib.h>
#include <string.h>

#include "vector.h"

int init_expanding_vector( struct expanding_vector *ev, int element_size, int initial_size )
{
  ev->size = 0;
  ev->alloced_size = initial_size;
  ev->data = malloc( element_size * initial_size );
  return( ev->data != NULL ); 
}

void clear_expanding_vector( struct expanding_vector *ev )
{
  ev->size = 0;
}

void push_item( struct expanding_vector *ev , range *push_range ) 
{
  if ( ev->size == ev->alloced_size ) {
    ev->alloced_size *= 2;
    ev->data = realloc( ev->data , ev->alloced_size * sizeof( range ) );
  }
  memcpy( ev->data + ev->size , push_range , sizeof( range ) );
  ++ev->size;
}


  

 
