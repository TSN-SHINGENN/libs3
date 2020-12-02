/**
 *	Copyright 2000 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2K-April-15 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file multios_socket_tcp_csmp_1_client.c 
 * @brief TCP/IP通信を行うためのシングルセッションライブラリ　クライアント側
 **/

#ifdef WIN32
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#else

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif

// #include <common/udrsock2msg.h>

/* multios */
#include <core/libmultios/multios_socket.h>
#include <core/libmultios/multios_pthread_mutex.h>
#include <core/libmultios/multios_pthread_once.h>
#include <core/libmultios/multios_malloc.h>
#include <core/libmultios/multios_string.h>
#include <core/libmultios/multios_endian.h>
#include <core/libmultios/multios_crc32_castagnoli.h>
#include <core/libmultios/multios_time.h>

//#include "udraccs.h"
#include "ips_socket_tcp_csmp1_1_client.h"

#ifdef LIBUDRDEBUG
static int debuglevel = 4;
#else
static int debuglevel = 1;
#endif
#define DBMS0   if (0) udr_disp_error
#define DBMS1   if (debuglevel >= 1) udr_disp_error
#define DBMS2   if (debuglevel >= 2) udr_disp_error
#define DBMS3   if (debuglevel >= 3) udr_disp_error
#define DBMS4   if (debuglevel >= 4) udr_disp_error
#define DBMS5   if (debuglevel >= 5) udr_disp_error

#define SOCKET_MAX_OPEN_COUNT 64
#define MALLOC_ALIGN

typedef struct _udrsock2_open_tbl {
    int socket_port_no;
    int sock_fd;
    char *preadstream_buf;
    char *pwritestream_buf;

    int timeout_flag;
    int sock_memiob_size;
    int sock_send_window_size;
    int sock_recv_window_size;
    int MemioXferSizeMAX;
    multios_pthread_mutex_t mutex;

    multios_castagonoli_crc32_t crc32c;

    union {
	unsigned int flags;
	struct {
	    unsigned int mutex:1;
	    unsigned int crc32c:1;
	} f;
    } init;

} udrsock2_open_tbl_t;

static volatile int udr_open_initflag = 0;
static udrsock2_open_tbl_t *open_list_p = NULL;

/* [SOCKET_MAX_OPEN_COUNT]; */

#define message_buffer_zero_clear(p) (multios_memset_path_through_level_cache((p), 0x0 ,UDRSOCK2_MESSAGE_PACKET_BUF_SIZE))

#define get_open_table(n) &open_list_p[(n)]
#define check_id(i) (( ( (i) < 0 ) || ( SOCKET_MAX_OPEN_COUNT <= (i)) ) ? -1 : 0)

#define UDRSOCK2_CONNECT_TIMEOUT 2	/* 指定時間(sec)以内に接続が確立しない場合タイムアウト */

#if 0
#define MULTIPAC_SOCKETREAD 1
#endif

#if 0
#define TCP_ACK_DRIVE 1
#endif

static int TimeConnectTimeout = UDRSOCK2_CONNECT_TIMEOUT;	/* サーバへコネクト時のタイムアウト時間 */
static int SocketRWTimeout = UDRSOCK2_TIMEOUT_RETRY_INTERVAL;

static unsigned int cmdln_no = 0;

static int func_socket_set_initialize_option(int fd, void *args);

/* private */
static int cmdio_req_c(const int fd, const char req_dir, char *cmdbuff,
		       const int cmdlength);
static int msg_req_and_reply_c(const int id,
			       udrsock2_msg_t * __restrict send_m,
			       udrsock2_msg_t * __restrict reply_m);

/* ブロック転送ルーチン */
static int client_memio_multipac(const int id, const char dir,
				 char *memb_data, const int32_t memb_seg,
				 const int32_t memb_offs,
				 const int32_t memb_size);
static int client_memio_trans_end_req(const int fd, const char trans_dir);
static int client_memio_write_1pac(const int fd, const char *pmemb_data,
				   const int memb_size);
static int client_memio_read_1pac(const int fd, char *pmemb_data,
				  const int memb_size);

#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int client_null_msg_send(const int id, udrsock2_msg_t * send_m);

/* UDRSOCK2 共通関数 */
static int client_msg_send(const int id, udrsock2_msg_t * send_m);	/* メッセージ送信ルーチン */
static int client_msg_recv(const int id, udrsock2_msg_t * recv_m);	/* メッセージ受信ルーチン */
static int socket_readstream(const int fd, void *ptr, const int length);	/* SocketStrem 送信ルーチン */
static int socket_writestream(const int fd, const void *ptr, const int length);	/* SocketStrem 受信ルーチン */
static int hdr_certify(const udrsock2_msg_t * const recv_m);	/* メッセージの確認 */

static void insert_crc_to_head(const int id, udrsock2_msg_t * const msg);
static int check_head_crc(const int id, udrsock2_msg_t * const msg);

/* デバッグ用関数 */
#if 0				/* for Debug */
static void udrsock2_dump_msg(udrsock2_msg_t * msg);	/* メッセージダンプ */
#endif

/**
 * @fn int udrsock2_client_set_connect_timeout(const int sec)
 * @brief connect タイムアウトする時間を設定します。
 * @param sec -1:デフォルト値 0以上:指定秒
 * @return 0 0固定
 */
int udrsock2_client_set_connect_timeout(const int sec)
{
    if (sec < 0) {
	TimeConnectTimeout = UDRSOCK2_CONNECT_TIMEOUT;
    } else {
	TimeConnectTimeout = sec;
    }

    return 0;
}

/**
 * @fn int udrsock2_client_set_communication_timeout(const int sec)
 * @brief 通信経路のタイムアウト値を設定・取得します。
 * @param sec 1以上:タイムアウト時間 0:デフォルト値 -1以下:現在の設定値
 * @return 設定されたタイムアウト時間
 */
int udrsock2_client_set_communication_timeout(const int sec)
{
    if (sec < 0) {
	return SocketRWTimeout;
    }

    SocketRWTimeout = (sec == 0) ? UDRSOCK2_TIMEOUT_RETRY_INTERVAL : sec;

    return SocketRWTimeout;
}

/**
 * @fn static func_socket_set_initalize_option( int fd, void *args)
 * @brief multios_socket_client_open()でsocketAPIをオープンしたときの初期化設定を列挙します
 */
static int func_socket_set_initialize_option(int fd, void *args)
{
    int result;
    udrsock2_open_tbl_t *tbl = (udrsock2_open_tbl_t *) args;

    result =
	multios_socket_get_windowsize(fd, &tbl->sock_send_window_size,
				      &tbl->sock_recv_window_size);
    if (result) {
	DBMS1(stderr,
	      "func_socket_set_initalize_option : multios_socket_get_windowsize fail\n");
	return -1;
    }

#if 0 /* for Debug */
    fprintf( stderr, "Set\n");
    fprintf( stderr, "tbl->sock_send_window_size = %d\n", tbl->sock_send_window_size);
    fprintf( stderr, "tbl->sock_recv_window_size = %d\n", tbl->sock_recv_window_size);
#endif

#if defined(WIN32)
    result =
	multios_socket_set_send_windowsize(fd,
					   tbl->sock_send_window_size *
					   32);
    if (result) {
	DBMS1(stderr,
	      "func_socket_set_initialize_option : multios_socket_set_send_windowsize( fd=%d, size=%d) fail\n",
	      fd, tbl->sock_send_window_size * 32);
	return -1;
    }
    result = multios_socket_set_recv_windowsize(fd, tbl->sock_send_window_size * 64);	/* 2の倍数となる十分な値とした.検証データは取っていない */
    if (result) {
	DBMS1(stderr,
	      "func_socket_set_initialize_option : multios_socket_set_recv_windowsize( fd=%d, size=%d) fail\n",
	      fd, tbl->sock_send_window_size * 32);
	return -1;
    }
    multios_socket_get_windowsize(fd, &tbl->sock_send_window_size,
				  &tbl->sock_recv_window_size);

#elif defined(QNX)
    /* QNX6.5.0で設定可能な最大サイズに変更 */
#define QNX6_Socket_Window_MAX (32 * 1024 * 7)
    multios_socket_set_recv_windowsize(fd, QNX6_Socket_Window_MAX);
    multios_socket_set_send_windowsize(fd, QNX6_Socket_Window_MAX);

    multios_socket_get_windowsize(fd, &tbl->sock_send_window_size,
				  &tbl->sock_recv_window_size);

#elif defined(Darwin)
    
#define MACOSX_Socket_Window_MAX ( 1 * 1024 * 1024)
    /* ATTO 10GbEのチューニング設定をする */

    multios_socket_set_recv_windowsize(fd, MACOSX_Socket_Window_MAX * 2);
    multios_socket_set_send_windowsize(fd, MACOSX_Socket_Window_MAX);

    multios_socket_get_windowsize(fd, &tbl->sock_send_window_size,
				  &tbl->sock_recv_window_size);

#endif

#if 0 /* for debug */
    fprintf( stderr, "Set\n");
    fprintf( stderr, "tbl->sock_send_window_size = %d\n", tbl->sock_send_window_size);
    fprintf( stderr, "tbl->sock_recv_window_size = %d\n", tbl->sock_recv_window_size);
#endif

    /* extend socket contol option */
    multios_socket_set_nagle(fd, 0);

#if 0
    /* 接続状態を確保 */
    setvalue = 1;
    setlength = sizeof(setvalue);
    setsockopt(tbl->sock_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &setvalue,
	       setlength);
#endif

    return 0;
}

