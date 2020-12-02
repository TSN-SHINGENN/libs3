/**
 *	Copyright 2016 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2016-November-12 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @breif マルチセッション対応 10Gbe対応
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <signal.h>
#else
/* POSIX */
#include <sys/time.h>
#include <arpa/inet.h>
#endif 

#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>
#include <time.h>

/* multios */
#include "multios_sys_types.h"
#include "multios_unistd.h"
#include "multios_socket.h"
#include "multios_pthread.h"
#include "multios_malloc.h"
#include "multios_simd.h"
#include "multios_simd_cpu_hint.h"
#include "multios_simd_memory.h"
#include "multios_endian.h"
#include "multios_crc32_castagnoli.h"
#include "multios_time.h"
#include "multios_actor_th_simple_mpi.h"

#ifdef DEBUG
static const int debuglevel = 5;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* this */
#include "multios_tcpip_mpr2_def.h"
#include "multios_tcpip_mpr2_server.h"

typedef struct timeval timeval_t;

#define ACK_REQUEST 100

typedef struct _child_udrsock2_server_table {
    int no;
    int sockfd;
    void *recv_buf;
    void *reply_buf;
    void *mtab;
    volatile int thread_is_terminate;
    multios_castagonoli_crc32_t crc32c;

    union {
	struct {
	    unsigned int crc32c:1;
	} f;
	unsigned int flags;
    } init;

    multios_pthread_t pt;
} child_udrsock2_server_table_t;

typedef struct _subio_port_allocation {
    int no;
    multios_pid_t child_pid;
} subio_port_allocation_t;

#define CHILD_SERVER_MAX 20
static const int QNX650_Socket_Window_MAX = 32 * 1024 * 7;

typedef struct _master_server_table {
    subio_port_allocation_t *subio_port_tab;
    int num_subio_port_tab;

    multios_tcpip_mpr2_server_callback_function_table_t udrs_funcs;

    multios_simd_processer_function_t cpuinfo;

    int master_sock_port;
    int master_sockfd;
    uint32_t nServAddr;

    jmp_buf sigchld_sjbuf;

    child_udrsock2_server_table_t parent_server;

    /* クライアント情報 */
    struct sockaddr cli_addr;

    /* subio server情報 */
    multios_pthread_t subio_pt;
    multios_actor_th_simple_mpi_t subio_ac;

    int subio_master_sockfd;
    int subio_port_tbl_no;
    int subio_num_sessions;

    multios_castagonoli_crc32_t crc32c;
    child_udrsock2_server_table_t subio_child_tab[CHILD_SERVER_MAX];

    union {
	struct {
	    unsigned int master_listen_loop_out:1;
	} f;
	unsigned int flags;
    } request;

    union {
	struct {
	    unsigned int crc32c:1;
	    unsigned int subio_port_tab:1;
	    unsigned int udrs_funcs_tab:1;
	    unsigned int cli_addr:1;
	    unsigned int subio_pt:1;
	    unsigned int subio_ac:1;
	    unsigned int cpuinfo:1;
	} f;
	unsigned int flags;
    } init;
} master_server_table_t;

#define message_buffer_zero_clear(p) (multios_simd_memset_path_through_level_cache_da((p), 0x0 ,UDRSOCK2_MESSAGE_PACKET_BUF_SIZE))
static master_server_table_t *MTab = NULL;

#define get_master_server_table() MTab

static int server_listen_loop(child_udrsock2_server_table_t*t);

/* UDRSOCK サーバ関数 */
static int server_msg_recv(child_udrsock2_server_table_t *t);
static int server_msg_reply(child_udrsock2_server_table_t *t);

static void server_cmdio_req(child_udrsock2_server_table_t *t);
static void server_ioctl_req(child_udrsock2_server_table_t *t);
static void server_memio_req(child_udrsock2_server_table_t *t);
static void server_initial_negotiation_req(child_udrsock2_server_table_t *t);

/* メモリI/O */
static int server_memio_write_req(child_udrsock2_server_table_t *t);
static int server_memio_read_req(child_udrsock2_server_table_t *t);
static int server_memio_multipac(child_udrsock2_server_table_t *t, const char trans_dir, int offs,
				 int trans_loop_count, const int left_size, const int data_size);
static int server_memio_trans_start_req(child_udrsock2_server_table_t *t, int flag) MULTIOS_HINT_UNUSED;
static int server_memio_write_1pac(child_udrsock2_server_table_t *t, const int offs, const int size);
static int server_memio_read_1pac(child_udrsock2_server_table_t *t, const int offs, const int memb_size);
static int server_memio_trans_end_req(child_udrsock2_server_table_t *t, const char trans_dir, const int memb,
					 const char *pmemb_buff,
					 const int32_t memb_seg,
					 const int32_t memb_offs,
					 const int32_t memb_size);

/* UDRSOCK2 共通関数 */
static int socket_readstream(const int fd, char *ptr, const int length);
static int socket_writestream(const int fd, const char *ptr, const int bytelength);
static int hdr_certify(const udrsock2_msg_t * recv_m);	/* メッセージの確認 */

static void udrsock2_dump_msg(udrsock2_msg_t * msg) MULTIOS_HINT_UNUSED;

static void insert_crc_to_head( child_udrsock2_server_table_t *t, udrsock2_msg_t *msg);
static int check_head_crc( child_udrsock2_server_table_t *t, const udrsock2_msg_t *msg);

/* server */
static void *subio_listen_server_thread( void *d );
static void subio_server_open(master_server_table_t *mtab, const int SockPort, const int num_sessions);

static void cmdio_parent_server_proc(child_udrsock2_server_table_t *t);
static void *subio_child_server_thread(void *p);
static void server_finalize_proc(void);

/* 外部変数 */

#if 0
#define OLD_PROTOCOL 1 /* データ転送のプロトコルフラグ */
#endif

#if 0
#define MULTIPAC_SOCKETREAD 1
#endif

#if 0 /* 送信中に受信側が止まってパケットが破棄されたため使わない */
#define TCP_ACK_DRIVE 1
#endif

#ifndef WIN32
static void onintr_child_sigpipe( int signum, siginfo_t *info, void *ctx )
{
    master_server_table_t *mtab = get_master_server_table();
#if 0
    child_udrsock2_server_table_t *c = NULL;
    int i;
#endif
    DBMS3( stderr, "onintr_child_sigpipe : caught signal [SIGPIPE]\n");

#if 0
    /* 子供サーバーをすべてキャンセルする */
    for( i=0; i < CHILD_SERVER_MAX; i++ ) {
	c = &mtab->subio_child_tab[i];
	if( !(c->sockfd < 0) ) {
	    multios_pthread_cancel(&c->pt);
	}
    }

    /* 強制的にサーバを殺す処理を始める */
    server_finalize_proc();

    DBMS3( stderr, "onintr_child_sigpipe : server exit\n");

    exit(EXIT_SUCCESS);
#endif
    /* マスターリスナーに死亡フラグを立てる */
    mtab->request.f.master_listen_loop_out = 1;

    return;
}

/**
 * @fn static void onintr_sigchld( inr signum, siginfo_t *info, void *ctx )
 * @brief 子どもが死んだ通知が来たので、データIOポートの使用権を解除します
 */
static void onintr_sigchld( int signum, siginfo_t *info, void *ctx )
{
    master_server_table_t *mtab = get_master_server_table();
    subio_port_allocation_t *port_tab = NULL;
    int i;
    DBMS3( stderr, "onintr_sigchld : caught signal [SIGCHLD]\n");

    if( NULL == mtab ) {
	DBMS1( stderr, "onintr_sigchld : mtab = NULL\n");
	return;
    }

    /* プロセス番号からSUBIO待ち受けポートの割り当てを破棄 */
    for( i=0; i<mtab->num_subio_port_tab; i++ ) {
	port_tab = &mtab->subio_port_tab[i];
    	if( port_tab->child_pid == info->si_pid ) {
	    break;
	}
    }

    if( i==mtab->num_subio_port_tab ) {
	DBMS1( stderr, "onintr_chigchild : unknown pid = %d\n", info->si_pid);
    } else {
	DBMS3( stderr, "onintr_chigchild : tab_no=%d pid=%d Released\n", port_tab->no, port_tab->child_pid);
	port_tab->child_pid = -1;
    }

    multios_waitpid(info->si_pid, NULL, 0);
    longjmp(mtab->sigchld_sjbuf, 0);

    return;
}
#endif

static void insert_crc_to_head( child_udrsock2_server_table_t *t, udrsock2_msg_t *msg)
{
   uint32_t crc32;
   multios_mtimespec_t now;

   multios_crc32_castagonoli_clear( &t->crc32c, NULL);
   multios_clock_getmtime( MULTIOS_CLOCK_REALTIME, &now);

   msg->hdr.crc32 = 0xffffffff;
   msg->hdr.msec = htonl(now.msec);

   multios_crc32_castagonoli_calc(&t->crc32c, msg, UDRSOCK2_HDR_SIZE-sizeof(msg->hdr.crc32), &crc32);

   MULTIOS_SET_LE_D32( &msg->hdr.crc32, crc32);

   DBMS5( stderr, "insert_crc_to_head : crc = 0x%08x\n", crc32);

    return;
}

static int check_head_crc( child_udrsock2_server_table_t *t, const udrsock2_msg_t *msg)
{
   uint32_t crc32;

   multios_crc32_castagonoli_clear( &t->crc32c, NULL);
   multios_crc32_castagonoli_calc(&t->crc32c, msg, UDRSOCK2_HDR_SIZE, &crc32);

   DBMS5( stderr, "check_head_crc : crc = 0x%08x\n", crc32);

   return (crc32) ? -1 : 0;
}


