/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-April-10 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file  mtxx_pthread_barrier.c
 * @brief POSIX pthread_barrierをラッピングしたスレッド同期制御ライブラリ
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#define USE_OWN_BARRIER
#else
/* unix like */
#include <pthread.h>

#if defined(QNX)
#include <sync.h>
#endif

#if defined(Darwin)
#define USE_OWN_BARRIER
#endif

/**
 * @note defined(Linux) || defined(QNX)
 * not use OWN_BARRIER
 **/
#endif

/* CRL */
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_malloc.h>

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* this */
#include "mtxx_pthread_mutex.h"
#include "mtxx_semaphore.h"
#include "mtxx_pthread_barrier.h"


#ifdef USE_OWN_BARRIER
typedef struct _own_barrier {
    unsigned int barrier_count;
    unsigned int left_count;
    s3::mtxx_pthread_mutex_t interthread_mutex;
    s3::mtxx_pthread_mutex_t masterproc_mutex;
    s3::mtxx_sem_t bs_start;
    s3::mtxx_sem_t bs_reply;
    union {
	unsigned int flags;
	struct {
	    unsigned int masterproc_mutex:1;
	    unsigned int interthread_mutex:1;
	    unsigned int bs_start:1;
	    unsigned int bs_reply:1;
	} f;
    } init;
} own_barrier_t;

static int own_barrier_init(own_barrier_t *const barrier, const void *attr,
			   const unsigned int count);
static int own_barrier_wait(own_barrier_t *const  barrier);
static int own_barrier_destroy(own_barrier_t *const barrier);
#endif /* end of USE_OWN_BARRIER */

typedef struct _ext_tab {
#ifdef USE_OWN_BARRIER
    own_barrier_t own_bari;
#else
    pthread_barrier_t posix_bari;
#endif
    union {
	unsigned int flags;
	struct {
	    unsigned int bari;
	} f;
    } init;
} ext_t;

#ifdef USE_OWN_BARRIER
/**
 * @fn static int own_barrier_init( own_barrier_t *const barrier, const void *attr, const unsigned int count);
 * @brief セマフォエミュレート版barrierの初期化
 * @param barrier own_barrier_tインスタンスポインタ
 * @param attr NULL固定
 * @param count バリア待ちに入る総スレッド数
 * @rteval -1 失敗
 */
static int own_barrier_init(own_barrier_t *const b, const void *attr,
			   const unsigned int count)
{
    int status, result;

    DBMS5(stderr, __func__" : attr=%p\n", attr);

    memset(b, 0x0, sizeof(own_barrier_t));

    b->left_count = b->barrier_count = count;

    result = s3::mtxx_pthread_mutex_init(&b->interthread_mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      __func__ " : s3::mtxx_pthread_mutex_init(interthread_mutex) fail\n");
	status = -1;
	goto out;
    } else {
	b->init.f.interthread_mutex = 1;
    }

    result = s3::mtxx_pthread_mutex_init(&b->masterproc_mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      __func__ " : s3::mtxx_pthread_mutex_init(masterproc_mutex) fail\n");
	status = -1;
	goto out;
    } else {
	b->init.f.masterproc_mutex = 1;
    }

    result = s3::mtxx_sem_init(&b->bs_start, 0, 0);
    if (result) {
	DBMS1(stderr,
	      __func__ " : s3::mtxx_sem_init(bs_start) fail\n");
	status = -1;
	goto out;
    } else {
	b->init.f.bs_start = 1;
    }

    result = s3::mtxx_sem_init(&b->bs_reply, 0, 0);
    if (result) {
	DBMS1(stderr,
	      __func__ " : s3::mtxx_sem_init(bs_start) fail\n");
	status = -1;
	goto out;
    } else {
	b->init.f.bs_reply = 1;
    }

    status = 0;

  out:
    if (status) {
	own_barrier_destroy(b);
    }

    return status;
}