static multios_pthread_mutex_t open_mutex;
static void udrsock2_client_open_init(void);
static int udrsock2_client_destroy(void);

/**
 * @fn static void udrsock2_client_open_init(void)
 * @brief udrsock2_clientの管理テーブルを初期化します。
 *  multios_pthread_once()からスレッドセーフで一度だけ呼ばれます。
 */
static void udrsock2_client_open_init(void)
{
    const unsigned int sizeof_udrsock2_open_list =
	sizeof(udrsock2_open_tbl_t) * SOCKET_MAX_OPEN_COUNT;
    unsigned int id;
    int result, status;

    result = multios_pthread_mutex_init(&open_mutex, NULL);
    if (result) {
	DBMS1(stderr,
	      "udrsock2_client_open_init : multios_pthread_mutex_init fail\n");
	return;
    }

    result =
	multios_malloc_align((void **) &open_list_p, sizeof(int),
			     sizeof_udrsock2_open_list);
    if (result) {
	return;
    }
    memset(open_list_p, 0x0,
	   sizeof(udrsock2_open_tbl_t) * SOCKET_MAX_OPEN_COUNT);

    for (id = 0; id < SOCKET_MAX_OPEN_COUNT; id++) {
	udrsock2_open_tbl_t *tbl = get_open_table(id);

	tbl->sock_fd = -1;
	multios_pthread_mutex_init(&tbl->mutex, NULL);
	tbl->init.f.mutex = 1;

	if (id == 0) {
	    result = multios_crc32_castagonoli_init(&tbl->crc32c);
	    if (result) {
		DBMS1(stderr,
		      "udrsock2_client_open_init : multios_crc32_castagonoli_init fail\n");
		status = -1;
		goto out;
	    }
	} else {
	    udrsock2_open_tbl_t *id0_tbl = get_open_table(0);
	    result =
		multios_crc32_castagonoli_init_have_a_obj(&tbl->crc32c,
							  &id0_tbl->
							  crc32c);
	    if (result) {
		DBMS1(stderr,
		      "udrsock2_client_open_init[id:%d] :  multios_crc32_castagonoli_init_have_a_obj fail\n",
		      id);
		status = -1;
		goto out;
	    }
	    tbl->init.f.crc32c = 1;
	}
    }
    status = 0;
    udr_open_initflag = ~0;

  out:

    if (status) {
	udrsock2_client_destroy();
    }

    return;
}

/**
 * @fn static int udrsock2_client_destroy(void)
 * @brief udrsock2プロトコル管理テーブルを破棄します。
 * @retval 0以外 失敗
 * @retval 0 成功
 */
static int udrsock2_client_destroy(void)
{
    unsigned int id;
    int result;
    int num_errors = 0;

    if (NULL == open_list_p) {
	return 0;
    }

    for (id = 0; id < SOCKET_MAX_OPEN_COUNT; id++) {
	udrsock2_open_tbl_t *tbl = get_open_table(id);

	if (tbl->init.f.mutex) {
	    result = multios_pthread_mutex_destroy(&tbl->mutex);
	    if (result) {
		DBMS1(stderr,
		      "udrsock2_client_destroy : id:%d multios_pthread_mutex_destroy fail\n",
		      id);
	    } else {
		tbl->init.f.mutex = 0;
	    }
	}

	if (tbl->init.f.crc32c) {
	    multios_crc32_castagonoli_destroy(&tbl->crc32c);
	    tbl->init.f.crc32c = 0;
	}

	if (tbl->init.flags) {
	    DBMS1(stderr,
		  "udrsock2_client_destroy : id:%d init.flags=0x%x\n", id,
		  tbl->init.flags);
	    num_errors++;
	}
    }

    if (num_errors == 0) {
	free(open_list_p);
	open_list_p = NULL;
    }

    return num_errors;
}


 /*
  * @fn int udrsock2_client_open(const char *servername, const int socket_port_no)
  * @brief UDRSocketプロトコルv2 クライアント側でサーバオープン
  * @param servername   サーバー名又は IP アドレス
  * @param socket_port_no ソケットポート番号
  * @retval -1 失敗
  * @retval 0 成功
  */
