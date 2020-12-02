/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2014-March-23 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file win32e_pthread_mutex_cond.c
 * @brief pthread_mutex_*()/pthread_cond_* APIのエミュレータのWIN32用ライブラリコード
 */

#ifndef WIN32
#error This source code is program for WIN32
#else
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#if defined(WINVER)
#undef WINVER
#endif
#if defined(_WIN32_WINNT)
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0501	/* このソースコードは WindowsXP以上を対象にします。 */
#define WINVER 0x0501		/*  ↑ */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/timeb.h>
#include <errno.h>

/* libs3 */
#include "libmpxx/mpxx_time.h"

/* this */
#include "_win32e_pthread_mutex_cond.h"

static int win32e_pthread_cond_wait_ex(win32e_pthread_cond_t * const cv,
				       win32e_pthread_mutex_t *
				       const external_mutex,
				       mpxx_mtimespec_t * const ts);

/**
 * @fn int win32e_pthread_mutex_init( win32e_pthread_mutex_t *m_p, const void *attr)
 * @brief POSIXのpthread_mutex_init()関数のWIN32版エミュレータ
 * @param m_p win32e_pthread_mutex_t構造体ポインタ
 * @param attr NULLを指定
 * @return 0…成功、-1…失敗
 */
int win32e_pthread_mutex_init(win32e_pthread_mutex_t * m_p,
			      const void *attr)
{
    (void) attr;		/* 未使用 */

    memset(m_p, 0x0, sizeof(win32e_pthread_mutex_t));
#if 0				/* old */
    InitializeCriticalSection(&m_p->g_section);
#else
    m_p->hSem = CreateSemaphore(NULL, 1, 1, NULL);
    if (NULL == m_p->hSem) {
	return -1;
    }
#endif
    return 0;
}

/**
 * @fn int win32e_pthread_mutex_destroy( win32e_pthread_mutex_t*m_p )
 * @brief POSIXのpthread_mutex_destroy()関数のWIN32版エミュレータ
 * @param m_p win32e_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int win32e_pthread_mutex_destroy(win32e_pthread_mutex_t * m_p)
{
#if 0				/* old */
    DeleteCriticalSection(&m_p->g_section);
    return 0;
#else
    int result;
    result = CloseHandle(m_p->hSem);
    if (result) {
	return 0;
    }
    return -1;
#endif
}

/**
 * @fn int win32e_pthread_mutex_lock( win32e_pthread_mutex_t *m_p)
 * @brief POSIXのpthread_mutex_lock()関数のWIN32版エミュレータ
 * @param m_p win32e_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int win32e_pthread_mutex_lock(win32e_pthread_mutex_t * m_p)
{
#if 0				/* old */
    for (;;) {
	EnterCriticalSection(&m_p->g_section);
	if (m_p->g_section.RecursionCount != 1) {
	    for (;;) {
		/**
		* @note  デッドロックのエミュレーション
		*/
		Sleep(0);
	    }
	}
	break;
    }
    return 0;
#else
    DWORD dwResult;

    dwResult = WaitForSingleObject(m_p->hSem, INFINITE);
    if (dwResult == WAIT_OBJECT_0) {
	return 0;
    }
    return -1;
#endif
}

/**
 * @fn int win32e_pthread_mutex_unlock( win32e_pthread_mutex_t *m_p)
 * @brief POSIXのpthread_mutex_unlock()関数のWIN32版エミュレータ
 * @param m_p win32e_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int win32e_pthread_mutex_unlock(win32e_pthread_mutex_t * m_p)
{
#if 0				/* old */
    LeaveCriticalSection(&m_p->g_section);
    return 0;
#else
    int result = ReleaseSemaphore(m_p->hSem, 1, NULL);

    return (0 != result) ? 0 : -1;
#endif
}

