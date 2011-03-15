//TODO : mmap/munmap, test fcntl, finish sig*** . more *stat functions to fix?
//TODO : detect separate segments.
//IDEA :  Stress-test tty ioctls with vi

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <linux/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <asm/ldt.h>
// I would perhaps like to include this but it breaks st_atime into st_atim !!etc.
//#include <sys/stat.h> 
#include <fcntl.h>
#include <a.out.h>
#include <termios.h>

#include <asm/ioctls.h>

#include "debugprint.h"
#include "minix-ipc.h"
#include "callnr_minix.h"
#include "callnr_linux.h"
#include "minix_termios.h"

#include "syscalls.h"
#include "stat-minix-linux.h"

#include "minix_ioc_tty.h"
#include "minix_signal.h"
#include "minix_wait.h"



extern int data_seg;
extern int code_seg;
extern struct user_regs_struct regs_saved;
extern struct user_regs_struct regs;
extern int fork_pid;
extern int status; // Used in waitpid

extern int* scratchpad; 
extern int mess_addr; // address of message structure in the childs space
extern int after_signal_handler;
extern int child_context;
extern int options_set;

extern char* syscall_strings[];

#define PRIO_MIN -20
#define OK 0
#define MIN(a,b) ( ( (a) < (b) ) ? (a) : (b))

#define ERESTARTSYS 512

extern message m;
extern int tracer_pid;

extern void print_regs(struct user_regs_struct regs);
extern void print_stack(void);
extern void remove_segments(struct user_regs_struct * regs);
extern void install_segments_from_parent(struct user_regs_struct * regs);
extern void copy_to_child (void* addr_src, int addr_dst, int size);
extern void copy_from_child (int addr_src, void* addr_dst, int size);
extern void copy_scratchpad_from_child(void);
extern void copy_scratchpad_to_child(void);

#define CALL_FN(name, syscall, arg1, arg2, arg3, arg4, arg5) \
void name (message* m) \
{ \
  regs.eax = syscall;	\
  regs.ebx = (int)arg1;	\
  regs.ecx = (int)arg2; \
  regs.edx = (int)arg3; \
  regs.esi = (int)arg4; \
  regs.edi = (int)arg5; \
  do_call(syscall);   \
  m->m_type = regs.eax;\
}		       \
\

#define IGNORE_CALL(name) \
void name (message* m)\
{\
 COMMENT("Ignoring system call 0x%x\n", m->m_type);\
 m->m_type = 0;					\
}\


void do_call(int nr)
{
  int instr_save;  // Save 4 bytes at [eip]
  char int_instr[4] = {0xcd, 0x80, 0xcd, 0x80};

  // insert the in 0x80 instruction  
  instr_save = ptrace (PTRACE_PEEKDATA, fork_pid, regs.eip + code_seg , 0);
  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip + code_seg , (* (int*) int_instr));
 
  regs.eax = nr;

  COMMENT("syscall %d. ( %s )   ", nr, syscall_strings[nr]);

  do{
    ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
    ptrace (PTRACE_SINGLESTEP, fork_pid,  NULL, 0); 
    if (waitpid(fork_pid, &status, 0) == -1)
      FATAL("Waitpid failed in do_call !\n");
    if (WSTOPSIG(status) != SIGTRAP)
      COMMENT("Warning : do_call wait returns with signal : %d\n", WSTOPSIG(status));
  }
  while(WSTOPSIG(status) != SIGTRAP);
  /* Restart if the system call was interrupted by a signal (and ignore the signal ! )
     Sorry.
     TODO : store the signal and send it to the traced process after the system call has returned properly. 
  */

  // Pick up new regs
  ptrace (PTRACE_GETREGS, fork_pid,  NULL, &regs); 

  COMMENT("retval = %lx (%ld)", regs.eax, regs.eax);
  if (regs.eax < 0)
    COMMENT(" which is %s\n", (char*)strerror(-regs.eax));
  else
    COMMENT("\n");

  // Restore code
  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip-2 + code_seg, (void*) instr_save);
}


/****************************************************

 SYSTEM CALLS

***************************************************/

/****************************************************

 REGULAR CALLS

***************************************************/

CALL_FN(call_read, LINUX_READ, m->m1_i1, m->m1_p1 + data_seg, m->m1_i2, 0, 0)

CALL_FN(call_write, LINUX_WRITE, m->m1_i1, m->m1_p1 + data_seg, m->m1_i2, 0, 0)

CALL_FN(call_close, LINUX_CLOSE, m->m1_i1, 0,0,0,0)

CALL_FN(call_creat, LINUX_CREAT,  m->m3_p1 + data_seg, m->m3_i2 ,0,0,0)

CALL_FN(call_link, LINUX_LINK, m->m1_p1 + data_seg, m->m1_p2,0,0,0)

CALL_FN(call_unlink, LINUX_UNLINK, m->m3_p1 + data_seg, 0,0,0,0)

CALL_FN(call_chdir, LINUX_CHDIR, m->m3_p1 + data_seg, 0,0,0,0)

CALL_FN(call_mknod, LINUX_MKNOD, m->m1_p1 + data_seg, m->m1_i2, m->m1_i3,0,0)

CALL_FN(call_chmod, LINUX_CHMOD, m->m3_p1 + data_seg, m->m3_i2,0,0,0)

CALL_FN(call_chown, LINUX_CHOWN, m->m1_p1 + data_seg,  m->m1_i2, m->m1_i3,0,0)

CALL_FN(call_getpid, LINUX_GETPID, 0,0,0,0,0)

