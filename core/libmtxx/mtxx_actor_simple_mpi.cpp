/**
 *      Copyright 2008 TSNｰSHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2008-july-04 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file multios_actor_th_simple_mpi.c
 * @brief スレッド間で、シンプルな非同期通信を行う為のクラスライブラリ(旧asynccmd.c)
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include <libmpxx/mpxx_sys_types.h>
#include <libmpxx/mpxx_stdlib.h>

#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* this */
#include "mtxx_pthread_mutex.h"
#include "mtxx_semaphore.h"

#include "mtxx_actor_simple_mpi.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _actor_th_simple_mpi_ext {
    int init_flag;
    s3::mtxx_pthread_mutex_t mutex;
    s3::mtxx_sem_t sem_master, sem_slave;

    union {
	uint32_t flags;
	struct {
	    uint32_t mutex:1;
	    uint32_t sem_master:1;
	    uint32_t sem_slave:1;
	} f;
    } init;
} actor_simple_mpi_ext_t;

#define get_actor_simple_mpi_ext(s) MPXX_STATIC_CAST(actor_simple_mpi_ext_t*, (s)->ext)

/**
 * @fn int s3::mtxx_actor_simple_mpi_init(s3::mtxx_actor_simple_mpi_t *const self_p)
 * @brief インスタンスを初期化します
 * @param self_p mtxx_actor_simple_mpi_t構造体インスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_actor_simple_mpi_init(s3::mtxx_actor_simple_mpi_t *const self_p)
{
    int result, status;
    actor_simple_mpi_ext_t *e;

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_init : exec" EOL_CRLF);
    memset(self_p, 0x0, sizeof(s3::mtxx_actor_simple_mpi_t));

    e = (actor_simple_mpi_ext_t *)
	malloc(sizeof(actor_simple_mpi_ext_t));
    if (NULL == e) {
	DBMS1(stderr, "s3::mtxx_actor_simple_mpi_init : malloc fail" EOL_CRLF);
	return -1;
    }
    memset(e, 0x0, sizeof(actor_simple_mpi_ext_t));
    self_p->ext = e;

    result = s3::mtxx_sem_init(&e->sem_master, 0, 1);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_actor_simple_mpi_init : s3::mtxx_sem_init(sem_master) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.sem_master = 1;

    result = s3::mtxx_sem_init(&e->sem_slave, 0, 0);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_actor_simple_mpi_init : s3::mtxx_sem_init(sem_slave) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.sem_slave = 1;

    result = s3::mtxx_pthread_mutex_init(&e->mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      "s3::mtxx_actor_simple_mpi_init : mtxx_pthread_mutex_init fail" EOL_CRLF);
	status = -1;
	goto out;
    }
    e->init.f.mutex = 1;

    status = 0;

  out:

    if (status) {
	s3::mtxx_actor_simple_mpi_destroy(self_p);
    }

    return status;
}

/**
 * @fn int s3::mtxx_actor_simple_mpi_destroy(s3::mtxx_actor_simple_mpi_t *const self_p)
 * @brief インスタンスを破棄します
 * @param self_p multios_actor_th_simple_mpi_t構造体インスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗(この時処理が不安定になります）
 */
int s3::mtxx_actor_simple_mpi_destroy(s3::mtxx_actor_simple_mpi_t *const self_p)
{
    int result;
    actor_simple_mpi_ext_t *const e = get_actor_simple_mpi_ext(self_p);

    DBMS4(stderr, "s3::mtxx_actor_simple_mpi_destroy : execute" EOL_CRLF);

    if (e->init.f.sem_master) {
	result = s3::mtxx_sem_destroy(&e->sem_master);
	if (result) {
	    DBMS1(stdout,
		  "s3::mtxx_actor_simple_mpi_destroy : mtxx_sem_destroy(master) fail" EOL_CRLF);
	    return -1;
	}
	e->init.f.sem_master = 0;
    }

    if (e->init.f.sem_slave) {
	result = s3::mtxx_sem_destroy(&e->sem_slave);
	if (result) {
	    DBMS1(stdout,
		  "s3::mtxx_actor_simple_mpi_destroy : mtxx_sem_destroy(slave) fail" EOL_CRLF);
	    return -1;
	}
	e->init.f.sem_slave = 0;
    }

    if (e->init.f.mutex) {
	result = s3::mtxx_pthread_mutex_destroy(&e->mutex);
	if (result) {
	    DBMS1(stdout,
		  "s3::mtxx_actor_simple_mpi_destroy : mtxx_pthread_mutex_destroy fail" EOL_CRLF);
	    return -1;
	}
	e->init.f.mutex = 0;
    }

    if (e->init.flags) {
	DBMS1(stdout,
	      "s3::mtxx_actor_simple_mpi_destroy :  e->init.flags = 0x%llx" EOL_CRLF,
	      (long long)e->init.flags);
	return -1;
    }

    if (NULL != e) {
	free(e);
    }

    return 0;
}

