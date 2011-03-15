typedef unsigned short minix_tcflag_t;
typedef unsigned char minix_cc_t;
typedef unsigned int minix_speed_t;

#define MINIX_NCCS		   20	/* size of cc_c array, some extra space
				 * for extensions. */

/* Primary terminal control structure. POSIX Table 7-1. */
struct minix_termios {
  minix_tcflag_t c_iflag;		/* input modes */
  minix_tcflag_t c_oflag;		/* output modes */
  minix_tcflag_t c_cflag;		/* control modes */
  minix_tcflag_t c_lflag;		/* local modes */
  minix_speed_t  c_ispeed;		/* input speed */
  minix_speed_t  c_ospeed;		/* output speed */
  minix_cc_t c_cc[MINIX_NCCS];		/* control characters */
};

/* Window size. This information is stored in the TTY driver but not used.
 * This can be used for screen based applications in a window environment. 
 * The ioctls TIOCGWINSZ and TIOCSWINSZ can be used to get and set this 
 * information.
 */

struct minix_winsize
{
	unsigned short	ws_row;		/* rows, in characters */
	unsigned short	ws_col;		/* columns, in characters */
	unsigned short	ws_xpixel;	/* horizontal size, pixels */
	unsigned short	ws_ypixel;	/* vertical size, pixels */
};
