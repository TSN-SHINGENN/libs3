/**
 *      Copyright 2003- TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2003-April-06 Active
 *              Last Alteration $Author: takeda $
 */

/**
 *  @file mtxx_omp_critical.c
 *  @brief 複数のスレッドの処理から一つの処理のみ活性化させます
 *	OpenMPをエミュレートするライブラリの一部です。
 *	critical構文と同等(以上)の機能をライブラリ化しています。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* system */
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* libs3/mpxx */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_malloc.h>

#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* libs3/mtxx */
#include "mtxx_pthread.h"
#include "mtxx_pthread_mutex.h"

/* this */
#include "mtxx_omp_critical.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _omp_critical_ext {
    s3::mtxx_pthread_cond_t cond;
    s3::mtxx_pthread_mutex_t mutex;
    s3::mtxx_pthread_mutex_t waiter_id_mutex;

    uint64_t now_sequence_no;

    uint64_t start_sequence_no;
    uint64_t waiter_id_counter;

    s3::mtxx_omp_critical_attr_t attr;

    union {
	unsigned int flags;
	struct {
	    unsigned int is_canceled;
	} f;
    } stat;

    union {
	unsigned int flags;
	struct {
	    unsigned int cond:1;
	    unsigned int mutex:1;
	    unsigned int waiter_id_mutex:1;
	} f;
    } init;
} omp_critical_ext_t;

#define get_omp_critical_ext(s) MPXX_STATIC_CAST(omp_critical_ext_t*,(s)->ext)

/**
 * @fn int s3::mtxx_omp_critical_init(s3::mtxx_omp_critical_t *self_p, uint64_t start_seq_no, s3::mtxx_omp_critical_attr_t *attr_p)
 * @brief s3::mtxx_omp_criticalオブジェクトのインスタンスを初期化します
 * @param self_p s3::mtxx_omp_criticalオブジェクトインスタンスポインタ
 * @param start_seq_no 開始シーケンス番号
 * @param attr_p s3::mtxx_critical_attr_tポインタ NULL指定でフラグ０と等価
 * @retval NULL 失敗
 * @retval NULL以外 成功
 */
