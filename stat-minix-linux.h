#include <time.h>

typedef short          minix_dev_t;	   /* holds (major|minor) device pair */
typedef char           minix_gid_t;	   /* group id */
typedef unsigned long  minix_ino_t; 	   /* i-node number (V3 filesystem) */
typedef unsigned short minix_mode_t;	   /* file type and permissions bits */
typedef short        minix_nlink_t;	   /* number of links to a file */
typedef unsigned long  minix_off_t;	   /* offset within a file */
typedef int            minix_pid_t;	   /* process id (must be signed) */
typedef short          minix_uid_t;	   /* user id */
typedef unsigned long minix_zone_t;	   /* zone number */
typedef unsigned long minix_block_t;	   /* block number */
typedef unsigned long  minix_bit_t;	   /* bit number in a bit map */
typedef unsigned short minix_zone1_t;	   /* zone number for V1 file systems */
typedef unsigned short minix_bitchunk_t; /* collection of bits in a bitmap */

struct minix_stat {
  minix_dev_t st_dev;			/* major/minor device number */
  minix_ino_t st_ino;			/* i-node number */
  minix_mode_t st_mode;		/* file mode, protection bits, etc. */
  short int st_nlink;		/* # links; TEMPORARY HACK: should be nlink_t*/
  minix_uid_t st_uid;			/* uid of the file's owner */
  short int st_gid;		/* gid; TEMPORARY HACK: should be gid_t */
  minix_dev_t st_rdev;
  minix_off_t st_size;		/* file size */
  time_t st_atime_minix;		/* time of last access */
  time_t st_mtime_minix;		/* time of last data modification */
  time_t st_ctime_minix;		/* time of last file status change */
};


struct linux_stat64 {
	unsigned long long	st_dev;
	unsigned char	__pad0[4];

	unsigned long	__st_ino;

	unsigned int	st_mode;
	unsigned int	st_nlink;

	unsigned long	st_uid;
	unsigned long	st_gid;

	unsigned long long	st_rdev;
	unsigned char	__pad3[4];

	long long	st_size;
	unsigned long	st_blksize;

	/* Number 512-byte blocks allocated. */
	unsigned long long	st_blocks;

	unsigned long	st_atime;
	unsigned long	st_atime_nsec;

	unsigned long	st_mtime;
	unsigned int	st_mtime_nsec;

	unsigned long	st_ctime;
	unsigned long	st_ctime_nsec;

	unsigned long long	st_ino;
};


