#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifdef __i386__
#if defined(__KERNEL__) && !defined(STDC_HEADERS)
#if ((__GNUC_MINOR__ >= 8) || (__GNUC_MAJOR >=3))
#warning "This code is tested with gcc 2.7.2.x only. Using egcs/gcc 2.8.x needs"
#warning "additional patches that have not been sufficiently tested to include by"
#warning "default."
#warning "See http://www.suse.de/~florian/kernel+egcs.html for more information"
#error "Remove this if you have applied the gcc 2.8/egcs patches and wish to use them"
#endif
#endif
#endif

#include <linux/posix_types.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#ifndef __KERNEL_STRICT_NAMES
//typedef fd_set __kernel_fd_set;
//typedef dev_t __kernel_dev_t;
//typedef ino_t __kernel_ino_t;
//typedef mode_t __kernel_mode_t;
typedef nlink_t __kernel_nlink_t;
typedef off_t __kernel_off_t;
typedef pid_t __kernel_pid_t;
typedef uid_t __kernel_uid_t;
typedef gid_t __kernel_gid_t;
//typedef daddr_t __kernel_daddr_t;
//typedef off_t __kernel_loff_t;
typedef size_t __kernel_size_t;
typedef ptrdiff_t __kernel_ptrdiff_t;
typedef time_t __kernel_time_t;
//typedef clock_t __kernel_clock_t;
typedef caddr_t __kernel_caddr_t;
/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

#endif /* __KERNEL_STRICT_NAMES */

/*
 * Below are truly Linux-specific types that should never collide with
 * any application/library that wants linux/types.h.
 */

struct ustat {
	daddr_t	f_tfree;
	__kernel_ino_t		f_tinode;
	char			f_fname[6];
	char			f_fpack[6];
};

#endif /* _LINUX_TYPES_H */