int s3::mtxx_omp_critical_init(s3::mtxx_omp_critical_t * self_p,
			      uint64_t start_seq_no,
			      s3::mtxx_omp_critical_attr_t * attr_p)
{
    omp_critical_ext_t *e;
    int result, status;

    DBMS4(stdout, "s3::mtxx_omp_critical_init :exec" EOL_CRLF);
    memset(self_p, 0x0, sizeof(s3::mtxx_omp_critical_t));

    e = (omp_critical_ext_t *) mpxx_malloc(sizeof(omp_critical_ext_t));
    if (NULL == e) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_init : mpxx_malloc(omp_critical_ext_t) fail" EOL_CRLF);
	return -1;
    }
    memset(e, 0x0, sizeof(omp_critical_ext_t));
    self_p->ext = (void *) e;

    e->now_sequence_no = e->start_sequence_no = self_p->start_sequence_no =
	start_seq_no;
    e->waiter_id_counter = 0;

    if (NULL == attr_p) {
	e->attr.flags = 0;
    } else {
	e->attr = *attr_p;
    }

    result = s3::mtxx_pthread_cond_init(&e->cond, NULL);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_init : pthread_cond_init fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.cond = 1;

    result = s3::mtxx_pthread_mutex_init(&e->mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_init : pthread_mutex_init fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.mutex = 1;

    result = s3::mtxx_pthread_mutex_init(&e->waiter_id_mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_init : pthread_mutex_init(waiter_id_mutex) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.waiter_id_mutex = 1;

    status = 0;

  out:

    if (status) {
	    s3::mtxx_omp_critical_destroy(self_p);
    }

    return status;
}

/**
 * @fn int s3::mtxx_omp_critical_destroy( s3::mtxx_omp_critical_t *self_p )
 * @brief s3::mtxx_omp_critical_tインスタンスを破棄します
 * @param self_p s3::mtxx_omp_criticalオブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗( 動作が不安定になります ）
 */
int s3::mtxx_omp_critical_destroy(s3::mtxx_omp_critical_t * self_p)
{
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);
    int result;

    if (e->init.f.mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&e->mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_signle_destroy : s3::mtxx_pthread_mutex_destroy fail" EOL_CRLF);
	} else {
	    e->init.f.mutex = 0;
	}
    }

    if (e->init.f.cond) {
	result = s3::mtxx_pthread_cond_destroy(&e->cond);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_signle_destroy : s3::mtxx_pthread_cond_destroy fail" EOL_CRLF);
	} else {
	    e->init.f.cond = 0;
	}
    }

    if (e->init.f.waiter_id_mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&e->waiter_id_mutex);
	if (result) {
	    DBMS1(stderr,
		  "s3::mtxx_omp_signle_destroy : s3::mtxx_pthread_mutex_destroy(waiter_id_mutex) fail" EOL_CRLF);
	} else {
	    e->init.f.waiter_id_mutex = 0;
	}
    }

    if (e->init.flags) {
	DBMS1(stderr,
	      "s3::mtxx_omp_signle_destroy : e->init.flags = 0x%llx" EOL_CRLF,
	      (long long)e->init.flags);
	return -1;
    }

    if (NULL != e) {
	mpxx_free(e);
	self_p->ext = NULL;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_omp_critical_end(s3::mtxx_omp_critical_t *self_p, uint64_t seq_no);
 * @brief セクション終了を通知します。
 * @param self_p s3::mtxx_omp_criticalオブジェクトインスタンスポインタ
 * @param seq_no 処理番号(is_unrefer_sequence_noが1の場合は無視します)
 * @retval EINVAL ( 処理待ちに入ったときの処理番号と異なる )
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::mtxx_omp_critical_end(s3::mtxx_omp_critical_t * self_p,
			     uint64_t seq_no)
{
    int result;
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);

    if (!e->attr.f.is_unrefer_sequence_no) {
	if (e->now_sequence_no != seq_no) {
	    return EINVAL;
	}
    }

    e->now_sequence_no++;

    result = s3::mtxx_pthread_cond_broadcast(&e->cond);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_end : s3::mtxx_pthread_cond_broadcast fail" EOL_CRLF);
	return -1;
    }

    result = s3::mtxx_pthread_mutex_unlock(&e->mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_end : : s3::mtxx_pthread_mutex_unlock fail" EOL_CRLF);
	return -1;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_omp_critical_start(s3::mtxx_omp_critical_t *self_p, uint64_t waiting_seq_no)
 * @brief 処理番号待ちしてセクションをスタートします
 *       cancelを検出して戻ったた場合は セクションを終了する際にs3::mtxx_omp_critical_end_and_cancel_all()を呼んでください
 * @param self_p s3::mtxx_omp_criticalオブジェクトインスタンスポインタ
 * @param waiting_seq_no 処理待ち番号(attr.f.is_unused_procnoが1の場合無視されます)
 * @retval 1 cancelされた
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_critical_start(s3::mtxx_omp_critical_t * self_p,
			       uint64_t waiting_seq_no)
{
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);
    int loop_flag = 1;
    int status = -1;
    int result;

    result = s3::mtxx_pthread_mutex_lock(&e->waiter_id_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_start : pthread_mutex_lock(waiter_id_mutex) fail" EOL_CRLF);
	return -1;
    }

    if (e->attr.f.is_unrefer_sequence_no) {
	waiting_seq_no = e->start_sequence_no + e->waiter_id_counter;
    }
    e->waiter_id_counter++;

    result = s3::mtxx_pthread_mutex_unlock(&e->waiter_id_mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_start : pthread_mutex_lock(waiter_id_mutex) fail" EOL_CRLF);
	return -1;
    }

    result = s3::mtxx_pthread_mutex_lock(&e->mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_start : pthread_mutex_lock fail" EOL_CRLF);
	return -1;
    }

    while (loop_flag) {
	if (e->stat.f.is_canceled) {
	    status = 1;
	    loop_flag = 0;
	} else if (e->now_sequence_no == waiting_seq_no) {
	    status = 0;
	    loop_flag = 0;
	} else {
		s3::mtxx_pthread_cond_signal(&e->cond);

		s3::mtxx_pthread_cond_wait(&e->cond, &e->mutex);
	}
    }

    return status;
}

/**
 * @fn int s3::mtxx_omp_critical_end_and_cancel_all( s3::mtxx_omp_critical_t *self_p );
 * @brief セクション終了とシーケンス開始待ち状態のセクションにキャンセルを通知します。
 * @param self_p s3::mtxx_omp_criticalオブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_omp_critical_end_and_cancel_all(s3::mtxx_omp_critical_t *
					    self_p)
{
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);
    int result;

    e->stat.f.is_canceled = 1;

    result = s3::mtxx_pthread_cond_broadcast(&e->cond);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_cancel_all : s3::mtxx_pthread_cond_broadcast fail" EOL_CRLF);
	return -1;
    }

    result = s3::mtxx_pthread_mutex_unlock(&e->mutex);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_omp_critical_cancel_all : s3::mtxx_pthread_mutex_unlock fail" EOL_CRLF);
	return -1;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_omp_critical_is_canceled( s3::mtxx_omp_critical_t *self_p );
 * @brief インスタンスからキャンセル要求フラグを取得します
 * @retval 1 キャンセル状態
 * @retval 0 キャンセルではない
 */
int s3::mtxx_omp_critical_is_canceled(s3::mtxx_omp_critical_t * self_p)
{
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);

    return (e->stat.f.is_canceled) ? 1 : 0;
}

/**
 * @fn uint64_t s3::mtxx_omp_critical_get_now_sequence_no( s3::mtxx_omp_critical_t *self_p)
 * @brief 現在のシーケンス番号を取得します
 *	特に排他ロックはしていません。start/endの中では、現在のシーケンス番号が戻ります。
 *	cancelされている場合は、そのシーケンス番号が戻ります。
 *	その他の時点では不定です。
 * @return 現在のシーケンス番号値
 */
uint64_t s3::mtxx_omp_critical_get_now_sequence_no(s3::mtxx_omp_critical_t *
						  self_p)
{
    omp_critical_ext_t *const e = get_omp_critical_ext(self_p);

    return e->now_sequence_no;
}
