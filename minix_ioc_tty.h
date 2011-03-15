/*	sys/ioc_tty.h - Terminal ioctl() command codes.
 *							Author: Kees J. Bot
 *								23 Nov 2002
 *
 */

#include "minix_ioctl.h"

/* Terminal ioctls. */

//#define MINIX_TCGETS		MINIX_IOR('T',  8, struct termios) /* tcgetattr */
#define MINIX_TCGETS		MINIX_IOR('T',  8, 36) /* tcgetattr */
#define MINIX_TCSETS		MINIX_IOW('T',  9, 36) /* tcsetattr, TCSANOW */
#define MINIX_TCSETSW		MINIX_IOW('T', 10, 36) /* tcsetattr, TCSADRAIN */
#define MINIX_TCSETSF		MINIX_IOW('T', 11, 36) /* tcsetattr, TCSAFLUSH */
#define MINIX_TCSBRK		MINIX_IOW('T', 12, 4)	      /* tcsendbreak */
//#define MINIX_TCDRAIN		MINIX_IO ('T', 13)		      /* tcdrain */
//#define MINIX_TCFLOW		MINIX_IOW('T', 14, int)	      /* tcflow */
#define MINIX_TCFLSH		MINIX_IOW('T', 15, sizeof(int))	      /* tcflush */
//#define	MINIX_TIOCGWINSZ	MINIX_IOR('T', 16, struct winsize)
#define	MINIX_TIOCGWINSZ	MINIX_IOR('T', 16, 8)
#define	MINIX_TIOCSWINSZ	MINIX_IOW('T', 17, 8)
#define	MINIX_TIOCGPGRP	MINIX_IOW('T', 18, sizeof(int))
#define	MINIX_TIOCSPGRP	MINIX_IOW('T', 19, sizeof(int))
//#define MINIX_TIOCSFON_OLD	MINIX_IOW('T', 20, u8_t [8192])
//#define MINIX_TIOCSFON	MINIX_IOW_BIG(1, u8_t [8192])
