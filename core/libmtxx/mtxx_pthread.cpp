/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *	
 *	Basic Author: Seiichi Takeda  '2014-Feb-11 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file multios_pthread.c 
 * @brief POSIX pthreadをラッピングしたスレッド制御ライブラリ
 */

#ifdef WIN32
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winbase.h>
#include <process.h>
#include <errno.h>
#endif

#ifndef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef QNX
#include <sys/errno.h>
#else
#include <errno.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <time.h>
#if defined(Linux)
/* pthread_setaffinity_npを使うために _GNU_SOURCEを定義します */
#define _GNU_SOURCE
#define __USE_GNU
#endif
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#endif

#if defined(__AVM2__)
#include <pthread_np.h>
#endif

#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_malloc.h>

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>


#include "mtxx_actor_simple_mpi.h"
#include "mtxx_pthread.h"


#if defined(WIN32)

typedef enum enum_tctl_cmd {
    TCTL_INIT = 1,
    TCTL_eoc,
} enum_tctl_cmd_t;

typedef struct {
    void *arg;
    void *(*thread_func) (void *);
    s3::mtxx_actor_simple_mpi_t *mp;
} win32_startup_thread_args_t;

static unsigned WINAPI win32_startup_thread(void *args)
{
    int result;
    int cmd;
    unsigned status;
    void *ret_ptr=NULL;

    win32_startup_thread_args_t *p = (win32_startup_thread_args_t *) args;
    void *const thread_args =
	mpxx_malloca(sizeof(win32_startup_thread_args_t));

    result =
	s3::mtxx_actor_simple_mpi_recv_request(p->mp, &cmd,
				 s3::MTXX_ACTOR_SIMPLE_MPI_BLOCK);
    if (result) {
	status = ~0;
	goto out;
    }

    if (NULL == thread_args) {
	result =
	    s3::mtxx_actor_simple_mpi_reply(p->mp,
					s3::MTXX_ACTOR_SIMPLE_MPI_RES_NACK);
	if (result) {
	    status = ~0;
	    goto out;
	}
	status = ~0;
	goto out;
    }
    memcpy(thread_args, p, sizeof(win32_startup_thread_args_t));
    p = MPXX_STATIC_CAST(win32_startup_thread_args_t*, thread_args);

    result =
	s3::mtxx_actor_simple_mpi_reply(p->mp,
					s3::MTXX_ACTOR_SIMPLE_MPI_RES_ACK);
    if (result) {
	status = ~0;
	goto out;
    }

    ret_ptr = p->thread_func(p->arg);

    status = 0;
  out:

    if (NULL != thread_args) {
	mpxx_freea(thread_args);
    }

    _endthreadex((unsigned) (size_t) ret_ptr);

    return status;
}
#endif

/**
 * @fn int s3::mtxx_pthread_create(s3::mtxx_pthread_t * pt, void *attr,
 *		       void *(*start_routine) (void *), void *arg)
 * @brief POSIXの pthread_create()関数のマルチOS用ラッパ
 * @param pt multios_pthread_t構造体ポインタ
 * @param attr NULLを指定
 * @param start_routine スレッド起動関数ポインタ
 *	戻り値のポインタ値は無視されます
 * @param arg スレッド起動関数用変数ポインタ
 * @retval 0 成功
 * @retval EAGAIN 別のスレッドを作成するのに十分なリソースがないか、システムで設定されたスレッド数の上限に達していた。
 * @retval EINVAL attrは指定できません(NULL固定です)
 * @retval -1 失敗 その他致命的なエラー
 */
