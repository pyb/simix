//#define VERBOSE 

#ifdef VERBOSE
#define COMMENT(...) \
  fprintf(stderr, __VA_ARGS__)
#else 
#define COMMENT(...) 
#endif

#define ERROR(...) \
  fprintf(stderr, __VA_ARGS__)

#define FATAL(...) error(1, 0, __VA_ARGS__)
