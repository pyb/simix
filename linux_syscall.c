#include "debugprint.h"
#include "minix-ipc.h"
#include "syscalls.h"
#include "callnr_minix.h"

void linux_syscall(message* mess)
{
  int val;

  switch(mess->m_type)
    {
    case MINIX_EXIT:
      call_exit(mess);
      // does not return !
      
    case MINIX_FORK:
      call_fork(mess);
      return;

    case MINIX_READ:
      call_read(mess);
      return;
      
    case MINIX_WRITE:
      call_write(mess);
      return;

    case MINIX_VM_MUNMAP:
      call_munmap(mess);
      return;

    case MINIX_VM_MUNMAP_TEXT:
      call_munmap_text(mess);
      return;

    case MINIX_OPEN:
      call_open(mess);
      return;
      
    case MINIX_CLOSE:
      call_close(mess);
      return;

    case MINIX_WAIT:
      call_waitpid(mess, 0);
      return;

    case MINIX_CREAT:
      call_creat(mess);
      return;

    case MINIX_LINK:
      call_link(mess);
      return;

    case MINIX_UNLINK:
      call_unlink(mess);
      return;

    case MINIX_WAITPID:
      call_waitpid(mess, 1);
      return;

    case MINIX_CHDIR:
      call_chdir(mess);
      return;

    case MINIX_TIME:
      call_time(mess);
      return;

    case MINIX_MKNOD:
      call_mknod(mess);
      return;

    case MINIX_CHMOD:
      call_chmod(mess);
      return;

    case MINIX_CHOWN:
      call_chown(mess);
      return;

    case MINIX_BRK:
      call_brk(mess);
      return;

    case MINIX_STAT:
      call_stat(mess, CALL_STAT);
      return;

    case MINIX_LSEEK:
      call_lseek(mess);
      return;

    case MINIX_GETPID:
      call_getpid(mess);
      return;

    case MINIX_MOUNT:
      call_mount(mess);
      return;

    case MINIX_UMOUNT:
      call_umount(mess);
      return;

    case MINIX_SETUID:
      call_setuid(mess);
      return;

    case MINIX_GETUID:
      call_getuid(mess);
      return;

    case MINIX_STIME:
      call_stime(mess);
      return;

    case MINIX_PTRACE:
      call_ptrace(mess);
      return;

    case MINIX_ALARM:
      call_alarm(mess);
      return;

    case MINIX_FSTAT:
      call_stat(mess, CALL_FSTAT);
      return;

    case MINIX_PAUSE:
      call_pause(mess);
      return;

    case MINIX_UTIME:
      call_utime(mess);
      return;

    case MINIX_ACCESS:
      call_access(mess);
      return;

    case MINIX_SYNC:
      call_sync(mess);
      return;

    case MINIX_KILL:
      call_kill(mess);
      return;

    case MINIX_RENAME:
      call_rename(mess);
      return;

    case MINIX_MKDIR:
      call_mkdir(mess);
      return;

    case MINIX_RMDIR:
      call_rmdir(mess);
      return;

    case MINIX_DUP:
      call_dup(mess);
      return;

    case MINIX_PIPE:
      call_pipe(mess);
      return;

    case MINIX_TIMES:
      call_times(mess);
      return;

    case MINIX_SYMLINK:
      call_symlink(mess);
      return;

    case MINIX_SETGID:
      call_setgid(mess);
      return;

    case MINIX_GETGID:
      call_getgid(mess);
      return;

    case MINIX_SIGNAL:
      call_signal(mess);
      return;

    case MINIX_RDLNK:
      call_rdlnk(mess);
      return;

    case MINIX_LSTAT:
      call_stat(mess, CALL_LSTAT);
      return;

    case MINIX_IOCTL:
      call_ioctl(mess);
      return;

    case MINIX_FCNTL:
      call_fcntl(mess);
      return;

    case MINIX_FS_READY:
      call_fs_ready(mess);
      return;

    case MINIX_EXEC:
      call_exec(mess);
      return;

    case MINIX_UMASK:
      call_umask(mess);
      return;

    case MINIX_CHROOT:
      call_chroot(mess);
      return;

    case MINIX_SETSID:
      call_setsid(mess);
      return;

    case MINIX_GETPGRP:
      call_getpgrp(mess);
      return;

    case MINIX_SIGACTION:
      call_sigaction(mess);
      return;

    case MINIX_SIGSUSPEND:
      call_sigsuspend(mess);
      return;

    case MINIX_SIGPENDING:
      call_sigpending(mess);
      return;

    case MINIX_SIGPROCMASK:
      call_sigprocmask(mess);
      return;

    case MINIX_SIGRETURN:
      call_sigreturn(mess);
      return;

    case MINIX_REBOOT:
      call_reboot(mess);
      return;

    case MINIX_SVRCTL:
      call_svrctl(mess);
      return;

    case MINIX_SYSUNAME:
      call_sysuname(mess);
      return;

    case MINIX_GETSYSINFO:
      call_getsysinfo(mess);
      return;

    case MINIX_GETDENTS:
      call_getdents(mess);
      return;

    case MINIX_LLSEEK:
      call_llseek(mess);
      return;

    case MINIX_FSTATFS:
      call_fstatfs(mess);
      return;

    case MINIX_SELECT:
      call_select(mess);
      return;

    case MINIX_FCHDIR:
      call_fchdir(mess);
      return;

    case MINIX_FSYNC:
      call_fsync(mess);
      return;

    case MINIX_GETPRIORITY:
      call_getpriority(mess);
      return;

    case MINIX_SETPRIORITY:
      call_setpriority(mess);
      return;

    case MINIX_GETTIMEOFDAY:
      call_gettimeofday(mess);
      return;

    case MINIX_SETEUID:
      call_seteuid(mess);
      return;

    case MINIX_SETEGID:
      call_setegid(mess);
      return;

    case MINIX_TRUNCATE:
      call_truncate(mess);
      return;

    case MINIX_FTRUNCATE:
      call_ftruncate(mess);
      return;

    case MINIX_FCHMOD:
      call_fchmod(mess);
      return;

    case MINIX_FCHOWN:
      call_fchown(mess);
      return;

    case MINIX_GETSYSINFO_UP:
      call_getsysinfo_up(mess);
      return;

    case MINIX_SPROF:
      call_sprof(mess);
      return;

    case MINIX_CPROF:
      call_cprof(mess);
      return;

    default:
      COMMENT("unknown syscall %x.\n", mess->m_type);
      mess->m_type = 0;  // Return val : success
      
    }
}