static int socket_readstream(const int fd, char *ptr, const int length)
{
/*
 @fn static int socket_readstream(const int fd, char *ptr, const int length)
	概要：
	ソケット通信で受信されたストリームデータを読み込みます。
	引数：
	int fd : ファイルハンドル
	char *ptr : 受信バッファポインタ
	受信バッファの大きさ

	返り値：
	int 実際に受信されたストリームサイズ(Byte)
	-1 : リードエラー 
	備考：
		無し
*/

    int rc;
    int get_data_size = 0;
    int need_length = 0;
    char *p_ptr;

    DBMS5(stdout, "socket_readstream : execute\n");
    DBMS5(stdout, "socket_readstream : need byte size = %d\n", length);

    need_length = length;
    p_ptr = ptr;

    while (1) {
	rc = multios_socket_recv_withtimeout(fd, p_ptr, need_length, 0, 0);
	if (rc < 1) {
	    break;
	}
	get_data_size += rc;
	if (get_data_size == length) {
	    rc = get_data_size;
	    break;
	}
	p_ptr = ptr + get_data_size;
	need_length = length - get_data_size;
    }

    if (rc == -1) {	/* 接続切れ */
	DBMS5(stdout, "socket_readstream : strerr:%s(%d)\n", strerror(errno), errno);
        return -1;
    }

    if (rc == 0) {
	DBMS5(stdout, "socket_readstream : rc=0 strerr:%s(%d)\n", strerror(errno), errno);
	return -1;
    }

    if (length != rc) {
	DBMS1(stdout, "socket_readstream : recv fail \n");
	DBMS3(stdout, "socket_readstream : need length = %d\n", length);
	DBMS3(stdout, "socket_readstream : recb length = %d\n", rc);
    }

    DBMS5(stdout, "socket_readstream : %x %x %x %x \n",
	  ptr[0], ptr[1], ptr[2], ptr[3]);

    return rc;
}

static int socket_writestream(const int fd, const char *ptr, const int bytelength)
{
/*
	socket_writestream(int fd,char *ptr,int nlength)
	概要：
	ソケット通信でストリームデータを書き込みます。
	引数：
		int fd : ファイルハンドル
		char *ptr : 送信データバッファポインタ
		int nlength : 送信データ長（バイト）
		送信データの大きさ

	返り値：
		int 実際に送信されたストリームサイズ(Byte)
	備考：
		無し
*/
    int left, written;

    DBMS5(stdout, "socket_writestream : execute \n");

    left = bytelength;
    DBMS5(stdout, "socket_writestream : ptr=0x%p send size =%d\n", ptr, left);

    while (left > 0) {
	written = multios_socket_send_withtimeout(fd, ptr, left, 0, 3);

	if (written <= 0) {
	    DBMS5(stdout, "socket_writestream : multios_socket_send_withtimeout err=%s(%d)\n", strerror(errno), errno);
	    return (written);	/* err, return -1 */
	}
	left -= written;
	ptr += written;
    }

    return (bytelength - left);	/* return  >= 0 */
}

static void child_server_close(void)
{

    return;
}

/**
 * @fn int multios_tcpip_mpr2_server_attach(const char *const IPv4_ServAddr, const uint32_t SockPort, const multios_tcpip_mpr2_server_callback_function_table_t *const funcs)
 * @brief  UDRSocketv2通信のサーバー側のオープン制御
 * @param  待ちうけIPV4アドレス
 * @param  SockPort 待ち受けポート番号
 * @param  funcs udrsock2_xfer_function_table_t構造体ポインタ。コールバック関数設定
 * @retval 0 プロセスクローズ要求
 * @retval -1 失敗
 */
int multios_tcpip_mpr2_server_attach(const char *const IPv4_ServAddr, const uint32_t SockPort, const multios_tcpip_mpr2_server_callback_function_table_t *const funcs)
{
    int result, status;
    multios_socket_socklen_t clilen;
    fd_set fds, rfds;
    master_server_table_t *mtab = NULL;
    subio_port_allocation_t *subio_port_tab = NULL;
    int child_sockfd;
    multios_pid_t mypid;
    int i;
    struct in_addr in_serv_addr;

#ifndef WIN32
    struct sigaction sa_sigprm;
    sigset_t sigint_block, sigchld_block;
#endif

#ifndef WIN32
    memset( &sa_sigprm, 0x0, sizeof(sa_sigprm));
    sigemptyset(&sigint_block);
    sigemptyset(&sigchld_block);

    sigaddset( &sigint_block, SIGINT );
    sigaddset( &sigint_block, SIGCHLD );
#endif

    if( NULL != IPv4_ServAddr ) {
	result = multios_inet_aton(IPv4_ServAddr, &in_serv_addr);
	if(result==0) {
	    DBMS1(stderr, "udrsock2_server_open : bind addr=INADDR_ANY,  because can't convert IPAddr=%s\n", IPv4_ServAddr);
	    in_serv_addr.s_addr = htonl(INADDR_ANY);
	}
    } else {
	in_serv_addr.s_addr = htonl(INADDR_ANY);
    }

    /* signalを使って制御するために static変数にバッファを確保する */
    if( NULL != MTab ) {
	DBMS1(stderr,  "udrsock2_server_open : aleready server opened\n");
	return -1;
    }

    MTab = malloc(sizeof(master_server_table_t));
    if( NULL == MTab ) {
	DBMS1(stderr, "udrsock2_server_open : MTab malloc fail\n");
	return -1;
    }

    /* init */
    memset( MTab, 0x0, sizeof(master_server_table_t));
    MTab->master_sockfd = -1;

    mtab = get_master_server_table();

    /* 親サーバ情報初期化 */
    memset( &mtab->parent_server, 0x0, sizeof(child_udrsock2_server_table_t));
    mtab->parent_server.no   = -1;
    mtab->parent_server.mtab = mtab;
    mtab->nServAddr = in_serv_addr.s_addr;

    /* 子サーバ情報初期化 */
    for( i=0; i<CHILD_SERVER_MAX; i++ ) {
	child_udrsock2_server_table_t *t = &mtab->subio_child_tab[i];
	memset( t, 0x0, sizeof(child_udrsock2_server_table_t));
	t->no = i;
	t->sockfd = -1;
	t->mtab = (void*)mtab;
    }

    /* subioサーバー待ち受けポートテーブル初期化 */
    mtab->subio_master_sockfd = -1;
    mtab->num_subio_port_tab = UDRSOCK2_SUBIO_NUM_PORTS;
    mtab->subio_port_tab = malloc( sizeof(subio_port_allocation_t) * mtab->num_subio_port_tab );
    if( NULL == mtab->subio_port_tab ) {
	DBMS1( stderr, "udrsock2_server_open : subio_port_tab malloc fail(err:%s)\n", strerror(errno));
	status = -1;
	goto out;
    }
    memset( mtab->subio_port_tab, 0x0, sizeof(subio_port_allocation_t) * mtab->num_subio_port_tab );
    for( i=0; i<mtab->num_subio_port_tab; i++ ) {
	subio_port_tab = &mtab->subio_port_tab[i];
	subio_port_tab->no = i + 1;
	subio_port_tab->child_pid = -1;
    }
    mtab->init.f.subio_port_tab = 1;

    DBMS3( stderr, "udrsock2_server_open : SockPort = %d\n", SockPort);
    clilen = sizeof(mtab->cli_addr);
    mtab->master_sock_port = SockPort;

    result = multios_crc32_castagonoli_init( &mtab->crc32c);
    if( result ) {
	DBMS1( stderr, "udrsock2_server_open : multios_crc32_castagonoli_init fail\n");
	status = -1;
	goto out;
    }
    mtab->init.f.crc32c = 1;

    /* cpu情報の読み出し */
    multios_simd_check_processor_function(&mtab->cpuinfo);
    mtab->init.f.cpuinfo = 1;

    /* サーバー接続関数の登録 */
    if( NULL != funcs ) {
	mtab->udrs_funcs = *funcs;
	mtab->init.f.udrs_funcs_tab = 1;
    }
#ifndef WIN32
    memset( &sa_sigprm, 0x0, sizeof(sa_sigprm));
    sa_sigprm.sa_handler = SIG_IGN;

    if( sigaction( SIGCHLD, &sa_sigprm, NULL ) < 0 ) {
	DBMS1( stderr, "udrsock2_server_open : SIGCHILD -> SIG_IGN fail\n" );
	status =-1;
	goto out;
    }
#endif

    /* server socket */
    mtab->master_sockfd = 
	multios_socket_server_openforIPv4(mtab->nServAddr, SockPort, 1, UDRSOCK2_BIND_MAX_TRY,
				   PF_INET, SOCK_STREAM, 0, NULL);
    if (mtab->master_sockfd < 0) {
	DBMS1(stdout,
	      "udrsock2_server_open : multios_socket_server_openforIPv4 fail\n");
	status = -1;
	goto out;
    }

    DBMS3(stderr, "bind succeeded, NIC Address = %s Port = %d\n", inet_ntoa(in_serv_addr), SockPort);

    /* QNX6.5.0で設定可能な最大サイズに変更 */
    multios_socket_set_send_windowsize( mtab->master_sockfd, QNX650_Socket_Window_MAX);
    multios_socket_set_recv_windowsize( mtab->master_sockfd, QNX650_Socket_Window_MAX);

    /* タイムアウト処理 初期化 */
    FD_ZERO(&fds);
    FD_SET(mtab->master_sockfd, &fds);

    /* グローバルジャンプ位置設定 */
    setjmp(mtab->sigchld_sjbuf);

#ifndef WIN32
    /* 親がSIGCHLDを受けたときの処理を設定 */
    memset( &sa_sigprm, 0x0, sizeof(sa_sigprm));
    sa_sigprm.sa_sigaction = onintr_sigchld;
    sa_sigprm.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;

    if( sigaction( SIGCHLD, &sa_sigprm, NULL ) < 0 ) {
	DBMS1( stderr, "udrsock2_server_open : SIGCHILD -> onintr_sigchld fail\n");
	status = -1;
	goto out;
    }
#endif

    while(1) {
	timeval_t tv;

	/* ソケットメッセージタイムアウト時間 設定 */
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	rfds = fds;

	result =
	    select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	if (result == 0) {
		DBMS5(stderr, "udrsock2_server_open : socket listen\n");

	    continue;
	}

#ifndef WIN32
	sigprocmask( SIG_BLOCK, &sigchld_block, NULL);
#endif

	if( result == EINTR ) {
	    DBMS5(stderr, "udrsock2_server_open : recive signal listen\n");
#ifndef WIN32
	    sigprocmask( SIG_UNBLOCK, &sigchld_block, NULL);
#endif
	    continue;
	}

	if ((child_sockfd =
	     multios_socket_accept(mtab->master_sockfd,
				   (struct sockaddr *) &mtab->cli_addr,
				   &clilen)) == -1 ) {
	    DBMS1( stderr, "udrsock2_server_open : multios_socket_accept(%s) fail\n", strerror( errno ));

	    continue; /* 再起動するように書き換え */
	}

	/* SUBIOポートの空きを検索 */
	for( i=0; i<mtab->num_subio_port_tab; i++ ) {
	    subio_port_tab = &mtab->subio_port_tab[i];
	    if( subio_port_tab->child_pid == -1) {
		break;
	    }
	}
  	if( i == mtab->num_subio_port_tab ) {
	    /* subioポートが割り当てられないので強制切断 */
	    DBMS1(stderr, "udrsock2_server_open : can't assign subio socket port\n");
	    DBMS3(stderr, "udrsock2_server_open : connection refused compulsorily\n");

    	    multios_socket_shutdown(child_sockfd, SHUT_RDWR);
    	    multios_socket_close(child_sockfd);
	    continue;
	} else {
	    mtab->subio_port_tbl_no = i; /* 実際は子供で使われる */
	}

	/* 子どもを作る */
#ifndef WIN32
	mypid = fork();
#else
	mypid = 0;
#endif

	switch(mypid) {
	case -1:
	    /* 小創りに失敗 */
	    DBMS1( stderr, "udrsock2_server_open : reate child process fail \n");
	    child_server_close();
	    break;
	case 0:
	    /* 生まれた子どもプロセス */
#ifdef UDR
		DBMS3(stderr, "udrsock2_server_open : forked pid = %d assined child port = %d\n", getpid(), subio_port_tab->no);
#endif
	    mtab->parent_server.sockfd = child_sockfd;

#ifndef WIN32
	    /* 子どもプロセスのSIGCHLDは無効ににする */
	    memset( &sa_sigprm, 0x0, sizeof(sa_sigprm));
	    sa_sigprm.sa_sigaction = SIG_IGN;
	    if( sigaction( SIGCHLD, &sa_sigprm, NULL ) < 0 ) {	
		DBMS1( stderr, "udrsock2_server_open : SIGCHILD -> SIG_IGN fail\n");
		exit(EXIT_FAILURE);
		goto out;
	    }

	    /* 子どもプロセスのパイプがキレた場合は全てのセッションを終了して子どもを終了 */
	    memset( &sa_sigprm, 0x0, sizeof(sa_sigprm));
	    sa_sigprm.sa_sigaction = onintr_child_sigpipe;
	    sa_sigprm.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
	    if( sigaction( SIGPIPE, &sa_sigprm, NULL ) < 0 ) {
		DBMS1( stderr, "udrsock2_server_open : SIGPIPE -> onintr_sigchld fail\n");
		status = -1;
		goto out;
	    }
#endif

	    result = multios_actor_th_simple_mpi_init( &mtab->subio_ac);
	    if(result) {
		DBMS1( stderr, "udrsock2_server_open : multios_actor_th_simple_mpi_init fail\n");
		status = -1;
		goto out;
	    }
	    mtab->init.f.subio_ac = 1;

	    if( NULL !=mtab->udrs_funcs.init ) {
		mtab->udrs_funcs.init();
	    }

	    /* 親サーババッファ初期化 */
	    cmdio_parent_server_proc(&mtab->parent_server);

	    if( NULL != mtab->udrs_funcs.client_shutdown ) {
		DBMS3(stderr, "udrsock2_server_open : mtab->udrs_funcs.client_shutdown() execute\n");
	        mtab->udrs_funcs.client_shutdown();
	    }

	    server_finalize_proc();

	    DBMS3( stderr, "udrsock2_server_open : child exit(EXIT_SUCCESS)\n");
	    /* 子どもが正常終了します */
	    exit(EXIT_SUCCESS);

	    /* 処理されることはないbreak */
	    break;
	default:
	    /* 親プロセスの登録処理 */
	    multios_socket_close(child_sockfd);
	    subio_port_tab->child_pid = mypid;
#ifndef WIN32
	    sigprocmask( SIG_UNBLOCK, &sigchld_block, NULL);
#endif
	}
    }

    status = 0;
out:    
    server_finalize_proc();

    return status;
}

