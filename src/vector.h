
struct expanding_vector 
{
  int size;
  int alloced_size;
  int element_size;
  void * data;
};

int init_expanding_vector( struct expanding_vector * , int element_size , int initial_size );
