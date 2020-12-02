/**
 *      Copyright 2003- TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2003-June-06 Active
 *              Last Alteration $Author: takeda $
 */

/**
 *  @file mtxx_omp_single.c
 *  @brief 複数のスレッド内で唯一 一度のみ処理を活性化させます
 *	OpenMPをエミュレートするライブラリの一部です。
 *	single構文と同等の機能をライブラリ化しています。
 */

/* POSIX/CLR */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#endif

#ifndef WIN32
#include <pthread.h>
#endif

/* libs3 */
#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "libmpxx/dbms.h"

/* this */
#ifdef WIN32
#include "_win32e_pthread_mutex_cond.h"
#endif

#include "mtxx_pthread_once.h"
#include "mtxx_omp_single.h"

typedef struct _omp_single_table {
    short no;
#ifdef WIN32
    win32e_pthread_mutex_t win32e_mutex;
#else
    pthread_mutex_t posix_mutex;
#endif
    s3::mtxx_omp_single_attr_t attr;

    union {
	unsigned int flags;
	struct {
	    unsigned int is_done:1;
	    unsigned int is_attached:1;
	} f;
    } stat;
} omp_single_table_t;

#define OMPSINGLEOBJS_MAX 256

typedef struct _omp_single_table_list {
#ifdef WIN32
    win32e_pthread_mutex_t win32e_mutex;
#else
    pthread_mutex_t posix_mutex;
#endif

    omp_single_table_t objs[OMPSINGLEOBJS_MAX];
    unsigned int remain;

    union {
	unsigned int flags;
	struct {
	    unsigned int is_initialized:1;
	} f;
    } stat;
} omp_single_table_list_t;

static omp_single_table_list_t omp_single_list;

#define get_omp_single_list() &omp_single_list
#define get_omp_single_objs_max() OMPSINGLEOBJS_MAX

#ifdef WIN32
static void win32_omp_single_once_init(void);
#else
static void posix_omp_single_once_init(void);
static const pthread_mutex_t posix_mutex_init = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef WIN32
/**
 * @fm static win32_omp_single_once_init(void)
 * @brief WIN32系OSでomp_singleを利用するための一度限りの初期化
 */
static void win32_omp_single_once_init(void)
{
    omp_single_table_list_t *m = get_omp_single_list();
    unsigned int i;

    memset(m, 0x0, sizeof(omp_single_table_list_t));
    m->remain = get_omp_single_objs_max();
    win32e_pthread_mutex_init(&m->win32e_mutex, NULL);

    for (i = 0; i < m->remain; i++) {
	omp_single_table_t *t = &m->objs[i];
	t->no = (short) i;
	win32e_pthread_mutex_init(&t->win32e_mutex, NULL);
    }
    m->stat.f.is_initialized = 1;

    return;
}
#else
/**
 * @fn static posix_omp_single_once_init(void)
 * @brief posix系OSでomp_singleを利用するための一度限りの初期化
 */
static void posix_omp_single_once_init(void)
{
    omp_single_table_list_t *m = get_omp_single_list();
    unsigned int i;

    memset(m, 0x0, sizeof(omp_single_table_list_t));
    m->remain = get_omp_single_objs_max();
    m->posix_mutex = posix_mutex_init;

    for (i = 0; i < m->remain; i++) {
	omp_single_table_t *t = &m->objs[i];
	t->no = (short) i;
	t->posix_mutex = posix_mutex_init;
    }
    m->stat.f.is_initialized = 1;

    return;
}
#endif

