/*
  This program runs a minix executable by trapping its messages (int 0x21) and turning them into Linux system calls
  To do that, we prepare the registers and stack, then insert an int 0x80 in the child's memory and do a PTRACE_SINGLESTEP
*/

//TODO : test ioctl, fcntl, finish signal management
// TODO : test statfs. Exit status
// Any idea for networking ?

#define _MINIX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asm/ldt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <a.out.h>
#include <sys/mman.h>

#include "debugprint.h"
#include "minix-ipc.h"
#include "callnr_minix.h"
#include "syscalls.h"
#include "a.out.minix.h"


// (assuming 4k page)
#define P_ALIGN(address) (( (address) & 0xfffff000 ) + 0x1000)

// Important data about the traced Minix process :
int fork_pid;
int code_seg = 0;
int data_seg = 0;
struct user_regs_struct regs;
struct user_regs_struct regs_saved;

int status;
int exit_called = 0;
int options_set = 0;

int mess_addr; // address of message structure in the childs space

int sigreturn_context;
int handler_context;
int child_context;
int after_signal_handler;

int* stack_address;
int* scratchpad = (int*)SCRATCHPAD;
int tracer_pid;

message m;
int* mess_buf;

struct minix_exec minix_header;

// linux_syscall() was moved out of this file only because of its length.
void linux_syscall(message* mess);

void print_regs(struct user_regs_struct regs)
{
  COMMENT("eax = %lx (%ld)\n", regs.eax, regs.eax);
  COMMENT("ebx = %lx\n", regs.ebx);
  COMMENT("ecx = %lx\n", regs.ecx);
  COMMENT("edx = %lx\n", regs.edx);
  COMMENT("eip = %lx\n", regs.eip);
  COMMENT("xcs = %lx\n", regs.xcs); 
  COMMENT("xds = %lx\n", regs.xds);
  COMMENT("xss = %lx\n", regs.xss);
  COMMENT("esi = %lx\n", regs.esi);
  COMMENT("edi = %lx\n", regs.edi);
  COMMENT("ebp = %lx\n", regs.ebp);
  COMMENT("esp = %lx\n", regs.esp);
}

void print_stack()
{
  int i = 0;
  COMMENT("stack:\n");
  for (i = 0 ; i < 128 ; i++)
    COMMENT("%08lx ", ptrace (PTRACE_PEEKDATA, fork_pid, regs.esp + data_seg + 4*i , 0));
  COMMENT("\n");
}

void copy_to_child (void* addr_src, int addr_dst, int size)
{
  int* buf_src;
  int i;
  if(size % 4)
    FATAL("Size not multiple of 4!\n");
  
  buf_src = (int*) addr_src;
  for (i = 0 ; i < (size / 4) ; i++)
    ptrace (PTRACE_POKEDATA, fork_pid, addr_dst + (4 * i) , buf_src[i]);
}

void copy_from_child (int addr_src, void* addr_dst, int size)
{
  int* buf_dst;
  int i;
  if(size % 4)
    FATAL("Size not multiple of 4!\n");
  
  buf_dst = (int*) addr_dst;
  for (i = 0 ; i < (size / 4) ; i++)
    buf_dst[i] = ptrace (PTRACE_PEEKDATA, fork_pid, addr_src + (4 * i) , 0);
}

void copy_scratchpad_from_child()
{
  copy_from_child ((int)scratchpad, scratchpad, SCRATCHPAD_SIZE);
}

void copy_scratchpad_to_child()
{
  copy_to_child(scratchpad, (int) scratchpad, SCRATCHPAD_SIZE);
}

void reserve_scratchpad()
{
  int e,r;
  r = (int)mmap ((void*)SCRATCHPAD, 0x1000,
	PROT_EXEC|PROT_READ|PROT_WRITE, 
	MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, 
	-1,
	0);
  if (r == -1)
    {
      e = errno;
      COMMENT("scratchpad mmap failed with errno %d which is %s\n", e, (char*)strerror(e));
      _exit(1);
    }
}
// See notes.txt on segmentation
// See Minix Book top of p416
#define DESCRIPTOR_SELECTION_BIT 0x4
#define STACK_SPACE 2048

