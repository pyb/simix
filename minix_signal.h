typedef unsigned long minix_sigset_t;

struct minix_sigaction {
  void* minix_sa_handler;	
  minix_sigset_t sa_mask;	
  int sa_flags;		
};