CALL_FN(call_umount, LINUX_UMOUNT, m->m3_p1 + data_seg, 0,0,0,0)

CALL_FN(call_setuid, LINUX_SETUID, m->m1_i1, 0,0,0,0)

CALL_FN(call_getuid, LINUX_GETUID, 0,0,0,0,0)

CALL_FN(call_stime, LINUX_STIME, m->m2_l1, 0,0,0,0)

CALL_FN(call_alarm, LINUX_ALARM, m->m1_i1, 0,0,0,0)

CALL_FN(call_pause, LINUX_PAUSE, 0,0,0,0,0)

CALL_FN(call_access, LINUX_ACCESS, m->m3_p1 + data_seg, m->m3_i2, 0,0,0)

CALL_FN(call_sync, LINUX_SYNC, 0,0,0,0,0)

CALL_FN(call_kill, LINUX_KILL, m->m1_i1, m->m1_i2,0,0,0)

CALL_FN(call_rename, LINUX_RENAME, m->m1_p1 + data_seg, m->m1_p2 + data_seg,0,0,0)

CALL_FN(call_mkdir, LINUX_MKDIR, m->m1_p1 + data_seg, m->m1_i2,0,0,0)

CALL_FN(call_rmdir, LINUX_RMDIR, m->m3_p1 + data_seg, 0,0,0,0)

CALL_FN(call_symlink, LINUX_SYMLINK, m->m1_p1 + data_seg, m->m1_p2,0,0,0)

CALL_FN(call_setgid, LINUX_SETGID, m->m1_i1, 0,0,0,0)

CALL_FN(call_getgid, LINUX_GETGID,  0, 0,0,0,0)

CALL_FN(call_rdlnk, LINUX_READLINK, m->m1_p1 + data_seg, m->m1_p2 + data_seg, m->m1_i2,0,0)

CALL_FN(call_umask, LINUX_UMASK,  m->m1_i1, 0,0,0,0)

CALL_FN(call_chroot, LINUX_CHROOT,  m->m3_p1 + data_seg, 0,0,0,0)

CALL_FN(call_setsid, LINUX_SETSID,  0,0,0,0,0)

CALL_FN(call_getpgrp, LINUX_GETPGRP, m->m1_i1, m->m1_p1 + data_seg,0,0,0)

CALL_FN(call_fstatfs, LINUX_FSTATFS, m->m1_i1, m->m1_p1 + data_seg,0,0,0)

CALL_FN(call_fchdir, LINUX_FCHDIR, m->m1_i1, 0,0,0,0)

CALL_FN(call_fsync, LINUX_FSYNC, m->m1_i1, 0,0,0,0)

CALL_FN(call_setpriority, LINUX_SETPRIORITY, m->m1_i1, m->m1_i2, m->m1_i3,0,0)

CALL_FN(call_truncate, LINUX_TRUNCATE, m->m2_p1 + data_seg, m->m2_l1,0,0,0)

CALL_FN(call_ftruncate, LINUX_FTRUNCATE, m->m2_l2, m->m2_l1,0,0,0)

CALL_FN(call_fchmod, LINUX_FCHMOD, m->m3_i1, m->m3_i2, 0,0,0)

CALL_FN(call_fchown, LINUX_FCHOWN, m->m1_i1, m->m1_i2, m->m1_i3,0,0)

CALL_FN(call_munmap, LINUX_MUNMAP, m->VMUM_ADDR + data_seg, m->VMUM_LEN, 0,0,0)

CALL_FN(call_getdents, LINUX_GETDENTS, m->m1_i1, m->m1_p1 + data_seg, m->m1_i2,0,0)

CALL_FN(call_select, LINUX_SELECT, m->SEL_NFDS, m->SEL_READFDS + data_seg, m->SEL_WRITEFDS + data_seg, m->SEL_ERRORFDS + data_seg, m->SEL_TIMEOUT + data_seg)



/*********** SPECIAL CASES  *************/

// There is no LINUX_SETEUID, SETEGID so we implement with SETRESUID, SETRESGID
CALL_FN(call_seteuid, LINUX_SETRESUID, -1, m->m1_i1, -1,0,0)

CALL_FN(call_setegid, LINUX_SETRESGID, -1, m->m1_i1, -1,0,0)

// Will need to separate munmap and munmap_text
CALL_FN(call_munmap_text, LINUX_MUNMAP, m->VMUM_ADDR + code_seg, m->VMUM_LEN, 0,0,0)

/****************************************************

 UNKNOWN CALLS

***************************************************/

// I have found no information on this syscall. Neither on minix or linux !
void call_fs_ready (message* m)
{
  FATAL("Error : FS_READY system call invoked ! Unknown by Linux.\n");
}

// I have no info on this syscall.
void call_cprof (message* m)
{
  FATAL("Error : unknown system call CPROF invoked !\n");
}

// I have no info on this syscall.
void call_getsysinfo_up (message* m)
{
  FATAL("Error : unknown system call GETSYSINFO_UP invoked !\n");
}

// I have no info on this syscall.
void call_getsysinfo (message* m)
{
  FATAL("Error : unknown system call GETSYSINFO invoked !\n");
}

// I have no info on this syscall.
void call_sprof (message* m)
{
  FATAL("Error : unknown system call SPROF invoked !\n");
}

// Special server control function. Unknown by Linux !
void call_svrctl (message* m)
{
  FATAL("Error : Minix specific system call SRVCTL invoked !\n");
}

/****************************************************

NOT IMPLEMENTED 

***************************************************/
 
