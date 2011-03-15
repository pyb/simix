/* The <sys/wait.h> header contains macros related to wait(). The value
 * returned by wait() and waitpid() depends on whether the process 
 * terminated by an exit() call, was killed by a signal, or was stopped
 * due to job control, as follows:
 *
 *				 High byte   Low byte
 *				+---------------------+
 *	exit(status)		|  status  |    0     |
 *				+---------------------+
 *      killed by signal	|    0     |  signal  |
 *				+---------------------+
 *	stopped (job control)	|  signal  |   0177   |
 *				+---------------------+
 */


#define M_LOW(v)		( (v) & 0377)
#define M_HIGH(v)	( ((v) >> 8) & 0377)

#define M_WNOHANG         1	/* do not wait for child to exit */
#define M_WUNTRACED       2	/* for job control; not implemented */

#define M_WIFEXITED(s)	(M_LOW(s) == 0)			    /* normal exit */
#define M_WEXITSTATUS(s)	(M_HIGH(s))			    /* exit status */
#define M_WTERMSIG(s)	(M_LOW(s) & 0177)		    /* sig value */
#define M_WIFSIGNALED(s)	(((unsigned int)(s)-1 & 0xFFFF) < 0xFF) /* signaled */
#define M_WIFSTOPPED(s)	(M_LOW(s) == 0177)		    /* stopped */
#define M_WSTOPSIG(s)	(M_HIGH(s) & 0377)		    /* stop signal */