/* *
 * @fn static void server_finish_proc(void)
 * @brief サーバーの完全終了処理です。
 *  すべてのリソースを強制的に解放します。
 */
static void server_finalize_proc(void)
{
    master_server_table_t *mtab = get_master_server_table();
    int i;

    DBMS3( stderr, "server_finalize_proc : execute\n");

    if( NULL == mtab ) {
	DBMS1(stdrrr, "server_finalize_proc : mtab is NULL\n");
	return;
    }

    multios_sleep(3);

    /* 親サーバ終了処理  */
    if( !(mtab->parent_server.sockfd < 0) ) {
    	    multios_socket_shutdown(mtab->parent_server.sockfd, SHUT_RDWR);
    	    multios_socket_close(mtab->parent_server.sockfd);
	    mtab->parent_server.sockfd = -1;
    }

    /* 確保していたサーバー用バッファを解放 */
    if( NULL != mtab->parent_server.recv_buf ) {
	multios_mfree(mtab->parent_server.recv_buf);
	mtab->parent_server.recv_buf = NULL;
    }

    if( NULL != mtab->parent_server.reply_buf ) {
	multios_mfree(mtab->parent_server.reply_buf);
	mtab->parent_server.recv_buf = NULL;
    }

    if(mtab->parent_server.init.f.crc32c) {
	multios_crc32_castagonoli_destroy(&mtab->parent_server.crc32c);
	mtab->parent_server.init.f.crc32c = 0;
    }

    if(mtab->parent_server.init.flags) {
	DBMS1( stderr, "mtab->parent_server.init.flags = 0x%x\n", mtab->parent_server.init.flags);
    }

    /* subiolistenサーバーが開いていた場合は終了 */
    if( !(mtab->subio_master_sockfd < 0 ) ) {
	multios_socket_shutdown(mtab->subio_master_sockfd, SHUT_RDWR);
	multios_socket_close(mtab->subio_master_sockfd);
	mtab->subio_master_sockfd = -1;
    }

    /* 子供サーバーをすべてキャンセルする */
    for( i=0; i < CHILD_SERVER_MAX; i++ ) {
	child_udrsock2_server_table_t *t = &mtab->subio_child_tab[i];
	t = &mtab->subio_child_tab[i];
	if( !(t->sockfd < 0) ) {
	    multios_pthread_cancel(&t->pt);
	}
    }

    /* subio子供サーバ終了処理 */
    for( i=0; i<CHILD_SERVER_MAX; i++ ) {
	child_udrsock2_server_table_t *t = &mtab->subio_child_tab[i];
	if( !(t->sockfd < 0 ) ) {
    	    multios_socket_shutdown(t->sockfd, SHUT_RDWR);
    	    multios_socket_close(t->sockfd);
	    t->sockfd = -1;
	}
	if( NULL != t->recv_buf ) {
	    multios_mfree(t->recv_buf);
	    t->recv_buf = NULL;
	}

	if( NULL != t->reply_buf ) {
	    multios_mfree(t->reply_buf);
	    t->reply_buf = NULL;
	}

	if( t->init.f.crc32c ) {
	    multios_crc32_castagonoli_destroy(&t->crc32c);
	    t->init.f.crc32c = 0;
	}

	if(t->init.flags) {
	    DBMS1( stderr, "server_finalize_proc : mtab->subio_child_tab[%d] init.flags = 0x08x\n", t->init.flags);
	}
    }

    /* マスター待ち受けサーバー終了処理 */
    if( !(mtab->master_sockfd < 0 ) ) {
	multios_socket_shutdown(mtab->master_sockfd, SHUT_RDWR);
	multios_socket_close(mtab->master_sockfd);
	mtab->master_sockfd = -1;
    }

    /* データ領域開放処理 */
    if(mtab->init.f.subio_port_tab) {
	free(mtab->subio_port_tab);
	mtab->subio_port_tab = NULL;
	mtab->init.f.subio_port_tab = 0;
    }

    /* asynccmdオブジェクト破棄 */
    if(mtab->init.f.subio_ac) {
	multios_actor_th_simple_mpi_destroy(&mtab->subio_ac);
	mtab->init.f.subio_ac = 0;
    }

    /* crc32cオブジェクト破棄 */
    if(mtab->init.f.crc32c) {
	multios_crc32_castagonoli_destroy(&mtab->crc32c);
	mtab->init.f.crc32c = 0;
    }

    if( NULL != MTab ) {
	free(MTab);
	MTab = mtab = NULL;
    }

    return;
}