void install_segments_from_parent(struct user_regs_struct * regs)
{
  regs->xcs = ( (3 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 3 with privilege 0b11
  regs->xds = ( (1 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 1 with privilege 0b11
  // Extra segments are sometimes used too.
  regs->xes = ( (1 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 1 with privilege 0b11
  regs->xfs = ( (1 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 1 with privilege 0b11
  regs->xgs = ( (1 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 1 with privilege 0b11

  regs->xss = ( (2 << 3) | 3 |  DESCRIPTOR_SELECTION_BIT );  // Switch from GDT to LDT entry 1 with privilege 0b11
  //  regs->esp -= data_seg;
  //  regs->ebp -= data_seg;
}


void remove_segments(struct user_regs_struct * regs)
{
  regs->xcs = 0x73;
  regs->xds = 0x7b;
  regs->xss = 0x7b;
  regs->xes = 0x7b;
  regs->xfs = 0x7b;
}

void read_header (char* pathname, int* text_s)
{
  struct exec exec_header;
  int fd;
  fd = open(pathname, O_RDONLY);
  if (fd==-1)
    FATAL("Cannot read file!\n");
  if (!read(fd, &exec_header, sizeof(struct exec)))
    FATAL("Cannot read file!\n");

    *text_s = exec_header.a_text;
}

#define INITIAL_HEAP 0x5000


/*
  User-mode exec routine. 
   read a.out header, then :
   1) Make the heap  as large as the data segment + a bit more space
   2) copy text to address 0
   3) copy data to address data_seg
   4) Fix stack with correct argv, envp as expected by minix (this was done after execution started in previous versions)
   5) run code !
*/

void um_exec(char* pathname, int argc, char** argv, char** envp)
{

  int text_size, data_size, bss_size;
  void* current_brk;
  FILE* file;
  int i;
  int e,r;
  int reserved_stack_space[STACK_SPACE];

  stack_address = (int*)&reserved_stack_space;  // That will be the stack seen by the minix process, when it starts executing.
  reserved_stack_space[0] = argc;
  
  // copy argv so that minix finds it where it expects it
  for (i = 0 ; i < argc ; i++)
    {
      reserved_stack_space[i + 1] = (int) argv[i] - data_seg;
    }
  
  if (argv[argc] != 0)
    FATAL("I was expecting a NULL at argv[argc]\n");

  reserved_stack_space[argc + 1] = 0;  // final null

  // same idea with envp
  i = 0;
  while(1)
    {
      int env_item = (int) envp[i];
      if (env_item) env_item -= data_seg;
      reserved_stack_space[argc + 2 + i] = env_item;
      i++;

      if (!env_item) break;

    }

  text_size = minix_header.a_text;
  data_size = minix_header.a_data;
  bss_size = minix_header.a_bss;


  // step 1
  if ((int)sbrk(P_ALIGN(text_size + data_size + bss_size) + INITIAL_HEAP) == -1)
    FATAL("sbrk failed !\n");

  //  COMMENT("mapping to code segment code_seg = %x with size %x\n", code_seg, P_ALIGN(text_size + data_size + bss_size));
  //step 2
  r = (int)mmap ((void*)code_seg, P_ALIGN(text_size + data_size + bss_size),
	PROT_EXEC|PROT_READ|PROT_WRITE, 
	MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, 
	-1,
	0);
  if (r == -1)
    {
      e = errno;
      COMMENT("mmap failed with errno %d which is %s\n", e, (char*)strerror(e));
      FATAL("mmap failed\n");
    }

  file = fopen(pathname, "r");
  fseek(file, minix_header.a_hdrlen, SEEK_SET);

  if (fread((void*)code_seg, text_size, 1, file) < 1)
    FATAL("fread text failed\n");

  //step 3
  if (fread((void*)code_seg + text_size, data_size, 1, file) < 1)
    FATAL("fread data failed\n");
  fclose(file);

  // step 4 see above!

  // step 5
  kill(getpid(), SIGUSR1); // should cause the parent to change eip and segments
  while(1);
    {
      COMMENT("Ooops that didnt work\n");
    }
}


void init_signals()
{
  sigset_t set;
  struct sigaction act;

  sigfillset(&set);
  sigprocmask(SIG_SETMASK, &set, NULL);
  memset(&act, 0, sizeof(act));
  sigfillset(&act.sa_mask);

  act.sa_handler = SIG_IGN;
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);

  sigemptyset(&set);
  sigprocmask(SIG_SETMASK, &set, NULL);
}

void create_ldt_entry()
{
      struct user_desc table_entry;
      int num_bytes;

      // Data segment
      table_entry.entry_number = 1;
      table_entry.base_addr = data_seg;
      table_entry.limit = 0xfffff; // max limit because stack is up there somewhere... don't understand why data seg is used for stack access !
      // Will have to investigate at some point

      table_entry.seg_32bit = 0x1;
      table_entry.contents = 0; // data segment with direction up
      table_entry.read_exec_only = 0x0;  // RW
      table_entry.limit_in_pages = 0x1;
      table_entry.seg_not_present = 0x0;
      table_entry.useable = 0x1;

      /* Writes a user_desc struct to the ldt */
      num_bytes = syscall( __NR_modify_ldt,
			   1, // LDT_WRITE
			   &table_entry,
			   sizeof(struct user_desc)
			   );
      // Stack segment

      table_entry.entry_number = 2;
      table_entry.base_addr = data_seg;
      table_entry.limit = 0x0; // max limit (=0 for expand-down) because stack is up there somewhere... don't understand why data seg is used for stack access !
      table_entry.seg_32bit = 0x1;
      table_entry.contents = 1; // data segment with expand-down
      table_entry.read_exec_only = 0x0;  // RW
      table_entry.limit_in_pages = 0x1;
      table_entry.seg_not_present = 0x0;
      table_entry.useable = 0x1;
      num_bytes = syscall( __NR_modify_ldt,
			   1, // LDT_WRITE
			   &table_entry,
			   sizeof(struct user_desc)
			   );
      // this is the end of the SO code example

      // Code segment

      table_entry.entry_number = 3;
      table_entry.base_addr = code_seg;
      table_entry.limit = 0xfffff; // max limit (=0 for expand-down) because stack is up there somewhere... don't understand why data seg is used for stack access !
      table_entry.seg_32bit = 0x1;
      //      table_entry.contents = 1; // code segment with expand-down
      //      table_entry.read_exec_only = 0x0;  // RW
      
      table_entry.contents = 2;  // code segment 
      table_entry.read_exec_only = 0x0;   //readable
     
      table_entry.limit_in_pages = 0x1;
      table_entry.seg_not_present = 0x0;
      table_entry.useable = 0x1;
      num_bytes = syscall( __NR_modify_ldt,
			   1, // LDT_WRITE
			   &table_entry,
			   sizeof(struct user_desc)
			   );
}

int main (int argc, char** argv, char** envp)
{

  int i;

  char* pathname;
  FILE* file;
  
  if (argc > 1)
    pathname = argv[1];
  else
    FATAL("need an argument!\n");

  init_signals();
  setbuf(stdout, NULL);  // COMMENT buffering is troublesome during debugging

  COMMENT("Control process pid : %d\n", getpid());
  mess_buf = (int*)(&m);

  code_seg = (int)sbrk(0);
  file = fopen(pathname, "r");
  if (fread(&minix_header, sizeof(struct minix_exec), 1, file) < 1)
    FATAL("Cannot read file!\n");
  if (BADMAG(minix_header))
    FATAL("BADMAG!\n");
  fclose(file);
  
  // We find out if it's separate I+D segments or not from the header
  data_seg = (minix_header.a_flags & A_SEP) ? code_seg + minix_header.a_text : code_seg;

  fork_pid = fork();
  if (fork_pid == 0)
    {
    // Child

      COMMENT("Child process pid : %d\n", getpid());
      ptrace(PTRACE_TRACEME, 0, 0, 0); // Request tracing by parent. only arg 0 matters

      // See 'LDT manipulations...' in notes.txt

      // Create LDT entry

      create_ldt_entry();

      reserve_scratchpad();
      um_exec(pathname, argc-1, &(argv[1]), envp);  // we skip the 1st arg..
      
      // will not return !
    }
  else
    {
      siginfo_t sig;

      reserve_scratchpad();

      tracer_pid = getpid();

      // Parent. Child PID is in fork_pid
      while(1)
	{
	  if (waitpid(fork_pid, &status, 0) == -1)
	    FATAL("Waitpid error in main loop\n");
	    
	  if (!options_set)
	    {
	      ptrace (PTRACE_SETOPTIONS, fork_pid,  NULL, PTRACE_O_TRACEEXIT | PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK);	 
	      options_set = 1;
	    }

	  if (WIFSTOPPED(status))
	    {
	      ptrace (PTRACE_GETSIGINFO, fork_pid,  NULL, &sig);
	      ptrace (PTRACE_GETREGS, fork_pid,  NULL, &regs);

	      COMMENT("%d Process %d interrupted with %x\n", tracer_pid, fork_pid, sig.si_signo);	      
	      print_regs(regs);
	  
	      regs_saved = regs;

	      if ((sig.si_signo == SIGSEGV) && (regs.ecx != 3))
		{
		  COMMENT("Segmentation fault !\n");
		  print_regs(regs);
		  _exit(1);
		}

	      
	      if (sig.si_signo == SIGUSR1)
		/* SIGUSR1 was generated by um_exec(), now is the time to install the new segments. */
		{
		  int new_stack;
		  install_segments_from_parent(&regs);
		  regs.eip = minix_header.a_entry;
		  
		  // stack address is in global var stack_address... 
		  new_stack = ptrace (PTRACE_PEEKDATA, fork_pid, &stack_address, 0);

		  regs.esp = new_stack - data_seg;
		  regs.ebp = new_stack - data_seg;
		  
		  regs.edi = 0;
		  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0);
		  continue;
		}

	      
	      // Sigreturn detected (sigreturn is the only possibility here)
	      if ((sig.si_signo == SIGTRAP) && (handler_context == 1))
		{
		  handler_context = 0;
		  remove_segments(&regs);
		  regs.eip += code_seg;
		  regs.esp += data_seg;
		  regs.ebp += data_seg;

		  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
		  sigreturn_context = 1;
		  kill(fork_pid, SIGTRAP); // To interrupt the child process just as it sigreturn completes.
		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0);
		  continue;
		}

	      if ((sig.si_signo == SIGTRAP) && (sigreturn_context == 1))
		{ 
		  //	  Interrupted with SIGTRAP after sigreturn ; a signal handler is trying to return.
		  install_segments_from_parent(&regs);

		  regs.eip -= code_seg;
		  regs.esp -= data_seg;
		  regs.ebp -= data_seg;

		  sigreturn_context = 0;
		  after_signal_handler = 1;
		  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0); 
		  continue;  // This should close the signal management process.
		}

	      if (sig.si_signo == SIGUSR2)
		{
		  // Interrupted with SIGUSR2 : start of signal handler
		  // fix retcode address
		  int retcode = ptrace (PTRACE_PEEKDATA, fork_pid, regs.esp, 0);
		  ptrace (PTRACE_POKEDATA, fork_pid, regs.esp, retcode - code_seg);
		  
		  install_segments_from_parent(&regs);
		  regs.eip -= code_seg;
		  regs.esp -= data_seg;
		  regs.ebp -= data_seg;

		  handler_context = 1;
		  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);

		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0); // Stop just before sigreturn   gets invoked
		  continue;
		}

	      // I am getting SIGTRAPs that I don't quite understand after returning from a signal handler. Ignore it
	      //	      if ((sig.si_signo == SIGTRAP) && after_signal_handler)
	      if (sig.si_signo == SIGTRAP)
		{
		  after_signal_handler = 0;

		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0); // Stop just before sigreturn gets invoked
		  continue;
		}

	      if ((sig.si_signo == SIGSTOP) && child_context)
		{
		  after_signal_handler = 0;
		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0); // Stop just before sigreturn gets invoked
		  continue;
		}
	
	      // temporary
	      if (sig.si_signo == SIGCHLD)
		{
		  COMMENT("SIGCHILD detected\n");
		  ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0); // Do not Pass it on (TODO :fix this bug that happens if I pass on the SIGCHILD)
		  continue;
		}

	      // an int 0x21 gives a SIGSEGV signal with regs.ecx = 3
	      if ((sig.si_signo != SIGSEGV) || (regs.ecx != 3))
		{
		  COMMENT("Process %d interrupted with UNKNOWN signal %d\n", fork_pid, sig.si_signo);
		  // PYB : Change stack segment back

		  // TODO : Check if we are going to use a signal handler  : for instance, see what happened to eip at the next stop
		  remove_segments(&regs);
		  regs.esp += data_seg;
		  regs.ebp += data_seg;
		  regs.eip += code_seg;

		  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
		  

		  kill(fork_pid, SIGUSR2); // To interrupt the child process just as it enters the signal handler.
		  // We need to restore our segments then
		  
		  ptrace(PTRACE_SYSCALL, fork_pid, NULL, sig.si_signo);  // Relay signal to the child
		  continue;
		}


	      /* At this point, it must be a system call :
	       ebx contains message pointer
	       eax = destination process (should be MM =  PM_PROC_NR	=   0	)
	       normally ecx = 3 (sendrec)
	      */

	      // Grab message from child process' memory ; convert the syscall
	      mess_addr = regs.ebx;  
	      copy_from_child(mess_addr + data_seg, mess_buf, sizeof(message));
	  
	      regs_saved = regs;

	      linux_syscall(&m);  // All the conversion work happens here
  
	      /*
		Write answer 
		Grab message from child process' memory ; convert the syscall
		(in particular, return value is in message->m_type)
	      */
	      copy_to_child(mess_buf, mess_addr + data_seg, sizeof(message));

	      regs = regs_saved;
	      regs.eax = 0; // sendrec successful
	      regs.eip += 2; // Next instruction after the int 21. For some reason ptrace will loop (int21) forever otherwise !
	      ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
	      ptrace (PTRACE_SYSCALL, fork_pid,  NULL, 0);
	    }
	  else 
	    {
	      COMMENT("Strange : non-WIFSTOPPED stop from %d ! Child probably died\n Exit: %d, signaled : %d\n ", fork_pid, WIFEXITED(status), WIFSIGNALED(status));
	      _exit(1);
	    }
	}
      _exit(0);
    }
}
