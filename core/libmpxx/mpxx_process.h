#ifndef INC_MPXX_SYS_WAIT_H
#define INC_MPXX_SYS_WAIT_H

#include <sys/types.h>

#ifndef _MPXX_PID_T
#define _MPXX_PID_T
#if defined(WIN32) || defined(XILKERNEL)
typedef int mpxx_pid_t;
#else /* POSIX */
typedef pid_t mpxx_pid_t;
#endif
#endif /* end of _MPXX_PID_T */

#ifdef WIN32
#define MPXX_WAIT_WNOHANG	1	/* dont hang in wait */
#define MPXX_WAIT_WUNTRACED	2	/* tell about stopped, untraced children */
#else
#define MPXX_WAIT_WNOHANG WNOHANG
#define MPXX_WAIT_WUNTRACED WUNTRACED
#endif

int mpxx_waitpid(mpxx_pid_t pid, int *stat_loc, int options);
int mpxx_kill(mpxx_pid_t pid, int sig);

#endif /* end of INC_MPXX_SYS_WAIT_H */