/**
 * @fn int win32e_pthread_mutex_trylock( win32e_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_mutex_trylock()関数のWIN32版エミュレータ
 * @param m_p win32e_pthread_mutex_t構造体ポインタ
 * @retval 0 成功
 * @retval EBUSY TRYLOCK失敗
 * @retval -1 失敗
 */
int win32e_pthread_mutex_trylock(win32e_pthread_mutex_t * m_p)
{
#if 0				/* old */
    int result;

    result = TryEnterCriticalSection(&m_p->g_section);
    if (m_p->g_section.RecursionCount > 1) {
	/**
	 * @note 同じスレッドで取得した場合には、trylock動作が成功してしまう為の処理
	 */

	/**
	 * @note 以前は RecursionCount != 1の時で判定していたが マルチプロセッサの場合は
	 *    TRYLOCK失敗後に RecursionCountの値が変化することがあり得るため。
	 *    つうか、本来はコチラが適当なはず
	 */
	LeaveCriticalSection(&m_p->g_section);

	/**
	 * @note trylock失敗
	 */
	return -1;
    } else if (result == 0) {
	/**
	 * @note trylock失敗
	 */
	return EBUSY;
    }

    /**
     * @note trylock成功
     */

    return 0;
#else
    DWORD dwResult;

    dwResult = WaitForSingleObject(m_p->hSem, 0);
    switch (dwResult) {
    case WAIT_OBJECT_0:
	return 0;
    case WAIT_ABANDONED:
	return EINVAL;
    case WAIT_TIMEOUT:
	return EAGAIN;
    default:
	return -1;
    }
#endif
}

static int win32e_ftime64(mpxx_mtimespec_t * const ts_p)
{
    return mpxx_clock_getmtime(MPXX_CLOCK_REALTIME, ts_p);
}

static uint64_t _time_in_ms(void)
{
    mpxx_mtimespec_t tb;

    win32e_ftime64(&tb);

    return ((tb.sec) * 1000) + tb.msec;
}

static uint64_t _time_in_ms_from_mtimespec(const mpxx_mtimespec_t *
					   const ts)
{
    uint64_t t = ts->sec * 1000;
    t += ts->msec;

    return t;
}

/* CriticalSection内部のセマフォを参照するための構造 */
struct _pthread_crit_t {
    void *debug;
    LONG count;
    LONG r_count;
    HANDLE owner;
    HANDLE sem;
    ULONG_PTR spin;
};

/**
 * @fn int win32e_pthread_mutex_timedlock(win32e_pthread_mutex_t * m_p, mpxx_mtimespec_t * ts)
 * @brief POSIXのpthread_mutex_timedlock()関数のWIN32版エミュレータ
 * @param m_p s3::c11_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int win32e_pthread_mutex_timedlock(win32e_pthread_mutex_t * m_p,
				   mpxx_mtimespec_t * ts)
{
    uint64_t t, ct;

    /* Try to lock it without waiting */
    if (!win32e_pthread_mutex_trylock(m_p)) {
	return 0;
    }

    ct = _time_in_ms();
    t = _time_in_ms_from_mtimespec(ts);

    if (1) {
	DWORD dwResult;
	/* まだ時間があるか */
	if (ct > t) {
	    return EAGAIN;	/* TIMEOUT */
	}
	/* セマフォで待つ */
	dwResult = WaitForSingleObject(m_p->hSem, (DWORD) (t - ct));	/* 残り時間 */
	switch (dwResult) {
	case WAIT_OBJECT_0:
	    return 0;
	case WAIT_ABANDONED:
	    return EINVAL;
	case WAIT_TIMEOUT:
	    return EAGAIN;
	}
    }
    return -1;
}

/**
 * @fn int win32e_pthread_cond_init(win32e_pthread_cond_t * self_p, void *attr)
 * @brief POSIXのpthread_cond_init()関数のWIN32版エミュレータ
 * @param self_p win32e_pthread_cond_t構造体ポインタ
 * @param attr NULL固定
 * @retval 0 成功
 * @retval -1 失敗
 */