/**
 * @fn static int own_barrier_destroy(own_barrier_t *const barrier)
 * @brief セマフォエミュレート版barrierの破棄
 * @param barrier own_barrier_tインスタンスポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
static int own_barrier_destroy(own_barrier_t *const b)
{
    int result, status;

    if (b->init.f.masterproc_mutex) {
	/* barrier_waitの親の処理が終わっているか確認する。 */
	result = s3::mtxx_pthread_mutex_lock(&b->masterproc_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_lock(masterproc_mutex) fail\n");
	    return -1;
	}
	result = s3::mtxx_pthread_mutex_unlock(&b->masterproc_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_unlock(masterproc_mutex) fail\n");
	    return -1;
	}

	result = s3::mtxx_pthread_mutex_destroy(&b->masterproc_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_init(masterproc_mutex) fail\n");
	} else {
	    b->init.f.masterproc_mutex = 0;
	}
    }


    if (b->init.f.bs_start) {
	result = s3::mtxx_sem_destroy(&b->bs_start);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_sem_destroy(bs_start) fail\n");
	} else {
	    b->init.f.bs_start = 0;
	}
    }

    if (b->init.f.bs_reply) {
	result = s3::mtxx_sem_destroy(&b->bs_reply);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_sem_destroy(bs_reply) fail\n");
	} else {
	    b->init.f.bs_reply = 0;
	}
    }

    if (b->init.f.interthread_mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&b->interthread_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_destroy(interthread_mutex) fail\n");
	} else {
	    b->init.f.interthread_mutex = 0;
	}
    }

    /**
     * @note 初期化フラグが立っていないことの確認
     */
    if (b->init.flags) {
	DBMS1(stderr, __func__ " : b->init.flags = 0x%08x\n",
	      b->init.flags);
	status = -1;
	goto out;
    }

    status = 0;
  out:
    return status;
}

/**
 * @fn static int own_barrier_wait( own_barrier_t *const barrier)
 * @brief セマフォエミュレート版スレッドの同期待ち
 * @param barrier own_barrier_tインスタンスポインタ
 * @retval -1 失敗 (-1を介した場合はその後の動作は不定になります)
 * @retval 0 成功
 */
static int own_barrier_wait(own_barrier_t *const b)
{
    int result;
    const unsigned int barrier_count = b->barrier_count;

    result = s3::mtxx_pthread_mutex_lock(&b->interthread_mutex);
    if (result) {
	DBMS1(stderr,
	      __func__ " : s3::mtxx_pthread_mutex_lock(interthread_mutex) fail\n");
	return -1;
    }
    b->left_count--;
    if (b->left_count == 0) {
	size_t n;
	/* バリア制御マスター処理 */

	result = s3::mtxx_pthread_mutex_lock(&b->masterproc_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_lock(masterproc_mutex) fail\n");
	    return -1;
	}

	/* 1.待機中の奴らにsem_post */
	for (n = 1; n < barrier_count; ++n) {
	    result = s3::mtxx_sem_post(&b->bs_start);
	    if (result) {
		DBMS1(stderr,
		      __func__ " : s3::mtxx_sem_post(bs_start) fail\n");

		/* エラー終了.mutex解除 */
		result =
		    s3::mtxx_pthread_mutex_unlock(&b->interthread_mutex);
		if (result) {
		    DBMS1(stderr,
			  __func__ " : s3::mtxx_pthread_mutex_unlock(interthread_mutex) fail\n");
		}
		return -1;
	    }
	}

	/* 2.待機中の奴らがstartしたのを確認 */
	for (n = 1; n < barrier_count; ++n) {
	    result = s3::mtxx_sem_wait(&b->bs_reply);
	    if (result) {
		DBMS1(stderr,
		      __func__ " : s3::mtxx_sem_wait(bs_reply) fail\n");

		/* エラー終了.mutex解除 */
		result =
		    s3::mtxx_pthread_mutex_unlock(&b->interthread_mutex);
		if (result) {
		    DBMS1(stderr,
			  __func__ " : s3::mtxx_pthread_mutex_unlock(interthread_mutex) fail\n");
		}
		return -1;
	    }
	}

	/* 3.カウンタ初期化 */
	b->left_count = barrier_count;

	/* 4.mutex解除 */
	result = s3::mtxx_pthread_mutex_unlock(&b->interthread_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_unlock(interthread_mutex) fail\n");
	    return -1;
	}

	result = s3::mtxx_pthread_mutex_unlock(&b->masterproc_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_unlock(masterproc_mutex) fail\n");
	    return -1;
	}

    } else {
	/* バリア制御スレーブ処理 */

	/* 1.mutex解除 */
	result = s3::mtxx_pthread_mutex_unlock(&b->interthread_mutex);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_pthread_mutex_unlock(interthread_mutex) fail\n");
	    return -1;
	}
	/* 2.セマフォでスタート待ち */
	result = s3::mtxx_sem_wait(&b->bs_start);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_sem_wait(bs_start) fail\n");
	    return -1;
	}

	/* 3.スタート受信のリプライ */
	result = s3::mtxx_sem_post(&b->bs_reply);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : s3::mtxx_sem_wait(bs_reply) fail\n");
	    return -1;
	}
    }

    return 0;
}
#endif	/* end of USE_OWN_BARRIER */

