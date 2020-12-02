/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *        Basic Author: Seiichi Takeda  '2012-May-17 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mtxx_pthread_once.c 
 * @brief POSIX pthread_onceをエミュレートした一回きりの関数実行ライブラリ
 *	POSIX pthread_once()は戻り値0で全て成功できます。
 *	しかし mtxx_pthread_once()は内部リソースを食いつぶすと戻り値-1で一回きり実行が完了しません。
 *	これは WIN32の為にエミュレーションした関数実装になっているためです。
 */

/* POSIX/CLR */
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef WIN32
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <stdlib.h>
#include <windows.h>
#endif

#ifndef WIN32
#include <pthread.h>
#endif

/* this */
#ifdef WIN32
#include "_win32e_pthread_mutex_cond.h"
#endif

/* libs3 */

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* this */
#include "mtxx_pthread_once.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _once_table {
    short no;
#ifdef WIN32
    win32e_pthread_mutex_t mutex;
    union {
	unsigned int flags;
	struct {
	    unsigned int is_done:1;
	} f;
    } stat;
#else
    pthread_once_t once_stat;
#endif
} once_table_t;

#define ONCEOBJS_MAX 256

typedef struct _once_table_list {
#ifdef WIN32
    win32e_pthread_mutex_t win32e_mutex;
    win32e_pthread_mutex_t win32e_once_mutex;
#else
    pthread_mutex_t posix_mutex;
    pthread_mutex_t posix_once_mutex;
#endif

    once_table_t objs[ONCEOBJS_MAX];
    unsigned int remain;

    union {
	unsigned int flags;
	struct {
	    unsigned int is_initialized:1;
	} f;
    } stat;
} once_table_list_t;

static once_table_list_t once_list;

#define get_once_list() &once_list
#define get_once_objs_max() ONCEOBJS_MAX

#ifdef WIN32
static void win32_pthread_once_init_ex(void);
#else				/* POSIX */
static void posix_pthread_once_init_ex(void);
static const pthread_once_t posix_once_init = PTHREAD_ONCE_INIT;
static const pthread_mutex_t posix_mutex_init = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef WIN32
/**
 * @fn static void win32_pthread_once_init_ex(void)
 * @brief (WIN32)pthread_once管理テーブルを初期化します
 */
static void win32_pthread_once_init_ex(void)
{
    once_table_list_t *m = get_once_list();
    unsigned int i;

    memset(m, 0x0, sizeof(once_table_list_t));
    m->remain = get_once_objs_max();
    win32e_pthread_mutex_init(&m->win32e_mutex, NULL);
    win32e_pthread_mutex_init(&m->win32e_once_mutex, NULL);

    for (i = 0; i < m->remain; i++) {
	once_table_t *t = &m->objs[i];
	t->no = (short) i;
	win32e_pthread_mutex_init(&t->mutex, NULL);
    }
    m->stat.f.is_initialized = 1;

    return;
}
#else				/* POSIX */
/**
 * @fn static void posix_pthread_once_init_ex(void)
 * @brief (POSIX)pthread_once管理テーブルを初期化します
 */
static void posix_pthread_once_init_ex(void)
{
    once_table_list_t *m = get_once_list();
    unsigned int i;

    memset(m, 0x0, sizeof(once_table_list_t));
    m->remain = get_once_objs_max();
    m->posix_mutex = posix_mutex_init;
    m->posix_once_mutex = posix_mutex_init;

    for (i = 0; i < m->remain; i++) {
	once_table_t *t = &m->objs[i];
	t->no = (short) i;
	t->once_stat = posix_once_init;
    }
    m->stat.f.is_initialized = 1;

    return;
}
#endif

/**
 * @fn int s3::mtxx_pthread_once_exec_init(void)
 * @brief s3::mtxx_pthread_once_exec管理テーブルを初期化します。
 *	s3::mtxx_pthread_once_execライブラリを使用するために必ず一回以上呼び出してください。
 *	2回目以降は何も処理せず抜けます。
 *	スレッドセーフです。
 * @retval 0固定(成功）
 * @retval ENOMEM リソースの獲得に失敗した
 */