/**
 * @fn int s3::mtxx_omp_single_get_instance( s3::mtxx_omp_single_t *self_p, s3::mtxx_omp_single_attr_t *attr_p)
 * omp_singleのインスタンスを取得します.既にインスタンスを取得している場合は何もしません
 * openMPのsingle構文と異なるのは再度実行させたい場合は、次の二つの方法のどちらかをとる必要があります。
 *  1)s3::mtxx_omp_single_reset_instance() で、実行したフラグをリセットしてください。
 *  2)s3::mtxx_omp_single_detach_instance() で、取得したインスタンスを解放して、再取得してください
 * @param self_p s3::mtxx_omp_single_tインスタンス取得ポインタ
 * @param attr_p s3::mtxx_omp_single_attr_t属性フラグ集合(NULLで全て０と等価) 
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_single_get_instance(s3::mtxx_omp_single_t * self_p,
				    s3::mtxx_omp_single_attr_t * attr_p)
{
    int result, status;
    static s3::mtxx_pthread_once_t omp_single_once = MTXX_PTHREAD_ONCE_INIT;
    omp_single_table_list_t *const m = get_omp_single_list();
    omp_single_table_t *t;
    short i;

    if (NULL == self_p) {
	return EINVAL;
    }
#ifdef WIN32
    result =
	s3::mtxx_pthread_once(&omp_single_once, win32_omp_single_once_init);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_get_instance : s3::mtxx_pthread_once(win32_omp_simgle_once) fail\n");
	return -1;
    }
#else				/* POSIX */
    result =
	s3::mtxx_pthread_once(&omp_single_once, posix_omp_single_once_init);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_get_instance : s3::mtxx_pthread_once(posix_omp_single_once_init) fail\n");
	return -1;
    }
#endif

#ifdef WIN32
    win32e_pthread_mutex_lock(&m->win32e_mutex);
#else
    pthread_mutex_lock(&m->posix_mutex);
#endif

    if (NULL != self_p->ext) {
	t = (omp_single_table_t *) self_p->ext;
	if ((t->no < 0) || !(t->no < get_omp_single_objs_max())) {
	    status = EINVAL;
	    goto out;
	}
	status = 0;
	goto out;
    }


    if (m->remain == 0) {
	status = ENOMEM;
	goto out;
    }

    for (i = 0; i < get_omp_single_objs_max(); i++) {
	t = &m->objs[i];
	if (!(t->stat.f.is_attached)) {
	    break;
	}
    }
    if (i == get_omp_single_objs_max()) {
	status = ENOMEM;
	goto out;
    }

    t->stat.flags = 0;
    t->stat.f.is_attached = 1;
    if (NULL != attr_p) {
	t->attr = *attr_p;
    } else {
	t->attr.flags = 0;
    }
    self_p->ext = (void *) t;
    m->remain--;

    status = 0;

  out:

#ifdef WIN32
    win32e_pthread_mutex_unlock(&m->win32e_mutex);
#else				/* POSIX */
    pthread_mutex_unlock(&m->posix_mutex);
#endif

    return status;
}

