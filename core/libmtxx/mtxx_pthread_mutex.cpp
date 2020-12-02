/**
 *	Copyright 2005 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2005-Nov-17 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mtxx_pthread_mutex.cpp
 * @brief POSIX pthread_mutexをラッピングした 排他制御ライブラリ
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <stdio.h>
#include <windows.h>
#else

/* POSIX Series */
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#endif

#if defined(Darwin)
#include <time.h>
#endif

/* libs3 */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_malloc.h>
#include <libmpxx/mpxx_time.h>

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "libmpxx/dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#ifdef WIN32
#include "_win32e_pthread_mutex_cond.h"
#endif
#include "mtxx_pthread_mutex.h"


typedef struct _mtxx_pthread_mutex_ext {
#ifdef WIN32
/* Microsoft Windows Series */
    win32e_pthread_mutex_t win32_mutex;
#else
/* UNIX Series */
    pthread_mutex_t mutex;
#endif
} _pthread_mutex_ext_t;

#define _get_pthread_mutex_ext(s) MPXX_STATIC_CAST(_pthread_mutex_ext_t*,(s)->m_ext)

/**
 * @fn int mtxx_pthread_mutex_init(mtxx_pthread_mutex_t * m_p, const void *attr)
 * @brief POSIXのpthread_mutex_init()関数のマルチOS用ラッパ
 *	mtxx_pthread_mutex_tインスタンスを初期化します
 * @param m_p mtxx_pthread_mutex_tインスタンスポインタ
 * @param attr NULLを指定
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_mutex_init(s3::mtxx_pthread_mutex_t * m_p,
			       const void *attr)
{
    int result;
    _pthread_mutex_ext_t * e = NULL;

    DBMS5(stdout, "s3::mtxx_pthread_mutex_init : execute" EOL_CRLF);

    e = (_pthread_mutex_ext_t *) mpxx_malloc(sizeof(_pthread_mutex_ext_t));
    if (NULL == e) {
	return -1;
    }
    memset(e, 0x0, sizeof(_pthread_mutex_ext_t));

    m_p->m_ext = e;

#ifdef WIN32
    result = win32e_pthread_mutex_init(&e->win32_mutex, attr);
#else
    result = pthread_mutex_init(&e->mutex, attr);
#endif
    if (result) {
	DBMS5(stderr,
	      "s3::mtxx_pthread_mutex_init : pthread_mutex_init() fail" EOL_CRLF);
	if (NULL != e) {
	    mpxx_free(e);
	}
	return -1;
    }
    return 0;
}

/**
 * @fn int s3::mtxx_pthread_mutex_destroy(s3::mtxx_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_mutex_destroy()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_mutex_tインスタンスを破棄します
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_mutex_destroy(s3::mtxx_pthread_mutex_t * m_p)
{
    int result;
    _pthread_mutex_ext_t * const e = _get_pthread_mutex_ext(m_p);

#ifdef WIN32
    result = win32e_pthread_mutex_destroy(&e->win32_mutex);
#else
    result = pthread_mutex_destroy(&e->mutex);
#endif
    if (result) {
	return -1;
    }

    mpxx_free(e);

    return 0;
}

/**
 * @fn int s3::mtxx_pthread_mutex_lock(s3::mtxx_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_mutex_lock()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_mutex_tインスタンスのMUTEXをロックします
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_mutex_lock(s3::mtxx_pthread_mutex_t * m_p)
{
    _pthread_mutex_ext_t * const e = _get_pthread_mutex_ext(m_p);
    int result;

#ifdef WIN32
    result = win32e_pthread_mutex_lock(&e->win32_mutex);
#else
    result = pthread_mutex_lock(&e->mutex);
#endif

    return (result) ? -1 : 0;
}

/**
 * @fn int s3::mtxx_pthread_mutex_unlock(s3::mtxx_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_mutex_unlock()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_mutex_tインスタンスのMUTEXロックを解除します
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_mutex_unlock(s3::mtxx_pthread_mutex_t * m_p)
{
    _pthread_mutex_ext_t * const e = _get_pthread_mutex_ext(m_p);
    int result;
#ifdef WIN32
    /* Microsoft Windows Series */
    result = win32e_pthread_mutex_unlock(&e->win32_mutex);
#else
    /* UNIX Series */
    result = pthread_mutex_unlock(&e->mutex);
#endif
    return (result) ? -1 : 0;
}

/**
 * @fn int s3::mtxx_pthread_mutex_trylock(s3::mtxx_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_mutex_trylock()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_mutex_tインスタンスのMUTEXをロックを試します
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @retval 0 成功
 * @retval EBUSY 現在ロックされていてロックに失敗した
 * @retval EINVAL mutex が適切に初期化されていない。 
 * @retval それ以外 その他のエラー
 */
