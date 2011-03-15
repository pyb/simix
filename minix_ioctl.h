/*	minix/ioctl.h - Ioctl helper definitions.	Author: Kees J. Bot

#include <minix/types.h>

/* Ioctls have the command encoded in the low-order word, and the size
 * of the parameter in the high-order word. The 3 high bits of the high-
 * order word are used to encode the in/out/void status of the parameter.
 */
#define MINIX_IOCPARM_MASK		0x0FFF
#define MINIX_IOCPARM_MASK_BIG	0x0FFFFF
#define MINIX_IOC_VOID		0x20000000
#define MINIX_IOCTYPE_MASK		0xFFFF
#define MINIX_IOC_IN			0x40000000
#define MINIX_IOC_OUT		0x80000000
#define MINIX_IOC_INOUT		(MINIX_IOC_IN |MINIX_IOC_OUT)

/* Flag indicating ioctl format with only one type field, and more bits
 * for the size field (using maskMINIX_IOCPARM_MASK_BIG).
 */
#define MINIX_IOC_BIG		0x10000000

#define MINIX_IO(x,y)	((x << 8) | y |MINIX_IOC_VOID)
#define MINIX_IOR(x,y,t)	((x << 8) | y | ((t &MINIX_IOCPARM_MASK) << 16) |\
				MINIX_IOC_OUT)
#define MINIX_IOW(x,y,t)	((x << 8) | y | ((t &MINIX_IOCPARM_MASK) << 16) |\
				MINIX_IOC_IN)
#define MINIX_IORW(x,y,t)	((x << 8) | y | ((t &MINIX_IOCPARM_MASK) << 16) |\
				MINIX_IOC_INOUT)

#define MINIX_IOW_BIG(y,t)  (y | ((t &MINIX_IOCPARM_MASK_BIG) << 8) \
	|MINIX_IOC_IN |MINIX_IOC_BIG)
#define MINIX_IOR_BIG(y,t)  (y | ((t &MINIX_IOCPARM_MASK_BIG) << 8) \
	|MINIX_IOC_OUT |MINIX_IOC_BIG)
#define MINIX_IORW_BIG(y,t) (y | ((t &MINIX_IOCPARM_MASK_BIG) << 8) \
	|MINIX_IOC_INOUT |MINIX_IOC_BIG)

/* Decode an ioctl call. */
#define _MINIX_IOCTL_SIZE(i)		(((i) >> 16) &MINIX_IOCPARM_MASK)
#define _MINIX_IOCTL_IOR(i)		((i) &MINIX_IOC_OUT)
#define _MINIX_IOCTL_IORW(i)		((i) &MINIX_IOC_INOUT)
#define _MINIX_IOCTL_IOW(i)		((i) &MINIX_IOC_IN)

/* Recognize and decode size of a 'big' ioctl call. */
#define _MINIX_IOCTL_BIG(i) 		((i) &MINIX_IOC_BIG)
#define _MINIX_IOCTL_SIZE_BIG(i)	(((i) >> 8) &MINIX_IOCPARM_MASK_BIG)