int win32e_pthread_cond_init(win32e_pthread_cond_t * self_p, void *attr)
{
    (void) attr;		/* 未使用 */

    memset(self_p, 0x0, sizeof(win32e_pthread_cond_t));

    self_p->m_waiters_count = 0;
    self_p->m_was_broadcast = 0;

    /* Create a manual-reset event. */
    self_p->m_waiters_done = CreateEvent(NULL,	/* no security */
					 FALSE,	/* manual-reset */
					 FALSE,	/* non-signaled initially */
					 NULL);	/* unnamed */

    InitializeCriticalSection(&self_p->m_waiters_count_lock);

    self_p->m_Sem = CreateSemaphore(NULL, 0, 0xffff, NULL);
    if (NULL == self_p->m_Sem) {
	return -1;
    }

    return 0;
}

static char *get_error_msg(ULONG ErrorCode)
{
    static char msg[128];
    char buf[96];
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			  NULL, ErrorCode, 0, buf, sizeof(buf), NULL);

    if (count != 0) {
	_snprintf(msg, 128, "Error: %u(%s)", ErrorCode, buf);
    } else {
	_snprintf(msg, 128, "Error: %u", ErrorCode);
    }
    return msg;
}

 /**
 * @fn int win32e_pthread_cond_wait_ex(win32e_pthread_cond_t * const cv, win32e_pthread_mutex_t* const external_mutex, mpxx_timespec_t * const ts)
 * @brief POSIXのwin32e_pthread_cond_wait()関数のWIN32版エミュレータ
 * @param cv win32e_pthread_cond_t構造体ポインタ
 * @param external_mutex win32e_pthread_mutex_t構造体ポインタ
 * @param ts 絶対時間タイムアプト時刻ポインタ（ＮＵＬＬでタイムアウトしない）
 * @retval 0 成功
 * @retval -1 失敗
 * @retval EAGAIN タイムアウトした(MINGW対策のため）
 * @retval EBUSY いずれかのスレッドが現在のcondにたいして待機している
 */
static int
win32e_pthread_cond_wait_ex(win32e_pthread_cond_t * const self_p,
			    win32e_pthread_mutex_t * const external_mutex,
			    mpxx_mtimespec_t * const ts)
{
    //int my_generation = 0;
    int last_waiter = 0;
    uint64_t ct, t;

    /* initialize */
    t = INFINITE;
    ct = _time_in_ms();

    if (NULL != ts) {
	t = _time_in_ms_from_mtimespec(ts);
	if (ct >= t) {
	    /* timed out */
	    return EAGAIN;
	}
    }

    /* Avoid race conditions. */
    EnterCriticalSection(&self_p->m_waiters_count_lock);

    if (self_p->m_waiters_count == 0) {
	if (NULL == external_mutex->cond_ext) {
	    external_mutex->cond_ext = self_p;
	} else if (external_mutex->cond_ext != external_mutex->cond_ext) {
	    LeaveCriticalSection(&self_p->m_waiters_count_lock);
	    return EBUSY;
	}
    }
    /* Increment count of waiters. */
    ++(self_p->m_waiters_count);
    LeaveCriticalSection(&self_p->m_waiters_count_lock);

    if (1) {
	DWORD dret;
	DWORD hold_time_ms = INFINITE;

	if (NULL != ts) {
	    hold_time_ms = (DWORD) (t - ct);
	}

	/* Wait until the event is signaled. */
	dret =
	    SignalObjectAndWait(external_mutex->hSem, self_p->m_Sem,
				hold_time_ms, FALSE);
	if (dret == WAIT_TIMEOUT) {
	    EnterCriticalSection(&self_p->m_waiters_count_lock);
	    --(self_p->m_waiters_count);
	    LeaveCriticalSection(&self_p->m_waiters_count_lock);
	    return EAGAIN;
	}
	if (dret == ~0UL ) {
	    DWORD dwErrcode;
	    dwErrcode = GetLastError();
	    fprintf(stderr, "_win32e_physdiskio_fsync : %s\n",
		    get_error_msg(dwErrcode));
	}
    }

    EnterCriticalSection(&self_p->m_waiters_count_lock);
    --(self_p->m_waiters_count);
    last_waiter = (self_p->m_was_broadcast
		   && (self_p->m_waiters_count == 0)) ? -1 : 0;
    LeaveCriticalSection(&self_p->m_waiters_count_lock);

    if (last_waiter) {
	SignalObjectAndWait(self_p->m_waiters_done, external_mutex->hSem,
			    INFINITE, FALSE);
    } else {
	WaitForSingleObject(external_mutex->hSem, INFINITE);
    }

    return 0;
}