// We do not implement ptrace. It would never work anyway. According to the internets recursive ptraces are forbidden
void call_ptrace (message* m)
{
  FATAL("Error : ptrace was called ! Not implemented.\n");
}

// In minix, dup() is implemented in fcntl.
void call_dup (message* m)
{
  FATAL("Error : DUP system call invoked directly !\n");
}

// In minix, signal() is implemented via sigaction.
void call_signal (message* m)
{
  FATAL("Error : SIGNAL system call invoked directly !\n");
}

// This shouldn't get called at all 
void call_sigreturn (message* m)
{
  FATAL("Error : SIGRETURN system call invoked !\n");
}

/* The form of this call is OS specific. Disallow it. 
   We don't want to implement reboot anyway. */

void call_reboot (message* m)
{
  FATAL("Error : REBOOT system call invoked !\n");
}


/*
  This call is not completely standardised across OS's 
(  This is generally possible with superuser-only Unix syscalls.)
  It is Minix specific and should not be allowed here.
*/
void call_mount (message* m)
{
  FATAL("Error :MOUNT system call invoked ! Exiting\n");
}

/*
  Minix call sysuname(2) - transfer uname(3) strings.  
 This is used in the implementation of Minix uname. 
 For now this syscall return an error
 But it can also be implemented here later (unlikely!)
*/
void call_sysuname (message* m)
{
  COMMENT("Sysuname called !\n");
}

/****************************************************

SPECIAL FORMS

***************************************************/

void call_exit (message* m)
{
  char int_instr[4] = {0xcd, 0x80, 0xcd, 0x80};
  int exit_code;

  exit_code = m->m1_i1;
  regs.ebx = exit_code;
  regs.eax = LINUX_EXIT;
  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip + code_seg, (* (int*) int_instr));

  ptrace (PTRACE_CONT, fork_pid,  NULL, 0);  // child process should be terminated after this !

  COMMENT("exiting process %d with code %d\n", fork_pid,exit_code ); 
  _exit(exit_code);
}

// Minix first, then Linux
int termios_translations_i[][2] =
  {
    { 0x0001, BRKINT},  
    { 0x0002, ICRNL},
    { 0x0004, IGNBRK},
    { 0x0008, IGNCR},
    { 0x0010, IGNPAR},
    { 0x0020, INLCR},
    { 0x0040, INPCK},
    { 0x0080, ISTRIP},
    { 0x0100, IXOFF},
    { 0x0200, IXON},
    { 0x0400, PARMRK}
  };

int n_termios_translations_i = 11;

int n_termios_mask_i = ~(BRKINT | ICRNL | IGNBRK | IGNCR | IGNPAR | INLCR | INPCK | ISTRIP | IXOFF | IXON | PARMRK);

int termios_translations_o[][2] =
  {
    { 0x0001, OPOST}
  };

int n_termios_translations_o = 1;

int n_termios_mask_o = ~(OPOST);

int termios_translations_c[][2] =
  {
    { 0x0001, CLOCAL},
    { 0x0002, CREAD},
    { 0x0004, CS6},
    { 0x0008, CS7},
    { 0x0010, CSTOPB},
    { 0x0020, HUPCL},
    { 0x0040, PARENB},
    { 0x0080, PARODD}
  };

int n_termios_translations_c = 8;

int n_termios_mask_c = ~(CLOCAL | CREAD | CS6 | CS7 | CSTOPB | HUPCL | PARENB | PARODD);

int termios_translations_l[][2] =
  {
    { 0x0001, ECHO},
    { 0x0002, ECHOE},
    { 0x0004, ECHOK},
    { 0x0008, ECHONL},
    { 0x0010, ICANON},
    { 0x0020, IEXTEN},
    { 0x0040, ISIG},
    { 0x0080, NOFLSH},
    { 0x0100, TOSTOP}
  };

int n_termios_translations_l = 9;

int n_termios_mask_l = ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | IEXTEN | ISIG | NOFLSH | TOSTOP);

int termios_translations_cc[][2] =
  {
    { 0, VEOF},
    { 1, VEOL},
    { 2, VERASE},
    { 3, VINTR},
    { 4, VKILL },
    { 5, VMIN},
    { 6, VQUIT},
    { 7, VTIME},
    { 8, VSUSP},
    { 9, VSTART},
    { 10, VSTOP}
  };

int n_termios_translations_cc = 11;


int termios_flag_translate_m2l(int minix_flags, int table[][2], int table_size)
{
  int i;
  int linux_flags = 0;
  for (i = 0 ; i < table_size ; i++)
    {
      if (table[i][0] & minix_flags)
	linux_flags |= table[i][1];
    }
  return linux_flags;
}

// Note : we preserve Linux flags that we don't know about. (they remain even in minix_flags)
int termios_flag_translate_l2m(int linux_flags, int table[][2], int table_size)
{
  int i;
  int minix_flags = 0;
  for (i = 0 ; i < table_size ; i++)
    {
      //turn off flag if needed 
      if (table[i][1] & linux_flags)
	{
	  minix_flags |= table[i][0];
	}
    }
  return minix_flags;
}