int s3::mtxx_pthread_mutex_trylock(s3::mtxx_pthread_mutex_t * m_p)
{
    _pthread_mutex_ext_t * const e = _get_pthread_mutex_ext(m_p);
    int status;

#ifdef WIN32
    /* Microsoft Windows Series */
    status = win32e_pthread_mutex_trylock(&e->win32_mutex);
#else
    /* UNIX Series */
    status = pthread_mutex_trylock(&e->mutex);
#endif

    return status;
}

#if defined(Darwin)
static int _darwin_ftime64(mpxx_mtimespec_t * const ts_p)
{
    return mpxx_clock_getmtime(MPXX_CLOCK_REALTIME, ts_p);
}

static uint64_t _darwin_time_in_ms(void)
{
    mpxx_mtimespec_t tb;
	
    _darwin_ftime64(&tb);
	
    return ((tb.sec) * 1000) + tb.msec;
}

static uint64_t _darwin_time_in_ms_from_mtimespec(const mpxx_mtimespec_t * const ts)
{
    uint64_t t = ts->sec * 1000;
    t += ts->msec;

    return t;
}

int _darwin_pthread_mutex_timedlock(pthread_mutex_t * const m_p, mpxx_mtimespec_t * const absmtime_p)
{
    uint64_t t, ct;
    int result;
    struct timespec wts = { 0, 10 * 1000 * 1000};
    /* Try to lock it without waiting */
    result = pthread_mutex_trylock(m_p);
    if( result != EBUSY ) {
	return result;
    }

    ct = _darwin_time_in_ms();
    t = _darwin_time_in_ms_from_mtimespec(absmtime_p);

    while(1) {
	if( ct > t ) {
	    return ETIMEDOUT;
	}
	result = nanosleep( &wts, NULL);
	if(result) {
	    return result;
	}
	result = pthread_mutex_trylock(m_p);
  	if( result != EBUSY ) {
	    return result;
	}
	ct = _darwin_time_in_ms();
    }
}
#endif

/**
 * @fn int s3::mtxx_pthread_mutex_timedlock(s3::mtxx_pthread_mutex_t *m_p, s3::mtxx_mtimespec_t *abstime_p)
 * @brief POSIXのpthread_mutex_timedlock()関数のマルチOS用ラッパ
 *	この関数は無期限にブロックされることがない点を除いて、s3::mtxx_pthread_mutex_lock() 関数と同じ動作をします。
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @param abstime_p 絶対時刻でのタイムアウト時間mpxx_mtimespec_t構造体ポインタ
 * @retval 0 ロック成功
 * @retval EINVAL 不正なパラメータ
 * @retval EAGAIN 又は ETIMEDOUT 操作がタイムアウトした
 */
int s3::mtxx_pthread_mutex_timedlock(s3::mtxx_pthread_mutex_t *m_p, mpxx_mtimespec_t *abstime_p)
{
    int ret;
    _pthread_mutex_ext_t * const e = _get_pthread_mutex_ext(m_p);

#if defined(Linux) || defined(__AVM2) || defined(QNX)
    struct timespec posix_abstime, *posix_abstime_p = NULL;

    DBMS5( stderr,"s3::mtxx_pthread_mutex_timedlock(POSIX) : execute" EOL_CRLF);

    if (NULL != abstime_p) {
	posix_abstime.tv_sec = abstime_p->sec;
	posix_abstime.tv_nsec = abstime_p->msec * (1000 * 1000);
	posix_abstime_p = &posix_abstime;
    } else {
	posix_abstime.tv_sec = ~0;
	posix_abstime.tv_nsec = ~0;
	posix_abstime_p = NULL;
    }

    ret = pthread_mutex_timedlock(&e->mutex, posix_abstime_p);
#elif defined(WIN32) || defined(Darwin)
    mpxx_mtimespec_t m_abstime, *m_abstime_p = NULL;

    DBMS5( stderr,"s3::mtxx_pthread_mutex_timedlock(WIN32 or darwin) : execute" EOL_CRLF);

    if (NULL != abstime_p) {
	m_abstime.sec = abstime_p->sec;
	m_abstime.msec = abstime_p->msec;
	m_abstime_p = &m_abstime;
    } else {
	m_abstime.sec = ~0;
	m_abstime.msec = ~0;
	m_abstime_p = NULL;
    }
#if defined(WIN32)
    ret = win32e_pthread_mutex_timedlock(&e->win32_mutex, m_abstime_p);
#elif defined(Darwin)
    ret = _darwin_pthread_mutex_timedlock(&e->mutex, m_abstime_p);
#else
#error 'is not implemented oroginal _pthread_mutex_timedlock() on this OS'
#endif
#else
#error 'is not implemented s3::mtxx_pthread_mutex_timedlock() on this OS'
#endif

#if !(defined(__AVM2__) || defined(WIN32))
    if (ret == ETIME) {
	ret = ETIMEDOUT;
    }
#endif

    if (ret) {
	DBMS5(stderr, "s3::mtxx_pthread_mutex_timedlock err" EOL_CRLF);
    }

    return ret;
}