/**
 * @fn int s3::mtxx_omp_single_exec( s3::mtxx_omp_single_t* self_p, s3::mtxx_omp_single_callback_t func, void *args)
 * @brief コールバック指定された関数を一度だけ実行します。
 *	openMPのsingle構文はある特定のスレッドにのみ囲われたセクションを実行します。
 *	 それをコールバック関数として処理させます。
 * @param self s3::mtxx_omp_single_tインスタンス
 * @param func 実行する関数
 * @param args funcに引き渡す引数ポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_single_exec(s3::mtxx_omp_single_t* self_p,
			    s3::mtxx_omp_single_callback_t func, void *args)
{
    omp_single_table_t *const t = (omp_single_table_t *)self_p->ext;
    int result;

#ifdef WIN32
    if (t->attr.f.is_nowait) {
	result = win32e_pthread_mutex_trylock(&t->win32e_mutex);
	if (result == EBUSY) {
	    DBMS3(stderr,
		  "s3::mtxx_omp_single_exec : win32e_pthread_mutex_trylock EBUSY\n");
	    return 0;
	} else if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_single_exec : win32e_pthread_mutex_trylock fail\n");
	    return -1;
	}
    } else {
	result = win32e_pthread_mutex_lock(&t->win32e_mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_single_exec : win32e_pthread_mutex_lock fail\n");
	    return -1;
	}
    }
#else
    if (t->attr.f.is_nowait) {
	result = pthread_mutex_trylock(&t->posix_mutex);
	if (result == EBUSY) {
	    return 0;
	} else if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_single_exec : pthread_mutex_trylock fail\n");
	    return -1;
	}
    } else {
	result = pthread_mutex_lock(&t->posix_mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_single_exec : pthread_mutex_lock fail\n");
	    return -1;
	}
    }
#endif

    if (!t->stat.f.is_done) {
	(func) (args);
	t->stat.f.is_done = 1;
    }
#ifdef WIN32
    result = win32e_pthread_mutex_unlock(&t->win32e_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : win32e_pthread_mutex_unlock fail\n");
	return -1;
    }
#else
    result = pthread_mutex_unlock(&t->posix_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : pthread_mutex_unlock fail\n");
	return -1;
    }
#endif

    return 0;
}

/**
 * @fn int s3::mtxx_omp_single_reset_instance( s3::mtxx_omp_single_t self)
 * @brief 一度だけ実行したフラグをクリアします。
 *	openMPのsingle構文と同じような動作を行うためには,並列処理中のスレッド内でバリア同期し、この関数を呼び出してください。
 * @param self s3::mtxx_omp_single_tインスタンス
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_single_reset_instance(s3::mtxx_omp_single_t *self_p)
{
    omp_single_table_t *const t = (omp_single_table_t *)self_p->ext;
    int result;

#ifdef WIN32
    result = win32e_pthread_mutex_lock(&t->win32e_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : win32e_pthread_mutex_lock fail\n");
	return -1;
    }
#else
    result = pthread_mutex_lock(&t->posix_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : pthread_mutex_lock fail\n");
	return -1;
    }
#endif

    t->stat.f.is_done = 0;

#ifdef WIN32
    result = win32e_pthread_mutex_unlock(&t->win32e_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : win32e_pthread_mutex_unlock fail\n");
	return -1;
    }
#else
    result = pthread_mutex_unlock(&t->posix_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_exec : pthread_mutex_unlock fail\n");
	return -1;
    }
#endif

    return 0;
}

/**
 * @fn int s3::mtxx_omp_single_detach_instance( s3::mtxx_omp_single_t *self_p)
 * @brief s3::mtxx_omp_single_tインスタンスを解放します。
 *    解放処理後は *self_p を NULLに書き換えます。
 * @param self_p s3::mtxx_omp_single_tインスタンス ポインタ(NULLが指定された場合は何もしません)
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_single_detach_instance(s3::mtxx_omp_single_t * self_p)
{
    omp_single_table_list_t *const m = get_omp_single_list();
    omp_single_table_t *const t = (omp_single_table_t *) self_p->ext;
    int result, status;

#ifdef WIN32
    result = win32e_pthread_mutex_lock(&m->win32e_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_detach_instance : win32e_pthread_mutex_lock fail\n");
	return -1;
    }
#else
    result = pthread_mutex_lock(&m->posix_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_detach_instance : pthread_mutex_lock fail\n");
	return -1;
    }
#endif

    if (NULL == self_p->ext) {
	status = 0;
	goto out;
    }

    if (!(t->stat.f.is_attached)) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_detach_instance : t->stat.f.is_attached=%d\n",
	      t->stat.f.is_attached);
	status = -1;
	goto out;
    }

    t->stat.f.is_attached = 0;
    t->stat.f.is_done = 0;
    m->remain++;

    self_p->ext = NULL;
    status = 0;

  out:

#ifdef WIN32
    result = win32e_pthread_mutex_unlock(&m->win32e_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_detach_instance : win32e_pthread_mutex_lock fail\n");
	return -1;
    }
#else
    result = pthread_mutex_unlock(&m->posix_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_single_detach_instance : pthread_mutex_lock fail\n");
	return -1;
    }
#endif
    return status;

}

/**
 * int s3::mtxx_omp_single_get_number_of_remaining_instance(int *max_p)
 * アタッチ可能なインスタンス数を返します
 */
int s3::mtxx_omp_single_get_number_of_remaining_instance(int *max_p)
{
    omp_single_table_list_t *const m = get_omp_single_list();

    if (NULL != max_p) {
	*max_p = get_omp_single_objs_max();
    }

    return m->remain;
}