// TODO : this is BROKEN as the flags inside termios are different under minix and linux !
void call_ioctl (message* m)
{
  struct termios* p_ltios;
  struct minix_termios mtios;
  struct termios ltios;
  int request;
  int termios_addr;
  int i, nccs;
  static int iflag_rest;
  static int oflag_rest;
  static int cflag_rest;
  static int lflag_rest;

  minix_cc_t minix_c_cc[MINIX_NCCS];
  static cc_t c_line = 0;  // saved at every TCGETS, restored at every TCSETS

  termios_addr = (int)m->ADDRESS + data_seg;

  switch (m->TTY_REQUEST)
    {
      //TODO : TCFLOW etc (I believe this is the same as Linux TCXONC ?)
    case MINIX_TCGETS: request = TCGETS ; break; 
    case MINIX_TCSETS: request = TCSETS ; break; 
    case MINIX_TCSETSW: request = TCSETSW ; break; 
    case MINIX_TCSETSF: request = TCSETSF ; break; 
    case MINIX_TCSBRK: request = TCSBRK ; break; 
    case MINIX_TCFLSH: request = TCFLSH ; break; 
    case MINIX_TIOCGWINSZ: request = TIOCGWINSZ; break; 
    case MINIX_TIOCSWINSZ: request = TIOCSWINSZ ; break; 
    case MINIX_TIOCGPGRP: request = TIOCGPGRP ; break; 
    case MINIX_TIOCSPGRP: request = TIOCSPGRP ; break; 
    default : 
      {
	COMMENT("Error : Unknown IOCTL %x!\n", m->TTY_REQUEST);
	m->m_type = -1;
	return;
      }
    }
  regs.ebx = m->TTY_LINE;
  regs.ecx = request;
  switch(request)
    {
    case TCGETS:
      regs.edx = (int)scratchpad;
      break;

    case TCSETS:
    case TCSETSW:
    case TCSETSF:
      // convert struct termios to Linux style 
      regs.edx = (int)scratchpad;
      copy_from_child(termios_addr, &mtios, sizeof(struct minix_termios));
      p_ltios = (struct termios*) scratchpad;
      p_ltios->c_iflag = (iflag_rest) | termios_flag_translate_m2l(mtios.c_iflag, termios_translations_i, n_termios_translations_i) ;	
      p_ltios->c_oflag = (oflag_rest) | termios_flag_translate_m2l(mtios.c_oflag, termios_translations_o, n_termios_translations_o) ;	
      p_ltios->c_cflag = (cflag_rest) | termios_flag_translate_m2l(mtios.c_cflag, termios_translations_c, n_termios_translations_c) ;	
      p_ltios->c_lflag = (lflag_rest) | termios_flag_translate_m2l(mtios.c_lflag, termios_translations_l, n_termios_translations_l) ;	
      p_ltios->c_ispeed = mtios.c_ispeed;
      p_ltios->c_ospeed = mtios.c_ospeed;
      p_ltios->c_line = c_line;

      nccs = MIN(MINIX_NCCS, NCCS);    
      for (i = 0 ; i < NCCS ; i++)
	p_ltios->c_cc[i] = 0;
      for (i = 0 ; i < nccs ; i++)
	p_ltios->c_cc[i] = mtios.c_cc[i];

      copy_scratchpad_to_child();
      break;
    default:
      break;
    }

  do_call(LINUX_IOCTL);   

  switch(request)
    {
    case TCGETS:
      //       fix struct termios 
      copy_scratchpad_from_child();
      p_ltios = (struct termios*) scratchpad;

      iflag_rest = p_ltios->c_iflag & n_termios_mask_i; // Save flags unknown by Minix
      cflag_rest = p_ltios->c_cflag & n_termios_mask_c; 
      oflag_rest = p_ltios->c_oflag & n_termios_mask_o; 
      lflag_rest = p_ltios->c_lflag & n_termios_mask_l; 

      mtios.c_iflag = termios_flag_translate_l2m(p_ltios->c_iflag, termios_translations_i, n_termios_translations_i) ;	
      mtios.c_oflag = termios_flag_translate_l2m(p_ltios->c_oflag, termios_translations_o, n_termios_translations_o) ;	
      mtios.c_cflag = termios_flag_translate_l2m(p_ltios->c_cflag, termios_translations_c, n_termios_translations_c) ;	
      mtios.c_lflag = termios_flag_translate_l2m(p_ltios->c_lflag, termios_translations_l, n_termios_translations_l) ;	

      c_line = p_ltios->c_line;
      
      mtios.c_ispeed = p_ltios->c_ispeed;
      mtios.c_ospeed = p_ltios->c_ospeed;
      nccs = MIN(MINIX_NCCS, NCCS);    
      for (i = 0 ; i < MINIX_NCCS ; i++)
	mtios.c_cc[i] = 0;
      for (i = 0 ; i < nccs ; i++)
	mtios.c_cc[i] = p_ltios->c_cc[i];


      copy_to_child((void*) &mtios, termios_addr, sizeof(struct minix_termios)); 
      break;

    case TCSETS:
    case TCSETSW:
    case TCSETSF:
      break;

    default:
      break;
    }

  m->m_type = regs.eax;
}

void call_brk (message* m)
{
  unsigned int newsize;
  unsigned int brksize;


  regs.ebx = (unsigned int) m->m1_p1 + data_seg; // addr
  brksize = regs.ebx;

  do_call(LINUX_BRK);  

  newsize = regs.eax; // Return val
  if (newsize < brksize) // error ?
    {
      m->m_type = -1;
    }
  else
    {
      m->m_type = 0;
      m->m2_p1 = (char*) (newsize - data_seg);
    }
}


/* This implements stat, fstat and lstat.
We have to manually translate the Linux (new) stat struct to Minix's stat struct. */

// NB : The minix lstat() code assumes LSTAT might not exist. No problem here