int
s3::mtxx_pthread_create(s3::mtxx_pthread_t * pt, void *attr,
		       void *(*start_routine) (void *), void *arg)
{
    if (NULL != attr) {
	return EINVAL;
    }
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    return pthread_create(pt, attr, start_routine, arg);
#elif defined(WIN32)

    /* WIN32 & C Runtim API */
    {
	int result;
	win32_startup_thread_args_t p;
	s3::mtxx_actor_simple_mpi_t mpi;
	s3::enum_mtxx_actor_simple_mpi_res_t res;
	result = s3::mtxx_actor_simple_mpi_init(&mpi);
	if (result) {
	    return EAGAIN;
	}

	p.arg = arg;
	p.thread_func = start_routine;
	p.mp = &mpi;

	*pt =
	    (HANDLE) _beginthreadex(NULL, 0,
				    win32_startup_thread, (LPVOID) & p, 0,
				    NULL);
	if (*pt != NULL) {
	    result =
		s3::mtxx_actor_simple_mpi_send_request(&mpi, TCTL_INIT,
						     &res);
	    if (s3::MTXX_ACTOR_SIMPLE_MPI_RES_ACK != res) {
		WaitForSingleObject(*pt, INFINITE);
		*pt = NULL;
	    }
	}
	s3::mtxx_actor_simple_mpi_destroy(&mpi);
    }

    if (NULL == *pt) {
	return EAGAIN;
    } else {
	return 0;
    }
#else
#error 'can not define multios_pthread_create()'
#endif
}

/**
 * @fn int s3::mtxx_pthread_join(s3::mtxx_pthread_t *const pt, void **value_ptr)
 * @brief POSIXのpthread_join関数のマルチOS用ラッパ
 * @param pt multios_pthread_t構造体ポインタ
 * @param value_ptr 無視されますのでNULL指定してください
 * @retval 0 成功
 * @retval EDEADLK デッドロックが検出された (例えば、二つのスレッドが互いに join しようとした場合)、または thread に呼び出したスレッドが指定されている。 
 * @retval EINVAL thread が join 可能なスレッドではない。
 *		別のスレッドがすでにこのスレッドの join 待ちである。 
 * @retval ESRCH ID が thread のスレッドが見つからなかった。
 * @retval -1 その他の不明なエラー
 */
int s3::mtxx_pthread_join(s3::mtxx_pthread_t *const pt, void ** const value_ptr)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    /* POSIX */
    return pthread_join(*pt, value_ptr);
#elif defined(WIN32)
    int status;
    DWORD result;

#if 1				/* WIN32 */
    (void) value_ptr;		/* 未使用 */
    result = (int) WaitForSingleObject(*pt, INFINITE);
    if (result == WAIT_FAILED) {
	result = -1;
    } else {
	result = 0;
    }
#if 0
    if (NULL != value_ptr) {
	result = GetExitCodeThread(*pt, (LPDWORD) value_ptr);
    }
#endif
#endif
    CloseHandle(*pt);

    if (result) {
	status = -1;
    } else {
	status = 0;
    }
    return status;
#else
#error 'can not define multios_pthread_join()'
#endif
}

/**
 * @fn void s3::mtxx_pthread_exit(void *const value_ptr)
 * @brief POSIXのpthread_exit()関数のマルチOS用ラッパ
 * @param value_ptr 無視されますのでNULLを指定してください。
 */
void s3::mtxx_pthread_exit(void *const value_ptr)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    /* POSIX */
    pthread_exit(value_ptr);
#elif defined(WIN32)
    (void) value_ptr;		/* 未使用 */
#if 1				/* WIN32 & C Runtime */
    _endthreadex((unsigned) value_ptr);
#else				/* C Runtime */
    ExitThread(0);
#endif
#else				/* others */
#error 'can not define multios_pthread_exit()'
#endif

    return;
}

/**
 * @fn void s3::mtxx_sched_yield(void)
 * @brief POSIXのsched_yield()関数のマルチOS用ラッパ
 */
void s3::mtxx_sched_yield(void)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    /* POSIX */
    sched_yield();
#elif defined(WIN32)
    /* WIN32 */
    Sleep(0);
#else
#error 'can not define multios_sched_yield()'
#endif
    return;
}