/**
 * @fn int s3::mtxx_actor_simple_mpi_send_request(s3::mtxx_actor_simple_mpi_t * const self_p,
 *				 const int cmd, s3::enum_mtxx_actor_simple_mpi_res_t *const res_p)
 * @brief MasterスレッドからのコマンドリクエストをだしてSlaveの応答を待機します
 * @param self_p multios_actor_th_simple_mpi_t構造体インスタンスポインタ
 * @param cmd コマンド番号
 * @param res_p 応答取得変数ポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::mtxx_actor_simple_mpi_send_request(s3::mtxx_actor_simple_mpi_t * const self_p,
					 const int cmd, s3::enum_mtxx_actor_simple_mpi_res_t *const res_p)
{
    s3::enum_mtxx_actor_simple_mpi_res_t res;
    actor_simple_mpi_ext_t *const e = get_actor_simple_mpi_ext(self_p);

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_send_request : request" EOL_CRLF);
    s3::mtxx_sem_wait(&e->sem_master);

    s3::mtxx_pthread_mutex_lock(&e->mutex);
    self_p->req_cmd = cmd;
    s3::mtxx_pthread_mutex_unlock(&e->mutex);

    s3::mtxx_sem_post(&e->sem_slave);

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_send_request : ack wait" EOL_CRLF);

    s3::mtxx_sem_wait(&e->sem_master);

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_send_request : becalled" EOL_CRLF);

    s3::mtxx_pthread_mutex_lock(&e->mutex);
    res = self_p->res;
    s3::mtxx_pthread_mutex_unlock(&e->mutex);

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_send_request : done" EOL_CRLF);

    s3::mtxx_sem_post(&e->sem_master);

    if (NULL != res_p) {
	*res_p = res;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_actor_simple_mpi_recv_request(s3::mtxx_actor_simple_mpi_t *const self_p, int *const cmd_p,
					 const s3::enum_mtxx_actor_simple_mpi_block_t block_type)
 * @brief スレーブ側スレッドでコマンドを受信します
 * @param self_p multios_actor_th_simple_mpi_t構造体インスタンスポインタ
 * @param cmd_p コマンド番号取得変数ポインタ
 * @param block_type recv状態設定
 *	SLAVE_NOBLOCK マスター側からコマンドが送信がない場合,EAGAINで返ります
 *	SLAVE_BLOCK マスターからのコマンド送信があるまでブロックします
 *	SLAVE_NOBLOCKWATCH 非ブロック状態だがコマンド番号が設定された場合、番号を確認します
 * @retval 0 コマンドを受信した
 * @retval EINVAL　不正な引数
 * @retval EAGAIN Master側でコマンドリクエストがなく、非ブロック状態で抜けた
 */
int s3::mtxx_actor_simple_mpi_recv_request(s3::mtxx_actor_simple_mpi_t *const self_p, int *const cmd_p,
					 const s3::enum_mtxx_actor_simple_mpi_block_t block_type)
{
    int cmd;
    int result;
    int cnt;
    actor_simple_mpi_ext_t *const e = get_actor_simple_mpi_ext(self_p);

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_recv_request : exec" EOL_CRLF);

    switch (block_type) {
    case s3::MTXX_ACTOR_SIMPLE_MPI_BLOCK:
	s3::mtxx_sem_wait(&e->sem_slave);
	break;
    case s3::MTXX_ACTOR_SIMPLE_MPI_NOBLOCK:
	result = s3::mtxx_sem_trywait(&e->sem_slave);
	if (result) {
	    /* no blocking モードでのループアウト */
	    return EAGAIN;
	}
	break;
    case s3::MTXX_ACTOR_SIMPLE_MPI_NOBLOCKWATCH:
	result = s3::mtxx_sem_getvalue(&e->sem_slave, &cnt);
	if (result) {
	    return EINVAL;
	} else if (cnt == 0) {
	    return EAGAIN;
	}
	break;
    default:
	return EINVAL;
    }

    DBMS5(stdout, "s3::mtxx_actor_simple_mpi_recv_request : be called" EOL_CRLF);

    s3::mtxx_pthread_mutex_lock(&e->mutex);
    cmd = self_p->req_cmd;
    s3::mtxx_pthread_mutex_unlock(&e->mutex);

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
	mpxx_i64toadec( (int64_t)cmd, xtoa_buf, xtoa_bufsz);
	DBMS5(stdout, "multios_actor_th_simple_mpi_recv_req : cmd=%s" EOL_CRLF,
	    xtoa_buf);
    }

    if (NULL != cmd_p) {
	*cmd_p = cmd;
    }

    return 0;
}

/**
 * @fn int s3::mtxx_actor_simple_mpi_reply(s3::mtxx_actor_th_simple_mpi_t *self_p,
 *					  const s3::enum_mtxx_actor_th_simple_mpi_res_t res)
 * @brief SLAVE側スレッドからのACK/NACK応答を返します。
 * @param self_p multios_actor_th_simple_mpi_t構造体インスタンスポインタ
 * @param res SLAVE_RES_ACK/SLAVE_RES_NACK
 * @retval EINVAL　不正な引数
 * @retval 0 成功
 */
int s3::mtxx_actor_simple_mpi_reply(s3::mtxx_actor_simple_mpi_t *self_p,
					  const s3::enum_mtxx_actor_simple_mpi_res_t res)
{
    actor_simple_mpi_ext_t *const e = get_actor_simple_mpi_ext(self_p);

    if (!
	(res == s3::MTXX_ACTOR_SIMPLE_MPI_RES_NACK
	 || res == s3::MTXX_ACTOR_SIMPLE_MPI_RES_ACK)) {
	return EINVAL;
    }

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)res, xtoa_buf, xtoa_bufsz);
	DBMS5(stdout,
	  "s3::mtxx_actor_simple_mpi_reply_res : execute(ack=%s)" EOL_CRLF, xtoa_buf);
    }

    s3::mtxx_pthread_mutex_lock(&e->mutex);
    self_p->res = res;
    s3::mtxx_pthread_mutex_unlock(&e->mutex);

    s3::mtxx_sem_post(&e->sem_master);

    return 0;
}