void call_stat (message* m, int mode)
{
  int save_buf[128];
  int i;
  struct linux_stat64 result;
  struct minix_stat minix_result;
  
  int minix_stat_addr; // struct stat* buf;
  
  memset(scratchpad, 0, sizeof(result));
  
  copy_scratchpad_to_child();

  regs.ecx = (int)scratchpad;
  switch(mode)
    {
    case CALL_FSTAT:
      regs.ebx = m->m1_i1; // fd
      minix_stat_addr = (int) m->m1_p1 + data_seg; // struct stat* buf;
      do_call(LINUX_FSTAT64);  
      break;
    case CALL_STAT:
      regs.ebx = (int) m->m1_p1 + data_seg; // path
      minix_stat_addr = (int) m->m1_p2 + data_seg; // struct stat* buf;     
      do_call(LINUX_STAT64);  
      break;
    case CALL_LSTAT:
      regs.ebx = (int) m->m1_p1 + data_seg; // path
      {
	char* strbuf[256];
	copy_from_child(regs.ebx, (void*) strbuf, 256);
	//	COMMENT("Statting from %s\n", strbuf);
      }
      minix_stat_addr = (int) m->m1_p2 + data_seg; // struct stat* buf;     
      do_call(LINUX_LSTAT64);  
      break;
    }

  m->m_type = regs.eax;


  copy_scratchpad_from_child();

  result = * ((struct linux_stat64*) scratchpad);

  minix_result.st_dev = result.st_dev;		       
  minix_result.st_ino = result.st_ino;
  minix_result.st_mode = result.st_mode;	
  minix_result.st_nlink = result.st_nlink;
  minix_result.st_uid = result.st_uid;	
  minix_result.st_gid = result.st_gid;	
  minix_result.st_rdev = result.st_rdev;
  minix_result.st_size = result.st_size;	
  minix_result.st_atime_minix = result.st_atime;
  minix_result.st_mtime_minix = result.st_mtime;
  minix_result.st_ctime_minix = result.st_ctime;

  copy_to_child(&minix_result, minix_stat_addr, sizeof(struct minix_stat));
  
}

 
// utime (char* name, struct utimbuf *timp)
void call_utime (message* m)
{
  void* timp;
  if ((m->m2_i1) == 0)
    timp = NULL;
  else
// This will point to a structure (m2_l1, m2_l2) equivalent to timp.
    timp = (void*) mess_addr + data_seg + 12; 

  regs.ebx = (int) m->m2_p1 + data_seg; //name	
  regs.ecx = (int) timp; 

  do_call(MINIX_UTIME);   
  m->m_type = regs.eax; 
}

/*
 * To avoid negative return values, Linux "getpriority()" will
 * not return the normal nice-value, but a negated value that
 * has been offset by 20 (ie it returns 40..1 instead of -20..19)
 * to stay compatible.
 */

void call_getpriority (message* m)
{
  int real_prio;
  regs.ebx = m->m1_i1; //which
  regs.ecx = m->m1_i2; //who

  do_call(LINUX_GETPRIORITY);
  if (regs.eax < 0)
    {
    m->m_type = regs.eax; // this is an error
    }
  else 
    {
      real_prio = -(regs.eax - 20);
      m->m_type = real_prio - PRIO_MIN;   // Convert 40..1 ret value into 0...max
    }
}

void call_gettimeofday (message* m)
{
  regs.ebx = (int) mess_addr + data_seg + 12;	 // Fill equiv. timeval struct (m2_l1, m2_l2)to be read by minix
  regs.ecx = 0; // time zone info not used in minix
  do_call(LINUX_GETTIMEOFDAY);   
  m->m_type = regs.eax;
}

// Unfortunately, the status bitfield is not the same under Minix and Linux, it has to be translated
// There is also the gotcha : signal 7 is WAIT under minix and WAITPID under linux
// mode = 0 for wait and 1 for waitpid
void call_waitpid (message* m, int mode)
{
  int linux_status = 0;
  int minix_status = 0;

  if(mode) 
    regs.ebx = m->m1_i1; //pid
  else 
    regs.ebx = -1;

  regs.ecx = (int)scratchpad;
  if(mode)
    regs.edx = m->m1_i2; //options
  else regs.edx = 0;

  do {
    do_call(LINUX_WAITPID);   
  }
  while(regs.eax == -ERESTARTSYS); // It can happen that this syscall is interrupted, need to restart manually !
  // see also http://www.secureprogramming.com/?action=view&feature=recipes&recipeid=8

  copy_scratchpad_from_child();
  linux_status = *(int*)scratchpad;

  if(WIFEXITED(linux_status))
    {
      minix_status = (WEXITSTATUS(linux_status) << 8);
    }
  else if(WIFSIGNALED(linux_status))
    {
      minix_status = WTERMSIG(linux_status) & 0xff;
    }
  else if(WIFSTOPPED(linux_status))
    {
      minix_status = (WSTOPSIG(linux_status) << 8) | (0177);
    }
  
  if(minix_status & 0xffff0000)
    {
      FATAL("BUG : minix status\n"); // should help me find bugs here
    }

  m->m_type = regs.eax;

  m->m2_i1 = minix_status;
}

void call_time (message* m)
{
  regs.ebx = (int)NULL; // No argument. Give time in the return value only.
  do_call(LINUX_TIME);   
  m->m2_l1 = regs.eax;  // minix looks for return value here
  m->m_type = regs.eax;
}