/**
 * @fn int s3::mtxx_pthread_cancel(s3::mtxx_pthread_t *const pt)
 * @brief POSIXのpthread_cancel関数のラッパ
 *	WIN32では終了コードを返せるようですが、強制的に-1に設定します。
 * @param pt multios_pthread_t構造体ポインタ
 * @retval 0 成功
 * @retval 0以外 失敗
 */
int s3::mtxx_pthread_cancel(s3::mtxx_pthread_t *const pt)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    /* POSIX */
    return pthread_cancel(*pt);
#elif defined(WIN32)
    /* WIN32 */
    return TerminateThread(*pt, -1);
#else
#error 'can not define multios_pthread_cancel()'
#endif

}

/**
 * @fn int s3::mtxx_pthread_detach(s3::mtxx_pthread_t *const pt)
 * @brief 実行中のスレッドをデタッチ状態にします。
 * @param pt multios_pthread_t構造体ポインタ
 */
int s3::mtxx_pthread_detach(s3::mtxx_pthread_t *const pt)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    return pthread_detach(*pt);
#elif defined(WIN32)
    (void) pt;			/* 未使用 */
    /* Win32ではデフォルトでdetach状態なので */
    return 0;
#else
#error 'can not define multios_pthread_detach()'
#endif
}

/**
 * @fn int s3::mtxx_pthread_setaffinity_np(s3::mtxx_pthread_t *const pt, const uint32_t cpu_mask)
 * @brief ***　機能検証中 ***
 *	スレッドの CPU affinity の設定/取得を行います。
 *      pthread_setaffinity_np関数のラッパです。
 *      ※ QNXは実装されておらず常に -1 を返します。
 *      ※ MACOSXは実装されておらず常に -1 を返します。
 * @param pt multios_pthread_tポインタ
 * @param cpu_mask LSBからCPU0の有効ビットに相当します。1:有効 0:無効
 * @retval 0 成功
 * @retval 0以外 失敗
 */
int s3::mtxx_pthread_setaffinity_np(s3::mtxx_pthread_t *const pt,
				   const uint32_t cpu_mask)
{
#if defined(Linux) || defined(__AVM2__)
    unsigned int j;
#if defined(Linux)
    cpu_set_t cpuset;
#elif defined(__AVM2__)
    cpuset_t cpuset;
#endif

    CPU_ZERO(&cpuset);
    for (j = 0; j < (unsigned int) (sizeof(uint32_t) * 8); j++) {
	if (cpu_mask & (1 << j)) {
	    CPU_SET(j, &cpuset);
	}
    }

    return pthread_setaffinity_np(*pt, sizeof(cpuset), &cpuset);
#elif defined(WIN32)
    DWORD_PTR dword_ptr_result;
    DWORD_PTR dword_ptr_cpu_mask = (DWORD_PTR) cpu_mask;

    dword_ptr_result = SetThreadAffinityMask(*pt, dword_ptr_cpu_mask);

    return (dword_ptr_result) ? 0 : -1;
#elif defined(QNX) || defined(Darwin) || defined(__AVM2__)
    /* @note QNXは実装されていません. */
    /* @note MACOSXは実装されていません. */

    DBMS5(stderr, "multios_pthread_setaffinity_np : pt=%p\n", pt);
    DBMS5(stderr, "multios_pthread_setaffinity_np : cpu_mask=%u\n",
	  cpu_mask);

    /* not impliment */
    return -1;
#else
#error 'can not define multios_pthread_setaffinity_np()'
#endif
}


/**
 * @fn s3::mtxx_pthread_t mtxx_pthread_self(void)
 * @param pt s3::mtxx_pthread_tポインタ
 * @param cpu_mask LSBからCPU0の有効ビットに相当します。1:有効 0:無効
 * @retval 0 成功
 * @retval 0以外 失敗
 */
s3::mtxx_pthread_t mtxx_pthread_self(void)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__)
    return pthread_self();
#elif defined(WIN32)
    return GetCurrentThread();
#else
#error 'can not define multios_pthread_self()'
#endif
}
