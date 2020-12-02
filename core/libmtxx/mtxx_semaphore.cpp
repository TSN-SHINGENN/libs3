/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *       Basic Author: Seiichi Takeda  '2004-Aug-23
 *		 Last Alteration $Author: takeda $
 */

/**
 * @file mtxx_semaphore.c
 * @brief POSIX semaphoreをラッピングした名前無しセマフォライブラリ
 *	POSIX系OSのセマフォライブラリはシグナルで割り込まれる場合があるので、全てのOSで独自実装を使っています。
 */

/**
 * @note POSIXセマフォはシグナルによって割り込まれることがある.これが問題になることがある。
 *       POSIXセマフォにはmutexを使って作成したOWNセマフォを使うようにする。
 */
#ifdef WIN32
#if 0				/* for Debug */
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#include <windows.h>
#endif

/* POSIX */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if defined(QNX) || defined(Linux) || defined(Darwin) || defined(XILKERNEL)
#define USE_OWNSEM
#elif defined(WIN32)
/* WIN32の処理は関数の中なのでここでの処理はしない */
#else
#warning 'not select semaphore processing, so use default function'
#include <semaphore.h>
#endif

/* libs3 */
#include "libmpxx/mpxx_malloc.h"

/* this */
#include "mtxx_pthread_mutex.h"
#include "mtxx_semaphore.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

#define MAX_VALUE 65536

#ifdef USE_OWNSEM
typedef struct _own_sem {
    unsigned int semaphore;
    unsigned int waiters_count_;
    libs3::mtxx_pthread_mutex_t mutex;
    libs3::mtxx_pthread_cond_t cond;

    union {
	unsigned int flags;
	struct {
	    unsigned int mutex:1;
	    unsigned int cond:1;
	} f;
    } init;
} own_sem_t;

static int own_sem_init(own_sem_t * const ownsem, const int pshared, const unsigned int value);
static int own_sem_trywait(own_sem_t * const ownsem);
static int own_sem_wait(own_sem_t * const ownsem);
static int own_sem_post(own_sem_t * const ownsem);
static int own_sem_destroy(own_sem_t * const ownsem);
static int own_sem_getvalue(own_sem_t * const ownsem, int * const sval);
#endif				/* end of USE_OWNSEM */

typedef struct ext_tab {
#if defined(WIN32)
    HANDLE hSemaphore;
    s3::mtxx_pthread_mutex_t mutex;
#elif defined(USE_OWNSEM)
    own_sem_t own_sem;
#else
    sem_t posix_sem;
#endif
    union {
	unsigned int flags;
	struct {
	    unsigned int mutex:1;
	    unsigned int sem:1;
	} f;
    } init;
} ext_t;

/**
 * @fn int s3::mtxx_sem_init(s3::mtxx_sem_t *const sem, int pshared, unsigned int value)
 * @brief POSIXのsem_init()関数のマルチOS用ラッパ
 * @param sem multios_sem_t構造体ポインタ
 * @param pshared 基本は0( 0以外はPOSIX以外では使えなくなる
 * @param value 初期値
 * @return 0…成功、0以外…失敗
 */
int s3::mtxx_sem_init(s3::mtxx_sem_t *const sem, int pshared, unsigned int value)
{
    int status, result;
    ext_t *e;

    memset(sem, 0x0, sizeof(s3::mtxx_sem_t));

    e = (ext_t *)mpxx_malloc(sizeof(ext_t));
    if (NULL == e) {
	return -1;
    }
    memset(e, 0x0, sizeof(ext_t));
    sem->ext = e;

#if defined(WIN32)
    if (value > MAX_VALUE) {
	status = -1;
	goto out;
    }
    if (pshared != 0) {
	status = -1;
	goto out;
    }

    result = s3::mtxx_pthread_mutex_init(&e->mutex, NULL);
    if (result) {
	status = -1;
	goto out;
    }
    e->init.f.mutex = 1;

    e->hSemaphore = CreateSemaphore(NULL, value, MAX_VALUE, NULL);
    if (NULL == e->hSemaphore) {
	status = -1;
	goto out;
    }
    e->init.f.sem = 1;
#elif defined(USE_OWNSEM)
    result = own_sem_init(&e->own_sem, pshared, value);
    if (result) {
	status = -1;
	goto out;
    }
    e->init.f.sem = 1;
#else
    result = sem_init(&e->posix_sem, pshared, value);
    if (result) {
	status = -1;
	goto out;
    }
    e->init.f.sem = 1;
#endif

    status = 0;

  out:
    if(status) {
	s3::mtxx_sem_destroy(sem);
    }

    return status;
}

/**
 * @fn int s3::mtxx_sem_wait(s3::mtxx_sem_t *const sem)
 * @brief POSIXのsem_wait()関数のマルチOS用ラッパ
 * @param sem multios_sem_t構造体ポインタ
 * @return 0…成功、-1…予期しないエラー,それ以外…エラーコード
 */