int s3::mtxx_pthread_once_exec_init(void)
{
#ifdef WIN32
    static volatile int initialized = 0;
    static HANDLE mtx = NULL;

    if (!initialized) {
	if (!mtx) {
	    HANDLE lomtx;
	    lomtx = CreateMutex(NULL, 0, NULL);
	    if (NULL == lomtx) {
		return ENOMEM;
	    }
	    if (InterlockedCompareExchangePointer(&mtx, lomtx, NULL) !=
		NULL) {
		CloseHandle(lomtx);
	    }
	}

	WaitForSingleObject(mtx, INFINITE);
	if (!initialized) {
	    win32_pthread_once_init_ex();
	    initialized = 1;
	}
	ReleaseMutex(mtx);
    }
#else
    static pthread_once_t initialized = PTHREAD_ONCE_INIT;

    pthread_once(&initialized, posix_pthread_once_init_ex);
#endif

    return 0;
}

/**
 * @fn int s3::mtxx_pthread_once_exec_get_instance(s3::mtxx_pthread_once_exec_t *o_p)
 * @brief s3::mtxx_pthread_once_execのインスタンスを取得します
 *	指定されたs3::mtxx_pthread_once_exec_t構造体オブジェクト変数が0クリアされている時のみインスタンスを取得できます。
 *	既にインスタンスを取得している場合はそのままの状態で0を返します。
 * @param o_p s3::mtxx_pthread_once_exec_t構造体オブジェクト変数ポインタ
 * @retval 0 成功
 * @retval 0以外 とにかく失敗
 *	ENOMEM 空きがない
 *	EINVAL NULLが指定された
 */
int s3::mtxx_pthread_once_exec_get_instance(s3::mtxx_pthread_once_exec_t *
					   o_p)
{
    once_table_list_t *m = get_once_list();
    once_table_t *t;
    int status = 0;

    if (NULL == o_p) {
	return EINVAL;
    }

    if (o_p->ext != NULL) {
	return 0;
    }
#ifdef WIN32
    win32e_pthread_mutex_lock(&m->win32e_mutex);
#else				/* POSIX */
    pthread_mutex_lock(&m->posix_mutex);
#endif

    if (m->remain == 0) {
	status = ENOMEM;
	goto out;
    }

    t = &m->objs[ONCEOBJS_MAX - m->remain];
    m->remain--;

    o_p->ext = (void *) t;

  out:

#ifdef WIN32
    win32e_pthread_mutex_unlock(&m->win32e_mutex);
#else				/* POSIX */
    pthread_mutex_unlock(&m->posix_mutex);
#endif

    return status;
}

/**
 * @fn int s3::mtxx_pthread_once_exec( s3::mtxx_pthread_once_exec_t o, void (*init_routine)(void))
 * @brief 指定された関数を一度だけ実行します。複数のスレッドから呼ばれた場合は、処理が終了するまでこの関数でブロックされます
 *	POSIX系OSの処理は pthread_once()の処理に従います。
 *	WIN32ではエミュレートしているため、プロセスをforkしたときの処理は CriticalSectionに従います。
 * @param o s3::mtxx_pthread_once_exec_tオブジェクトポインタ
 * @param init_routine 実行する関数ポインタ
 * @return 常に0を返します
 */
int s3::mtxx_pthread_once_exec(s3::mtxx_pthread_once_exec_t o,
			      void (*init_routine) (void))
{
    once_table_t *t = (once_table_t *) o.ext;

    if (NULL == t) {
	return 0;
    }
#ifdef WIN32
    win32e_pthread_mutex_lock(&t->mutex);
    if (!t->stat.f.is_done) {
	init_routine();
	t->stat.f.is_done = 1;
    }
    win32e_pthread_mutex_unlock(&t->mutex);
#else
    pthread_once(&t->once_stat, init_routine);
#endif
    return 0;
}

/**
 * @fn int s3::mtxx_pthread_once_exec_get_number_of_remaining_instance(int *max_p);
 * @brief 取得可能なインスタンス数を取得します
 * @param max_p 最大数取得変数ポインタ
 * @retval -1 初期化されていない
 * @retval 0以上 残りのインスタンス数
 */