void call_lseek (message* m)
{
  regs.ebx = m->m2_i1; //fd
  regs.ecx = m->m2_l1; //offset
  regs.edx = m->m2_i2;  //whence
  do_call(LINUX_LSEEK);   
  m->m2_l1 = regs.eax; // Minix reads return val here for some reason
  m->m_type = regs.eax;
}

void call_open (message* m)
{
  char* buf[256];
  int flags;
  flags = m->m1_i2;
  if (flags & O_CREAT)
    {
      regs.ebx = (int) m->m1_p1 + data_seg; // name
      regs.ecx = flags;
      regs.edx = m->m1_i3; //mode
    }
  else
    {
      regs.ebx = (int) m->m3_p1 + data_seg; // name
      regs.ecx = flags;
    }

  copy_from_child(regs.ebx, buf, 256/4);
  buf[10] = 0;

  do_call(LINUX_OPEN);   
  m->m_type = regs.eax;
  /*  if ( (! (flags & O_CREAT)) && regs.eax < 0)
      COMMENT("failed to open %s\n", buf);
  */
}

/* This only minix syscall I know with more than 5 arguments.
Passing these to Linux is a different ball game.
The arguments are reordered and packed in the message so that Linux can read them
*/

void call_mmap (message* m)
{
  void* addr;
  size_t len;
  int prot;
  int flags;
  int fd;
  off_t offset;
  int retval;

  addr = (void*) m->VMM_ADDR;
  len = (size_t) m->VMM_LEN;
  prot = m->VMM_PROT;
  flags = m->VMM_FLAGS;
  fd = m->VMM_FD;
  offset = (off_t) m->VMM_OFFSET;
 
  ptrace (PTRACE_POKEDATA, fork_pid,  mess_addr, addr);
  ptrace (PTRACE_POKEDATA, fork_pid + 4,  mess_addr, len);  
  ptrace (PTRACE_POKEDATA, fork_pid + 8,  mess_addr, prot); 
  ptrace (PTRACE_POKEDATA, fork_pid + 12,  mess_addr, flags); 
  ptrace (PTRACE_POKEDATA, fork_pid + 16,  mess_addr, fd); 
  ptrace (PTRACE_POKEDATA, fork_pid + 20,  mess_addr, offset >> 12); 
  /* We use mmap2, only difference is that offset is in multiples of 4096.
     This shouldn't cause more problems as, according to the manpage, offset should be a multiple of page size anyway ! */

  regs.ebx = (int) mess_addr + data_seg; // For nargs > 5, Linux just wants a pointer to the arguments.
  do_call(LINUX_MMAP2);   
  retval = regs.eax; 

  // Give minix the return val it expects
  if(retval < 0)
    m->m_type = retval;
  else 
    {
     m->m_type = OK;
     m->VMM_ADDR = retval;
    }
}

// This is used in Minix lseek64. Not really a standard syscall ?
// TODO : test this... (I have tested a bit with mkfs now !)
void call_llseek (message* m)
{
  int fd;
  unsigned int offset_lo, offset_hi;
  int whence;
  
  int result_hi, result_lo;

  fd = m->m2_i1;
  offset_lo = m->m2_l1;
  offset_hi = m->m2_l2;
  whence = m->m2_i2;
  
  regs.ebx = fd;
  regs.ecx = offset_hi;
  regs.edx = offset_lo;
  regs.esi = (int) mess_addr + data_seg; // result
  regs.edi = whence;

  // *result (temp. location). This should point to a 64 bit int.
  do_call(LINUX_LLSEEK); 

  result_lo = ptrace (PTRACE_PEEKDATA, fork_pid, mess_addr + data_seg, 0); 
  result_hi = ptrace (PTRACE_PEEKDATA, fork_pid, mess_addr + data_seg + 4, 0); 

  // Restore everything because it might have been wiped out
  m->m2_i1 = fd;
  m->m2_i2 = whence;
  // We want the new offset values here
  m->m2_l1 = result_lo;
  m->m2_l2 = result_hi;
  m->m_type = regs.eax;
}



/* Convert linux sigset_t struct to minix's ( which is just a long int)
 We only take the 23 posix signals we know about into account. 
 They are all in the first word of struct sigset_t
 Note SIGEMT and SIGWINCH don't match, we don't support them, using them will cause problems 
*/


/* Signal numbers/names match between Linux and Minix (mostly)
no translation needed
TODO : check that this works. Giving correct pointers to callback functions may be tricky
*/
// TODO : bug ! translate linux sigset_t's to minix sigset_t
// and linux sigaction <-> minix sigaction (assumed to be different)

void call_sigaction (message* m)
{
  struct minix_sigaction m_act, m_oact;
  unsigned long m_set, m_oset;
  struct sigaction* p_act = (struct sigaction*) scratchpad;
  struct sigaction* p_oact = (struct sigaction*) (scratchpad + sizeof(struct sigaction));


  copy_from_child ((int)m->m1_p1+data_seg,
		   &m_act,
		   sizeof(struct minix_sigaction));

  memset(p_act, 0, sizeof(struct sigaction));
  memset(p_oact, 0, sizeof(struct sigaction));
  p_act->sa_handler = m_act.minix_sa_handler + code_seg;
  p_act->sa_flags = m_act.sa_flags;
  p_act->sa_mask.__val[0] = m_act.sa_mask;

  copy_scratchpad_to_child();
  regs.ebx = m->m1_i2; // sig
  regs.ecx = (int)p_act;
  regs.edx = (int)p_oact;

  COMMENT("SIGACTION for signal %d!\n", (int)regs.ebx);
  do_call(LINUX_SIGACTION);   

  copy_scratchpad_from_child();
  m_oact.minix_sa_handler= p_oact->sa_handler - code_seg;
  m_oact.sa_flags = p_oact->sa_flags; 
  m_oact.sa_mask = p_oact->sa_mask.__val[0];

  copy_to_child(&m_oact, (int) m->m1_p2 + data_seg, sizeof(struct minix_sigaction));
  m->m_type = regs.eax;

}