typedef struct _mpc {
#ifdef WIN32
    win32e_pthread_cond_t win32c;
#else
    pthread_cond_t c;
#endif
} _pthread_cond_ext_t;

#define _get_pthread_cond_ext(s) MPXX_STATIC_CAST(_pthread_cond_ext_t*, (s)->c_ext)

/**
 * @fn int s3::mtxx_pthread_cond_init(s3::mtxx_pthread_cond_t * c_p, void *_attr)
 * @brief POSIXのpthread_cond_init()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_cond_tインスタンスを初期化します。
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @param _attr 属性を示すポインタであるが、基本的にNULL
 * @retval 0 成功
 * @retval -1 失敗
 */

int s3::mtxx_pthread_cond_init(s3::mtxx_pthread_cond_t * c_p, void *_attr)
{
    int ret;
    _pthread_cond_ext_t *m;

    m = (_pthread_cond_ext_t *)mpxx_malloc(sizeof(_pthread_cond_ext_t));
    if (NULL == m) {
	DBMS1(stderr, "mpxx_malloc fail" EOL_CRLF);
	ret = -1;
	goto out;
    }
#ifdef WIN32
    ret = win32e_pthread_cond_init(&m->win32c, _attr);
#else				/* WIN32 */
    ret = pthread_cond_init(&m->c, _attr);
#endif				/* WIN32 */
    if (ret) {
	DBMS1(stderr, "s3::mtxx_pthread_cond_init err" EOL_CRLF);
    }

  out:
    if (ret) {
	if (NULL != m) {
	    mpxx_free(m);
	}
    } else {
	c_p->c_ext = m;
    }

    return ret;
}

/**
 * @fn int s3::mtxx_pthread_cond_wait(s3::mtxx_pthread_cond_t * c_p, s3::mtxx_pthread_mutex_t * m_p)
 * @brief POSIXのpthread_cond_wait()関数のマルチOS用ラッパ
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_cond_wait(s3::mtxx_pthread_cond_t * c_p,
			      s3::mtxx_pthread_mutex_t * m_p)
{
    int ret;
    _pthread_cond_ext_t * const m = _get_pthread_cond_ext(c_p);
    _pthread_mutex_ext_t * const me = _get_pthread_mutex_ext(m_p);

#ifndef WIN32
    ret = pthread_cond_wait(&m->c, &me->mutex);
#else				/* WIN32 */
    ret = win32e_pthread_cond_wait(&m->win32c, &me->win32_mutex);
#endif				/* WIN32 */
    if (ret) {
	DBMS1(stderr, "s3::mtxx_pthread_cond_wait err" EOL_CRLF);
    }

    return ret;
}

/**
 * @fn int s3::mtxx_pthread_cond_timedwait(s3::mtxx_pthread_cond_t * c_p, s3::mtxx_pthread_mutex_t * m_p, mpxx_mtimespec_t *abstime_p)
 * @brief POSIXのpthread_cond_timedwait()関数のマルチOS用ラッパ
 *	※ WIN32版の場合はタイムアウトは実装されていません。
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @param m_p s3::mtxx_pthread_mutex_tインスタンスポインタ
 * @param abstime_p 絶対時刻でのタイムアウト時間mpxx_mtimespec_t構造体ポインタ
 * @retval 0 成功
 * @retval EAGAIN又は ETIMEDOUT タイムアウトした
 * @retval EINTR シグナルに割り込まれた
 */