int s3::mtxx_pthread_once_exec_get_number_of_remaining_instance(int *max_p)
{
    once_table_list_t *m = get_once_list();
    int remain;

    if (NULL != max_p) {
	*max_p = ONCEOBJS_MAX;
    }

    if (!m->stat.f.is_initialized) {
	return -1;
    }
#ifdef WIN32
    win32e_pthread_mutex_lock(&m->win32e_mutex);
#else				/* POSIX */
    pthread_mutex_lock(&m->posix_mutex);
#endif

    remain = m->remain;

#ifdef WIN32
    win32e_pthread_mutex_unlock(&m->win32e_mutex);
#else				/* POSIX */
    pthread_mutex_unlock(&m->posix_mutex);
#endif

    return remain;
}

/**
 * @fn int s3::mtxx_pthread_once_exec_reset_instance( s3::mtxx_pthread_once_exec_t o)
 * @brief 使用しているインスタンスを初期化して関数を再度実行可能な状態にします。
 * @param o s3::mtxx_pthread_once_exec_t構造体
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::mtxx_pthread_once_exec_reset_instance(s3::mtxx_pthread_once_exec_t o)
{
    once_table_t *t = (once_table_t *) o.ext;

    if (NULL == t) {
	return -1;
    }
#ifdef WIN32
    win32e_pthread_mutex_lock(&t->mutex);
    if (t->stat.f.is_done) {
	t->stat.f.is_done = 0;
    }
    win32e_pthread_mutex_unlock(&t->mutex);
#else
    t->once_stat = posix_once_init;
#endif
    return 0;
}

/**
 * @fn int s3::mtxx_pthread_once( s3::mtxx_pthread_once_t *once_control_p, void (*init_routine)(void))
 * @brief 一回きり関数を実行します。
 *	POSIX pthread_once()のエミュレート関数です。実行できる回数に制限があります。
 * @param once_control_p 制御変数ポインタ。一回きりの実行のためにS3::MTXX_PTHREAD_ONCE_INIT値で初期化してください。
 *	once_control制御変数はstaticで値を保存してください。
 * @param init_routine 実行される関数ポインタ
 * @retval EINVAL 引数が不正
 * @retval ERANGE 管理できる容量を超えてしまった
 * @retval 0 成功
 * @retval -1 失敗(致命的なエラー)（管理できる容量を超えてしまった可能性があります。） ONCEOBJ_MAX値を増やしてください
 */
int s3::mtxx_pthread_once(s3::mtxx_pthread_once_t * once_control_p,
			 void (*init_routine) (void))
{
    s3::mtxx_pthread_once_exec_t once = { 0 };
    once_table_list_t *m = get_once_list();
    once_table_t *t;
    int result;

    result = s3::mtxx_pthread_once_exec_init();
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_pthread_once : s3::mtxx_pthread_once_exec_init fail" EOL_CRLF);
	return -1;
    }

    if ((NULL == once_control_p) || (NULL == init_routine)) {
	return EINVAL;
    }

    if (!(*once_control_p < ONCEOBJS_MAX)) {
	return ERANGE;
    }
#ifdef WIN32
    win32e_pthread_mutex_lock(&m->win32e_once_mutex);
#else				/* POSIX */
    pthread_mutex_lock(&m->posix_once_mutex);
#endif

    if (!(*once_control_p < 0)) {
	once.ext = (void *) &m->objs[*once_control_p];
    } else {
	result = s3::mtxx_pthread_once_exec_get_instance(&once);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_pthread_once : s3::mtxx_pthread_once_exec_get_instance fail" EOL_CRLF);
	    return -1;
	}

	t = (once_table_t *) once.ext;
	*once_control_p = t->no;
    }

#ifdef WIN32
    win32e_pthread_mutex_unlock(&m->win32e_once_mutex);
#else				/* POSIX */
    pthread_mutex_unlock(&m->posix_once_mutex);
#endif

    s3::mtxx_pthread_once_exec(once, init_routine);

    return 0;
}