void call_sigpending (message* m)
{
  unsigned long minix_set;
  int* set_addr = scratchpad;

  regs.ebx = (int)set_addr;
  do_call(LINUX_SIGPENDING);   

  copy_scratchpad_from_child();
  minix_set = set_addr[0];
  
  m->m2_l1 = minix_set;
  m->m_type = regs.eax;
}

void call_sigsuspend (message* m)
{
  int* set_addr = scratchpad;

  memset(set_addr, 0, sizeof(sigset_t));
  set_addr[0] = m->m2_l1; // set
  copy_scratchpad_to_child();  

  regs.ebx = (int)set_addr;
  do_call(LINUX_SIGSUSPEND);   

  m->m_type = regs.eax;
}

#define SIG_INQUIRE        4	

/* 
   We must translate minix sigset_t (unsigned long)
   to linux sigset_t which is a struct (1024 bits = 32 unsigned longs ! Only the first one matters, we set the rest to 0)
*/
void call_sigprocmask (message* m)
{
  unsigned long minix_set, minix_oset;
  sigset_t* set, *oset;

  int* set_addr = scratchpad;
  int* oset_addr = scratchpad + sizeof(sigset_t);

  memset(scratchpad, 0, sizeof(sigset_t));
  
  if(m->m2_i1 == SIG_INQUIRE)
    set_addr = 0; // set = NULL, just request oset
  else
    {
      minix_set =  m->m2_l1;

      ERROR("set : %lx\n", minix_set);
      set_addr[0] = minix_set;
    }
  
  copy_scratchpad_to_child();

  regs.ebx = m->m2_i1; // how
  regs.ecx = (int)set_addr;  // set
  regs.edx = (int)oset_addr;  // oset (Minix always requires oset to be filled)
  do_call(LINUX_SIGPROCMASK);   

  copy_scratchpad_from_child();
  minix_oset = oset_addr[0];
  
  m->m2_l1 = minix_oset;
  
  m->m_type = regs.eax;
}

// The arguments need doctoring a bit
void call_pipe (message* m)
{
  int pipefd[2];
  regs.ebx = (int) mess_addr + data_seg;	 // int pipefd[2]
  do_call(LINUX_PIPE);

  pipefd[0] = ptrace (PTRACE_PEEKDATA, fork_pid, mess_addr + data_seg, 0);   
  pipefd[1] = ptrace (PTRACE_PEEKDATA, fork_pid, mess_addr + data_seg + 4, 0);   

  m->m1_i1 = pipefd[0];
  m->m1_i2 = pipefd[1];
  m->m_type = regs.eax;
}

/*
  Clock_t is a lont int. 4 clock_t's should fit in a message !
  Otherwise we have a problem. 
*/
void call_times (message* m)
{
  struct tms * tms_return;
  tms_return = (struct tms*) scratchpad;

  regs.ebx = (int)scratchpad;
  do_call(LINUX_TIMES);   

  copy_scratchpad_from_child();
  
  // Put data back in return message, in the right order this time
  m->m4_l1 = tms_return->tms_utime;
  m->m4_l2 = tms_return->tms_stime;
  m->m4_l3 = tms_return->tms_cutime;
  m->m4_l4 = tms_return->tms_cstime;
  m->m_type = regs.eax;
}

#define F_FREESP           8	/* fcntl command not present in Linux */

void call_fcntl (message* m)
{
  regs.ebx = m->m1_i1;	 //fd
  regs.ecx = m->m1_i2; // cmd

  if (regs.ecx == F_FREESP)         // unsupported command
    {
      m->m_type = -EINVAL; // is EBADRQC better for this?
      return;
    }
  //decide whether regs.edx is args or lock  
  switch(regs.ecx)  
    {
    case F_DUPFD:
    case F_SETFD:
    case F_SETFL:
      regs.edx = m->m1_i3;  // arg
      break;
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
      regs.edx = (int) m->m1_p1 + data_seg; // lock
      break;
    default:
      regs.edx = 0;// no arguments, set to 0 to be extrasafe
      break;
    }

  do_call(LINUX_FCNTL);   
  m->m_type = regs.eax;
}