/**
 * @fn int win32e_pthread_cond_wait_ex(win32e_pthread_cond_t * cv, win32e_pthread_mutex_t* external_mutex, mpxx_timespec_t *ts)
 * @brief POSIXのwin32e_pthread_cond_wait()関数のWIN32版エミュレータ
 * @param cv win32e_pthread_cond_t構造体ポインタ
 * @param external_mutex win32e_pthread_mutex_t構造体ポインタ
 * @param ts 絶対時間タイムアプト時刻ポインタ（ＮＵＬＬでタイムアウトしない）
 * @retval 0 成功
 * @retval -1 失敗
 * @retval EAGAIN タイムアウトした
 */
int
win32e_pthread_cond_wait(win32e_pthread_cond_t * cv,
			 win32e_pthread_mutex_t * external_mutex)
{
    return win32e_pthread_cond_wait_ex(cv, external_mutex, NULL);
}


/**
 * @fn int win32e_pthread_cond_wait(win32e_pthread_cond_t *self_p, win32e_pthread_mutex_t* external_mutex, mpxx_timespec_t *abstime)
 * @brief POSIXのwin32e_pthread_cond_wait()関数のWIN32版エミュレータ
 * @param self_p win32e_pthread_cond_t構造体ポインタ
 * @param external_mutex win32e_pthread_mutex_t構造体ポインタ
 * @param abstime 絶対時間タイムアプト時刻ポインタ（ＮＵＬＬでタイムアウトしない）
 * @retval 0 成功
 * @retval -1 失敗
 * @retval EAGAIN タイムアウトした
 */
int
win32e_pthread_cond_timedwait(win32e_pthread_cond_t * self_p,
			      win32e_pthread_mutex_t * external_mutex,
			      mpxx_mtimespec_t * abstime)
{
    return win32e_pthread_cond_wait_ex(self_p, external_mutex, abstime);
}

int win32e_pthread_cond_signal(win32e_pthread_cond_t * self_p)
{
    EnterCriticalSection(&self_p->m_waiters_count_lock);
    if (self_p->m_waiters_count > 0) {
	ReleaseSemaphore(self_p->m_Sem, 1, NULL);
    }
    LeaveCriticalSection(&self_p->m_waiters_count_lock);

    return 0;
}

int win32e_pthread_cond_broadcast(win32e_pthread_cond_t * self_p)
{
    EnterCriticalSection(&self_p->m_waiters_count_lock);
    if (self_p->m_waiters_count > 0) {
	self_p->m_was_broadcast = 1;

	ReleaseSemaphore(self_p->m_Sem, self_p->m_waiters_count, NULL);
	LeaveCriticalSection(&self_p->m_waiters_count_lock);

	WaitForSingleObject(self_p->m_waiters_done, INFINITE);
	self_p->m_was_broadcast = 0;
	return 0;
    }
    LeaveCriticalSection(&self_p->m_waiters_count_lock);

    return 0;
}

int win32e_pthread_cond_destroy(win32e_pthread_cond_t * self_p)
{

    DeleteCriticalSection(&self_p->m_waiters_count_lock);

    CloseHandle(self_p->m_Sem);
    CloseHandle(self_p->m_waiters_done);

    return 0;
}

#endif				/* end of WIN32 */