int udrsock2_client_open(const char *servername, const int socket_port_no)
{
    int id = 0;
    int fd = -1;
    int result = -1;
    int status = -1;

    udrsock2_open_tbl_t *tbl;
    static multios_pthread_once_t udrsock2_client_open_once = MULTIOS_PTHREAD_ONCE_INIT;

    result = multios_pthread_once(&udrsock2_client_open_once,
			 udrsock2_client_open_init);
    if(result) {
	DBMS1( stderr, "udrsock2_client_open : multios_pthread_once fail\n");
	return -1;
    }

    if (NULL == open_list_p) {
	return -1;
    }

    result = multios_pthread_mutex_lock(&open_mutex);
    if (result) {
	DBMS1(stderr,
	      "udrsock2_client_open : multios_pthread_mutex_lock(open_mutex) fail\n");
	return -1;
    }

    /* search nouse_tbl */
    for (id = 0; id < SOCKET_MAX_OPEN_COUNT; id++) {
	tbl = get_open_table(id);
	if (tbl->sock_fd < 0) {
	    break;
	}
    }
    if (id == SOCKET_MAX_OPEN_COUNT) {
	status = -1;
	goto out;
    }
    DBMS3(stderr, "udrsock2_client_open : id[%d] server:%s port=%d\n", id,
	  servername, socket_port_no);

    fd = multios_socket_client_open(servername, socket_port_no,
				    TimeConnectTimeout, PF_INET,
				    SOCK_STREAM, 0,
				    func_socket_set_initialize_option,
				    tbl);
    if (fd < 0) {
	DBMS5(stderr, "udrsock2_client_open : socket():err=%d", errno);
	status = -1;
	goto out;
    }

    DBMS5(stderr,
	  "fd=%d sock_send_window_size=%d sock_recv_window_size=%d\n", fd,
	  tbl->sock_send_window_size, tbl->sock_recv_window_size);

    /* ソケット通信用バッファ確保 */
#ifdef MALLOC_ALIGN
    result =
	multios_malloc_align((void **) &tbl->preadstream_buf, 512,
			     UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if (result) {
	DBMS1(stderr, "udrsock2_open_c: malloc preadstream_buf fail\n");
	status = -1;
	goto out;
    }
    result =
	multios_malloc_align((void **) &tbl->pwritestream_buf, 512,
			     UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if (result) {
	DBMS1(stderr, "udrsock2_open_c: malloc pwritestream_buf fail\n");
	status = -1;
	goto out;
    }
#else				/* old */
    if ((tbl->preadstream_buf =
	 (char *) malloc(UDRSOCK2_MESSAGE_PACKET_BUF_SIZE))
	== NULL) {
	DBMS1(stderr, "udrsock2_open_c: preadstream_buf malloc fail");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }
    if ((tbl->pwritestream_buf =
	 (char *) malloc(UDRSOCK2_MESSAGE_PACKET_BUF_SIZE))
	== NULL) {
	DBMS1(stderr, "udrsock2_open_c: pwritestream_buf malloc fail");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }
#endif

    /* fd を登録 */
    tbl->socket_port_no = socket_port_no;
    tbl->sock_fd = fd;

#if 1 /* old */
    tbl->sock_memiob_size = (64 * 1024);
#else /* MSSの対策をするかどうか */
    {
    	const size_t Sizof_64KiByte = 64 * 1024;
	size_t sizof_mss = 0;

	result = multios_socket_get_tcp_mss( tbl->sock_fd, &sizof_mss );
	if(result) {
	    tbl->sock_memiob_size = Sizof_64KiByte;
	} else { 
	    tbl->sock_memiob_size = ( Sizof_64KiByte ) - ( Sizof_64KiByte % sizof_mss);
	}
    }
#endif

#if 0				/* for Debug */
    fprintf(stdout, " open_c success\n");
    fprintf(stdout, " put preadstream_buf = 0x%p\n", tbl->preadstream_buf);
    fprintf(stdout, " put pwritestream_buf = 0x%p\n",
	    tbl->pwritestream_buf);
    fprintf(stdout, " put socket fd = %d\n", tbl->sock_fd);
    fprintf(stdout, " put socket id = %d\n", id);
#endif

    status = id;

  out:
    if (status < 0 && fd >= 0) {
	udrsock2_client_close(id);
    } else {

#if defined(SunOS) || defined(Linux)
    /****
     *	@note SunOS/Linuxは実際になにかを発行してみないと
     *	Socketのパイプが確立しているかどうかわからないそのためのトラップ
     ***/

	{
	    char buf[16];

	    DBMS5(stdout, "udrsock2_open_c : Trap for SunOS/Linux execute\n");

	    result = udrsock2_client_cmd_read_req(id, buf, 16);
	    if (result) {
		DBMS5(stdout,
		      "udrsock2_open_c : udrsock2_cmd_read_req_c\n");
		udrsock2_client_close(id);
	    }

	    DBMS5(stdout, "udrsock2_open_c : Trap for SunOS/Linux Success\n");
	}
#endif

    }

    result = multios_pthread_mutex_unlock(&open_mutex);
    if (result) {
	DBMS1(stderr,
	      "udrsock2_open_c : multios_pthread_mutex_unlock(open_mutex) fail\n");
    }

    return status;
}

/**
 * @fn int udrsock2_client_close(const int id)
 * @brief udrsockプロトコルでの通信をクローズします。
 * @retval -1 失敗
 * @retval 0 成功
 */
int udrsock2_client_close(const int id)
{
    int result = -1;
    int status = -1;
    int fd = -1;
    udrsock2_open_tbl_t *tbl;

    if(!udr_open_initflag) {
	return -1;
    }

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    fd = tbl->sock_fd;

    if (fd < 0) {
	status = -1;
	goto out;
    }

    result = multios_socket_shutdown(fd, SHUT_RDWR);
    if (result) {
	DBMS1(stderr, "udrsock2_close_c: shutdown fail\n");
    }

    result = multios_socket_close(fd);
    status = (result) ? -1 : 0;

  out:

    if (!status) {

	tbl->sock_fd = -1;
	/* ソケット通信用バッファ解放 */
	if (NULL != tbl->preadstream_buf) {
#ifdef MALLOC_ALIGN
	    multios_mfree(tbl->preadstream_buf);
#else
	    free(tbl->preadstream_buf);
#endif
	    tbl->preadstream_buf = NULL;
	}
	if (NULL != tbl->pwritestream_buf) {
#ifdef MALLOC_ALIGN
	    multios_mfree(tbl->pwritestream_buf);
#else
	    free(tbl->pwritestream_buf);
#endif
	    tbl->pwritestream_buf = NULL;
	}
	tbl->timeout_flag = 0;
    }
#if 0				/* for Debug */
    fprintf(stdout, " close_c success = %d\n", status);
    fprintf(stdout, " put preadstream_buf = %x\n", tbl->preadstream_buf);
    fprintf(stdout, " put pwritestream_buf = %x\n", tbl->pwritestream_buf);
    fprintf(stdout, " put socket fd = %d\n", tbl->sock_fd);
    fprintf(stdout, " put socket id = %d\n", id);
#endif

    return status;
}

/*
 * @fn static int msg_req_and_reply_c(const int id, udrsock2_msg_t * __restrict send_m, udrsock2_msg_t * __restrict reply_m)
 * @brief メッセージを送信し、さらにレスポンスを待ちます。
 * @retval UDRSOCK2_NO_ERR 成功
 * @retval UDRSOCK2_ERR_FAIL 失敗
 * @retval UDRSOCK2_ERR_CONNECT 通信経路が切断された
 */
static int
msg_req_and_reply_c(const int id, udrsock2_msg_t * __restrict send_m,
		    udrsock2_msg_t * __restrict reply_m)
{
    int result = -1;
    udrsock2_open_tbl_t * const tbl = get_open_table(id);

    DBMS5(stderr, "msg_req_and_reply_c : execute\n");

    multios_socket_set_nagle(tbl->sock_fd, 0);

#if 0 /* for Debug */
    udrsock2_dump_msg(send_m);
#endif


    /* send process */
    result = client_msg_send(id, send_m);
    if (result != UDRSOCK2_NO_ERR) {
	DBMS1(stderr,
	      " msg_req_and_reply_c : send error -> result = %d \n",
	      result);
	return UDRSOCK2_ERR_FAIL;
    }

    /* recv process */
    result = client_msg_recv(id, reply_m);

    DBMS5(stderr, " msg_req_and_reply_c : result = %d \n", result);

    switch (result) {
    case (UDRSOCK2_NO_ERR):
#define NOUSE_NULL_MSG


#ifndef NOUSE_NULL_MSG
	/**
	 * @note TCPプロトコルのACKを高速に返してもらうためにNULLパケットを送信する。
	 * ファーストタイマを使った 200msの遅延ACKを回避する
	 */
	result = client_null_msg_send(id, send_m);
	if (result != UDRSOCK2_NO_ERR) {
	    return UDRSOCK2_ERR_FAIL;
	}
#endif
	return UDRSOCK2_NO_ERR;
	break;
    case (UDRSOCK2_EOF):
	DBMS5(stdout, " msg_req_and_reply_c : server shutdown \n");
	DBMS5(stdout, " msg_req_and_reply_c : EOF prosess\n");
	return UDRSOCK2_ERR_FAIL;
    default:
	DBMS1(stdout,
	      " msg_req_and_reply_c : unknown result code = %d \n",
	      result);
	DBMS1(stdout, " msg_req_and_reply_c : Fail\n");
	return UDRSOCK2_ERR_FAIL;
    }

    return UDRSOCK2_ERR_CONNECT;
}

/**
 * @fn static int client_msg_send(const int id, udrsock2_msg_t * send_m)
 * @brief メッセージを送信します
 * @param id UDRプロトコルオープンテーブルID
 * @param send_m udrsock2_msg_t 送信メッセージポインタ
 * @retval UDRSOCK2_NO_ERR 成功
 * @retval UDRSOCK2_ERR_FAIL 失敗
 */
static int client_msg_send(const int id, udrsock2_msg_t * send_m)
{
    int result;
    udrsock2_open_tbl_t * const tbl = get_open_table(id);
    const int length =
	UDRSOCK2_HDR_SIZE + UDRSOCK2_GET_FRAME_SIZE(send_m);
    const uint8_t * const __restrict streamdata = (const uint8_t *) send_m;

    /* CRCを追加 */
    insert_crc_to_head(id, send_m);

    /* 分割して送信すると遅いので、一括で送信する */
    result = socket_writestream(tbl->sock_fd, streamdata, length);
    if (result != length) {
	return UDRSOCK2_ERR_FAIL;
    }
    return UDRSOCK2_NO_ERR;
}

/**
 * @fn static int client_msg_recv(const int id, udrsock2_msg_t * recv_m)
 * @brief メッセージを受信します
 * @param id UDRプロトコルオープンテーブルID
 * @param recv_m udrsock2_msg_t 受信メッセージポインタ
 * @retval UDRSOCK2_NO_ERR 成功
 * @retval UDRSOCK2_ERR_FAIL 失敗
 * @retval UDRSOCK2_ERR_DATA 無効なデータパケットを受信した
 * @retval UDRSOCK2_EOF 通信経路がシャットダウン（閉鎖）された
 */
static int client_msg_recv(const int id, udrsock2_msg_t * recv_m)
{
    udrsock2_open_tbl_t * const tbl = get_open_table(id);
    int length;
    uint8_t * __restrict streamdata = (uint8_t*)recv_m;
    int result;

    DBMS5(stdout, "client_msg_recv : execute\n");

    /* ヘッダーとフレームを分割して受信する */
    message_buffer_zero_clear(recv_m);
    /* ヘッダー受信 */
    length = UDRSOCK2_HDR_SIZE;
    result = socket_readstream(tbl->sock_fd, streamdata, length);
    DBMS5(stdout,
	  "client_msg_recv : socket_readstream result=%d errno=%d\n",
	  result, errno);
    if (result < 0) {
	return UDRSOCK2_EOF;
    }

    if (result != length) {
	return UDRSOCK2_ERR_FAIL;
    }

    result = hdr_certify(recv_m);
    if (result != UDRSOCK2_NO_ERR) {
	return result;
    }

    result = check_head_crc(id, recv_m);
    if (result) {
	DBMS1(stderr, "client_msg_recv : check_head_crc fail\n");
    }

    /* フレーム部受信 */
    length = UDRSOCK2_GET_FRAME_SIZE(recv_m);
    if (length == 0) {
	return UDRSOCK2_NO_ERR;
    }

    streamdata = ((uint8_t*) recv_m + UDRSOCK2_HDR_SIZE);
    result = socket_readstream(tbl->sock_fd, streamdata, length);
    if (result < 0) {
	return UDRSOCK2_EOF;
    }

    if (result != length) {
	return UDRSOCK2_ERR_FAIL;
    }

    if (recv_m->hdr.msg_type != UDRSOCK2_REPLY_MSG) {
	DBMS1(stdout,
	      "client_msg_recv : socket message direction error \n");
	return UDRSOCK2_ERR_DATA;
    }

    return UDRSOCK2_NO_ERR;
}

/**
 * @fn static int socket_readstream(const int fd, void *ptr, const int length)
 * @brief ソケット通信でソケット層からデータを読み取ります
 * @param fd ソケットファイルディスクプリタ
 * @param ptr バッファポインタ
 * @param length 読み込みサイズ
 * @retval -1 通信経路に問題が起こった
 * @retval 1以上 受信したデータサイズ
 * @retval 0 受信できなかった
 */
static int socket_readstream(const int fd, void *ptr, const int length)
{
    int rc = -1;
    int get_data_size = 0;
    int need_length = length;
    int recv_opt;

    char * __restrict p_ptr = ptr;

    DBMS5(stdout, "socket_readstream : execute\n");
    DBMS5(stdout, " socket_readstream : need byte size = %d\n", length);

#ifdef Linux
    recv_opt = MSG_WAITALL;
#else
    recv_opt = 0;
#endif

    while (1) {
	rc = multios_socket_recv_withtimeout(fd, p_ptr, need_length,
					     recv_opt, SocketRWTimeout);
	if (rc == 0) {
	    DBMS5(stderr, "socket_readstream : recvlen=%d, server shutdown??\n", rc);
	} else if (rc < 0) {
	    break;
	}
	get_data_size += rc;
	if (get_data_size == length) {
	    rc = get_data_size;
	    goto out;
	}
	p_ptr += rc;
	need_length -= rc;
    }

    /*******************
	ERROR Process
    ********************/
    if (rc == -1) {
	return 0;
    }

    if (rc == 0) {
	DBMS3(stderr, "socket_readstream : recvlen = %d\n", rc);
	return -1;
    }

  out:

    return rc;
}

/**
 * @fn static int socket_writestream(const int fd, const void *ptr, const int length)
 * @brief ソケット通信でストリームデータをソケット層へ書き込みます
 * @param fd ソケットファイルディスクプリタ
 * @param ptr バッファポインタ
 * @param length 書き込む
 * @retval -1 通信経路に問題が起こった
 * @retval 0以上 書き込んだ
 */
static int socket_writestream(const int fd, const void *ptr,
			      const int length)
{
    int left = -1, written = -1;
    int sendlength = -1;
    const char * __restrict p_ptr = ptr;

    DBMS5(stdout, "socket_writestream : execute\n");

    left = length;
    DBMS5(stdout, "socket_writestream : send size =%d\n", left);

    while (left > 0) {
	sendlength = left;

	written =
	    multios_socket_send_withtimeout(fd, p_ptr, sendlength, 0,
					    SocketRWTimeout);
	if (written <= 0) {
	    DBMS5(stderr, "socket_writestream : writelen = %d\n", written);
	    return (written);	/* err, return -1 */
	}

	left -= written;
	p_ptr += written;
    }

    return (length - left);	/* return  >= 0 */
}

/**
 * @fn static void insert_crc_to_head(const int id, udrsock2_msg_t * const msg)
 * @brief UDRプロトコルヘッダV2にCRCを埋め込みます
 * @param id UDRプロトコルオープンテーブルID
 * @param msg udrsock2_msg_t構造体ポインタ
 */
static void insert_crc_to_head(const int id, udrsock2_msg_t * const msg)
{
    udrsock2_open_tbl_t * const tbl = get_open_table(id);
    uint32_t crc32;
    multios_mtimespec_t now;

    multios_crc32_castagonoli_clear(&tbl->crc32c, NULL);
    multios_clock_getmtime(MULTIOS_CLOCK_REALTIME, &now);

    msg->hdr.crc32 = 0xffffffff;
    msg->hdr.msec = htonl(now.msec);

    multios_crc32_castagonoli_calc(&tbl->crc32c, msg,
				   UDRSOCK2_HDR_SIZE -
				   sizeof(msg->hdr.crc32), &crc32);

    MULTIOS_SET_LE_D32(&msg->hdr.crc32, crc32);

    DBMS5(stderr, "insert_crc_to_head : crc = 0x%08x\n", crc32);

    return;
}

/**
 * @fn static int check_head_crc(const int id, udrsock2_msg_t * const msg)
 * @brief UDRプロトコルヘッダV2に埋め込まれたCRCをチェックします
 * @param id UDRプロトコルオープンテーブルID
 * @param msg udrsock2_msg_t構造体ポインタ
 * @retval -1 計算結果があわない
 * @retval 0 計算結果があった 
 */
static int check_head_crc(const int id, udrsock2_msg_t * const msg)
{
    udrsock2_open_tbl_t * const tbl = get_open_table(id);
    uint32_t crc32;

    multios_crc32_castagonoli_clear(&tbl->crc32c, NULL);
    multios_crc32_castagonoli_calc(&tbl->crc32c, msg, UDRSOCK2_HDR_SIZE,
				   &crc32);

    DBMS5(stderr, "check_head_crc : crc = 0x%08x\n", crc32);

    return (crc32) ? -1 : 0;
}


/**
 * @fn int udrsock2_client_inital_negotiation(const int id, udrsock2_client_inital_negotiation_t *p)
 * @brief Client-Server間の初期設定パラメータのやりとり(ネゴシエーション)をします
 * @param id open_id値
 * @param p udrsock2_client_inital_negotiation_t構造体ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
int udrsock2_client_inital_negotiation(const int id,
				       udrsock2_client_inital_negotiation_t
				       * p)
{
    int result = -1;
    int status = -1;
    int fd = -1;

    udrsock2_msg_t *send_m, *reply_m;
    udrsock2_open_tbl_t *tbl = NULL;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);
    fd = tbl->sock_fd;

    if (fd < 0) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (tbl->timeout_flag) {
	return -1;
    }

    multios_pthread_mutex_lock(&tbl->mutex);

    DBMS5(stderr, "udrsock2_client_inital_negotiation : mutex_lock\n");
    DBMS3(stderr,
	  "udrsock2_client_inital_negotiation : p->subio_num_sessions=%d\n",
	  p->subio_num_sessions);

    send_m = (udrsock2_msg_t *) tbl->pwritestream_buf;
    reply_m = (udrsock2_msg_t *) tbl->preadstream_buf;

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_INIT_REQ;
    send_m->hdr.reply_type = 0;
    send_m->hdr.frame_type = UDRSOCK2_INIT_FRAME;
    send_m->hdr.result_code = htonl(0);
    send_m->hdr.frame_len = htonl(sizeof(udrsock2_init_frame_t));
    send_m->frame.init.subio_num_sessions = (uint8_t) (p->subio_num_sessions);	/* int to char */


    /* リクエスト送信 */
    result = msg_req_and_reply_c(id, send_m, reply_m);

#if 0
    udrsock2_dump_msg(reply_m);
#endif

    /* リプライ解析 */
    if (result < UDRSOCK2_NO_ERR) {
	DBMS1(stdout,
	      "udrsock2_client_inital_negotiation : message passing fail\n");
	status = UDRSOCK2_ERR_CONNECT;
	tbl->timeout_flag = ~0;
	goto out;
    }

    if ((reply_m->hdr.msg_type != UDRSOCK2_REPLY_MSG) ||
	(reply_m->hdr.request_type != UDRSOCK2_INIT_REQ)) {
	DBMS1(stdout,
	      "udrsock2_client_inital_negotiation : err responce \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (reply_m->hdr.frame_type != UDRSOCK2_INIT_FRAME) {
	DBMS1(stdout,
	      "udrsock2_client_inital_negotiation : ignore frame type = %d\n",
	      reply_m->hdr.frame_type);
	status = -1;
	goto out;
    }

    p->subio_num_sessions = (int) (reply_m->frame.init.subio_num_sessions);	/* chat ro int */
    p->subio_socket_port_no =
	ntohl(reply_m->frame.init.subio_socket_port_no);
    DBMS3(stderr,
	  "udrsock2_client_inital_negotiation : res>p->memio_num_sessions=%d\n",
	  p->subio_num_sessions);
    DBMS3(stderr,
	  "udrsock2_client_inital_negotiation : res>p->memio_socket_port_no=%d\n",
	  p->subio_socket_port_no);

    status = 0;

  out:

    if (!(fd < 0)) {
	DBMS5(stderr,
	      "udrsock2_client_inital_negotiation : mutex_clear\n");
	multios_pthread_mutex_unlock(&tbl->mutex);
    }

    return status;
}

/**
 * @fn int udrsock2_cmd_write_req_c(const int id, const char *cmdbuff, const int cmdlength)
 * @brief コマンド文字列をサーバーに送信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param cmdbuff コマンド文字列バッファ
 * @cmdlen コマンド文字列長
 * @retval 0 成功
 * @retval 0以外 UDRSOCKエラーコード
 */
int udrsock2_client_cmd_write_req(const int id, const char *cmdbuff,
				  const int cmdlength)
{
    return cmdio_req_c(id, UDRSOCK2_DIR_WRITE, (char *) cmdbuff,
		       cmdlength);
}

/**
 * @fn int udrsock2_cmd_read_req_c(const int id, char *cmdbuff, const int cmdlength)
 * @brief コマンド返値をサーバーから受信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param cmdbuff コマンド文字列バッファ
 * @param cmdlen コマンド文字列最大長
 * @retval 0 成功
 * @retval 0以外 UDRSOCKエラーコード
 */
int udrsock2_client_cmd_read_req(const int id, char *cmdbuff,
				 const int cmdlength)
{
    return cmdio_req_c(id, UDRSOCK2_DIR_READ, cmdbuff, cmdlength);
}

/**
 * @fn udrsock2_client_cmd_rw_req(const int id, const char *senddat, char *recvbuf, int sendlength, int recvlength)
 * @brief コマンド文字列の送受信を行います。
 * @param id UDRプロトコルオープンテーブルID
 * @param senddat 送信文字列
 * @param recvbuf 戻り値受信文字列バッファ
 * @param sendlenth 送信文字列サイズ
 * @param recvlength 戻り値受信文字列最大長
 * @retval 0 成功
 * @retval 0以外 UDRSOCKエラーコード
 */
int
udrsock2_client_cmd_rw_req(const int id, const char *senddat,
			   char *recvbuf, int sendlength, int recvlength)
{
    int status = -1;
    int result = -1;
    int reply_msg_length = 0;
    int reply_cmdln_no = -1;
    int fd = -1;

    udrsock2_msg_t * __restrict send_m, * __restrict reply_m;
    udrsock2_open_tbl_t *tbl;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);
    fd = tbl->sock_fd;

    if (fd < 0) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (tbl->timeout_flag)
	return -1;

    multios_pthread_mutex_lock(&tbl->mutex);

    DBMS5(stderr, "udrsock2_cmdrw_c : mutex_lock\n");

    send_m = (udrsock2_msg_t *) tbl->preadstream_buf;
    reply_m = (udrsock2_msg_t *) tbl->pwritestream_buf;

    if (sendlength > UDRSOCK2_CMDIO_DSIZE) {
	DBMS1(stderr, " cmdio_req_c :too long send data length");
	DBMS1(stderr, " senddat %d : %s\n", sendlength, senddat);
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (recvlength > UDRSOCK2_CMDIO_DSIZE)
	recvlength = UDRSOCK2_CMDIO_DSIZE;	/* limit */

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.result_code = htonl(0);
    send_m->hdr.reply_type = 0;

    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_CMDIO_REQ;
    send_m->frame.cmdio.dir = UDRSOCK2_DIR_RW;
    send_m->hdr.frame_len =
	htonl((sizeof(udrsock2_cmdio_frame_t) - UDRSOCK2_CMDIO_DSIZE +
	       sendlength));
    send_m->hdr.frame_type = UDRSOCK2_CMDIO_FRAME;
    send_m->frame.cmdio.send_len = htonl(sendlength);
    send_m->frame.cmdio.reply_len_max = htonl(recvlength);
    send_m->frame.cmdio.cmdln_no = htonl(cmdln_no);
    strncpy((char *) send_m->frame.cmdio.data, senddat, sendlength);

    DBMS5(stderr, " send[%d] %d %d = %s\n", cmdln_no, sendlength,
	  recvlength, senddat);

    /* リクエスト送信 */
    result = msg_req_and_reply_c(id, send_m, reply_m);

    /* リプライ解析 */
    if (result < UDRSOCK2_NO_ERR) {
	DBMS1(stdout, " cmd_rw_c : negotiation fail\n");
	status = UDRSOCK2_ERR_CONNECT;
	tbl->timeout_flag = ~0;
	goto out;
    }

    if ((reply_m->hdr.msg_type != UDRSOCK2_REPLY_MSG) ||
	(reply_m->hdr.request_type != UDRSOCK2_CMDIO_REQ)) {
	DBMS1(stdout, " cmd_read_c : err responce \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    reply_msg_length = ntohl(reply_m->frame.cmdio.reply_len_max);
    reply_cmdln_no = ntohl(reply_m->frame.cmdio.cmdln_no);

    DBMS5(stderr, " reply[%d] %d = %s\n",
	  reply_cmdln_no, reply_msg_length, reply_m->frame.cmdio.data);

    strncpy(recvbuf, (char *) reply_m->frame.cmdio.data, recvlength);

    cmdln_no++;

    status = 0;

  out:
    if (!(fd < 0)) {
	DBMS5(stderr, "udrsock2_cmdrw_c : mutex_clear\n");

	multios_pthread_mutex_unlock(&tbl->mutex);
    }

    return status;
}

/**
 * @fn int cmdio_req_c(const int id, char *cmdbuff, const int cmdlength)
 * @brief コマンド文字列の送信または受信を行います。
 * @param id UDRプロトコルオープンテーブルID
 * @param req_dir UDRSOCK2_DIR_READ:リード UDRSOCK2_DIR_WRITE:ライト
 * @param cmdbuff コマンド文字列バッファ
 * @param cmdlen コマンド文字列最大長
 * @retval 0 成功
 * @retval 0以外 UDRSOCKエラーコード
 */
static int
cmdio_req_c(const int id, const char req_dir, char *cmdbuff,
	    const int cmdlength)
{
    int result = -1;
    int status = -1;
    int fd = -1;
    udrsock2_msg_t * __restrict send_m, * __restrict reply_m;
    udrsock2_open_tbl_t *tbl = NULL;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    fd = tbl->sock_fd;

    if (fd < 0) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (tbl->timeout_flag)
	return -1;

    multios_pthread_mutex_lock(&tbl->mutex);

    DBMS5(stderr, "udrsock2_cmdio_c : mutex_lock\n");

    send_m = (udrsock2_msg_t *) tbl->pwritestream_buf;
    reply_m = (udrsock2_msg_t *) tbl->preadstream_buf;

    DBMS5(stdout, " cmdio_req_c : cmdbuff_pointer = 0x%p  cmdlength = %d",
	  cmdbuff, cmdlength);

    if (cmdlength > UDRSOCK2_CMDIO_DSIZE) {
	DBMS1(stderr, " cmdio_req_c :too long data_length");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_CMDIO_REQ;
    send_m->hdr.reply_type = 0;
    send_m->hdr.result_code = htonl(0);

    send_m->frame.cmdio.dir = req_dir;
    switch (req_dir) {
    case (UDRSOCK2_DIR_READ):
	send_m->hdr.frame_type = UDRSOCK2_CMDIO_FRAME;
	send_m->hdr.frame_len =
	    htonl(sizeof(udrsock2_cmdio_frame_t) - UDRSOCK2_CMDIO_DSIZE);
	send_m->frame.cmdio.reply_len_max = htonl(cmdlength);
	break;
    case (UDRSOCK2_DIR_WRITE):
	send_m->hdr.frame_type = UDRSOCK2_CMDIO_FRAME;
	send_m->hdr.frame_len = htonl(sizeof(udrsock2_cmdio_frame_t));
	send_m->frame.cmdio.send_len = htonl(cmdlength);
	strncpy((char *) send_m->frame.cmdio.data, cmdbuff, cmdlength);
	break;
    default:
	DBMS1(stdout, " cmdio_req_c : unknown direction falg\n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    /* リクエスト送信 */
    result = msg_req_and_reply_c(id, send_m, reply_m);

    /* リプライ解析 */
    if (result < UDRSOCK2_NO_ERR) {
	DBMS1(stdout, " cmd_read_io : negotiation fail\n");
	status = UDRSOCK2_ERR_CONNECT;
	tbl->timeout_flag = ~0;
	goto out;
    }

    if ((reply_m->hdr.msg_type != UDRSOCK2_REPLY_MSG) ||
	(reply_m->hdr.request_type != UDRSOCK2_CMDIO_REQ)) {
	DBMS1(stdout, " cmd_read_c : err responce \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (req_dir == UDRSOCK2_DIR_READ) {
	strncpy(cmdbuff, (char *) reply_m->frame.cmdio.data, cmdlength);
    }

    status = 0;

  out:

    if (!(fd < 0)) {
	DBMS5(stderr, "udrsock2_cmdio_c : mutex_clear\n");

	multios_pthread_mutex_unlock(&tbl->mutex);
    }

    return status;
}

/**
 * @fn int udrsock2_client_ioctl_req(const int id, const int cmd, char *cmdbuff, int *cmdlength_p)
 * @brief ioctlコマンドを送受信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param cmd ioctlコマンド値
 * @param cmdbuff ioctlコマンド引数
 * @param cmdlength_p ioctlコマンド引数長変数ポインタ
 * @retval -1 致命的な失敗
 * @retval UDRSOCK2_ERR_FAIL 何か問題があり失敗した
 * @retval UDRSOCK2_ERR_CONNECT ハンドシェイクに失敗した
 * @retval 0以外 reply_m->hdr.result_codeに入った値を返します。
 * @retval 0 成功
 */
int udrsock2_client_ioctl_req(const int id, const int cmd, char *cmdbuff,
			      int *cmdlength_p)
{
    int result = -1;
    int status = -1;
    int fd = -1;
    int cmdlength;

    udrsock2_msg_t * __restrict send_m, * __restrict reply_m;
    udrsock2_open_tbl_t *tbl = NULL;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);
    fd = tbl->sock_fd;

    if (fd < 0) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (tbl->timeout_flag) {
	return -1;
    }

    cmdlength = (NULL == cmdlength_p) ? 0 : *cmdlength_p;

    multios_pthread_mutex_lock(&tbl->mutex);

    DBMS5(stderr, "udrsock2_ioctl_c : mutex_lock\n");

    send_m = (udrsock2_msg_t *) tbl->pwritestream_buf;
    reply_m = (udrsock2_msg_t *) tbl->preadstream_buf;

    if (cmdlength > UDRSOCK2_IOCTL_DSIZE) {
	DBMS1(stderr, " ioctl_req_c :too long data_length");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_IOCTL_REQ;
    send_m->hdr.reply_type = 0;
    send_m->hdr.frame_type = UDRSOCK2_IOCTL_FRAME;
    send_m->hdr.result_code = htonl(0);
    send_m->hdr.frame_len = htonl(sizeof(udrsock2_ioctl_frame_t));
    send_m->frame.ioctl.cmd = htonl(cmd);
    send_m->frame.ioctl.size = htonl(cmdlength);

    if (cmdlength) {
	memcpy(&send_m->frame.ioctl.data, cmdbuff, cmdlength);
    }

    /* リクエスト送信 */
    result = msg_req_and_reply_c(id, send_m, reply_m);

#if 0
    udrsock2_dump_msg(reply_m);
#endif

    /* リプライ解析 */
    if (result < UDRSOCK2_NO_ERR) {
	DBMS1(stdout, " ioctl_req_c : negotiation fail\n");
	status = UDRSOCK2_ERR_CONNECT;
	tbl->timeout_flag = ~0;
	goto out;
    }

    if ((reply_m->hdr.msg_type != UDRSOCK2_REPLY_MSG) ||
	(reply_m->hdr.request_type != UDRSOCK2_IOCTL_REQ)) {
	DBMS1(stdout, " ioctl_req_c : err responce \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (NULL != cmdlength_p) {
	if (cmdlength > (int) ntohl(reply_m->frame.ioctl.size)) {
	    cmdlength = (int) ntohl(reply_m->frame.ioctl.size);
	}
	if (cmdlength) {
	    memcpy(cmdbuff, &reply_m->frame.ioctl.data, cmdlength);
	}
	*cmdlength_p = cmdlength;
	DBMS5(stdout, "reply_m->frame.ioctl.size = %d\n", *cmdlength_p);
    }

    if (ntohl(reply_m->hdr.result_code)) {
	status = ntohl(reply_m->hdr.result_code);
	goto out;
    }

    status = 0;

  out:

    if (!(fd < 0)) {
	DBMS5(stderr, "udrsock2_ioctl_c : mutex_clear\n");
	multios_pthread_mutex_unlock(&tbl->mutex);
    }

    return status;
}

/**
 * @brief int udrsock2_client_memio_write(const int id, const int memb_offs,  const int memb_size, const void *pbuff)
 * @briefa 大容量データを送信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param memb_offs 送信相手先バッファのオフセット値
 * @param memb_size 送信するデータサイズ
 * @param pbuff データバッファのポインタ
 * @retval -1 失敗
 * @retval 0 成功
 **/
int
udrsock2_client_memio_write(const int id, const int memb_offs,
			    const int memb_size, const void *pbuff)
{
    int memb_off_seg = 0;
    int result = -1;

    DBMS5(stderr, "udrsock2_memio_write_c : execute\n");
    DBMS5(stdout, "segment = %d\n", memb_off_seg);
    DBMS5(stdout, "memb_offs = %d\n", memb_offs);
    DBMS5(stdout, "memb_size = %d\n", memb_size);

    if (check_id(id)) {
	return -1;
    }

    result = client_memio_multipac(id, UDRSOCK2_DIR_WRITE,
				   (char *) pbuff, memb_off_seg,
				   memb_offs, memb_size);
    if (result != UDRSOCK2_NO_ERR) {
	return -1;
    }

    return 0;
}

/**
 * @fn static int client_memio_write_1pac(const int id, const char *pmemb_data, const int memb_size)
 * @brief memb_sizeで指定されたストリームデータを送信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param pmemb_data データブロックバッファポインタ
 * @retval -1 その他の致命的な失敗
 * @retval UDRSOCK2_ERR_FAIL ファイルディスクプリタが不正
 * @retval UDRSOCK2_NO_ERR 成功
 */
static int
client_memio_write_1pac(const int id, const char *pmemb_data,
			const int memb_size)
{
    int result = -1;
    int status = -1;
    int fd = -1;

    udrsock2_open_tbl_t *tbl;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    fd = tbl->sock_fd;

    if (fd < 0) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    if (tbl->timeout_flag) {
	return -1;
    }

    /* init */
    result = socket_writestream(fd, pmemb_data, memb_size);
    if (result == -1 || result != memb_size) {
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    status = UDRSOCK2_NO_ERR;
  out:

    return status;
}

/**
 * @fn static int memio_trans_start_req_c(const int id, const char trans_dir, const int32_t memb_seg, const int32_t memb_offs, const int32_t memb_size, const int loop_count, const int left_size)
 * @brief 大容量データ転送要求パラメータを送信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param trans_dir 転送方向( UDRSOCK2_B_READ : サーバ ｰ> クライアント, UDRSOCK2_B_WRITE : サーバ <- クライアント)
 * @param memb_seg 転送データの開始オフセットセグメント
 * @param memb_offs 転送データの開始オフセットアドレス
 * @param memb_size 定常時ソケット層に渡すデータブロックサイズ
 * @param loop_count ソケット層に渡すデータブロック数(否定常時含む）
 * @param left_size 否定常時（お尻)につくブロックサイズ
 * @retval UDRSOCK2_NO_ERR 成功
 * @retval -1 通信経路で致命的なエラー
 * @retval その他 UDRSOCK2エラーコード
 */
static int
memio_trans_start_req_c(const int id, const char trans_dir,
			const int32_t memb_seg, const int32_t memb_offs,
			const int32_t memb_size, const int loop_count,
			const int left_size)
{
    int result = -1;
    int status = -1;
    int fd = -1;

    udrsock2_open_tbl_t * const __restrict tbl = get_open_table(id);
    udrsock2_msg_t * __restrict send_m;

    DBMS5(stdout, " memio_trans_start_req_c : execute \n");

    /* データライト リクエスト */
    DBMS5(stdout, " memb_size = %d\n", memb_size);
    DBMS5(stdout, " loop_cont = %d\n", loop_count);
    DBMS5(stdout, " left_size = %d\n", left_size);

    if (check_id(id)) {
	return -1;
    }

    if (tbl->timeout_flag) {
	return -1;
    }

    send_m = (udrsock2_msg_t *) tbl->pwritestream_buf;

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_MEMIO_REQ;
    send_m->hdr.reply_type = 0;
    send_m->hdr.frame_type = UDRSOCK2_MEMIO_FRAME;
    send_m->hdr.frame_len = htonl(sizeof(udrsock2_memio_frame_t));

    send_m->frame.memio.dir = trans_dir;
    send_m->frame.memio.mem_segs = htonl(memb_seg);
    send_m->frame.memio.mem_offs = htonl(memb_offs);
    send_m->frame.memio.data_len = htonl(memb_size);
    send_m->frame.memio.num_loops = htonl(loop_count);
    send_m->frame.memio.left_len = htonl(left_size);
    send_m->frame.memio.memiob_len = htonl(tbl->sock_memiob_size);

    /* リクエスト送信 */
    multios_socket_set_nagle(fd, 1);
    /* send process */
    result = client_msg_send(id, send_m);
    if (result != UDRSOCK2_NO_ERR) {
	DBMS1(stderr,
	      " memio_trans_start_req_c : send error -> result = %d \n",
	      result);
	status = result;
	goto out;
    }

    DBMS5(stdout, " memio_trans_start_req_c : request success\n");

    status = UDRSOCK2_NO_ERR;

  out:

    return status;
}

/**
 * @fn static int client_memio_trans_end_req(const int id, const char trans_dir)
 * @brief 大容量データ送信終了要求を送信します
 * @param id UDRプロトコルオープンテーブルID
 * @param trans_dir 転送方向を指定( UDRSOCK2_B_READ : サーバ ｰ> クライアント, UDRSOCK2_B_WRITE : サーバ <- クライアント)
 * @retval -1 通信経路で致命的なエラー
 * @retval UDRSOCK2_ERR_FAIL 転送要求送信に失敗
 * @retval UDRSOCK2_NO_ERR 成功
 */
static int client_memio_trans_end_req(const int id, const char trans_dir)
{
    int result = -1;
    int status = -1;
    int fd = -1;

    udrsock2_msg_t *send_m, *reply_m;
    udrsock2_open_tbl_t * const __restrict tbl = get_open_table(id);

    if (check_id(id)) {
	DBMS1(stderr, "udrsock2_memio_trans_end_req_c : invalid id=%d\n",
	      id);
	return -1;
    }

    fd = tbl->sock_fd;

    if (fd < 0) {
	DBMS1(stderr, "udrsock2_memio_trans_end_req_c : invalid fd=%d\n",
	      fd);
	status = -1;
	goto out;
    }

    if (tbl->timeout_flag) {
	DBMS1(stderr,
	      "udrsock2_memio_trans_end_req_c : already timeout\n");
	return -1;
    }

    /* inital */
    send_m = (udrsock2_msg_t *) tbl->pwritestream_buf;
    reply_m = (udrsock2_msg_t *) tbl->preadstream_buf;

    /* make packet */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_MEMIO_END_REQ;
    send_m->hdr.reply_type = 0;
    send_m->hdr.frame_type = UDRSOCK2_NULL_FRAME;
    send_m->hdr.frame_len = htonl(0);
    send_m->hdr.result_code = htonl(0);

    /* リクエスト送信 */
    result = msg_req_and_reply_c(id, send_m, reply_m);

    if (result < UDRSOCK2_NO_ERR) {
	DBMS1(stderr,
	      "udrsock2_memio_trans_end_req_c  : transfer end request fail : %d\n",
	      result);
	status = -1;
	goto out;
    }

    /* responce check */
    switch (reply_m->hdr.reply_type) {
    case (UDRSOCK2_REPLY_SUCCESS):
	DBMS5(stdout, "udrsock2_memio_trans_end_req_c : reply success\n");
	status = UDRSOCK2_NO_ERR;
	break;
    case (UDRSOCK2_REPLY_ERROR):
    default:
	DBMS5(stdout, "udrsock2_memio_trans_end_req_c : reply error\n");
	status = UDRSOCK2_ERR_FAIL;
	break;
    }

  out:

    return status;
}

/**
 * @fn static int client_memio_multipac(const int id, const char dir, char *memb_data, const int32_t memb_seg, const int32_t memb_offs, const int32_t memb_size)
 * @brief 大容量データを送信又は受信します。
 * @param id UDRプロトコルオープンテーブルID
 * @param dir 転送方向を指定( UDRSOCK2_B_READ : サーバ ｰ> クライアント, UDRSOCK2_B_WRITE : サーバ <- クライアント)
 * @param memb_data 大容量データバッファポインタ
 * @param memb_seg バッファのセグメント
 *	memb_dataバッファのセグメントではない
 * @param memb_offs バッファのオフセット
 * 	memb_dataバッファのオフセットではない
 * @param memb_size 大容量データのサイズ
 * @retval -1 通信経路に致命的エラー
 * @retval UDRSOCK2_ERR_FAIL 転送に失敗
 * @retval UDRSOCK2_NO_ERR 成功
 */
static int
client_memio_multipac(const int id, const char dir, char *memb_data,
		      const int32_t memb_seg, const int32_t memb_offs,
		      const int32_t memb_size)
{
    int result = -1;
    int status = -1;
    int trans_loop_count = -1;
    int left_size = -1;
    int transfer_size = -1;
    int i = -1;
    int fd = -1;
#ifdef MULTIPAC_NAGLE
    int f_nagle = 1;
#endif
    udrsock2_open_tbl_t * const tbl = get_open_table(id);

    DBMS5(stdout, "udrsock2_memio_multipac_c : execute\n");
    DBMS5(stdout, "udrsock2_memio_multipac_c : memb_size = %d\n",
	  memb_size);

    if (check_id(id)) {
	DBMS1(stderr, "udrsock2_memio_multipac_c : invalid id=%d\n", id);
	return -1;
    }

    fd = tbl->sock_fd;

    if (fd < 0) {
	DBMS1(stderr, "udrsock2_memio_multipac_c : invalid fd=%d\n", fd);
	status = -1;
	goto out;
    }

    if (tbl->timeout_flag) {
	DBMS1(stderr, "udrsock2_memio_multipac_c : already timeout\n");
	return -1;
    }

    DBMS5(stderr, "udrsock2_memio_multipac_c : mutex_lock\n");
    multios_pthread_mutex_lock(&tbl->mutex);

    /* 転送開始 初期化 */
    trans_loop_count = (memb_size / tbl->sock_memiob_size);
    left_size = (memb_size % (tbl->sock_memiob_size));
    DBMS5(stdout, " trans_loop_count = %d\n", trans_loop_count);
    DBMS5(stdout, " left_size = %d\n", left_size);

    if (left_size > 0) {
	trans_loop_count++;
    }

    transfer_size = tbl->sock_memiob_size;
    result = memio_trans_start_req_c(id, dir,
				     memb_seg, memb_offs,
				     memb_size, trans_loop_count,
				     left_size);
    if (result != UDRSOCK2_NO_ERR) {
	DBMS5(stdout, "udrsock2_memio_multipac_c : reqest fail \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    /* 転送 */
    DBMS5(stdout, " trans_loop_count %d \n", trans_loop_count);

    switch (dir) {
    case (UDRSOCK2_DIR_WRITE):
	DBMS5(stdout, "udrsock2_memio_multipac_c : mode is write\n");

	/* バースト中のみ nagle アルゴリズムをON */
	multios_socket_set_nagle(fd, 1);

	for (i = trans_loop_count; i > 0; i--) {
	    DBMS5(stdout, "udrsock2_memio_multipac_c : i= %d\n", i);

#ifdef MULTIPAC_NAGLE
	    if (i <= 2 && (f_nagle)) {
		multios_socket_set_nagle(fd, 0);
		f_nagle = 0;
	    }
#endif
	    if ((i == 1) && (left_size > 0)) {
		transfer_size = left_size;
	    }

	    DBMS5(stdout,
		  "udrsock2_memio_multipac_c  : transfer_size = %d\n",
		  transfer_size);

	    result = client_memio_write_1pac(id, memb_data, transfer_size);
	    if (result != UDRSOCK2_NO_ERR) {
		DBMS1(stdout,
		      "udrsock2_memio_multipac_c  : 1pac transfer fail\n");
		status = UDRSOCK2_ERR_FAIL;
		goto out;
	    }

	    memb_data += transfer_size;
	}

	/* nagle アルゴリズムをOFF */
	multios_socket_set_nagle(fd, 0);

#ifdef TCP_ACK_DRIVE
	if (1) {
	    int len;
	    /* ACK促進のためのパディング回収 */
	    len =
		udrsock2_readstream(fd, tbl->preadstream_buf,
				    trans_loop_count);
	    if (len != trans_loop_count) {
		DBMS1(stderr,
		      "udrsock2_memio_multipac_s : readstream(pad) fail\n");
		status = UDRSOCK2_ERR_FAIL;
		goto out;
	    }
	}
#endif

	break;

    case (UDRSOCK2_DIR_READ):
	DBMS5(stdout, "udrsock2_memio_multipac_c : mode is read\n");

#ifdef MULTIPAC_SOCKETREAD
	/* nagle アルゴリズムをOFF */
	multios_socket_set_nagle(fd, 0);
	f_nagle = 0;

	for (i = trans_loop_count; i > 0; i--) {
	    DBMS5(stdout, "udrsock2_memio_multipac_c : i= %d\n", i);

	    if ((i == 1) && (left_size > 0)) {
		transfer_size = left_size;
	    }

	    DBMS5(stdout,
		  "udrsock2_memio_multipac_c : transfer_size = %d\n",
		  transfer_size);

	    result = client_memio_read_1pac(id, memb_data, transfer_size);
	    if (result != UDRSOCK2_NO_ERR) {
		DBMS1(stdout,
		      "udrsock2_memio_multipac_c : 1pac transfer fail\n");
		status = UDRSOCK2_ERR_FAIL;
		goto out;
	    }

	    memb_data += transfer_size;

#ifdef TCP_ACK_DRIVE
	    if (1) {
		int len;
		/* ACKを促進するためのパディングを送信 */
		len = socket_writestream(fd, "c", 1);
		if (len != 1) {
		    DBMS1(stdout,
			  "udrsock2_memio_multipac_c : 1pac transfer fail\n");
		    status = UDRSOCK2_ERR_FAIL;
		    goto out;
		}
	    }
#endif
	}
#else
	result = client_memio_read_1pac(id, memb_data, memb_size);
#endif

	break;

    default:
	DBMS1(stdout, "udrsock2_memio_multipac_c : unknown dir type\n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    /* 転送終了リクエスト */
    result = client_memio_trans_end_req(id, dir);
    if (result != UDRSOCK2_NO_ERR) {
	DBMS1(stdout, "udrsock2_memio_multipac_c : fail \n");
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }
    status = UDRSOCK2_NO_ERR;

  out:
    if (!(fd < 0)) {
	DBMS5(stderr, "udrsock2_memio64_c : mutex_clear\n");
	multios_pthread_mutex_unlock(&tbl->mutex);
    }

    return status;
}

/**
 * @fn int udrsock2_client_memio_read(const int id, const int memb_offs, const int memb_size, void *pbuff)
 * @brief 大容量ブロックデータを受信します
 * @param id UDRプロトコルオープンテーブルID
 * @param memb_offs メモリブロックオフセット
 * @param memb_size メモリブロックサイズ
 * @param pbuff データ取得バッファポインタ
 * @retval EINVAL 引数が不正
 * @retval 0 成功
 */
int udrsock2_client_memio_read(const int id, const int memb_offs,
			       const int memb_size, void *pbuff)
{
    int result = -1;
    int memb_off_seg = 0;

    DBMS5(stdout, "segment = %d\n", memb_off_seg);
    DBMS5(stdout, "memb_offs = %d\n", memb_offs);
    DBMS5(stdout, "memb_size = %d\n", memb_size);

    if (check_id(id)) {
	DBMS1(stderr, "udrsock2_memio_read_c : invalid id=%d\n", id);
	return EINVAL;
    }

    result = client_memio_multipac(id, UDRSOCK2_DIR_READ,
				   (char *) pbuff, memb_off_seg,
				   memb_offs, memb_size);
    if (result != UDRSOCK2_NO_ERR) {
	return -1;
    }
    return 0;
}

/**
 * @fn static int client_memio_read_1pac(const int id, char *pmemb_data, const int memb_size)
 * @brief 大容量データを１ブロック受信します
 * @param id UDRプロトコルオープンテーブルID
 * @param pmemb_data データバッファポインタ
 * @param memb_size 1ブロックのサイズ
 * @retval -1 致命的なエラー
 * @retval UDRSOCK2_ERR_FAIL プロトコルに関わるラー
 * @retval 0 成功
 */
static int
client_memio_read_1pac(const int id, char *pmemb_data, const int memb_size)
{
    int status = -1;
    int result = -1;
    int fd = -1;

    udrsock2_open_tbl_t * const tbl = get_open_table(id);

    DBMS5(stdout, "client_memio_read_1pac : execute \n");

    if (check_id(id)) {
	DBMS1(stderr, "client_memio_read_1pac : invalid id=%d\n", id);
	return -1;
    }

    fd = tbl->sock_fd;

    if (fd < 0) {
	DBMS1(stderr, "client_memio_read_1pac : invalid fd=%d\n", fd);
	status = -1;
	goto out;
    }

    result = socket_readstream(fd, pmemb_data, memb_size);

    if (result == -1) {
	tbl->timeout_flag = -1;
	status = -1;
	goto out;
    }

    if (result != memb_size) {
	DBMS1(stderr, "client_memio_read_1pac : invalid read size=%d/%d\n",
	      result, memb_size);
	status = UDRSOCK2_ERR_FAIL;
	goto out;
    }

    status = UDRSOCK2_NO_ERR;

  out:
    return status;
}

#if 0				/* for Debug */
static void udrsock2_dump_msg(udrsock2_msg_t * msg)
{
/*
	udrsock2_dump_msg(udrsock2_msg_t msg)

	デバッグ用コード
	メッセージデータの表示

	引数：
		udrsock2_msg_t msg : メッセージ構造体
	戻り値：
		無し
*/

    DBMS3(stdout, " socket message infomation \n");
    DBMS3(stdout, " messgae header part \n");
    DBMS3(stdout, "sizeof(header) = %d\n", sizeof(msg->hdr));
    DBMS3(stdout, " hdr.magic_no = 0x%08x \n", msg->hdr.magic_no);
    DBMS3(stdout, " hdr.magic_hd = 0x%08x \n", msg->hdr.magic_hd);
    DBMS3(stdout, " hdr.msg_type = %d \n", msg->hdr.msg_type);
    DBMS3(stdout, " hdr.request_type = %d \n", msg->hdr.request_type);
    DBMS3(stdout, " hdr.reply_type = %d \n", msg->hdr.reply_type);
    DBMS3(stdout, " hdr.result_code = %d \n", ntohl(msg->hdr.result_code));
    DBMS1(stdout, " hdr.frame_type = %d \n", msg->hdr.frame_type);
    DBMS1(stdout, " hdr.frame_len  = %d \n", ntohl(msg->hdr.frame_len));

    switch (msg->hdr.frame_type) {
    case (UDRSOCK2_NULL_FRAME):
	DBMS3(stdout, " frame type = NULL_FRAME \n");
	break;

    case (UDRSOCK2_CMDIO_FRAME):
	DBMS3(stdout, " frame type = CMD_FRAME \n");
	DBMS3(stsout, " cmdio.dir = %d\n", msg->frame.cmdio.dir);
	DBMS3(stdout, " cmdio.send_len = %d\n",
	      ntohl(msg->frame.cmdio.send_len));
	DBMS3(stdout, " cmdio.reply_len_max = %d\n",
	      ntohl(msg->frame.cmdio.reply_len_max));
	DBMS3(stdout, " cmdio.data = %s\n", msg->frame.cmdio.data);
	break;

    case (UDRSOCK2_IOCTL_FRAME):
	DBMS3(stdout, " frame type = IOCTL_FRAME \n");
	DBMS3(stdout, " ioctl.size = %d\n", ntohl(msg->frame.ioctl.size));
#if 0
	udr_dump_hexlist(msg->frame.ioctl.data,
			 ntohl(msg->frame.ioctl.size), 16);
#endif
	break;
#if 0
    case (UDRSOCK2_MEM_FRAME):
	DBMS3(stdout, " frame type = MEM_FRAME \n");
	DBMS3(stdout, " memio.dir = %d\n", msg->frame.memio.dir);
	DBMS3(stdout, " memio.size = %d\n",
	      WORD_DATA_GET(msg->frame.memio.size));
	DBMS3(stdout, " memio.size(high bit)= %x\n",
	      LOW_OFFS_GET(msg->frame.memio.size));
	DBMS3(stdout, " memio.offs = %d\n",
	      WORD_DATA_GET(msg->frame.memio.offs));
	DBMS3(stdout, " memio.offs(high bit)= %x\n",
	      LOW_OFFS_GET(msg->frame.memio.offs));
	DBMS3(stdout, " memio.remain_cnt = %d\n",
	      msg->frame.memio.remain_cnt);
	DBMS3(stdout, " memio.left_size = %d\n",
	      WORD_DATA_GET(msg->frame.memio.left_size));
	DBMS3(stdout, " memio.memiob_scc = %x\n",
	      msg->frame.memio.memiob_scc);
	break;
#endif

    case (UDRSOCK2_INIT_FRAME):
	DBMS3(stdout, " frame type = INIT_FRAME \n");

	break;
    }
}
#endif

/**
 * @fn static int hdr_certify(const udrsock2_msg_t * const recv_m)
 * @brief UDRSOCKv2プロトコルメッセージのマジック番号を確認します
 * @retval UDRSOCK2_ERR_ID パケットマジック番号が異なる
 * @retval UDRSOCK2_ERR_HD ヘッダマジックが異なる
 * @retval 0 UDRSOCK2_NO_ERR 成功
 */
static int hdr_certify(const udrsock2_msg_t * const recv_m)
{
    DBMS5(stdout, "msg_certify : execute \n");

    /* UDR パケットID確認 */
    if (ntohl(recv_m->hdr.magic_no) != UDRSOCK2_MAGIC_NO) {
	DBMS1(stderr, " msg_certify : no ID stream , ignored \n");
	return UDRSOCK2_ERR_ID;
    }

    /* UDR ハードウェア識別 ID確認 */
    if (ntohl(recv_m->hdr.magic_hd) != UDRSOCK2_MAGIC_HD) {
	DBMS1(stderr,
	      " msg_certify : differnce HARD ID stream , ignored \n");
	return UDRSOCK2_ERR_HD;
    }

    return UDRSOCK2_NO_ERR;
}

/**
 * @fn int udrsock2_client_set_memioxfersizemax(int size)
 * @brief udrsockプロトコルが一回でデータ転送可能な最大バイトサイズを設定します
 *	一回のネゴシエーション量を制限することで、ネットワーク接続が寸断されたのを確認するため
 * @param id UDRプロトコルオープンテーブルID
 * @param size 最大転送バイトサイズ(-1以下で現在の値を返します)
 * @retval -1 失敗
 * @retval 1以上 現在の最大転送バイトサイズ
 */
int udrsock2_client_set_memioxfersizemax(int id, int size)
{
    udrsock2_open_tbl_t *tbl;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    if (size < 0) {
	return tbl->MemioXferSizeMAX;
    } else if (size == 0) {
	return -1;
    }

    tbl->MemioXferSizeMAX = size;

    return tbl->MemioXferSizeMAX;
}


/**
 * @fn int udrsock2_client_get_memiob_size(const int id)
 * @brief 大容量データ転送時の1ブロックのサイズを取得します
 * @param id UDRプロトコルオープンテーブルID
 * @retval -1 失敗
 * @retval 0以上  大容量データ転送時の1ブロックのサイズ
 */
int udrsock2_client_get_memiob_size(const int id)
{
    udrsock2_open_tbl_t *tbl;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    return tbl->sock_memiob_size;
}

/**
 * @fn int udrsock2_client_set_memiob_size(const int id, const int size)
 * @brief 大容量データ転送時の1ブロックのサイズを設定します
 * @param id UDRプロトコルオープンテーブルID
 * @retval -1 失敗
 * @retval 0以上  大容量データ転送時の1ブロックのサイズ
 */
int udrsock2_client_set_memiob_size(const int id, const int size)
{
    udrsock2_open_tbl_t *tbl;

    if (check_id(id)) {
	return -1;
    }

    tbl = get_open_table(id);

    tbl->sock_memiob_size = size;

    return tbl->sock_memiob_size;
}

/**
 * @fn static int client_null_msg_send(const int id, udrsock2_msg_t *send_m)
 * @brief NULLパケットを送信します.
 * クライアント側のTCPプロトコルのACKの送信を補助します。
 * @param fd socket fd値
 * @param send_m 送信バッファ
 * @
 */
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int client_null_msg_send(const int id, udrsock2_msg_t * send_m)
{
    int result;
    int i;

    /* リクエスト パケット作成 */
    message_buffer_zero_clear(send_m);
    send_m->hdr.magic_no = htonl(UDRSOCK2_MAGIC_NO);
    send_m->hdr.magic_hd = htonl(UDRSOCK2_MAGIC_HD);
    send_m->hdr.msg_type = UDRSOCK2_REQUEST_MSG;
    send_m->hdr.request_type = UDRSOCK2_NULL;
    send_m->hdr.reply_type = 0;
    send_m->hdr.result_code = htonl(0);
    send_m->hdr.frame_type = UDRSOCK2_NULL_FRAME;
    send_m->hdr.frame_len = htonl(0);

    for (i = 0; i < 2; i++) {
	result = client_msg_send(id, send_m);
	if (result) {
	    return UDRSOCK2_ERR_FAIL;
	}
    }

    return UDRSOCK2_NO_ERR;
}