// This is a twist on the normal do_call
void call_fork (message* m)
{
  int minix_child_pid;
  int i, s1, s2, err;
  int instr_save;  // Save 4 bytes at [eip]
  char int_instr[4] = {0xcd, 0x80, 0x90, 0x90};
  struct user_regs_struct child_regs;
  
  options_set = 0; // We will need to configure ptrace options

  COMMENT("%d calling fork @ eip = %p\n", tracer_pid, regs.eip); 
  
  int save_eip;
  int tracer_fork;

  save_eip = regs.eip;
  // insert the in 0x80 instruction  
  instr_save = ptrace (PTRACE_PEEKDATA, fork_pid, regs.eip + code_seg , 0);
  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip + code_seg , (* (int*) int_instr));
 
  regs.eax = LINUX_FORK;

  ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);

  ptrace (PTRACE_SINGLESTEP, fork_pid,  NULL, 0); 

  /* After the fork we are tracing 2 processes. One of them is stopped with
     SIGTRAP,  and the other with SIG_STOP.
     We then fork the tracing process, the parent will keep tracing the original parent Minix process, and 
     the child tracing process will trace the Minix child.
  */
    
  s1 = waitpid(fork_pid, &status, 0);
  if (s1 < 0) FATAL("Waitpid fail 1 !\n");

  ptrace (PTRACE_GETREGS, fork_pid,  NULL, &regs);

  ptrace (PTRACE_GETEVENTMSG, fork_pid, 0, &minix_child_pid);

  // Restore code in both processes
  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip-2 + code_seg, (void*) instr_save);
  ptrace (PTRACE_POKEDATA, minix_child_pid,  regs.eip-2 + code_seg, (void*) instr_save);

  s1 = waitpid(minix_child_pid, &status, 0);
  if (s1 == -1) 
    {
      int err = errno;
      ERROR("errno = %d\n", err);
      FATAL("Waitpid fail 2 !\n");
    }
  

  ptrace (PTRACE_GETREGS, minix_child_pid,  NULL, &regs);

  ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip-2 + code_seg, (void*) instr_save);
  ptrace (PTRACE_POKEDATA, minix_child_pid,  regs.eip-2 + code_seg, (void*) instr_save);


  m->m_type = 0;
  ptrace (PTRACE_DETACH, minix_child_pid,  NULL, 0); 

  child_context = 1;
  after_signal_handler = 1;

  tracer_fork = fork();
  if (tracer_fork)
    { // parent
      m->m_type = fork_pid;
    }
  else 
    { // child
      m->m_type = 0;
      tracer_pid = getpid();
      fork_pid = minix_child_pid;

      ptrace (PTRACE_ATTACH, fork_pid,  NULL, 0); 

      s1 = waitpid(fork_pid, &status, 0);
      if (s1 == -1) FATAL("Waitpid fail 3 !\n");

      ptrace (PTRACE_GETREGS, fork_pid,  NULL, &regs);
    }
}

// This is used in call_exec below
void sigchild_handler(int signum)
{
  //  child is dead. exiting.
  _exit(0);
}


/* 
   We have to match Minix's EXEC with Linux's EXECVE
   We try to fix the argv and envp from the stack frame
   built by Minix execve()
   argv and envp are there but the string addresses have been shifted, we have to manually
   restore the origin
*/

//PYB: TODO : manage the initial SIGTRAP properly (eip need not be 0!)
void call_exec (message* m)
{
  int frame;
  int argc;
  int frame_size;
  int argv, envp;
  int i;
  int path;

  char buf[256];
  frame = (int) m->m1_p2 + data_seg;
  frame_size = m->m1_i2;
  path = (int) m->m1_p1 + data_seg;

  
  {
    char buf[256];
    copy_from_child (path, &buf, 256);
    COMMENT("%d: Executing %s\n", fork_pid, buf);
  }

  argc = ptrace (PTRACE_PEEKDATA, fork_pid, frame, 0);
  argv = frame + 4;
  envp = frame + 4 + (4 * argc) + 4;  // Counting argc and terminal NULL

  // Fix origin for argv
  for ( i = 0 ; i < argc ; i++)
    {
      int string_addr;
      string_addr = ptrace (PTRACE_PEEKDATA, fork_pid, frame + 4 + 4*i, 0);
      ptrace (PTRACE_POKEDATA, fork_pid, frame + 4 + 4*i , string_addr + frame);      
    }

  if (0 != ptrace (PTRACE_PEEKDATA, fork_pid, frame + 4 + 4*argc, 0))
    FATAL("Error : expected final NULL in argv\n");
      
  //  and now envp
  i = 0;
  while(1)
    {
      int string_addr;
      int pos;
      pos = frame + 4 + 4*argc + 4 + 4*i;
      string_addr = ptrace (PTRACE_PEEKDATA, fork_pid, pos, 0);
      if (string_addr == 0) break; // reached the end
      ptrace (PTRACE_POKEDATA, fork_pid, pos , string_addr + frame);      
      i++;
    } 
 
  regs.ebx = path;
  regs.ecx = argv;
  regs.edx = envp;

  copy_from_child(regs.ebx, buf, 256);
  //  buf[30] = 0;

  {
    int instr_save;  // Save 4 bytes at [eip]
    char int_instr[4] = {0xcd, 0x80, 0xcd, 0x80};
    int status;

    // insert the in 0x80 instruction  
    instr_save = ptrace (PTRACE_PEEKDATA, fork_pid, regs.eip + code_seg, 0);
    ptrace (PTRACE_POKEDATA, fork_pid,  regs.eip + code_seg, (* (int*) int_instr));
    
    regs.eax = LINUX_EXECVE;

    ptrace (PTRACE_SETREGS, fork_pid,  NULL, &regs);
    
    ptrace (PTRACE_SINGLESTEP, fork_pid,  NULL, 0); 
    waitpid (fork_pid, &status, 0);

    ptrace (PTRACE_GETREGS, fork_pid,  NULL, &regs); 
    if (regs.eax < 0)
      {
	COMMENT("EXECVE Fail\n");
	m->m_type = regs.eax;
      }	
    else
      /*
	success : we don't want to trace the child (no use if it's a Linux binary ;
	not possible if it's Minix, because it would cause a recursive ptrace...
      */
      {
	struct sigaction act;

	COMMENT("EXECVE Success. Quitting..\n");
	ptrace (PTRACE_DETACH, fork_pid,  NULL, 0); 

	exit(0);
      }
  }
}