/**
 * @fn int s3::mtxx_pthread_barrier_init( s3::mtxx_pthread_barrier_t *const barrier_p, const void *attr_p, unsigned int count)
 * @brief バリアオブジェクトを初期化します
 *	POSIX pthread_barrier_init()のラッパ
 * @param barrier_p s3::mtxx_pthread_barrier_tインスタンスポインタ
 * @param attr_p NULL固定で引数予約(pthread_barrierattr_t)
 * @param count pthread_barrier_wait() をcallして待機するスレッド数
 * @retval 0 成功
 * @retval 0以外 エラー番号又は-1
 */
int s3::mtxx_pthread_barrier_init(s3::mtxx_pthread_barrier_t *const barrier_p,
				 const void *attr_p, unsigned int count)
{
    int result, status;
    ext_t *e;

    if (attr_p != NULL) {
	return EINVAL;
    }
    e = (ext_t *) mpxx_malloc(sizeof(ext_t));
    if (NULL == e) {
	return ENOMEM;
    } else {
	memset(e, 0x0, sizeof(ext_t));
    }
    barrier_p->m_ext = e;

#ifdef USE_OWN_BARRIER
    result = own_barrier_init(&e->own_bari, attr_p, count);
#else				/* posix */
    result = pthread_barrier_init(&e->posix_bari, attr_p, count);
#endif
    if (result) {
	status = -1;
	goto out;
    } else {
	e->init.f.bari = 1;
    }

    status = 0;
  out:
    if (status) {
	s3::mtxx_pthread_barrier_destroy(barrier_p);
    }
    return status;
}

/**
 * @fn int s3::mtxx_pthread_barrier_wait( s3::mtxx_pthread_barrier_t *const barrier_p );
 * @brief バリア参加しているスレッドの同期待ちします
 *	POSIX pthread_barrier_wait()のラッパ
 * @param barrier_p s3::mtxx_pthread_barrier_tインスタンスポインタ
 * @retval 0 成功
 * @retval 0以外 エラー番号又は-1 (-1を介した場合はその後の動作は不定になります)
 */
int s3::mtxx_pthread_barrier_wait(s3::mtxx_pthread_barrier_t *const barrier_p)
{
    ext_t *const e = (ext_t *) barrier_p->m_ext;

#ifdef USE_OWN_BARRIER
    return own_barrier_wait(&e->own_bari);
#else				/* posix 1003.1 */
    return pthread_barrier_wait(&e->posix_bari);
#endif
}

/**
 * @fn int s3::mtxx_pthread_barrier_destroy(s3::mtxx_pthread_barrier_t *const barrier_p)
 * @brief バリアオブジェクトを破棄します
 *	POSIX pthread_barrier_destroy()のラッパ
 * @param barrier_p s3::mtxx_pthread_barrier_tインスタンスポインタ
 * @retval 0 成功
 * @retval 0以外 エラー番号又は-1 (-1を介した場合はその後の動作は不定になります)
 */
int s3::mtxx_pthread_barrier_destroy(s3::mtxx_pthread_barrier_t *const barrier_p)
{
    int result, status;
    ext_t *const e = (ext_t *) barrier_p->m_ext;
    if (NULL == e) {
	return 0;
    }

    if (e->init.f.bari) {
#ifdef USE_OWN_BARRIER
	result = own_barrier_destroy(&e->own_bari);
	if (result) {
	    DBMS1(stderr,
		  __func__ " : own_barrier_destroy(own_bari) fail\n");
	} else {
	    e->init.f.bari = 0;
	}
#else				/* posix 1003.1 */
	result = pthread_barrier_destroy(&e->posix_bari);
#endif
	if (result) {
	    DBMS1(stderr,
		  "%s : pthread_barrier_destroy(posix_bari) fail\n", __func__);
	    status = result;	/* エラーコードを入れる */
	} else {
	    e->init.f.bari = 0;
	}
    }

    /**
     * @note 初期化フラグが立っていないことの確認
     */
    if (e->init.flags) {
	DBMS1(stderr,
	      "%s : e->init.flags = 0x%08x\n", __func__,
	      e->init.flags);
	status = -1;
	goto out;
    }

    mpxx_free(barrier_p->m_ext);
    barrier_p->m_ext = NULL;
    status = 0;

  out:
    return status;
}
