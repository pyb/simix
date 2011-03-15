#define P_TABLE_SIZE 64

#define MINIX_NCALLS		 111	/* number of system calls allowed */

#define MINIX_EXIT		   1 
#define MINIX_FORK		   2 
#define MINIX_READ		   3 
#define MINIX_WRITE		   4 
#define MINIX_OPEN		   5 
#define MINIX_CLOSE		   6 
#define MINIX_WAIT		   7
#define MINIX_CREAT		   8 
#define MINIX_LINK		   9 
#define MINIX_UNLINK		  10 
#define MINIX_WAITPID		  11
#define MINIX_CHDIR		  12 
#define MINIX_TIME		  13
#define MINIX_MKNOD		  14 
#define MINIX_CHMOD		  15 
#define MINIX_CHOWN		  16 
#define MINIX_BRK		  17
#define MINIX_STAT		  18 
#define MINIX_LSEEK		  19
#define MINIX_GETPID		  20
#define MINIX_MOUNT		  21 
#define MINIX_UMOUNT		  22 
#define MINIX_SETUID		  23
#define MINIX_GETUID		  24
#define MINIX_STIME		  25
#define MINIX_PTRACE		  26
#define MINIX_ALARM		  27
#define MINIX_FSTAT		  28 
#define MINIX_PAUSE		  29
#define MINIX_UTIME		  30 
#define MINIX_ACCESS		  33 
#define MINIX_SYNC		  36 
#define MINIX_KILL		  37
#define MINIX_RENAME		  38
#define MINIX_MKDIR		  39
#define MINIX_RMDIR		  40
#define MINIX_DUP		  41 
#define MINIX_PIPE		  42 
#define MINIX_TIMES		  43
#define MINIX_SYMLINK		  45
#define MINIX_SETGID		  46
#define MINIX_GETGID		  47
#define MINIX_SIGNAL		  48
#define MINIX_RDLNK		  49
#define MINIX_LSTAT		  50
#define MINIX_IOCTL		  54
#define MINIX_FCNTL		  55
#define MINIX_FS_READY	  57
#define MINIX_EXEC		  59
#define MINIX_UMASK		  60 
#define MINIX_CHROOT		  61 
#define MINIX_SETSID		  62
#define MINIX_GETPGRP		  63

/* Posix signal handling. */
#define MINIX_SIGACTION	  71
#define MINIX_SIGSUSPEND	  72
#define MINIX_SIGPENDING	  73
#define MINIX_SIGPROCMASK	  74
#define MINIX_SIGRETURN	  75

#define MINIX_REBOOT		  76
#define MINIX_SVRCTL		  77
#define MINIX_SYSUNAME	  78
#define MINIX_GETSYSINFO	  79	/* to PM or FS */
#define MINIX_GETDENTS	  80	/* to FS */
#define MINIX_LLSEEK		  81	/* to FS */
#define MINIX_FSTATFS	 	  82	/* to FS */
#define MINIX_SELECT            85	/* to FS */
#define MINIX_FCHDIR            86	/* to FS */
#define MINIX_FSYNC             87	/* to FS */
#define MINIX_GETPRIORITY       88	/* to PM */
#define MINIX_SETPRIORITY       89	/* to PM */
#define MINIX_GETTIMEOFDAY      90	/* to PM */
#define MINIX_SETEUID		  91	/* to PM */
#define MINIX_SETEGID		  92	/* to PM */
#define MINIX_TRUNCATE	  93	/* to FS */
#define MINIX_FTRUNCATE	  94	/* to FS */
#define MINIX_FCHMOD		  95	/* to FS */
#define MINIX_FCHOWN		  96	/* to FS */
#define MINIX_GETSYSINFO_UP	  97	/* to PM or FS */
#define MINIX_SPROF             98    /* to PM */
#define MINIX_CPROF             99    /* to PM */

/* Calls provided by PM and FS that are not part of the API */
#define MINIX_EXEC_NEWMEM	100	/* from FS or RS to PM: new memory map for
				 * exec
				 */
#define MINIX_FORK_NB	  	101	/* to PM: special fork call for RS */
#define MINIX_EXEC_RESTART	102	/* to PM: final part of exec for RS */
#define MINIX_PROCSTAT	103	/* to PM */
#define MINIX_GETPROCNR	104	/* to PM */
#define MINIX_ALLOCMEM	105	/* to PM */
#if 0
#define MINIX_FREEMEM		106	/* to PM, not used, not implemented */
#endif
#define MINIX_GETPUID		107	/* to PM: get the uid of a process (endpoint) */
#define MINIX_ADDDMA		108	/* to PM: inform PM about a region of memory
				 * that is used for bus-master DMA
				 */
#define MINIX_DELDMA		109	/* to PM: inform PM that a region of memory
				 * that is no longer used for bus-master DMA
				 */
#define MINIX_GETDMA		110	/* to PM: ask PM for a region of memory
				 * that should not be used for bus-master DMA
				 * any longer
				 */

#define MINIX_DEVCTL		120	/* to FS, map or unmap a device */
#define MINIX_TASK_REPLY	121	/* to FS: reply code from drivers, not 
				 * really a standalone call.
				 */
#define MINIX_MAPDRIVER	122	/* to FS, map a device */

// Other
#define VM_RQ_BASE		0xC00

#define MINIX_VM_MMAP			(VM_RQ_BASE+10)
#	define VMM_ADDR			m5_l1
#	define VMM_LEN			m5_l2
#	define VMM_PROT			m5_c1
#	define VMM_FLAGS		m5_c2
#	define VMM_FD			m5_i1
#	define VMM_OFFSET		m5_i2
#	define VMM_RETADDR		m5_l1	/* result */
#define VM_UMAP			(VM_RQ_BASE+11)
#	define VMU_SEG			m1_i1
#	define VMU_OFFSET		m1_p1
#	define VMU_LENGTH		m1_p2
#	define VMU_RETADDR		m1_p3
#define MINIX_VM_MUNMAP		(VM_RQ_BASE+17)
#	define VMUM_ADDR		m1_p1
#	define VMUM_LEN			m1_i1
#define MINIX_VM_MUNMAP_TEXT          (VM_RQ_BASE+19)

/* Field names for SELECT (FS). */
#define SEL_NFDS       m8_i1
#define SEL_READFDS    m8_p1
#define SEL_WRITEFDS   m8_p2
#define SEL_ERRORFDS   m8_p3
#define SEL_TIMEOUT    m8_p4


/* Field names for messages to block and character device drivers. */
#define DEVICE    	m2_i1	/* major-minor device */
#define IO_ENDPT	m2_i2	/* which (proc/endpoint) wants I/O? */
#define COUNT   	m2_i3	/* how many bytes to transfer */
#define REQUEST 	m2_i3 	/* ioctl request code */
#define POSITION	m2_l1	/* file offset (low 4 bytes) */
#define HIGHPOS		m2_l2	/* file offset (high 4 bytes) */
#define ADDRESS 	m2_p1	/* core buffer address */
#define IO_GRANT 	m2_p1	/* grant id (for DEV_*_S variants) */

/* Field names for messages to TTY driver. */
#define TTY_LINE	DEVICE	/* message parameter: terminal line */
#define TTY_REQUEST	COUNT	/* message parameter: ioctl request code */
#define TTY_SPEK	POSITION/* message parameter: ioctl speed, erasing */
#define TTY_PGRP 	m2_i3	/* message parameter: process group */	