int
s3::mtxx_pthread_cond_timedwait(s3::mtxx_pthread_cond_t * c_p,
			       s3::mtxx_pthread_mutex_t * m_p,
			       mpxx_mtimespec_t * abstime_p)
{
    int ret;
    _pthread_cond_ext_t * const m = _get_pthread_cond_ext(c_p);
    _pthread_mutex_ext_t * const me = _get_pthread_mutex_ext(m_p);

#if defined(Linux) || defined(Darwin) || defined(__AVM2) || defined(QNX)
    struct timespec posix_abstime, *posix_abstime_p = NULL;

    DBMS5( stderr,"s3::mtxx_pthread_cond_timedwait(POSIX) : execute" EOL_CRLF);

    if (NULL != abstime_p) {
	posix_abstime.tv_sec = abstime_p->sec;
	posix_abstime.tv_nsec = abstime_p->msec * (1000 * 1000);
	posix_abstime_p = &posix_abstime;
    } else {
	posix_abstime.tv_sec = ~0;
	posix_abstime.tv_nsec = ~0;
	posix_abstime_p = NULL;
    }

    ret = pthread_cond_timedwait(&m->c, &me->mutex, posix_abstime_p);
#elif defined(WIN32)
    mpxx_mtimespec_t m_abstime, *m_abstime_p = NULL;

    DBMS5( stderr,"s3::mtxx_pthread_cond_timedwait(WIN32) : execute" EOL_CRLF);

    if (NULL != abstime_p) {
	m_abstime.sec = abstime_p->sec;
	m_abstime.msec = abstime_p->msec;
	m_abstime_p = &m_abstime;
    } else {
	m_abstime.sec = ~0;
	m_abstime.msec = ~0;
	m_abstime_p = NULL;
    }

    ret = win32e_pthread_cond_timedwait(&m->win32c, &me->win32_mutex, m_abstime_p);
#else
#error 'is not implemented s3::mtxx_pthread_cond_timedwait() on this OS'
#endif

#if !(defined(__AVM2__) || defined(WIN32))
    if (ret == ETIME) {
	ret = ETIMEDOUT;
    }
#endif

    if (ret) {
	DBMS5(stderr, "s3::mtxx_pthread_cond_wait err\n");
    }

    return ret;
}

/**
 * @fn int s3::mtxx_pthread_cond_signal(s3::mtxx_pthread_cond_t * c_p)
 * @brief POSIXのpthread_cond_signal()関数のマルチOS用ラッパ
 *	cond_wait()で待機しているスレッドの一つを再開させます.cond_wait()で待機状態でない場合は何も起こりません。
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_pthread_cond_signal(s3::mtxx_pthread_cond_t * c_p)
{
    int ret;
    _pthread_cond_ext_t * const m = _get_pthread_cond_ext(c_p);
    
#ifndef WIN32
    ret = pthread_cond_signal(&m->c);
#else				/* WIN32 */
    ret = win32e_pthread_cond_signal(&m->win32c);
#endif				/* WIN32 */
    if (ret) {
	DBMS1(stderr, "s3::mtxx_pthread_cond_signal err\n");
    }

    return ret;
}

/**
 * @fn int int s3::mtxx_pthread_cond_broadcast(s3::mtxx_pthread_cond_t * c_p)
 * @brief POSIXのpthread_cond_broadcast()関数のマルチOS用ラッパ
 *	cond_wait()で待機しているスレッド全てを再開させます.cond_wait()で待機状態でない場合は何も起こりません。
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @retval 0 成功
 * @retval 0以外 失敗
 */
int s3::mtxx_pthread_cond_broadcast(s3::mtxx_pthread_cond_t * c_p)
{
    int ret;
    _pthread_cond_ext_t * const m = _get_pthread_cond_ext(c_p);

#ifndef WIN32
    ret = pthread_cond_broadcast(&m->c);
#else				/* WIN32 */
    ret = win32e_pthread_cond_broadcast(&m->win32c);
#endif				/* WIN32 */
    if (ret) {
	DBMS1(stderr, "s3::mtxx_pthread_cond_broadcast err\n");
    }

    return ret;
}

/**
 * @fn int s3::mtxx_pthread_cond_destroy(s3::mtxx_pthread_cond_t * c_p)
 * @brief POSIXのpthread_cond_destroy()関数のマルチOS用ラッパ
 *	s3::mtxx_pthread_cond_tインスタンスを破棄します。
 * @param c_p s3::mtxx_pthread_cond_tインスタンスポインタ
 * @retval 0 成功
 * @retval EBUSY cond_waitしているスレッドがいる
 */
int s3::mtxx_pthread_cond_destroy(s3::mtxx_pthread_cond_t * c_p)
{
    int ret;
    _pthread_cond_ext_t * const m = _get_pthread_cond_ext(c_p);

#ifndef WIN32
    ret = pthread_cond_destroy(&m->c);
#else				/* WIN32 */
    ret = win32e_pthread_cond_destroy(&m->win32c);
#endif				/* WIN32 */
    if (ret) {
	DBMS1(stderr, "s3::mtxx_pthread_cond_destroy err\n");
    } else {
	if (NULL != m) {
	    mpxx_free(m);
	}
    }

    return ret;
}