int s3::mtxx_sem_wait(s3::mtxx_sem_t *const sem)
{
    ext_t *e = (ext_t *) sem->ext;

#if defined(WIN32)
    DWORD dwResult;

    dwResult = WaitForSingleObject(e->hSemaphore, INFINITE);
    switch (dwResult) {
    case WAIT_OBJECT_0:
	return 0;
    case WAIT_ABANDONED:
	return EINVAL;
    case WAIT_TIMEOUT:
	return -1;
    default:
	return -1;
    }

    return 0;
#elif defined(USE_OWNSEM)
    return own_sem_wait(&e->own_sem);
#else				/* POSIX */
    return sem_wait(&e->posix_sem);
#endif
}

/**
 * @fn int s3::mtxx_sem_trywait(s3::mtxx_sem_t *const sem)
 * @brief POSIXのsem_trywait()関数のマルチOS用ラッパ
 * @param sem mtxx_sem_t構造体ポインタ
 * @return 0…成功、-1…予期しないエラー,それ以外…エラーコード
 **/
int s3::mtxx_sem_trywait(s3::mtxx_sem_t *const sem)
{
    ext_t *e = (ext_t *) sem->ext;

#if defined(WIN32)
    DWORD dwResult;

    dwResult = WaitForSingleObject(e->hSemaphore, 0);
    switch (dwResult) {
    case WAIT_OBJECT_0:
	return 0;
    case WAIT_ABANDONED:
	return EINVAL;
    case WAIT_TIMEOUT:
	return -1;
    default:
	return -1;
    }

    return 0;
#elif defined(USE_OWNSEM)
    return own_sem_trywait(&e->own_sem);
#else				/* POSIX */
    return sem_trywait(&e->posix_sem);
#endif
}

/**
 * @fn int s3::mtxx_sem_post(s3::mtxx_sem_t *const sem)
 * @brief POSIXのsem_post()関数のマルチOS用ラッパ
 * @param sem multios_sem_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::mtxx_sem_post(s3::mtxx_sem_t *const sem)
{
    ext_t *e = (ext_t *) sem->ext;
#if defined(WIN32)
    int result;

    result = ReleaseSemaphore(e->hSemaphore, 1, NULL);

    return (0 != result) ? 0 : -1;
#elif defined(USE_OWNSEM)
    return own_sem_post(&e->own_sem);
#else				/* POSIX */
    return sem_post(&e->posix_sem);
#endif
}

/**
 * @fn int s3::mtxx_sem_getvalue(s3::mtxx_sem_t * sem, int *const sval)
 * @brief POSIXのsem_getvalue()関数のマルチOS用ラッパ
 * @param sem mtxx_sem_t構造体ポインタ
 * @param sval 値取得用変数ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::mtxx_sem_getvalue(s3::mtxx_sem_t * sem, int *const sval)
{
    ext_t *e = (ext_t *) sem->ext;
#if defined(WIN32)
    int result, status;
    LONG val = 0;
    /**
     * @brief sem_getvalueが無いので、以下の様にする。
     *        1.sem_trywait()で捕まえてみる。捕まらなかったら0を返す。
     *        2.捕まえちゃったら0を返す.
     */
    s3::mtxx_pthread_mutex_lock(&e->mutex);
    result = s3::mtxx_sem_trywait(sem);
    if (result) {
	/* 捕まえられなかった */
	*sval = 0;
	status = 0;
	goto out;
    }

    result = ReleaseSemaphore(e->hSemaphore, 1, &val);
    if (0 == result) {
	/* おかしくなってる */
	status = -1;
	goto out;
    }

    *sval = val + 1;
    status = 0;

  out:

    s3::mtxx_pthread_mutex_unlock(&e->mutex);

    return status;
#elif defined(USE_OWNSEM)
    return own_sem_getvalue(&e->own_sem, sval);
#else
    return sem_getvalue(&e->posix_sem, sval);
#endif
}

/**
 * @fn int s3::mtxx_sem_destroy(s3::mtxx_sem_t * sem)
 * @brief POSIXのsem_destroy()関数のマルチOS用ラッパ
 * @param sem multios_sem_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::mtxx_sem_destroy(s3::mtxx_sem_t * sem)
{
    ext_t *e = (ext_t *) sem->ext;
    int result, status;
#if defined(WIN32)
    if (e->init.f.mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&e->mutex);
	if (!result) {
	    e->init.f.mutex = 0;
	}
    }

    if (e->init.f.sem) {
	result = CloseHandle(e->hSemaphore);
	if (result) {
	    e->init.f.sem = 0;
	}
    }
#elif defined(USE_OWNSEM)
    if (e->init.f.sem) {
	result = own_sem_destroy(&e->own_sem);
	if (!result) {
	    e->init.f.sem = 0;
	}
    }
#else				/* POSIX */
    if (e->init.f.sem) {
	result = sem_destroy(&e->posix_sem);
	if (!result) {
	    e->init.f.sem = 0;
	}
    }
#endif

    status = (int) e->init.flags;
    if (!e->init.flags) {
	mpxx_free(e);
    }

    return status;
}


