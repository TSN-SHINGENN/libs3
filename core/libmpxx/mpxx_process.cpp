/**
 *	Copyright 2016 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2016-Nov.-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_process.c
 * @brief 
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#if defined(WIN32)
#include <windows.h>
#include <process.h>
#elif defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
#include <sys/types.h>
#include <sys/wait.h>
#include <process.h>
#endif

#include <stdint.h>

/* this */
#include "mpxx_sys_types.h"
#include "mpxx_process.h"

#ifdef DEBUG
static int debuglevel = 4;
#else
static int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

int mpxx_waitpid(mpxx_pid_t pid, int *stat_loc, int options)
{
#ifdef WIN32
    return _cwait(stat_loc, pid, WAIT_CHILD);
#else
    return waitpid(pid_t pid, int *stat_loc, int options)
#endif
}

int mpxx_kill(mpxx_pid_t pid, int sig)
{
#ifdef WIN32
    int ret;
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
    ret = TerminateProcess(h, 0) ? 0 : -1;
    CloseHandle(h);

    return 0;
#else
    return kill( pid, sig)
#endif
}

