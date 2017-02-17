#ifndef _LINUX_LIMITS_H
#define _LINUX_LIMITS_H

#include <asm/limits.h>

#include <linux/config.h>

#ifdef CONFIG_REDUCED_MEMORY

#define NR_OPEN		 128

#else /* !CONFIG_REDUCED_MEMORY */

#define NR_OPEN		 256

#endif /* !CONFIG_REDUCED_MEMORY */

#define NGROUPS_MAX       32	/* supplemental group IDs are available */
#ifndef ARG_MAX
#define ARG_MAX       131072	/* # bytes of args + environ for exec() */
#endif
#define CHILD_MAX        999    /* no limit :-) */
#define OPEN_MAX         256	/* # open files a process may have */
#define LINK_MAX         127	/* # links a file may have */
#define MAX_CANON        255	/* size of the canonical input queue */
#define MAX_INPUT        255	/* size of the type-ahead buffer */
#define NAME_MAX         255	/* # chars in a file name */
#ifndef PATH_MAX
#define PATH_MAX        1024	/* # chars in a path name */
#endif
#ifndef PIPE_BUF
#define PIPE_BUF        4096	/* # bytes in atomic write to a pipe */
#endif

#endif