static void *subio_listen_server_thread( void *d ) {
    int SockPort;
    int num_sessions;
    master_server_table_t *mtab = (master_server_table_t*)d;

    DBMS3( stderr, "subio_listen_server_thread : execute\n");

    SockPort = mtab->master_sock_port + ( 1 + mtab->subio_port_tbl_no );
    num_sessions = mtab->subio_num_sessions;

    subio_server_open( mtab, SockPort, num_sessions);

    return NULL;
}


static void cmdio_parent_server_proc(child_udrsock2_server_table_t *t)
{
    int result;
    master_server_table_t *mtab = (master_server_table_t*)t->mtab;

    DBMS3( stderr, "cmdio_parent_server_proc exec\n");

    result = multios_malloc_align((void**)&t->recv_buf, 512, UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if(result) {
	DBMS1( stderr, "cmdio_parent_server_proc : multios_malloc_align(recv_buf) fail\n");
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return;
    }

    result = multios_malloc_align((void**)&t->reply_buf, 512, UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if(result) {
	DBMS1( stderr, "cmdio_parent_server_proc : multios_malloc_align(reply_buf) fail\n");
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return;	
    }

    result = multios_crc32_castagonoli_init_have_a_obj(&t->crc32c, &mtab->crc32c);
    if(result) {
	DBMS1( stderr, "cmdio_parent_server_proc : multios_crc32_castagonoli_init_have_a_obj fail\n");
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return;	
    }
    t->init.f.crc32c = 1;

    server_listen_loop(t);

    return;
}


static void subio_server_open(master_server_table_t *mtab, const int SockPort, const int num_sessions)
{
    int result;
    multios_socket_socklen_t clilen;
    struct sockaddr cli_addr;
    fd_set fds, rfds;
    child_udrsock2_server_table_t *t = NULL;
    int i;
    int child_sockfd;
    int remain_sessions = num_sessions;
    enum_multios_actor_th_simple_mpi_res_t ac_res;
#if 0 /* for Debug */
    fprintf( stderr, "SockPort = %d\n", SockPort);
#endif

    /* init */
    memset( &cli_addr, 0x0, sizeof(cli_addr));
    clilen = sizeof(cli_addr);

    DBMS3( stderr, "subio_server_open : multios_socket_server_open pre\n");
    /* server socket */
    mtab->subio_master_sockfd =
	multios_socket_server_openforIPv4(mtab->nServAddr, SockPort, 1, UDRSOCK2_BIND_MAX_TRY,
				   PF_INET, SOCK_STREAM, 0, NULL);
    DBMS3( stderr, "subio_server_open : multios_socket_server_open Done fd =%d\n", mtab->subio_master_sockfd);
    if (mtab->subio_master_sockfd < 0) {
	DBMS1(stdout,
	      "subio_server_open : multios_socket_server_open fail\n");
	return;
    }

    /* QNX6.5.0で設定可能な最大サイズに変更 */
    DBMS5( stderr, "subio_server_open : multios_socket_set_windowsize pre\n");
    multios_socket_set_send_windowsize( mtab->subio_master_sockfd, QNX650_Socket_Window_MAX);
    multios_socket_set_send_windowsize( mtab->subio_master_sockfd, QNX650_Socket_Window_MAX);
    DBMS5( stderr, "subio_server_open : multios_socket_set_windowsize Done.\n");

    /* タイムアウト処理 初期化 */
    FD_ZERO(&fds);
    FD_SET(mtab->subio_master_sockfd, &fds);

    /* mainスレッドにサーバがlistnに入ることを通知 */
    DBMS5( stderr, "subio_server_open :  asynccmd_set_request(AC_ACK)\n");
    result = multios_actor_th_simple_mpi_send_request( &mtab->subio_ac, MULTIOS_ACTOR_TH_SIMPLE_MPI_RES_ACK, &ac_res);
    if(result) {
	DBMS1(stderr, "subio_server_open : multios_actor_th_simple_mpi_send_request fail\n");
	remain_sessions = 0;
    } else if( ac_res != MULTIOS_ACTOR_TH_SIMPLE_MPI_RES_NACK ) {
	DBMS1( stderr, "subio_server_open : asynccmd_ack(NACK)\n");
	remain_sessions = 0;
    } else {
	/* クライアントリクエスト監視のために無限ループ */
	DBMS4(stdout, "subio_server_open : subio_master_sockfd = %d\n", mtab->subio_master_sockfd);
    }	

    while (remain_sessions) {
	timeval_t tv;
	DBMS4( stderr, "subio_server_open : remain_session = %d\n", remain_sessions);

	/* ソケットメッセージタイムアウト時間 設定 */
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	rfds = fds;

	result =
	    select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	if (result == 0) {
	    DBMS5(stdout, "subio_server_open : socket listen \n");

	    continue;
	}

	if(result < 0) {
	    DBMS2( stdout,"subio_server_open : select error(%s)\n", strerror( errno ));
	    break;
	}

	if ((child_sockfd =
	     multios_socket_accept(mtab->subio_master_sockfd,
				   (struct sockaddr *) &cli_addr,
				   &clilen)) == -1 ) {
	    DBMS1( stderr, "subio_server_open : multios_socket_accept(%s)\n", strerror( errno ));
	    DBMS1( stderr, "subio_server_open : mtab->subio_master_sockfd = %d\n", mtab->subio_master_sockfd);

	    /* サーバーセッションを終了 */
	    break;
	}

	/* 開きテーブルの検索 */
	for( i=0; i<CHILD_SERVER_MAX; i++ ) {
	   t = &mtab->subio_child_tab[i];
	   if( t->sockfd == -1 ) {
		break;
	   }
	}
	if( i == CHILD_SERVER_MAX ) {
	    /* 子供スレッドの開き領域が無い */
	    DBMS1( stderr, "subio_server_open : no space of child server tanble"); 

	    /* 強制切断 */
	    multios_socket_shutdown(child_sockfd, SHUT_RDWR);
	    multios_socket_close(child_sockfd);
	    continue;
	}

	/* 子供コネクションの登録 */
	    t->sockfd = child_sockfd;
	/* 子供サーバースレッドの起動 */
	result = multios_pthread_create( &t->pt, NULL, subio_child_server_thread, (void*)t);
	if(result) {
	    DBMS1( stderr, "subio_server_open : can't create thread(subio_child_server_thread)\n");
	    t->sockfd = -1;
	    /* 強制切断 */
	    multios_socket_shutdown(child_sockfd, SHUT_RDWR);
	    multios_socket_close(child_sockfd);
	} else {
	    multios_pthread_detach(&t->pt);
	}
	remain_sessions --;
    }

    DBMS4(stdout, "subio_server_open : thread_exit\n");

    /* 全てのセッション接続が終了したら SUBIOサーバの役目は終了 */
    multios_socket_shutdown(mtab->subio_master_sockfd, SHUT_RDWR);
    multios_socket_close(mtab->subio_master_sockfd);
    mtab->subio_master_sockfd = -1;

    return ;
}

/**
 * @fn void *child_server_thread(void *p)
 * @brief コネクションが張られた後に起動する子供サーバスレッド
 *	接続が切れたらスレッドを終了します
 */
static void *subio_child_server_thread(void *p)
{
    child_udrsock2_server_table_t *t = (child_udrsock2_server_table_t*)p;
    master_server_table_t *mtab = (master_server_table_t*)t->mtab;
    int result;
    DBMS5( stderr, "child thread[%d] create\n", t->no);

    result = multios_malloc_align( (void**)&t->recv_buf, 512, UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if(result) {
	DBMS1( stderr, "subio_child_server_thread[%d] : recv_buf multios_malloc_align fail\n", t->no);
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return NULL;
    }

    result = multios_malloc_align( (void**)&t->reply_buf, 512, UDRSOCK2_MESSAGE_PACKET_BUF_SIZE);
    if(result) {
	DBMS1( stderr, "subio_child_server_thread[%d] : reply_buf malloc_malloc_align fail\n", t->no);
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return NULL;
    }

    result = multios_crc32_castagonoli_init_have_a_obj(&t->crc32c, &mtab->crc32c);
    if(result) {
	DBMS1( stderr, "cmdio_parent_server_proc : multios_crc32_castagonoli_init_have_a_obj fail\n");
#if defined(QNX)
	raise(SIGPIPE);
#endif
	return NULL;	
    }
    t->init.f.crc32c = 1;

    server_listen_loop(t);

    return NULL;
}

static int server_listen_loop(child_udrsock2_server_table_t *t)
{
/*
  @fn static int server_listen_loop(child_udrsock2_server_table_t *t)
	サーバ側でクライアントの要求を監視する
    引数：
	int sockfd : オープンしたソケットハンドル
	返り値:
		UDRSOCK2_CCLOSE : チャイルドプロセスクローズ
*/

    int result;
    fd_set fds, rfds;
    timeval_t tv;
    udrsock2_msg_t *recv_m = (udrsock2_msg_t*)t->recv_buf;
    udrsock2_msg_t *reply_m = (udrsock2_msg_t*)t->reply_buf;
    const master_server_table_t *mtab = (master_server_table_t*)t->mtab;
    const int sockfd = t->sockfd;

    /* 初期化 */
    multios_socket_set_nagle( sockfd, 0 );

    /* タイムアウト処理 初期化 */
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    /* クライアントリクエスト監視のために無限ループ */

    while (1) {
	/* 受信バッファをクリア */
	message_buffer_zero_clear(recv_m);
	message_buffer_zero_clear(reply_m);

	/* ソケットメッセージタイムアウト時間 設定 */
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	rfds = fds;

	result =
	    select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	/* 死亡要求を調べて、ループアウトする */
	if( mtab->request.f.master_listen_loop_out ) {
	    DBMS1( stderr, "server_listen_loop : listen loopout\n");
	    return UDRSOCK2_CCLOSE;
	}

	if (result == 0) {
	    DBMS5(stderr, "server_listen_loop : result = %d \n", result);
	    DBMS4(stderr, "server_listen_loop : socket read timeout \n");
	    continue;
	}

	/* リクエストデータ リード */
	result = server_msg_recv(t);

	if (result == UDRSOCK2_EOF) {	/* クライアントが閉じた */
	    return UDRSOCK2_CCLOSE;
	}

	if (result) {	/* エラーを検出 */
	    DBMS1( stderr, "server_listen_loop : recived ignore message, shutdown\n");
	    return result;
	}

	/* クライアント要求選択 */
	switch (recv_m->hdr.request_type) {
	case (UDRSOCK2_CMDIO_REQ):
	    DBMS5(stdout, "server_listen_loop : found UDRSOCK2_CMDIO_REQ\n");
	    server_cmdio_req(t);
	    DBMS5(stdout, "server_listen_loop : UDRSOCK2_CMD_REQ done\n");
	    break;

	case (UDRSOCK2_IOCTL_REQ):
	    DBMS5(stdout, "server_listen_loop : found UDRSOCK2_IOCTL_REQ\n");
	    server_ioctl_req(t);
	    DBMS5(stdout, "server_listen_loop : UDRSOCK2_IOCTL_REQ done\n");
	    break;

	case (UDRSOCK2_MEMIO_REQ):
	    DBMS5(stdout, "server_listen_loop : found UDRSOCK2_MEMIO_REQ\n");
	    server_memio_req(t);
	    DBMS5(stdout, "server_listen_loop : UDRSOCK2_MEMIO_REQ done\n");
	    break;
	case UDRSOCK2_INIT_REQ:
	    DBMS5(stdout, "server_listen_loop : found UDRSOCK2_INIT_REQ\n");
	    server_initial_negotiation_req(t);
	    DBMS5(stdout, "server_listen_loop : found UDRSOCK2_INIT_done\n");
	    break;
	case (UDRSOCK2_NULL):
	    /* 何もしないで抜けます */
	    break;
	default:
	    DBMS5(stderr, "server_listen_loop : unknown request_type = %d\n",
		  recv_m->hdr.request_type);
	}
    }
}

static int hdr_certify(const udrsock2_msg_t * recv_m)
{
    DBMS5(stdout, "hdr_certify : execute \n");

    /* UDR パケットID確認 */
    if (!(ntohl(recv_m->hdr.magic_no) && UDRSOCK2_MAGIC_NO)) {
	DBMS1(stdout, "hdr_certify : difference magic_no, ignored \n");
	return UDRSOCK2_ERR_ID;
    }

    /* UDR ハードウェア識別 ID確認 */
    if (!(ntohl(recv_m->hdr.magic_hd) & UDRSOCK2_MAGIC_HD)) {
	DBMS1(stdout,
	      "hdr_certify : differnce magic_hd, ignored \n");
	return UDRSOCK2_ERR_HD;
    }

    return UDRSOCK2_NO_ERR;
}


/**
 * @fn static int server_msg_recv(child_udrsock2_server_table_t *t)
 * @brief		メッセージを受信します。
 * 戻り値にて、受信データの状態を
 * @param t child_udrsock2_server_table_t構造体ポインタ
 * @retval UDRSOCK2_NO_ERR : メッセージ確認
 * @retval UDRSOCK2_ERR_FAIL :		
 * @retval UDRSOCK2_EOF : 接続が切れた
 **/
static int server_msg_recv(child_udrsock2_server_table_t *t)
{
    int bytesize;
    char *streamdata;
    int result;
    int fd = t->sockfd;
    udrsock2_msg_t *recv_m = (udrsock2_msg_t*)t->recv_buf;

    DBMS5(stdout, "server_msg_recv : execute\n");

    /* ヘッダーとフレームを分割して受信する */
	/* ヘッダー受信 */
    bytesize = UDRSOCK2_HDR_SIZE;
    streamdata = (char *) recv_m;

    result = socket_readstream(t->sockfd, streamdata, bytesize);
    DBMS5(stdout,
	  "server_msg_recv : udrsock2_readstream(head) result = %d\n",
	  result);

    if (result < 0) {
	return UDRSOCK2_EOF;
    }

    if (result != bytesize) {
	return UDRSOCK2_ERR_FAIL;
    }

    result = hdr_certify(recv_m);
    if (result != UDRSOCK2_NO_ERR) {
	return result;
    }

    result = check_head_crc( t, recv_m);
    if( result ) {
	DBMS1( stderr, "server_msg_recv : check_head_crc fail\n");
	return UDRSOCK2_ERR_FAIL;
    }
#if 0
    udrsock2_dump_msg(recv_m);
#endif

    /* フレーム部受信 */
    bytesize = UDRSOCK2_GET_FRAME_SIZE(recv_m);
    if (bytesize == 0) {
	return UDRSOCK2_NO_ERR;
    }

    streamdata += UDRSOCK2_HDR_SIZE;
    result = socket_readstream(fd, streamdata, bytesize);
    DBMS5(stdout,
	  "server_msg_recv : udrsock2_readstream(frame) result = %d\n",
	  result);

    if (result < 0) {
	return UDRSOCK2_EOF;
    }

    if (result != bytesize) {
	return UDRSOCK2_ERR_FAIL;
    }

    return UDRSOCK2_NO_ERR;
}

static int server_msg_reply(child_udrsock2_server_table_t *t)
{
/*
	int udrsock2_msg_send(udrsock2_msg_t *send_m)
		メッセージを送信します。
	引数：
		udrsock2_msg_t *send_m : 送信メッセージ
	返り値：
		成功 ： UDRSOCK2_NO_ERR
		失敗 :  UDRSOCK2_ERR_FAIL
*/

    /* ヘッダーとフレームを分割して送信する */

    int fd = t->sockfd;
    udrsock2_msg_t *send_m = t->reply_buf;
    int bytesize;
    int result;

    DBMS3( stderr, "server_msg_reply : execute\n");

    /* CRCを乗せる */
    insert_crc_to_head( t, send_m);

    multios_socket_set_nagle( t->sockfd, 0 );

    /* 分割して送信すると遅いので、一括で送信する */
    bytesize = UDRSOCK2_HDR_SIZE + UDRSOCK2_GET_FRAME_SIZE(send_m);

    DBMS3( stderr, "server_msg_reply : fd = %d send size = %d\n", fd, bytesize) ;

    result = socket_writestream(fd, (char*)send_m, bytesize);
    if (result != bytesize) {
	return UDRSOCK2_ERR_FAIL;
    }
    return UDRSOCK2_NO_ERR;
}

static void server_cmdio_req(child_udrsock2_server_table_t *t)
{
/*
	int udrsock2_cmdio_req_s(udrsock2_msg_t *recv_m)
	概要：
	クライアントからのリクエストに対し
	コマンドライトを実行し、
	実行結果をクライアントにレスポンスします。

	引数：
		udrsock2_msg_t *recv_m
	戻り値:
		失敗 : -1
		成功 : 0
*/

    int cmdln_no = -1;
    int result = -1;
    unsigned int char_len = 0, reply_len_max;

    udrsock2_msg_t *recv_m = t->recv_buf;
    udrsock2_msg_t *reply_m = t->reply_buf;
    const master_server_table_t *mtab = (master_server_table_t*)t->mtab;
    char *pdata = NULL;

    DBMS5(stdout, "server_cmdio_req : execute\n");

    /* ヘッダコピー */
    memcpy(reply_m, recv_m, UDRSOCK2_HDR_SIZE);

    /* リプライメッセージ ヘッダ作成 */
    reply_m->hdr.msg_type = UDRSOCK2_REPLY_MSG;
    reply_m->hdr.reply_type = UDRSOCK2_REPLY_SUCCESS;

    /* dir に従い、処理 */
    cmdln_no = ntohl(recv_m->frame.cmdio.cmdln_no);

    switch (recv_m->frame.cmdio.dir) {
    case (UDRSOCK2_DIR_WRITE):
	pdata = (char *) recv_m->frame.cmdio.data;
	char_len = ntohl(recv_m->frame.cmdio.send_len);

	/* コマンドタスクを呼び出す */
	result = mtab->udrs_funcs.dwrite(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_WRITE : %s", pdata);
	fflush(stdout);
#endif
	/* リプライフレーム作成 */
	reply_m->hdr.result_code = htonl(result);
	reply_m->hdr.frame_type = UDRSOCK2_NULL_FRAME;
	reply_m->hdr.frame_len  = 0;
	reply_m->frame.cmdio.cmdln_no = htonl(cmdln_no);
	break;

    case (UDRSOCK2_DIR_READ):
	reply_m->hdr.frame_type = UDRSOCK2_CMDIO_FRAME;
	pdata = (char *) reply_m->frame.cmdio.data;
	reply_len_max = ntohl(recv_m->frame.cmdio.reply_len_max);
	reply_m->frame.cmdio.cmdln_no = htonl(cmdln_no);
	reply_m->hdr.frame_len = htonl(sizeof(udrsock2_cmdio_frame_t));

	/* コマンドタスクを呼び出す */
	result = mtab->udrs_funcs.dread(pdata, reply_len_max);

#if 0
	DBMS5(stdout, "DIR_READ : %s", pdata);
	fflush(stdout);
#endif
	/* リプライフレーム作成 */
	reply_m->hdr.result_code = htonl(result);

	break;

    case (UDRSOCK2_DIR_RW):
	reply_m->hdr.frame_type = UDRSOCK2_CMDIO_FRAME;

	/* コマンドライト処理 */
	pdata = (char *) recv_m->frame.cmdio.data;
	char_len = ntohl(recv_m->frame.cmdio.send_len);

	/* コマンドタスクを呼び出す */
	DBMS5(stdout, "server_cmdio_req(UDRSOCK2_DIR_RW) : dwrite ='%s'\n", pdata);
	result = mtab->udrs_funcs.dwrite(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_RW(write) %d : %s", char_len, pdata);
	fflush(stdout);
#endif
	/* コマンドリード初期化 */
	pdata = (char *) reply_m->frame.cmdio.data;
	reply_len_max = ntohl(recv_m->frame.cmdio.reply_len_max);

	/* コマンドタスクを呼び出す */
	result = mtab->udrs_funcs.dread(pdata, reply_len_max);
	DBMS5(stdout, "server_cmdio_req(UDRSOCK2_DIR_RW) : dread ='%s'\n", pdata);

#if 0
	DBMS5(stdout, "DIR_RW(read) %d : %s", char_len, pdata);
	fflush(stdout);
#endif
	/* リプライフレーム作成 */
	reply_m->hdr.frame_len = htonl( (u_long)(sizeof(udrsock2_cmdio_frame_t) - UDRSOCK2_CMDIO_DSIZE + strlen(pdata)));
	reply_m->frame.cmdio.cmdln_no = htonl(cmdln_no);
	reply_m->hdr.result_code = htonl(result);
	break;

    default:
	DBMS1(stderr,
	      "server_cmdio_req : unknown direction request = %d\n",
	      recv_m->frame.cmdio.dir);
	reply_m->hdr.reply_type = UDRSOCK2_REPLY_ERROR;
	memcpy(reply_m, recv_m, UDRSOCK2_GET_FRAME_SIZE(recv_m));
	break;
    }

    /* クライアントにレスポンスを返す */
    server_msg_reply(t);

    return;
}

static void server_ioctl_req(child_udrsock2_server_table_t *t)
{
/*
	int udrsock2_ioctl_req_s(udrsock2_msg_t *recv_m)
	概要：
	ioctl コマンドを実行します。
	引数
		udrsock2_msg_t *recv_m
*/

    udrsock2_msg_t *recv_m = t->recv_buf;
    udrsock2_msg_t *reply_m = t->reply_buf;
    int char_len;
    int cmd;
    char *pdata;
    int result;
    const master_server_table_t *mtab = (master_server_table_t*)t->mtab;

    DBMS5(stdout, "server_ioctl_req : execute\n");

    /* ヘッダコピー */
    memcpy(reply_m, recv_m, UDRSOCK2_HDR_SIZE);
    /* リプライメッセージ ヘッダ作成 */
    reply_m->hdr.msg_type = UDRSOCK2_REPLY_MSG;
    reply_m->hdr.reply_type = UDRSOCK2_REPLY_SUCCESS;

    cmd = ntohl(recv_m->frame.ioctl.cmd);
    pdata = (char *) recv_m->frame.ioctl.data;
    char_len = ntohl(recv_m->frame.ioctl.size);

    /* コマンドタスクを呼び出す */
    result = mtab->udrs_funcs.ioctl(cmd, pdata, &char_len);

    /* リプライフレーム作成 */
    reply_m->hdr.result_code = htonl(result);
    reply_m->frame.ioctl.size = htonl(char_len);
    memcpy(reply_m->frame.ioctl.data, pdata, char_len);

    /* クライアントにレスポンスを返す */
    server_msg_reply(t);
    return;
}

static void server_memio_req(child_udrsock2_server_table_t *t)
{
    udrsock2_msg_t *recv_m = (udrsock2_msg_t*)t->recv_buf;

    DBMS5(stdout, "server_memio_req : execute \n");

    switch (recv_m->frame.memio.dir) {
    case (UDRSOCK2_DIR_READ):
	DBMS5(stdout, "server_memio_req : dir type read\n");
	server_memio_read_req(t);
	break;
    case (UDRSOCK2_DIR_WRITE):
	DBMS5(stdout, "server_memio_req : dir type write\n");
	server_memio_write_req(t);
	break;
    default:
	DBMS1(stdout, "server_memio_req : unknown direction type \n");
	DBMS1(stdout, "server_memio_req : type = %d\n",
	      recv_m->frame.memio.dir);
    }
    return;
}

static int server_memio_write_req(child_udrsock2_server_table_t *t)
{
/*
	udrsock2_memio_write_req_s(udrsock2_msg_t *recv_m)
	概要：
	 メモリ書き込みメッセージを受信したときに呼ばれ、データ書き込み処理
	 を行います。
	引数：
		udrsock2_msg_t *recv_m : 受信したメッセージ
		void   *memb_data : ブロック転送データポインタ
	返り値：
		成功 ： TRUE
		失敗 : FALSE
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/
    udrsock2_msg_t * recv_m = t->recv_buf;
    int memb_type;
    int32_t memb_seg;
    int32_t memb_offs;
    int32_t data_len;
    int num_trans_loops;
    int left_len;
    int pac_size;
    int result;

#ifdef OLD_PROTOCOL
    /* ライトデータ 要求承認 リプライ */
    if (udrsock2_memio_trans_start_req_s(t, UDRSOCK2_NO_ERR)
	== UDRSOCK2_ERR_FAIL)
	return UDRSOCK2_ERR_FAIL;
#endif

    /* 転送 */
    memb_type = ntohl(recv_m->frame.memio.mem_type);
    memb_seg = ntohl(recv_m->frame.memio.mem_segs);
    memb_offs = ntohl(recv_m->frame.memio.mem_offs);
    data_len = ntohl(recv_m->frame.memio.data_len);

    num_trans_loops = ntohl(recv_m->frame.memio.num_loops);
    left_len = ntohl(recv_m->frame.memio.left_len);
    pac_size = ntohl(recv_m->frame.memio.memiob_len);

    DBMS4(stdout, "server_memio_write_req : num_trans_loops = %d\n",
	  num_trans_loops);
    DBMS4(stdout, "server_memio_write_req : left_len = %d\n",
	  left_len);
    DBMS4(stdout, "server_memio_write_req : data_len = %d\n",
	  data_len);

#ifdef MULTIPAC_SOCKETREAD
    result =
	server_memio_multipac(t, UDRSOCK2_DIR_WRITE, memb_offs,
			      num_trans_loops, left_len, pac_size);
#else
    result = server_memio_write_1pac(t, memb_offs, data_len);
#endif


    /* 転送終了 */
    if (result == UDRSOCK2_NO_ERR) {
	result = server_memio_trans_end_req(t, UDRSOCK2_DIR_WRITE,
					       memb_type, NULL, memb_seg,
					       memb_offs, data_len);
    }
    return result;
}

static int server_memio_read_req(child_udrsock2_server_table_t *t)
{
/*
	udrsock2_memio_read_req_s(udrsock2_msg_t *recv_m)
	概要：
	 メモリ書き込みメッセージを受信したときに呼ばれ、データ書き込み処理
	 を行います。
	引数：
		udrsock2_msg_t *recv_m : 受信したメッセージ
		void   *memb_data : ブロック転送データポインタ
	返り値：
		成功 ： TRUE
		失敗 : FALSE
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/
    udrsock2_msg_t *recv_m = t->recv_buf;
    int memb_type;
    int32_t memb_seg;
    int32_t memb_offs;
    int32_t data_len;
    int num_trans_loops;
    int left_len;
    int pac_size;
    int result;

    DBMS5(stdout, "server_memio_read_req : execute \n");

    /* 初期化 */
    memb_type = ntohl(recv_m->frame.memio.mem_type);
    memb_seg = ntohl(recv_m->frame.memio.mem_segs);
    memb_offs = ntohl(recv_m->frame.memio.mem_offs);
    data_len = ntohl(recv_m->frame.memio.data_len);

    num_trans_loops = ntohl(recv_m->frame.memio.num_loops);
    left_len = ntohl(recv_m->frame.memio.left_len);
    pac_size = ntohl(recv_m->frame.memio.memiob_len);

#ifdef OLD_PROTOCOL
    /* ライトデータ 要求承認 リプライ */
    if (server_memio_trans_start_req(t, 0) == UDRSOCK2_ERR_FAIL) {
	return UDRSOCK2_ERR_FAIL;
    }
#endif

    /* 転送実行 */
    result = server_memio_multipac( t, UDRSOCK2_DIR_READ,
				   memb_offs, num_trans_loops, left_len,
				   pac_size);

    /* 転送終了 */
    if (result == UDRSOCK2_NO_ERR) {
	result = server_memio_trans_end_req( t, UDRSOCK2_DIR_READ,
					       memb_type, NULL, memb_seg,
					       memb_offs, data_len);
    }

    return result;
}

static int server_memio_write_1pac(child_udrsock2_server_table_t *t, const int offs, const int size)
{
/*
  @fn static int server_memio_write_1pac_s(child_udrsock2_server_table_t *t, const int offs, const int size)
	概要：
	 imem_write実行の為、ブロックデータを受信します。
	 レスポンスを返すのは、転送時間を延ばすためにやめました。(^^;)
	 最大転送サイズは UDRSOCK2_MEMIO_BLOCK_SIZE (Byte)です。
	引数：
		char *memb_data : サーバ側のバッファポインタ
		int size : レシーブメッセージサイず
	返り値：
		UDRSOCK2_ERR_CONNECT : 失敗
		UDRSOCK2_NO_ERR : 成功
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/

    char *p = NULL;
    int result;
    const master_server_table_t *mtab = (master_server_table_t*)t->mtab;

    DBMS5(stdout, "server_memio_write_1pac : execute \n");

    if( NULL == mtab->udrs_funcs.imemio_get_ptr ) {
	DBMS1( stdout, "server_memio_write_1pac : mtab->udrs_funcs.imemio_get_ptr function is not attached\n");
	return UDRSOCK2_ERR_FAIL;
    }
    p = (char*)mtab->udrs_funcs.imemio_get_ptr(offs);
    if( NULL == p ) {
	DBMS1( stdout, "server_memio_write_1pac : mtab->udrs_funcs.imemio_get_ptr is returned NULL\n");
    	return UDRSOCK2_ERR_FAIL;
    }

    result = socket_readstream( t->sockfd, p, size);
    if (result != size ) {
	return UDRSOCK2_ERR_FAIL;
    }

#ifdef USE_CPU_HINT
    /* CPUキャッシュの破棄 */
    multios_simd_cpu_hint_fence_store_to_memory();
    multios_simd_cpu_hint_invalidates_level_cache( p, size, notcall_memory_fence);
#endif

    return UDRSOCK2_NO_ERR;
}

static int server_memio_read_1pac(child_udrsock2_server_table_t *t, const int offs, const int memb_size)
{
/*
	char *memb_data(char *memb_data,int size)
	概要：
	 imem_read 実行の為、ブロックデータを送信します。
	 レスポンスを返すのは、転送時間を延ばすためにやめました。(^^;)
	 最大転送サイズは udrsock2_memiob_size (Byte)です。
	引数：
		char *memb_data : サーバ側バッファのポインタ

	返り値：
		UDRSOCK2_NO_ERR : 成功
		UDRSOCK2_ERR_CONNECT : 失敗

	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/

    int result;
    char *d = NULL;
    const master_server_table_t *mtab = (master_server_table_t*)t->mtab;

    DBMS5(stdout, "server_memio_read_1pac : execute \n");

    /* ライトデータ用意 */
    if( NULL == mtab->udrs_funcs.imemio_get_ptr ) {
	DBMS1( stdout, "server_memio_read_1pac : mtab->udrs_funcs.imemio_get_ptr function is not attached\n");
	return UDRSOCK2_ERR_FAIL;
    }

    d = (char*)mtab->udrs_funcs.imemio_get_ptr(offs);
    if( NULL == d ) {
	DBMS1( stdout, "server_memio_read_1pac : mtab->udrs_funcs.imemio_get_ptr is returned NULL\n");
    	return UDRSOCK2_ERR_FAIL;
    }

    DBMS5( stdout, "server_memio_read_1pac : ptr=0x%p size=%d\n", d, memb_size);

#ifdef USE_CPU_HINT
    multios_simd_cpu_hint_prefetch_line(d, memb_size, MULTIOS_MM_HINT_T2);
#endif

    /* データ送信 */
    result = socket_writestream( t->sockfd, d, memb_size);
    if (result == -1 || result != memb_size) {
	return UDRSOCK2_ERR_FAIL;
    }

#ifdef USE_CPU_HINT
    /* キャッシュの破棄 */
    multios_simd_cpu_hint_fence_store_to_memory();
    multios_simd_cpu_hint_invalidates_level_cache( d, memb_size, notcall_memory_fence);
#endif

    return UDRSOCK2_NO_ERR;
}

static int
server_memio_multipac(child_udrsock2_server_table_t *t, const char trans_dir, int offs, int trans_loop_count,
		      const int left_size, const int data_size)
{
/*
static int
server_memio_multipac(child_udrsock2_server_table_t *t, const char trans_dir, int offs, int trans_loop_count,
		      const int left_size, const int data_size)

	概要：
	 メモリブロックデータを分割して送信します

	引数：
		char *memb_data : 転送データのポインタ
		int trans_loop_count : 転送ループカウント
		int left_size ： 余り転送サイズ
		int data_size ： 通常転送サイズ
	返り値：
		UDRSOCK2_ERR_CONNECT : ソケット接続 不具合
		UDRSOCK2_ERR_FAIL  : 失敗
	 	UDRSOCK2_MEMIO_END : 成功
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/

    int result, status;
    int io_len = data_size;
    int f_nagle;

    DBMS5(stdout, "server_memio_multipac : execute \n");

    /* 転送 */

#ifdef OLD_PROTOCOL
    if (trans_dir == UDRSOCK2_DIR_READ) {
	result = udrsock2_memio_read_trans_send_req_s(t);
	if (result != UDRSOCK2_NO_ERR) {
	    DBMS1(stderr, "server_memio_multipac : start req err\n");
	    return UDRSOCK2_ERR_FAIL;
	}
	multios_socket_set_nagle( t->sockfd, 1 );
    }
#else
    if (trans_dir == UDRSOCK2_DIR_READ) {
	multios_socket_set_nagle( t->sockfd, 1 );
	f_nagle = 1;
    } else {
	multios_socket_set_nagle( t->sockfd, 0 );
	f_nagle = 0;
    }
#endif
    while (1) {
	DBMS5(stdout, "server_memio_multipac : trans_loop_count =%d\n",
	      trans_loop_count);
	if ((trans_loop_count == 1) && (left_size > 0)) {
	    io_len = left_size;
	}

	if( (trans_loop_count <= 2) && (f_nagle ) ) {
	    multios_socket_set_nagle( t->sockfd, 0 );
	    f_nagle = 0;
	}

	switch (trans_dir) {
	case (UDRSOCK2_DIR_READ):
	    result = server_memio_read_1pac( t, offs, io_len);
	    break;
	case (UDRSOCK2_DIR_WRITE):
	    result = server_memio_write_1pac(t, offs, io_len);
	    break;
	default:
	    status = UDRSOCK2_ERR_FAIL;
	    goto out;
	}

	switch (result) {
	case (UDRSOCK2_NO_ERR):
	    break;
	case (UDRSOCK2_ERR_FAIL):
	    DBMS5(stdout, "server_memio_multipac : 1pac transfer(dir=%d) fail\n", trans_dir);
	    status = UDRSOCK2_ERR_FAIL;
	    goto out;
	default:
	    DBMS1(stdout,
		  "server_memio_multipac : unknown result code = %d\n",
		  result);
	    status = UDRSOCK2_ERR_FAIL;
	    goto out;
	}

#ifdef TCP_ACK_DRIVE
	if( trans_dir == UDRSOCK2_DIR_WRITE ) {
	    int len;
	    /* ACK促進のためのパディング送信 */
	    len =  udrsock2_writestream( t->sockfd, "S", 1);
	    if( len != 1 ) {
		DBMS1(stderr, "server_memio_multipac : writestream(pad) fail\n");
		status = UDRSOCK2_ERR_FAIL;
		goto out;
	    }
	}
#endif

	if (trans_loop_count-- == 1) {
	    DBMS5(stdout,
		  "server_memio_multipac : transfer end,loop out\n");
	    status = UDRSOCK2_NO_ERR;
	    break;
	}

	DBMS5(stdout, "server_memio_multipac : transfer pointer move\n");
	offs += data_size;
    }

#ifdef TCP_ACK_DRIVE
    if(1) {
	if( trans_dir == UDRSOCK2_DIR_READ ) {
	    int len;
	    /* ACK促進のためのパディング回収 */
	    len = socket_readstream( t->sockfd, t->recv_buf, num_loops);
	    if( len != num_loops ) {
		DBMS1(stderr, "server_memio_multipac : readstream(pad) fail\n");
		status = UDRSOCK2_ERR_FAIL;
		goto out;
	    }
	}
    }
#endif

out:
    multios_socket_set_nagle( t->sockfd, 0 );

    return status;
}

#ifdef __GNUC__
static int __attribute__((unused))
#else
static int
#endif
udrsock2_memio_read_trans_send_req_s(child_udrsock2_server_table_t *t)
{

    udrsock2_msg_t *recv_m = (udrsock2_msg_t *)t->recv_buf;
    int result;

    /* リクエストデータ リード */
    DBMS5(stdout,
	  "udrsock2_memio_read_trans_send_req_s : recv_m pointer= 0x%p\n",
	  recv_m);
    result = server_msg_recv(t);

    if (recv_m->hdr.request_type != UDRSOCK2_MEMIO_REQ) {
	DBMS1(stderr, " request fail id = %d\n", recv_m->hdr.request_type);
	return UDRSOCK2_ERR_FAIL;
    }

    return UDRSOCK2_NO_ERR;
}

static int
server_memio_trans_start_req(child_udrsock2_server_table_t *t, int flag)
{
/*
	int udrsock2_memio_trans_start_req_s(udrsock2_msg_t *recv_m,int flag)
	概要：
	 バイナリブロック転送要求を実行可能であれば実行を許可します。
	 クライアントにはリプライを返します。

	引数：
		udrsock2_msg_t *recv_m
	返り値：
		-1 : 不許可
		 0 : 許可
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/

    udrsock2_msg_t *recv_m = (udrsock2_msg_t*)t->recv_buf;
    udrsock2_msg_t *reply_m = (udrsock2_msg_t*)t->reply_buf;
    int err = 0;

    DBMS5(stdout, "udrsock2_memio_trans_start_req_s : execute \n");

    message_buffer_zero_clear(reply_m);

/* 問題がなければ 転送許可 */
    /* フラグ確認 */


    /* init */
    memcpy(reply_m, recv_m, UDRSOCK2_HDR_SIZE);
    reply_m->hdr.msg_type = UDRSOCK2_REPLY_MSG;
    reply_m->hdr.reply_type = UDRSOCK2_REPLY_SUCCESS;
    reply_m->hdr.result_code = htonl(flag);
    reply_m->hdr.frame_type = UDRSOCK2_MEMIO_FRAME;
    reply_m->hdr.frame_len  = htonl(sizeof(udrsock2_memio_frame_t));
    memcpy(&reply_m->frame.memio, &recv_m->frame.memio, sizeof(udrsock2_memio_frame_t));

    server_msg_reply(t);

    return err;
}

static int
server_memio_trans_end_req(child_udrsock2_server_table_t *t, const char trans_dir, const int memb, const char *pmemb_buff,
			      const int32_t memb_seg, const int32_t memb_offs,
			      const int32_t memb_size)
{
/*
	udrsock2_memio_end_req_s()
	概要：
	 バイナリブロック転送終了要求を処理します。
	 クライアントにはリプライを返します。
	引数：
		int reply_flag : 転送終了 0
						-1 失敗
	返り値：
		-1 : 失敗
		 0 : 成功
	備考：
		ioctrlで、転送対象のメモリウインドウを設定しておく必要がある。
*/

    udrsock2_msg_t *recv_m = (udrsock2_msg_t *)t->recv_buf;
    udrsock2_msg_t *reply_m = (udrsock2_msg_t *)t->reply_buf;
    int err = UDRSOCK2_NO_ERR;
    int result;


    DBMS5(stdout, "server_memio_trans_end_req : execute \n");
    DBMS4(stdout, "server_memio_trans_end_req : memb_offs = %d\n",
	  memb_offs);
    DBMS4(stdout, "server_memio_trans_end_req : memb_size = %d\n",
	  memb_size);

    result = server_msg_recv(t);
    if (result != UDRSOCK2_NO_ERR)
	return result;

    /* レスポンスメッセージ作成 */
    memcpy(reply_m, recv_m, UDRSOCK2_GET_MSG_SIZE(recv_m));
    reply_m->hdr.msg_type = UDRSOCK2_REPLY_MSG;
    reply_m->hdr.frame_type = UDRSOCK2_NULL_FRAME;
    reply_m->hdr.frame_len = htonl(0);

    if (recv_m->hdr.request_type == UDRSOCK2_MEMIO_END_REQ) {
	reply_m->hdr.reply_type = UDRSOCK2_REPLY_SUCCESS;
        reply_m->hdr.result_code = htonl(0);
	err = UDRSOCK2_NO_ERR;
    } else {
	reply_m->hdr.reply_type = UDRSOCK2_REPLY_ERROR;
	err = UDRSOCK2_ERR_FAIL;
    }

    server_msg_reply(t);

    return err;
}

static void
udrsock2_dump_msg(udrsock2_msg_t * msg)
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

    fprintf(stdout, "udrsock2_dump_msg : socket message infomation \n");
    fprintf(stdout, " messgae header part \n");
    fprintf(stdout, " hdr.magic_no = 0x%08x \n", ntohl(msg->hdr.magic_no));
    fprintf(stdout, " hdr.msg_type = %d \n", msg->hdr.msg_type);
    fprintf(stdout, " hdr.request_type = %d \n", msg->hdr.request_type);
    fprintf(stdout, " hdr.reply_type = %d \n", msg->hdr.reply_type);
    fprintf(stdout, " hdr.result_code = %d \n", ntohl(msg->hdr.result_code));
    fprintf(stdout, " hdr.frame_type = %d \n", msg->hdr.frame_type);
    fprintf(stdout, " hdr.frame_len = %d \n", ntohl(msg->hdr.frame_len));

    switch (msg->hdr.frame_type) {
    case (UDRSOCK2_NULL_FRAME):
	fprintf(stdout, " frame type = NULL_FRAME \n");
	break;

    case (UDRSOCK2_CMDIO_FRAME):
	fprintf(stdout, " frame type = CMD_FRAME \n");
	fprintf(stdout, " cmdio.send_len = %d\n",
		ntohl(msg->frame.cmdio.send_len));

	fprintf(stdout, " cmdio.data = %s\n", msg->frame.cmdio.data);
	break;

    case (UDRSOCK2_IOCTL_FRAME):
	fprintf(stdout, " frame type = IOCTL_FRAME \n");
	fprintf(stdout, " ioctl.size = %d\n",
		ntohl(msg->frame.ioctl.size));
	fprintf(stdout, " ioctl.data = %s\n", msg->frame.ioctl.data);
	break;
#if 0
    case (UDRSOCK2_MEMIO_FRAME):
	fprintf(stdout, " frame type = MEM_FRAME \n");
	fprintf(stdout, " memio.dir = %d\n", msg->frame.memio.dir);
	fprintf(stdout, " memio.data_len = %d\n",
		ntohl(msg->frame.memio.data_len));
	fprintf(stdout, " memio.mem_segs = %x\n",
		ntohl(msg->frame.memio.mem_segs));
	fprintf(stdout, " memio.mem_offs = %d\n",
		ntohl(msg->frame.memio.mem_offs));
	fprintf(stdout, " memio.num_loops = %d\n",
		ntohl(msg->frame.memio.num_loops));
	fprintf(stdout, " memio.memiob_len = %d\n",
		ntohl(msg->frame.memio.memiob_len));
	fprintf(stdout, " memio.left_len = %d\n",
		ntohl(msg->frame.memio.left_len));
	break;
#endif
    }

    return;
}

void udrsock2_server_close(void)
{
    DBMS4(stdout, "udrsock2_server_close : execute\n");

#if 0
    if (master_sockfd >= 0) {
            shutdown(master_sockfd, 2);
            multios_msleep(100);
            close(master_sockfd);
     }
#endif

    return;
}

/**
 * @fn static void server_initial_negotiation_req(child_udrsock2_server_table_t *t)
 * @brief Client-Serverでの初期設定をします。
 *        MEMIO用のSocketセッション数を受け入れた後にサーバをスレッドで起動します
 */
static void server_initial_negotiation_req(child_udrsock2_server_table_t *t)
{
    udrsock2_msg_t *recv_m = t->recv_buf;
    udrsock2_msg_t *reply_m = t->reply_buf;
    int result, status;
    int subio_num_sessions;
    int subio_socket_port_no;
    master_server_table_t *mtab = (master_server_table_t*)t->mtab;
    subio_port_allocation_t *subio_port_tab = &mtab->subio_port_tab[mtab->subio_port_tbl_no];

    DBMS5(stdout, "server_initial_negotiation_req : execute\n");

    /* ヘッダコピー */
    memcpy(reply_m, recv_m, UDRSOCK2_HDR_SIZE);
    /* リプライメッセージ ヘッダ作成 */
    reply_m->hdr.msg_type = UDRSOCK2_REPLY_MSG;
    reply_m->hdr.reply_type = UDRSOCK2_REPLY_SUCCESS;

    /* サーバが既に起動している場合は以前のパラメータを返します */
    if(mtab->init.f.subio_pt) {
	DBMS3(stderr, "server_initial_negotiation_req : already create subio listen server\n");
	subio_num_sessions = mtab->subio_num_sessions;
	subio_socket_port_no = mtab->master_sock_port + subio_port_tab->no;;
    } else {
	/* パラメータ取得 */
	subio_num_sessions = (int)(recv_m->frame.init.subio_num_sessions); /* char to int */
	DBMS3(stderr, "server_initial_negotiation_req : subio_num_sessions = %d\n", subio_num_sessions);

	/* Sokcetポート決定 */
	subio_socket_port_no = mtab->master_sock_port + subio_port_tab->no;
	if( !(subio_num_sessions < UDRSOCK2_SUBIO_MAX_SESSIONS ) ) {
	    subio_num_sessions = UDRSOCK2_SUBIO_MAX_SESSIONS;
	}

	mtab->subio_num_sessions = subio_num_sessions;

	/* subio server リスナー起動確認用synccmdオブジェクト初期化  */
	if(subio_num_sessions>0) {
	    int accmd;

	    /* subioコネクションサーバーをスレッドで起動します。 */
	    DBMS3( stderr, "server_initial_negotiation_req : multios_pthread_create(subio_listen_server_thread)\n");
	    result = multios_pthread_create( &mtab->subio_pt, NULL, subio_listen_server_thread, (void*)mtab);
	    if(result) {
		DBMS1( stderr, "server_initial_negotiation_req : multios_pthread_create(subio_listen_server_thread) fail\n");
		subio_socket_port_no = 0;
		status = -1;
	    } else {
		DBMS4(stderr, "server_initial_negotiation_req : multios_pthread_create(subio_listen_server_thread) succeed\n");
		multios_pthread_detach( &mtab->subio_pt );
		mtab->init.f.subio_pt = 1;


		DBMS3(stderr, "server_initial_negotiation_req : check subio_server listened\n");
		/*subioコネクションサーバーがListen状態に入ったことを確認します */
		result = multios_actor_th_simple_mpi_recv_request( &mtab->subio_ac, &accmd, MULTIOS_ACTOR_TH_SIMPLE_MPI_BLOCK);
		if(result) {
		    DBMS1( stderr, "server_initial_negotiation_req : multios_actor_th_simple_mpi_recv_request fail\n");
		    subio_socket_port_no = 0;
		    status = -1;
		} else {
		    if( accmd != ACK_REQUEST ) {
			DBMS1( stderr, "server_initial_negotiation_req : multios_actor_th_simple_mpi_recv_request is not ACK_REQUEST\n");
			subio_socket_port_no = 0;
		    } else {
			multios_actor_th_simple_mpi_reply( &mtab->subio_ac, MULTIOS_ACTOR_TH_SIMPLE_MPI_RES_ACK);
			DBMS3(stderr, "confirmed subioserver is created, port no = %d\n", subio_socket_port_no);
			/* すべて完了 */
			status = 0;
		    }
		}
	    }
	}
    }

    /* リプライフレーム作成 */
    reply_m->hdr.result_code = htonl( result  ? -1 : 0);
    reply_m->frame.init.subio_num_sessions = (uint8_t)(mtab->subio_num_sessions);
    reply_m->frame.init.subio_socket_port_no = htonl(subio_socket_port_no);

#if 0
    udrsock2_dump_msg(reply_m);
#endif

    DBMS3( stderr, "server_initial_negotiation_req : server_msg_reply\n");
    /* クライアントにレスポンスを返す */
    server_msg_reply(t);

    if(status) {
	/* 強制的に通信をシャットダウンします */
#if defined(QNX)
	raise(SIGPIPE);
#endif
    }

    return;
}