#ifdef USE_OWNSEM
/**
 * @fn static int own_sem_init(own_sem_t * const ownsem, const int pshared, const unsigned int value)
 * @brief POSIXセマフォ エミュレータライブラリを初期化します
 * @param ownsem own_sem_t構造体ポインタ
 * @param pshared 対応していないので0固定
 * @param value 保持される初期値
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_sem_init(own_sem_t * const ownsem, const int pshared, const unsigned int value)
{
    int status, result;

    if (value > MAX_VALUE) {
	status = -1;
	goto out;
    }

    if (pshared != 0) {
	status = -1;
	goto out;
    }

    result = s3::mtxx_pthread_mutex_init(&ownsem->mutex, NULL);
    if (result) {
	status = -1;
	goto out;
    }
    ownsem->init.f.mutex = 1;
    ownsem->semaphore = value;
    ownsem->waiters_count_ = 0;

    result = s3::mtxx_pthread_cond_init(&ownsem->cond, NULL);
    if (result) {
	status = -1;
	goto out;
    }
    ownsem->init.f.cond = 1;

    status = 0;

  out:
    if (status) {
	own_sem_destroy(ownsem);
    }

    return status;
}

/**
 * @fn static int own_sem_trywait(own_sem_t * const ownsem)
 * @brief ブロックをともなわない sem_wait 。セマフォのカウントが取れた場合は１減少させて直ちに成功を返す。
 *	セマフォカウントが0の場合はエラー（失敗）を返す.
 * @param ownsem own_sem_t構造体ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_sem_trywait(own_sem_t * const ownsem)
{
    int status;

    s3::mtxx_pthread_mutex_lock(&ownsem->mutex);

    if (ownsem->semaphore == 0) {
	status = -1;
    } else {
	--ownsem->semaphore;
	status = 0;
    }

    s3::mtxx_pthread_mutex_unlock(&ownsem->mutex);

    return status;
}

/**
 * @fn static int own_sem_wait(own_sem_t * const ownsem)
 * @brief 指定されるセマフォのカウントが非 0 になるまで呼び出しスレッドの実行を停止する。
 *	そしてセマフォカウントを一息で 1 だけ減少させる。
 * @param ownsem own_sem_t構造体ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_sem_wait(own_sem_t * const ownsem)
{

    s3::mtxx_pthread_mutex_lock(&ownsem->mutex);

    ownsem->waiters_count_++;

    while (ownsem->semaphore == 0) {
	s3::mtxx_pthread_cond_wait(&ownsem->cond, &ownsem->mutex);
    }

    --ownsem->semaphore;
    --ownsem->waiters_count_;

    s3::mtxx_pthread_mutex_unlock(&ownsem->mutex);

    return 0;
}

/**
 * @fn static int own_sem_post(own_sem_t * const ownsem)
 * @brief sem で指定されるセマフォのカウントを一息で 1 だけ増加させる。
 *	この関数は決してブロックすることはない。
 * @param ownsem own_sem_t構造体ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_sem_post(own_sem_t * const ownsem)
{

    s3::mtxx_pthread_mutex_lock(&ownsem->mutex);

    ++ownsem->semaphore;

    if (ownsem->waiters_count_ != 0) {
	s3::mtxx_pthread_cond_signal(&ownsem->cond);
    }

    s3::mtxx_pthread_mutex_unlock(&ownsem->mutex);

    return 0;
}

/**
 * @fn static int own_sem_getvalue(own_sem_t * const ownsem, int * const sval_p)
 * @brief セマフォ値を取得します。
 * @param ownsem own_sem_t構造体ポインタ
 * @param セマフォ値取得変数ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_sem_getvalue(own_sem_t * const ownsem, int * const sval_p)
{
    int sval;

    s3::mtxx_pthread_mutex_lock(&ownsem->mutex);

    sval = ownsem->semaphore;

    s3::mtxx_pthread_mutex_unlock(&ownsem->mutex);

    if (NULL != sval_p) {
	*sval_p = sval;
    }

    return 0;
}

/**
 * @fn static int own_sem_destroy(own_sem_t * const ownsem)
 * @brief セマフォオブジェクトを破壊し、セマフォオブジェクトが保持していた資源を解放する。
 *	呼び出されるときにそのセマフォを獲得待ちしているスレッドがあってはならない。
 * @param ownsem own_sem_t構造体ポインタ
 * @retval 0 成功
 * @retval 0以外 失敗
 */
static int own_sem_destroy(own_sem_t * const ownsem)
{
    int result;

    if (ownsem->init.f.mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&ownsem->mutex);
	if (!result) {
	    ownsem->init.f.mutex = 0;
	}
    }

    if (ownsem->init.f.cond) {
	result = s3::mtxx_pthread_cond_destroy(&ownsem->cond);
	if (!result) {
	    ownsem->init.f.cond = 0;
	}
    }

    ownsem->semaphore = 0;

    return (int) ownsem->init.flags;
}
#endif				/* end of USE_OWNSEM */
