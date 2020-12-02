/**
 *      Copyright 2004 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2004-August-18 Active
 *              Last Alteration $Author: takeda $
 **/

/**
 * @file ips_socket.c
 * @brief Socket の ＯＳ依存を吸収しBSDソケットAPIに似せるためのラッパー及び、有用なAPI
 */

#include <stdint.h>

#if defined(WIN32)
/* MS-Windows */

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
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <time.h>

#if !defined(__MINGW32__)
#pragma comment (lib,"Ws2_32.lib")
#endif

#else

/* UNIX and likes std */
#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#if defined(SunOS)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/filio.h>
#endif

/* Socket */
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#if defined(QNX)
#include <setjmp.h>
#ifdef UDR
#include <udr/logger.h>
#endif

/* QNX6 */
#ifndef x86pc
#include <unix.h>
#include <sys/ioctl.h>
#if ! __QNXNTO__
#include <i86.h>
#endif
#else
#include <sys/ioctl.h>
#endif

#include <netdb.h>
#include <net/if.h>
#endif

#if defined(IRIX)
/* IRIX */
#include <sys/filio.h>
#endif				/* IRIX */

#if defined(Linux)
#include <setjmp.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <net/if.h>
#endif

#if defined(Darwin)
#include <setjmp.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netdb.h>
#endif

#include <netinet/tcp.h>
#include <arpa/inet.h>

#endif

static int socket_init_flag = 0;

typedef struct timeval timeval_t;

/* libs3 */
#include <core/libmpxx/_stdarg_fmt.h>
#include <core/libmpxx/mpxx_sys_types.h>
#include <core/libmpxx/mpxx_stdlib.h>
#include <core/libmpxx/mpxx_unistd.h>

#ifdef DEBUG
static int debuglevel = 5;
#else
static const int debuglevel = 0;
#endif

#include <core/libmpxx/dbms.h>


#if defined(WIN32)
#include <core/libmtxx/mtxx_pthread_once.h>
#endif

#include "ips_socket.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

#if defined(Linux) || defined(Darwin) || defined(QNX)
#define _USE_BSDSOCKET_TRAP
#elif defined(WIN32)
/**
 * BSD SOCKETの実装と異なり、トラップ処理が必要のない物はこちら
 */
#else
#error `need to use the trap ?  Please to determine`
#endif

#ifdef _USE_BSDSOCKET_TRAP
typedef void (*sighandler_t) (int);
static int sigpipe_flag = 0;
#define JMP_BUF jmp_buf
#define SETJMP(buf) setjmp(buf)
#define LONGJMP(buf, num) longjmp(buf, num)

static JMP_BUF op_sjbuf;

/**
 * @fn static void client_connect_onintr_sigpipe(int sig)
 * @brief オープン処理時にサーバー側の無効な切断があった場合に
 *	オープンに失敗したことの処理をします
 *	※ Linux/MACOSX用です。
 */
static void client_connect_onintr_sigpipe(int sig)
{
    DBMS2(stderr, "client_connect_onintr_sigpipe [SIGPIPE:"FMT_LLD"]" EOL_CRLF,
	(long long)sig);
    sigpipe_flag = 1;

    LONGJMP(op_sjbuf, 0);
}
#endif

#if defined(WIN32)
static int winsockerr2errno(const int wsaError);
static WORD wVersionRequested = MAKEWORD(2, 4); // 動作確認をした最大バージョンを書く
static s3::mtxx_pthread_once_t win32_startup_once = MTXX_PTHREAD_ONCE_INIT;
static void win32_winsock_startup(void);
static WSADATA wsaData = { 0 };
/**
 * @fn static void own_win32_winsock_startup_once(void);
 * @brief WIN32でWSAStartup()をおまじないで呼び出す一度限りの関数
 *	設定するバージョンは winsock_support_version に保存されます
 *	自ら決定するOS側で決定されたバージョンは _win32_socket_get_wsadata()で取得可能です
 *	OS側で決定されたバージョンは _win32_socket_get_wsadata()で取得可能です
 **/
static void own_win32_winsock_startup_once(void)
{
    errno = 0;
    /* WINSOCK おまじない */
    if(WSAStartup(wVersionRequested, &wsaData)) {
	int WSAReterr = WSAGetLastError();
	errno = winsockerr2errno(WSAReterr);
	DBMS1( stderr, __func__ " : WSAStartup fail, strerror=%s\n", strerror(errno));
    }
}

/** 
 * void s3::_win32_socket_set_winsock_startup_version( const _win32_socket_winsock_version_t ver)
 * @brief own_win32_winsock_startup_once()実行時にリクエストするバージョンを設定します。
 *	すでに own_win32_winsock_startup_once()が実行されている場合は変更しません
 * @param _win32_socket_winsock_version_t構造体 メジャーバージョンとマイナーバージョンを設定して下さい
 **/
void s3::_win32_socket_set_winsock_startup_version( const s3::_win32_socket_winsock_version_t ver)
{
    if(win32_startup_once == MTXX_PTHREAD_ONCE_INIT) {
	wVersionRequested=MAKEWORD(ver.major, ver.minor);
    }

    return;
}

/**
 * @fn int s3::_win32_socket_get_winsock_wsadata( s3::_win32_socket_winsock_version_t *const ver_p)
 * @brief winsock startup で決定された値を返します
 * @param ver_p s3::_win32_socket_winsock_version_t構造体ポインタ
 * @retval EINVAL vet_p がNULLだった
 * @retval EAGAIN 初期化要求するバージョン
 * @retval 0 決定されたバージョン
 */
int s3::_win32_socket_get_winsock_wsadata( s3::_win32_socket_winsock_version_t *const ver_p)
{
    if( NULL == ver_p ) {
	return EINVAL;
    }
    if(win32_startup_once == MTXX_PTHREAD_ONCE_INIT) {
	s3::_win32_socket_winsock_version_t reqver = { (uint8_t)LOWORD(wVersionRequested), (uint8_t)HIWORD(wVersionRequested) };
	*ver_p = reqver;
	return EAGAIN;
    } else {
	s3::_win32_socket_winsock_version_t retver = { LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion) };
	if( NULL != ver_p ) {
	    *ver_p = retver;
	}
	return 0;
    }
}

/**
 * @fn int s3::_win32_socket_WSAStartup_once(void)
 * @brief s3::ips_socket()実行時に行われる WSAStartup() の実行を、それより先に行います。すでに実行済の場合は何も起きません
 * @retval 0 成功（すでに実行済の場合は、処理は入りません
 * @retval 0以外 mpxx_pthread_once()の戻り値を参照
 */
int s3::_win32_socket_WSAStartup_once(void)
{
    const int status = s3::mtxx_pthread_once(&win32_startup_once, own_win32_winsock_startup_once);
    if(status) {
	DBMS1( stderr, __func__ " : s3::mtxx_pthread_once(WSAStartup) fail, strerror=%s\n", strerror(status));
	return status;
    }
    return 0;
}
#endif /* end of WIN32 */

/**
 * @fn int s3::ips_socket(int socket_family, int socket_type, int protocol)
 * @brief 通信のための端点(endpoint)を作成する. socket()のラッパー
 * @param socket_family 通信を行うドメインを指定
 *	※WINSOCK/BSDsocket ともに同様の定義があるため、そちらを参照してください
 * @param socket_type 通信方式(semantics)を指定する。
 *	※WINSOCK/BSDsocket ともに同様の定義があるため、そちらを参照してください
 * @param protocol ソケットによって使用される固有のプロトコルを指定する。
 *	※WINSOCK/BSDsocket ともに同様の定義があるため、そちらを参照してください
 * @retval 0以上 ソケット識別子
 * @retval -1 失敗(errno変数を設定)
 * @par errnoコード
 *	- EACCES 指定されたタイプまたはプロトコルのソケットを作成する許可が与えられていない。 
 *	- EAFNOSUPPORT 指定されたアドレスファミリーがサポートされていない。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EINVAL 知らないプロトコル、または知らないプロトコルファイル
 *	- EMFILE プロセスのファイルテーブルがあふれている
 *	- ENFILE オープンされた識別子が上限に達している
 *	- ENOBUFS/ENOMEM 充分な資源がない
 *	- EPROTONOSUPPORT サポートされてないプロトコルタイプ
 *	- それ以外又は-1 その他のエラー
 *	-    ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket(const int socket_family, const int socket_type,
		   const int protocol)
{
    int fd;

#if defined(WIN32)
   if (1) {
	int result = s3::mtxx_pthread_once(&win32_startup_once, own_win32_winsock_startup_once);
	if(result) {
	    DBMS1( stderr, __func__ " : s3::mtxx_pthread_once(WSAStartup) fail, strerror=%s\n", strerror(result));
	    return -1;
	}
   }
#endif /* end of WIN32 */

    fd = (int) socket(socket_family, socket_type, protocol);

#ifdef WIN32
    if (fd == (int) INVALID_SOCKET) {
	int reterr = WSAGetLastError();
	errno = winsockerr2errno(reterr);
	return -1;
    }
#else
    if (fd < 0) {
	return -1;
    }
#endif

    return fd;
}

/**
 * @fn int s3::ips_socket_connect_withtimeout(const int fd, const struct sockaddr *serv_addr_p, const unsigned int addr_size, const int timeout_sec)
 * @brief SOCKETライブラリconnectのタイムアウト付きラッパ。
 * @param fd ソケット識別子
 * @param serv_addr_p 接続先指定struct sockaddr構造体ポインタ
 * @param addr_size struct sockaddr構造体サイズ
 * @param timeout_sec タイムアウト時間（秒）
 * @retval 0 成功
 * @retval -1 失敗(errnoを参照）
 * @par errnoコード Windowsの為に通常のconnect()とエラーコードが異なります。
 *	- EACCES サーバーに接続できなかった。サーバーに到達できなかったかサーバーとの接続に時間がかかった
 *	- EIO 内部処理バグなどにより、処理が中断された
 */
int s3::ips_socket_connect_withtimeout(const int fd,
				       const struct sockaddr *serv_addr_p,
				       const unsigned int addr_size,
				       const int timeout_sec)
{
    int result;
    time_t stime;
    timeval_t tv;
    fd_set fds;
#ifdef _USE_BSDSOCKET_TRAP
    socklen_t len;
    int eno;
    sighandler_t sigpipe_buf;
#endif

    IFDBG4THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	DBMS4(stderr, "s3::ips_socket_connect_withtimeout : execute" EOL_CRLF);
	mpxx_i64toadec( (int64_t)timeout_sec, xtoa_buf, xtoa_bufsz); 
	DBMS4(stderr,
	  "s3::ips_socket_connect_withtimeout : timeout_sec=%s" EOL_CRLF,
	      xtoa_buf);
    }

#ifdef _USE_BSDSOCKET_TRAP
    /**
     * @brief Linux/MACOSX/QNX6 ではサーバのポートが開いてない場合、無条件でパイプが切れてしまうので
     *	トラップしてエラーになるようにする
     */
    sigpipe_flag = 0;

    DBMS5(stderr, "socket SIGPIPE process start" EOL_CRLF);
    sigpipe_buf = signal(SIGPIPE, (void (*)()) client_connect_onintr_sigpipe);
    if (sigpipe_buf == SIG_ERR) {
	DBMS1(stderr,
	      "s3::ips_socket_connect_withtimeout : set fail signal" EOL_CRLF);
	errno = EIO;
	return -1;
    }

    SETJMP(op_sjbuf);

    if (sigpipe_flag) {
	errno = EACCES;
	return -1;
    }
#endif

    DBMS4(stderr,
	  "s3::ips_socket_connect_withtimeout : s3::ips_socket_nodelay()" EOL_CRLF);

    /* ノンブロッキングモードへ設定 */
    result = s3::ips_socket_nodelay(fd, 1);
    if (result) {
	DBMS1(stderr,
	      "s3::ips_socket_connect_withtimeout : s3::ips_socket_nodelay(on) fail" EOL_CRLF);
	return -1;
    }

    /* connect()を実行するが、この時の戻り値は参照しない */
    result = s3::ips_socket_connect(fd, serv_addr_p, addr_size);

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)result, xtoa_buf_a, xtoa_bufsz);
	mpxx_i64toadec( (int64_t)errno, xtoa_buf_b, xtoa_bufsz);
	DBMS3(stderr,
	  "s3::ips_socket_connect_withtimeout : resut=%s errno=%s :%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b, strerror(errno));
    }

    /* select()でタイムアウトしたら コネクト失敗 */
    memset(&tv, 0, sizeof(timeval_t));
    tv.tv_sec = timeout_sec + 1;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET((uint32_t) fd, &fds);

    IFDBG4THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
    
	mpxx_i64toadec( (int64_t)fd, xtoa_buf, xtoa_bufsz);
	DBMS4(stderr, "s3::ips_socket_connect_withtimeout : fd=%s" EOL_CRLF,
	    xtoa_buf);
    }

    stime = time(NULL);

#ifdef _USE_BSDSOCKET_TRAP
  bsdsocket_loop:
#endif

    result = select(FD_SETSIZE, &fds, &fds, (fd_set *) NULL, &tv);

    IFDBG4THEN {    
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
	time_t ntime = time(NULL) - stime;

	mpxx_i64toadec( (int64_t)result, xtoa_buf_a, xtoa_bufsz);
	mpxx_i64toadec( (int64_t)errno, xtoa_buf_b, xtoa_bufsz);
        DBMS4(stderr, "select result=%s errno=%s:%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b, strerror(errno));

	mpxx_i64toadec( (int64_t)timeout_sec, xtoa_buf_a, xtoa_bufsz);
	mpxx_i64toadec( (int64_t)ntime, xtoa_buf_b, xtoa_bufsz);
        DBMS4(stderr, "timeout=%s time=%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b);
    }

    if (result == 0) {
	/* タイムアウトした */
	DBMS5(stderr,
	      "s3::ips_socket_connect_withtimeout : select timeout" EOL_CRLF);
	errno = EACCES;
	return -1;
    } else if ((int) (time(NULL) - stime) >= timeout_sec) {
	/* selectは正常に終わったように見えるがタイムアウトしてる */
	DBMS5(stderr,
	      "s3::ips_socket_connect_withtimeout : connect timeout" EOL_CRLF);
	errno = EACCES;
	return -1;
    }
#ifdef _USE_BSDSOCKET_TRAP
    /* LInuxの場合、失敗してもタイムアウトしないことがある */
    len = sizeof(eno);
    result = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &eno, &len);
    DBMS3(stderr, "getsockopt result="FMT_LLD" errno="FMT_LLD":%s" EOL_CRLF, (long long)result, (long long)eno,
	  strerror(eno));
    switch (eno) {
    case 0:
	break;
    case EHOSTUNREACH:
	DBMS3(stderr,
	      "s3::ips_socket_connect_withtimeout : No route to host" EOL_CRLF);
	errno = EACCES;
	return -1;
    case ECONNREFUSED:
	DBMS3(stderr, "s3::ips_socket_connect_withtimeout : timeout(3)" EOL_CRLF);
	errno = EACCES;
	return -1;
    default:
	s3::ips_sleep(1);
	goto bsdsocket_loop;
    }
#endif

    /* ブロッキングモードに戻す */
    result = s3::ips_socket_nodelay(fd, 0);
    if (result) {
	DBMS1(stderr,
	      "s3::ips_socket_connect_withtimeout : s3::ips_socket_nodelay(off) fail" EOL_CRLF);
	return -1;
    }
#ifdef _USE_BSDSOCKET_TRAP
    /*************************
	トラップを解除する
    ***************************/
    {
	int sockbuf_stocksize = 0;
	ioctl(fd, FIONREAD, &sockbuf_stocksize);
	DBMS5(stderr, " stokdat=" FMT_LLD EOL_CRLF, (long long)sockbuf_stocksize);
    }

    sigpipe_buf = signal(SIGPIPE, (void (*)()) sigpipe_buf);
    if (sigpipe_buf == SIG_ERR) {
	DBMS1(stderr,
	      "s3::ips_socket_connect_withtimeout : set back fail signal" EOL_CRLF);
	errno = EACCES;
	return -1;
    }
    DBMS5(stderr, "SIGPIPE process fini" EOL_CRLF);
#endif

    return 0;
}

/**
 * @fn int s3::ips_socket_send_withtimeout(const int fd, const void *ptr, const size_t len, const int flags, const int timeout_sec)
 * @brief SOCKETライブラリsendのタイムアウト付きラッパ。Winsockをラッピングではエラーコードをerrnoに変換します
 * @param fd ソケット識別子
 * @param ptr 送信データバッファポインタ
 * @param len 送信データバイト数 
 * @param flags メッセージタイプフラグ MSG_????で定義
 * @param timeout_sec 送信可能状態までの待ち時間
 * @retval 0以上 送信バイト数
 * @retval -1 エラー(errnoにエラーコード設定)
 * @par errnoコード
 *	- EACCES ソケットへの書き込み許可がなかった。
 *	- EAGAIN ソケットが非停止に設定されており、 要求された操作が停止した。
 *      - EWOULDBLOCK (LINUXのみ)EAGAINと同値（ POSIX.1-2001 は、この場合にどちらのエラーを返すことも認めている)
 *	- EBADF 無効なディスクリプターが指定された。 
 *	- ECONNRESET 接続が接続相手によりリセットされた。 (MINGW及びVC++2005より前では定義がないため-1となる)
 *      - EDESTADDRREQ ソケットが接続型 (connection-mode) ではなく、 かつ送信先のアドレスが設定されていない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EFAULT ユーザー空間として不正なアドレスがパラメーターとして指定された。
 *	- EINTR データが送信される前に、シグナルが発生した。
 *	- EINVAL 不正な引き数が渡された。 
 *	- EISCONN 接続型ソケットの接続がすでに確立していたが、受信者が指定されていた。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EMSGSIZE そのソケット種別 ではソケットに渡されたままの形でメッセージを送信する必要があるが、 メッセージが大き過ぎるため送信することができない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOBUFS ネットワーク・インターフェースの出力キューが一杯である。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOMEM メモリが足りない。 
 *	- ENOTCONN ソケットが接続されておらず、接続先も指定されていない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOTSOCK 引き数 sockfd はソケットではない。 
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- EOPNOTSUPP 引き数 flags のいくつかのビットが、そのソケット種別では不適切なものである。 
 *	- EPIPE 接続指向のソケットでローカル側が閉じられている。
 *	- ETIMEOUT 送信可能状態にならずタイムアウトした。select()の部分
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- 上記以外また-1 その他のエラー
 *	-   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_send_withtimeout(const int fd, const void *ptr,
				    const size_t len, const int flags,
				    const int timeout_sec)
{
    if (timeout_sec) {
	int result;
	fd_set fds;
	timeval_t tv;

	FD_ZERO(&fds);
	FD_SET((u_int) fd, &fds);

	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	result =
	    select(FD_SETSIZE, (fd_set *) NULL, &fds, (fd_set *) NULL,
		   &tv);
	if (!result) {
	    /* タイムアウト */
#ifdef WIN32
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	    errno = ETIMEDOUT;
#else
	    errno = -1;
#endif
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
	    errno = ETIMEDOUT;
#else
#error 'not implemented select process in s3::ips_socket_send_withtimeout()'
#endif
	    DBMS1(stderr,
		  "s3::ips_socket_send_withtimeout : socket read timeout" EOL_CRLF);
	    return -1;
	}
    }
#ifdef WIN32
    {
	int retval;
	retval = send(fd, MPXX_STATIC_CAST(const char*,ptr), (int) len, flags);
	if (retval == SOCKET_ERROR) {
	    errno = winsockerr2errno(WSAGetLastError());
	    return -1;
	}
	return retval;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return send(fd, ptr, len, flags);
#else
#error 'not implemented send process in s3::ips_socket_send_withtimeout()'
#endif


}

/**
 * @fn int s3::ips_socket_recv_withtimeout(const int fd, void *ptr, const size_t len, const int flags, const int timeout_sec)
 * @brief SOCKETライブラリrecvのタイムアウト付きラッパ。Winsockをラッピングではエラーコードをerrnoに変換します
 * @param fd ソケット識別子
 * @param ptr 送信データバッファポインタ
 * @param len 送信データバイト数 
 * @param flags メッセージタイプフラグ MSG_????で定義
 * @param timeout_sec 受信パケット到着までの待ち時間
 * @retval 0以上 送信バイト数
 * @retval -1 エラー(errnoにエラーコード設定)
 * @par errnoコード
 *	- EAGAIN/EWOULDBLOCK ソケットが非停止 (nonblocking) に設定されていて 受信操作が停止するような状況になったか、 受信に時間切れ (timeout) が設定されていて データを受信する前に時間切れになった。
 *	- EBADF 引き数 sockfd が不正なディスクリプタである。 
 *	- ECONNREFUSED リモートのホストでネットワーク接続が拒否された 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EFAULT 受信バッファへのポインタがプロセスのアドレス空間外を指している。
 *	- EINTR データを受信する前に、シグナルが配送されて割り込まれた。
 *	- EINVAL 不正な引き数が渡された。 
 *	- ENOTCONN ソケットに接続指向プロトコルが割り当てられており、 まだ接続されていない 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOTSOCK 引き数 sockfd がソケットを参照していない。 
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- 上記以外また-1 その他のエラー
 *	-   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_recv_withtimeout(const int fd, void *ptr,
				    const size_t len, const int flags,
				    const int timeout_sec)
{
    int result;

    if (timeout_sec) {
	fd_set fds;
	timeval_t tv;

	FD_ZERO(&fds);
	FD_SET((uint32_t) fd, &fds);

	memset(&tv, 0, sizeof(tv));

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	result =
	    select(FD_SETSIZE, &fds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	if (!result) {
#ifdef WIN32
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	    errno = ETIMEDOUT;
#else
	    errno = -1;
#endif
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
	    errno = ETIMEDOUT;
#else
#error 'not implemented select process in s3::ips_socket_send_withtimeout()'
#endif

	    /* タイムアウト */
	    DBMS1(stderr,
		  "s3::ips_socket_recv_withtimeout : socket read timeout" EOL_CRLF);
	    return -1;
	}
    }
#ifdef WIN32
    {
	int retval;
	retval = recv(fd, MPXX_STATIC_CAST(char*,ptr), (int) len, flags);
	if (retval == SOCKET_ERROR) {
	    errno = winsockerr2errno(WSAGetLastError());
	    return -1;
	}
	return retval;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return recv(fd, ptr, len, flags);
#else
#error 'not implemented recv process in s3::ips_socket_send_withtimeout()'
#endif
}

/**
 * @fn int s3::ips_socket_client_open(const char *servername, const int port_no, const int timeout_sec, const int socket_family, const int socket_type, const int protocol, int (*func_setsockopt)(int fd, void*args), void *func_setsockopt_args)
 * @brief クライアント向けsocket接続テンプレート(IPV6にはまだ未対応)
 * @param servername サーバー名またはIPアドレス
 * @param port_no 接続ポート番号
 * @param timeout_sec 接続タイムアウト時間（秒）
 * @param socket_family プロトコルファミリ(sys/socket.hに定義されるAF_???/ 又はPF_???)
 * @param socket_type ソケットタイプ(SOCK_???で定義される値)
 * @param protocol プロトコル
 * @param func_setsockopt connect()実施前にSocketを初期化するためのコールバック関数ポインタまたはNULL
 *      fd : socketオープン時のfd値
 *      args :  func_setsockopt_argsで指定されたポインタ
 *	コールバック関数の戻り値は、以下のようにしてください。
 *	0      : 成功
 *      0以外  : 失敗 s3::ips_socket_client_open()の処理を中止して、-1を戻り値とします。
 * @param func_setsockopt_args func_setsockoptで指定された関数に渡すポインタ
 * @retval -1 失敗
 * @retval 0以上 ファイルディスクプリタ値
 *	 Unix系では低水準ファイルIOディスクプリタ,Windows系ではwinsockのディスクプリタ値
 */
int s3::ips_socket_client_open(const char *servername, const int port_no,
			       const int timeout_sec,
			       const int socket_family,
			       const int socket_type, const int protocol,
			       int (*func_setsockopt) (int fd, void *args),
			       void *func_setsockopt_args)
{
    int result, status;
    int fd;
    struct sockaddr_in serv_addr;

#if defined(WIN32) || defined(SunOS) || defined(Darwin) || defined(IRIX) || defined(QNX)
    struct in_addr in;
    struct hostent *he;
#elif defined(Linux) || defined(__AVM2__)
    struct addrinfo hints, *ai;
#else
#error 'not decide process s3::ips_socket_client_open()'
#endif

    DBMS3(stderr, "s3::ips_socket_client_open : connect to %s" EOL_CRLF,
	  servername);

    /* socket接続 */
    fd = s3::ips_socket(socket_family, socket_type, protocol);
    if (fd < 0) {
	DBMS3(stderr,
	      "s3::ips_socket_client_open : s3::ips_socket fail" EOL_CRLF);
	return fd;
    }
#if defined(WIN32) || defined(SunOS) || defined(Darwin) || defined(IRIX) || defined(QNX)
    /* old code */
    /* get host IP address */
    he = gethostbyname(servername);
    if (he == NULL) {
	status = -1;
	goto out;
    }
    memcpy(&in.s_addr, *(he->h_addr_list), sizeof(in.s_addr));

    /* connect server */
    memset((char *) &serv_addr, 0xff, sizeof(serv_addr));
    serv_addr.sin_family = socket_family;
    serv_addr.sin_addr.s_addr = inet_addr(inet_ntoa(in));
    serv_addr.sin_port = htons((short) port_no);

#elif defined(Linux) || defined(__AVM2__)

    DBMS5(stderr, "new process" EOL_CRLF);

    /* IPv6以降はこちらを使わなければならない・・・とおもう */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = socket_family;
    hints.ai_socktype = socket_type;

    result = getaddrinfo(servername, NULL, &hints, &ai);
    if (result) {
	DBMS1(stderr, "s3::ips_socket_client_open : unknown host %s" EOL_CRLF,
	      servername);
	status = -1;
	goto out;
    }

    if (ai->ai_addrlen > sizeof(serv_addr)) {
	DBMS1(stderr,
	      "s3::ips_socket_client_open : sockaddr too large ("FMT_LLU") > ("FMT_LLU")" EOL_CRLF,
	      (long long)ai->ai_addrlen, (long long)sizeof(serv_addr));
	status = -1;
	goto out;
    }

    memcpy(&serv_addr, ai->ai_addr, ai->ai_addrlen);
    serv_addr.sin_port = htons((short) port_no);
    serv_addr.sin_family = socket_family;

#else
#error 'not decide process s3::ips_socket_client_open()'
#endif

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_c[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_d[MPXX_XTOA_DEC64_BUFSZ];
	uint32_t ip[4];

	ip[0] = ((serv_addr.sin_addr.s_addr & 0xff000000) >> 24);
	ip[1] = ((serv_addr.sin_addr.s_addr & 0x00ff0000) >> 16);
	ip[2] = ((serv_addr.sin_addr.s_addr & 0x0000ff00) >>  8);
	ip[3] =  (serv_addr.sin_addr.s_addr & 0x000000ff);
	mpxx_u32toadec( ip[0], xtoa_buf_a, xtoa_bufsz);
	mpxx_u32toadec( ip[1], xtoa_buf_b, xtoa_bufsz);
	mpxx_u32toadec( ip[2], xtoa_buf_c, xtoa_bufsz);
	mpxx_u32toadec( ip[3], xtoa_buf_d, xtoa_bufsz);

        DBMS5(stderr, "serv_addr.sin_addr.s_addr=%s.%s.%s.%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b, xtoa_buf_c, xtoa_buf_d);
    }

    if (NULL != func_setsockopt) {
	result = func_setsockopt(fd, func_setsockopt_args);
	if (result) {
	    status = -1;
	    goto out;
	}
    }

    /* connect */
    result =
	s3::ips_socket_connect_withtimeout(fd,
					   (struct sockaddr *) &serv_addr,
					   sizeof(serv_addr), timeout_sec);
    IFDBG4THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)result, xtoa_buf_a, xtoa_bufsz);
	mpxx_i64toadec( (int64_t)errno,  xtoa_buf_b, xtoa_bufsz);
	DBMS4(stderr,
	  "s3::ips_socket_client_open : s3::ips_socket_connect result=%s, errno=%s:%s" EOL_CRLF,
		xtoa_buf_a, xtoa_buf_b, strerror(errno));
    }

    if (result < 0) {
	DBMS5(stderr,
	      "s3::ips_socket_client_open : connect_withtimeout fail" EOL_CRLF);
	status = -1;
	goto out;
    } else if (result == timeout_sec) {
	DBMS5(stderr,
	      "s3::ips_socket_client_open : connect_withtimeout detect connection timeout" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

  out:

    if (status) {
	    s3::ips_socket_close(fd);
	return -1;
    }
    return fd;
}

/**
 * @fn int s3::ips_socket_close(const int fd)
 * @brief 指定されたソケットをクローズします
 * @param fd ソケット識別子
 * @retval 0 成功
 * @retval EBADF 有効なソケット識別子でない
 * @retval EINTR 処理がシグナルにより中断された
 * @retval EIO I/Oエラーが発生した
 * @retval EINPROGRESS ノンブロッキング処理の為、処理中である(WindowsVC++2005以降のみ)
 *	この場合、どう処理をすればよいのか不明です。とりあえずクローズ処理なので、無視しても良いかと思います。
 * @retval 上記以外また-1(Mingw) その他のエラー
 *	   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 * @par エラーコードについて
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_close(const int fd)
{
    int reterr;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)fd, xtoa_buf, xtoa_bufsz);
        DBMS3(stderr, "s3::ips_socket_close : execute(fd=%s)" EOL_CRLF,
	    xtoa_buf);
    }

#ifdef WIN32
    reterr = closesocket(fd);
    reterr = winsockerr2errno(reterr);
    if (reterr) {
	return reterr;
    }
#else
    reterr = close(fd);
    if (reterr) {
	return reterr;
    }
#endif

    return 0;
}

/**
 * @fn int s3::ips_socket_shutdown(const int fd, const int how)
 * @brief 全二重接続の一部を閉じます。shutdown()のラッパー
 * @param fd ソケット識別子
 * @param how 次のフラグの論理和を指定します
 *	SHUT_RD : 受信禁止
 *	SHUT_WR : 送信禁止
 *	SHUT_RDWR : 送受信禁止
 * @retval 0 成功
 * @retval EBADF 有効なソケット識別子でない
 * @retval ENOTCONN 指定されたソケットは接続されていない
 *	 (MINGW及びVC++2005より前では定義がないため-1となる)
 * @retval ENOTSOCK ソケット識別子でなくファイルである(Winsock系以外)
 * @retval EPIPE ネットワークがダウンした(Windows系）
 * @retval 上記以外また-1 その他のエラー
 *	   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 * @par エラーコードについて
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_shutdown(const int fd, const int how)
{
    int reterr;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)fd, xtoa_buf, xtoa_bufsz);
        DBMS3(stderr, "s3::ips_socket_shutdown : execute(fd=%s)" EOL_CRLF,
	    xtoa_buf);
    }

#ifdef WIN32
    reterr = shutdown(fd, how);
    reterr = winsockerr2errno(reterr);
    if (reterr) {
	return reterr;
    }
#else
    reterr = shutdown(fd, how);
    if (reterr) {
	return reterr;
    }
#endif

    return 0;

}

/**
 * @fn int s3::ips_socket_server_openforIPv4(const uint32_t nIPAddr, const int port, const int backlog, const int num_bind_retry, const int socket_family, const int socket_type, const int protocol)
 * @brief IPv4ソケットサーバー向けテンプレート関数
 * @param nIPAddr サーバプロセスをバインドするIPアドレス(ネットワークバイトオーダー）
 * @param port ポート番号
 * @param backlog listen時の接続キューの数
 * @param num_bind_retry bindに失敗した場合のリトライ数(一秒間隔)
 * @param socket_family socket()に渡すソケットファミリ番号
 * @param socket_type socket()に渡すソケットタイプ番号
 * @param protocol socket()に渡すプロトコル番号
 * @param attr_p s3::ips_socket_server_openforIPv4_attr_t関連属性構造体ポインタ
 * @retval 0以上 成功(fd値）
 * @retval -1以下 失敗
 */
int s3::ips_socket_server_openforIPv4(const uint32_t nIPAddr,
				      const int port, const int backlog,
				      const int num_bind_retry,
				      const int socket_family,
				      const int socket_type,
				      const int protocol, s3::ips_socket_server_openforIPv4_attr_t *attr_p)
{

    int status;
    int fd;
    int i;
    struct sockaddr_in serv_addr;
    struct linger linger_tab;
    s3::ips_socket_server_openforIPv4_attr_t attr = { 0 };

    if( NULL != attr_p ) {
	attr = *attr_p;
    }

    /* open TCP socket */
    fd = s3::ips_socket(socket_family, socket_type, protocol);
    if (fd < 0) {
	return fd;
    }

    if( attr.f.is_using_linger_option ) {
	/* ling */
	linger_tab.l_onoff = 1;
	linger_tab.l_linger = 1;
	if ((setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)MPXX_STATIC_CAST(void*,&linger_tab),
		    sizeof(linger_tab))) == -1) {
	    perror("setsockopt linger_option :");
	    status = -1;
	    goto out;
	}
    }

    /* bind local addr for client */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = socket_family;
    serv_addr.sin_addr.s_addr = nIPAddr;
    serv_addr.sin_port = htons((short) port);

    for (i = 0; i < num_bind_retry; ++i) {
	if (bind(fd, (struct sockaddr *) &serv_addr,
		 sizeof(serv_addr)) < 0) {
	    IFDBG3THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
		mpxx_i64toadec( (int64_t)i, xtoa_buf, xtoa_bufsz);
		DBMS3(stderr, "bind retry=%s" EOL_CRLF, xtoa_buf);
	    }
	    mpxx_sleep(2);
	    continue;
	} else {
	    DBMS3(stderr, "bind successed" EOL_CRLF);
	    break;
	}
    }

    if (i == num_bind_retry) {
	status = -1;
	goto out;
    }

    /* listen */

    if( !(backlog < 0) ) {
	if (listen(fd, backlog) == -1) {
	    perror("socket_bind_listen:");
	    status = -1;
	    goto out;
	}
    }

    status = 0;

  out:

    if (status) {
	    s3::ips_socket_close(fd);
	return -1;
    }

    return fd;
}

/**
 * @fn int s3::ips_socket_accept(const int server_open_fd, struct sockaddr *addr_p, s3::ips_socket_socklen_t * addrlen_p)
 * @brief accept()ラッパー
 * @param server_open_fd サーバーオープン時のファイルディスクプリタ
 * @param addr struct sockaddr構造体ポインタ（クライアントアドレス）
 * @param addrlen struct sockaddr構造体サイズ
 * @retval -1 成功(errnoにエラーコードをセット)
 * @retval 0以上 acceptしたシステムコールのファイルディスクプリタ値
 */
int
s3::ips_socket_accept(const int server_open_fd, struct sockaddr *addr_p,
		      s3::ips_socket_socklen_t * addrlen_p)
{
#ifdef SunOS
    int len;
    int result;

    result = accept(server_open_fd, addr_p, &len);
    if (NULL != addrlen)
	*addrlen = len;
    return result;
#elif defined(QNX) || defined(Linux) || defined(WIN32) || defined(Darwin) || defined(__AVM2__)
    return (int) accept(server_open_fd, addr_p, addrlen_p);
#else
#error 'not implimented s3::ips_socket_accept() on this OS'
#endif
}

/**
 * @fn int s3::ips_socket_nodelay(int fd, int on)
 * @brief ブロックモードの切り替え
 * @param fd FD値
 * @param on 1:no block 0: block
 */
int s3::ips_socket_nodelay(const int fd, const int on)
{
#ifdef WIN32
    int result;
    DWORD sockblock_msec = 1;

    sockblock_msec = (on) ? 1 : 0;	/* 最小値 : ブロッキング */

    result = ioctlsocket(fd, FIONBIO, &sockblock_msec);
    if (SOCKET_ERROR == result) {
	return -1;
    }
#else
    long flag = 0;
    int result;

    flag = fcntl(fd, F_GETFL, 0);
    if (on)
	flag |= O_NDELAY;
    else
	flag &= ~O_NDELAY;

    result = fcntl(fd, F_SETFL, flag);
    if (result == -1)
	return -1;
#endif


    return 0;
}

/**
 * @fn int s3::ips_socket_set_send_windowsize(const int fd, const int send_win)
 * @brief ソケットバッファ 送信ウィンドウサイズの設定
 * @param fd s3::ips_socketディスクプリタ値
 * @param send_win 送信ウインドウバッファサイズ
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::ips_socket_set_send_windowsize(const int fd, const int send_win)
{
#if defined(WIN32) || defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
    int value = 0;
    int before;
#ifdef WIN32
    int len = sizeof(int);
#else
    socklen_t len = sizeof(int);
#endif

    if (!(send_win < 0)) {
#ifdef WIN32
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &value, &len);
#else
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &value, &len);
#endif
	before = value;

	value = send_win;
#ifdef WIN32
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &value, len);
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &value, &len);
#else
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &value, len);
	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &value, &len);
#endif
	if (send_win != value) {
	    value = before;
#ifdef WIN32
	    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &value, len);
#else
	    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &value, len);
#endif
	}
	IFDBG5THEN {
	    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	    char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	    char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

	    mpxx_i64toadec( (int64_t)send_win, xtoa_buf_a, xtoa_bufsz);
	    mpxx_i64toadec( (int64_t)value, xtoa_buf_b, xtoa_bufsz);
	    DBMS5(stderr, "s3::ips_socket_set_send_windowsize : send_win=%s SO_SNDBUF=%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b);
	}
    }

    return 0;
#else
#error 'not implemented s3::ips_socket_set_send_windowsize()'
    return -1;
#endif
}


/**
 * @fn int s3::ips_socket_set_recv_windowsize(const int fd, const int recv_win)
 * @brief ソケットバッファ 受信ウィンドウサイズの設定
 * @param fd s3::ips_socketディスクプリタ値
 * @param recv_win 受信ウインドウバッファサイズ
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::ips_socket_set_recv_windowsize(const int fd, const int recv_win)
{
#if defined(WIN32) || defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
    int value = 0;
    int before;
#ifdef WIN32
    int len = sizeof(int);
#else
    socklen_t len = sizeof(int);
#endif

    if (!(recv_win < 0)) {
#ifdef WIN32
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &value, &len);
#else
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &value, &len);
#endif
	before = value;

	value = recv_win;
#ifdef WIN32
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &value, len);
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &value, &len);
#else
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &value, len);
	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &value, &len);
#endif

	if (recv_win != value) {
	    value = before;
#ifdef WIN32
	    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &value, len);
#else
	    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &value, len);
#endif
	}
	IFDBG5THEN {
	    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	    char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	    char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
    
	    mpxx_i64toadec( (int64_t)recv_win, xtoa_buf_a, xtoa_bufsz),
	    mpxx_i64toadec( (int64_t)value, xtoa_buf_b, xtoa_bufsz);
	    DBMS5(stderr, "s3::ips_socket_set_recv_windowsize : recv_win=%s SO_RCVBUF=%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b);
	}
    }
    return 0;
#else
#error 'not implemented s3::ips_socket_set_recv_windowsize()'

    return -1;
#endif
}


/**
 * @fn int s3::ips_socket_get_windowsize(const int fd, int *sendbuf_size_p, int *recvbuf_size_p)
 * @brief ソケットウインドウサイズの設定を取得します
 */
int s3::ips_socket_get_windowsize(const int fd, int *sendbuf_size_p,
				  int *recvbuf_size_p)
{
#if defined(WIN32) || defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)

    int value = 0;
#ifdef WIN32
    int len = sizeof(int);
#else
    socklen_t len = sizeof(int);
#endif

#ifdef WIN32
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &value, &len);
#else
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &value, &len);
#endif

    if (NULL != sendbuf_size_p) {
	*sendbuf_size_p = value;
    }
#ifdef WIN32
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &value, &len);
#else
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &value, &len);
#endif
    if (NULL != recvbuf_size_p) {
	*recvbuf_size_p = value;
    }

    return 0;
#else
#error 'not implemented s3::ips_socket_get_windowsize()'

    return -1;
#endif
}

/**
 * @fn int s3::ips_socket_set_nagle(const int fd, const int valid)
 * @brief TCP SocketのNagleアルゴリズムのON/OFFをします。
 * @param fd Socketファイルディスクプリタ値
 * @param valid 0:無効 0以外:有効
 * @retval -1 失敗
 * @retval 0 成功
 */
int s3::ips_socket_set_nagle(const int fd, const int valid)
{
    /* Nagle アルゴリズムの設定 */

#ifndef SunOS
    int setvalue;
    s3::ips_socket_socklen_t setlength;

    setvalue = valid ? 0 : 1;
    setlength = sizeof(setvalue);

#ifdef Linux
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &setvalue, setlength);
#else
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &setvalue,
	       setlength);
#endif

    return 0;
#else
    return -1;
#endif
}

/**
 * @fn int s3::ips_socket_clear_recv(const int fd)
 * @brief 到着するパケットを全て破棄します
 * @param fd socketファイルディスクプリタ
 * @retval -1 失敗
 * @retval 0以上 消去されたバイト長
 */
int s3::ips_socket_clear_recv(const int fd)
{
    int result;
    int byte = 0;
    unsigned long sockbuf_stocksize = 0;
    char *dummy = NULL;

    while (1) {

	/* 受信バッファに何か残っている場合は消す */
#ifndef WIN32
	ioctl(fd, FIONREAD, &sockbuf_stocksize);
#else
	ioctlsocket(fd, FIONREAD, &sockbuf_stocksize);
#endif

	if (sockbuf_stocksize == 0)
	    break;

	while (sockbuf_stocksize) {
	    char *dummy;

	    IFDBG5THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];
		mpxx_u64toadec( (uint64_t)sockbuf_stocksize, xtoa_buf, xtoa_bufsz);
		DBMS5(stderr,
		      "s3::ips_socket_clear_recv :  be able to read size=%s" EOL_CRLF, xtoa_buf);
	    }

	    dummy = (char *) malloc(sockbuf_stocksize + 12);
	    if (NULL == dummy) {
		byte = -1;
		goto out;
	    }

	    result =
		s3::ips_socket_recv_withtimeout(fd, dummy,
						sockbuf_stocksize,
						MSG_WAITALL, 0);
	    if (result < 0) {
		byte = -1;
		goto out;
	    }

	    free(dummy);
	    dummy = NULL;

#define UDRSOCK_BUFCLR_SLP_MSEC 10

	    mpxx_msleep(UDRSOCK_BUFCLR_SLP_MSEC);
#ifndef WIN32
	    ioctl(fd, FIONREAD, &sockbuf_stocksize);
#else
	    ioctlsocket(fd, FIONREAD, &sockbuf_stocksize);
#endif

	}
    }

  out:
    if (NULL != dummy) {
	free(dummy);
    }
    return byte;
}

/**
 * @fn int s3::ips_socket_get_valid_interface_list( s3::ips_socket_interface_list_t *ifs_p)
 * @brief 有効なIFのリストを取得します(現状はIPV4のみ)
 * @param ifs_p NIC IFリスト取得用構造体ポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::ips_socket_get_valid_interface_list(s3::ips_socket_interface_list_t
					    * ifs_p)
{
#ifdef WIN32
    INTERFACE_INFO if_list[IPS_SOCKET_IF_MAX];
    unsigned long ret_bytes;
    unsigned int tab_no, i;
    unsigned int num_ifs;
    int fd;

    fd = s3::ips_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
	DBMS1(stderr,
	      "s3::ips_socket_get_valid_interface_list : s3::ips_socket(SOCK_DGRAM) fail" EOL_CRLF);
	return -1;
    }
    if (WSAIoctl(fd, SIO_GET_INTERFACE_LIST, NULL, 0, &if_list,
		 sizeof(if_list), &ret_bytes, NULL,
		 NULL) == SOCKET_ERROR) {
	    s3::ips_socket_close(fd);
	return -1;
    }

    memset(ifs_p, 0x0, sizeof(s3::ips_socket_interface_list_t));
    num_ifs = (unsigned int) (ret_bytes / sizeof(INTERFACE_INFO));

    for (tab_no = 0, i = 0; i < num_ifs; i++) {
	struct sockaddr_in *sa =
	    (struct sockaddr_in *) &(if_list[i].iiAddress);
	s3::ips_socket_interface_tab_t *p = &ifs_p->ifinfo[tab_no];

	if (!(if_list[i].iiFlags & IFF_UP)) {
	    continue;
	}

	if (if_list[i].iiFlags & IFF_LOOPBACK) {
	    /* If there is a loopback address only, use it */
	    continue;
	}

	switch (sa->sin_family) {
	case AF_INET:
	    p->ipv4addr = sa->sin_addr;
	    p->ipv4mask =
		((struct sockaddr_in *) &if_list[i].iiNetmask)->sin_addr;
	    tab_no++;
	    break;
	case AF_INET6:
	    continue;
	}
    }
    ifs_p->num = tab_no;
    s3::ips_socket_close(fd);

    return 0;

#elif defined(Linux)
    struct ifreq ifreq;
    const struct ifreq *ifr, *ifend;
    struct ifconf ifc;
    struct ifreq ifs[MPXX_SOCKET_IF_MAX];
    int fd;
    unsigned int tab_no;

    fd = s3::ips_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
	DBMS1(stderr,
	      "s3::ips_socket_get_valid_interface_list : s3::ips_socket(SOCK_DGRAM) fail" EOL_CRLF);
	return -1;
    }
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
	DBMS1(stderr,
	      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFCONF) fail" EOL_CRLF);
	s3::ips_socket_close(fd);
	return -1;
    }

    memset(ifs_p, 0x0, sizeof(s3::ips_socket_interface_list_t));
    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (tab_no = 0, ifr = ifc.ifc_req; ifr < ifend; ifr++) {	/* iteratorのような */
	    s3::ips_socket_interface_tab_t *p = &ifs_p->ifinfo[tab_no];

	strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
	if (ioctl(fd, SIOCGIFFLAGS, &ifreq) < 0) {
	    continue;
	} else {
	    DBMS5(stderr, "ifreq.ifr_flags = 0x"FMT_LLX EOL_CRLF, (long long)ifreq.ifr_flags);

	    if (!(ifreq.ifr_flags & IFF_UP)) {
		continue;
	    }
	    if (ifreq.ifr_flags & IFF_LOOPBACK) {
		/* If there is a loopback address only, use it */
		continue;
	    }
	}

	if (ifr->ifr_addr.sa_family == AF_INET) {
	    /* IPアドレスの取得 */
	    if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
		DBMS1(stderr,
		      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFADDR) fail" EOL_CRLF);
		continue;
	    } else {
		p->ipv4addr =
		    ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr;
	    }

	    /* netmaskの取得 */
	    if (ioctl(fd, SIOCGIFNETMASK, &ifreq) < 0) {
		DBMS1(stderr,
		      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFNETMASK) fail" EOL_CRLF);
		continue;
	    } else {
		p->ipv4mask =
		    ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr;
	    }
	    tab_no++;
	}
    }
    ifs_p->num = tab_no;
    s3::ips_socket_close(fd);

    return 0;
#elif defined(QNX)
    /* QNX */

    struct ifreq ifreq;
    const struct ifreq *ifr, *ifend;
    struct ifconf ifc;
    struct ifreq ifs[MPXX_SOCKET_IF_MAX];
    int fd;
    unsigned int tab_no;

    fd = s3::ips_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
	DBMS1(stderr,
	      "s3::ips_socket_get_valid_interface_list : s3::ips_socket(SOCK_DGRAM) fail" EOL_CRLF);
	return -1;
    }
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl_socket(fd, SIOCGIFCONF, &ifc) < 0) {
	DBMS1(stderr,
	      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFCONF) fail" EOL_CRLF);
	s3::ips_socket_close(fd);
	return -1;
    }

    memset(ifs_p, 0x0, sizeof(s3::ips_socket_interface_list_t));
    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (tab_no = 0, ifr = ifc.ifc_req; ifr < ifend; ifr++) {	/* iteratorのような */
	    s3::ips_socket_interface_tab_t *p = &ifs_p->ifinfo[tab_no];

	memset(&ifreq, 0x0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
	if (ioctl_socket(fd, SIOCGIFFLAGS, &ifreq) < 0) {
	    continue;
	} else {
	    DBMS5(stderr, "ifreq.ifr_flags = 0x"FMT_LLX EOL_CRLF, (long long)ifreq.ifr_flags);

	    if (!(ifreq.ifr_flags & IFF_UP)) {
		continue;
	    }
	    if (ifreq.ifr_flags & IFF_LOOPBACK) {
		/* If there is a loopback address only, use it */
		continue;
	    }
	}

	if (ifr->ifr_addr.sa_family == AF_INET) {
	    /* IPアドレスの取得 */
	    memset(&ifreq, 0x0, sizeof(ifreq));
	    strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
	    if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
		DBMS1(stderr,
		      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFADDR) fail" EOL_CRLF);
		continue;
	    } else {
		p->ipv4addr =
		    ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr;
	    }

	    /* netmaskの取得 */
	    memset(&ifreq, 0x0, sizeof(ifreq));
	    strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
	    if (ioctl(fd, SIOCGIFNETMASK, &ifreq) < 0) {
		DBMS1(stderr,
		      "s3::ips_socket_get_valid_interface_list  : ioctl(SIOCGIFNETMASK) fail" EOL_CRLF);
		continue;
	    } else {
		p->ipv4mask =
		    ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr;
	    }
	    tab_no++;
	}
	if (!(tab_no < MPXX_SOCKET_IF_MAX)) {
	    break;
	}
    }
    ifs_p->num = tab_no;
    s3::ips_socket_close(fd);

    return 0;
#elif defined(Darwin)
    /* MACOSX and FreeBSD? */
    struct ifaddrs *ifa_list, *ifa;
    int result;
    unsigned int tab_no;

    result = getifaddrs(&ifa_list);
    if (result != 0) {
	return -1;
    }

    for (tab_no = 0, ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
	    s3::ips_socket_interface_tab_t *p = &ifs_p->ifinfo[tab_no];

	DBMS5(stderr, "ifa_name = %s" EOL_CRLF, ifa->ifa_name);
	DBMS5(stderr, "ifa_flags= 0x" FMT_ZLLX EOL_CRLF, 8, (long long)ifa->ifa_flags);

	if (ifa->ifa_flags & IFF_LOOPBACK) {
	    /* If there is a loopback address only, use it */
	    continue;
	}

	if (ifa->ifa_addr->sa_family == AF_INET) {
	    p->ipv4addr = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
	    p->ipv4mask =
		((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr;
	    tab_no++;
	}

	if (!(tab_no < MPXX_SOCKET_IF_MAX)) {
	    break;
	}
    }

    ifs_p->num = tab_no;
    freeifaddrs(ifa_list);

    return 0;
#else
    /* other */
    return -1;
#endif
}

#ifdef WIN32
/**
 * @fn static int winsockerr2errno( const int wsaError )
 * @brief WinsockのエラーコードをPOSIX及びC90のエラー番号に変換します。
 * @param wsaError winsockのエラーコード
 * @return errnoコード(-1:定義を設定していないエラー)
 */
static int winsockerr2errno(const int wsaError)
{
    switch (wsaError) {
    case 0:
	/* 成功 */
	return 0;
    case WSAENOTSOCK:
	/* 有効なオープンされたソケット識別子でない。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENOTSOCK;
#else
	return EBADF;
#endif
    case WSANOTINITIALISED:
	/* Winsockが初期化されていない */
	return EINVAL;
    case WSAENETDOWN:
	/* ネットワークがダウンした */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENETDOWN;
#else
	break;
#endif
    case WSAEFAULT:
	/* 不正なバッファポインタが渡った */
	return EFAULT;
    case WSAEINPROGRESS:
	/* 既にブロッキング処理中である */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EINPROGRESS;
#else
	break;
#endif
    case WSAEINVAL:
	/* 無効な引数が渡された */
	return EINVAL;
    case WSAENETRESET:
    case WSAECONNABORTED:
	/* ネットワーク接続が破棄された */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ECONNRESET;
#else
	break;
#endif
    case WSAENOPROTOOPT:
	/* 不正なプロトコルオプション */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENOPROTOOPT;	/* POSIXの場合 EPFNOSUPPORTの場合有り */
#else
	break;
#endif
    case WSAENOTCONN:
	/* ソケットは接続されていない */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENOTCONN;
#else
	break;
#endif
    case WSAEINTR:
	/* システムコールの割り込みが発生した。 */
	return EINTR;
    case WSAEMFILE:
	/* 使用中のソケットの数が多すぎる。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EMFILE;
#else
	break;
#endif
    case WSAEPROTOTYPE:
	/* プロトコルファミリがサポートされていない。 */
    case WSAEAFNOSUPPORT:
	/* アドレスファミリがサポートされていない。 */
    case WSAESOCKTNOSUPPORT:
	/* 指定されたソケットタイプはサポートされていない。 */
    case WSAEPFNOSUPPORT:
	/* プロトコルファミリがサポートされていない。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EPROTONOSUPPORT;
#else
	break;
#endif
    case WSAEREFUSED:
	/* 接続が接続相手によりリセットされた。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ECONNRESET;
#else
	break;
#endif
    case WSAEACCES:
	/* ソケット・ファイルへの書き込み許可がなかった */
	return EACCES;
    case WSAENOBUFS:
	/* ネットワーク・インターフェースの出力キューが一杯である。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENOBUFS;
#else
	break;
#endif
    case WSAEOPNOTSUPP:
	/* そのソケット種別では不適切なものである。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EOPNOTSUPP;
#else
	break;
#endif
    case WSAESHUTDOWN:
	/* 接続指向のソケットでローカル側が閉じられている。 */
	return EPIPE;
    case WSAEWOULDBLOCK:
	/* ソケットが非停止に設定されており、 要求された操作が停止した。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EWOULDBLOCK;
#else
	break;
#endif
    case WSAEMSGSIZE:
	/* メッセージが大き過ぎるため送信することができない。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EMSGSIZE;
#else
	break;
#endif
    case WSAECONNRESET:
	/* ネットワーク接続が相手によって破棄された。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ECONNRESET;
#else
	break;
#endif
    case WSAEADDRNOTAVAIL:
	/* 無効なネットワークアドレス。 */
	break;
    case WSAEDESTADDRREQ:
	/* 操作の実行に送信先アドレスが必要。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENOTCONN;
#else
	break;
#endif
    case WSAEHOSTUNREACH:
	/* 指定時間内に送信先にパケットが到着しなかった */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EHOSTUNREACH;
#else
	break;
#endif
    case WSAENETUNREACH:
	/* 指定されたネットワークホストに到達できない。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ENETUNREACH;
#else
	break;
#endif
    case WSAETIMEDOUT:
	/* 接続要求がタイムアウトした。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ETIMEDOUT;
#else
	break;
#endif
    case WSAECONNREFUSED:
	/* 接続が拒否された。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return ECONNREFUSED;
#else
	break;
#endif
    case WSAEADDRINUSE:
	/* bind()等しようとしたアドレスは、既に使用中である。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EADDRINUSE;
#else
	break;
#endif
    case WSAEALREADY:
	/* キャンセルしようとした非同期操作が既にキャンセルされている。 */
	/* connect()を呼び出したが、既に前回の呼び出しによって接続処理中である。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EALREADY;
#else
	break;
#endif
    case WSAEISCONN:
	/* ソケットは既に接続されている。 */
#if defined(_MSC_VER) && ( _MSC_VER >= 1500 )
	return EISCONN;
#else
	break;
#endif


    }

    return -1;
}
#endif

/**
 * @fn int s3::ips_socket_set_udp_broadcast( const int fd, int is_active)
 * @brief SOCKETでUDPプロトコル指定時にブロードキャストパケットを送信可能に設定します
 * @param fd ソケット識別子
 * @param is_active 0:ブロードキャスト無効 0以外:ブロードキャスト有効
 * @retval 0 成功
 * @retval EBADF ソケット識別子無効
 * @retval ENOPROTOOPT 指定された層(level)には設定できない
 */
int s3::ips_socket_set_udp_broadcast(const int fd, int is_active)
{
    int reterr;
    const int len = sizeof(int);
    int value = (is_active) ? 1 : 0;
/**
 * @note getsockopt()/setsockopt()についてのその他のエラー
 * @par エラーコード
 *	- ENOTSOCK(POSIX) 引き数 sockfd がソケットではなくファイルである。 
 *	- EFAULT(WIN32, POSIX) optval で指定されたアドレスがプロセスのアドレス空間の有効な部分ではない。 
 *	- getsockopt() の場合、 optlen がプロセスのアドレス空間の有効な部分でない場合にもこのエラーが返される。
 *	- EINVAL(WIN32, POSIX) setsockopt() で option が不正である。
 */

#if defined(WIN32)
    reterr =
	setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &value, len);
    reterr = winsockerr2errno(reterr);
    if (reterr) {
	return reterr;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    reterr =
	setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void *) &value, len);
    if (reterr) {
	return reterr;
    }
#else
#error 'not implemented s3::ips_socket_set_udp_broadcast()'
#endif

    return 0;
}

/**
 * @fn int s3::ips_socket_set_nonblock_mode( const int fd,  int is_active)
 * @brief SOCKETでデータ送受信関数でブロックするかどうかを指定します
 * @param fd ソケット識別子
 * @param is_active 0:ブロックする 0以外:ブロックしない
 * @retval 0 成功
 * @retval EBADF ソケット識別子無効
 * @retval EINTR シグナルにより割り込まれた 
 * @retval 上記以外また-1 その他のエラー
 *	   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_set_nonblock_mode(const int fd, int is_active)
{
    int reterr;

    /* ノンブロッキングモード */
#ifdef WIN32
    unsigned long nonblocking = (is_active) ? 1 : 0;

    reterr = ioctlsocket(fd, FIONBIO, &nonblocking);
    reterr = winsockerr2errno(reterr);
    if (reterr) {
	return reterr;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    int val;

    val = fcntl(fd, F_GETFL);
    val = (is_active) ? (val | O_NONBLOCK) : (val & ~O_NONBLOCK);

    reterr = fcntl(fd, F_SETFL, val);
    if (reterr) {
	return reterr;
    }
#else
#error 'not implemented s3::ips_sockset_set_nonblock_mode()'
#endif

    return 0;
}

/**
 * @fn int s3::ips_socket_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout_p)
 * @brief SOCKETで使用する為のselect() ラッパーです。主にWIN32でのerrnoを設定するためにラッピングしています。
 * @param nfds FD_SETSIZEで設定
 * @param readfds 読み込みか可能かどうかを監視するFD配列(必要ない場合はNULL)
 * @param writefds 書き込み可能かどうかを監視するFD配列(必要ない場合はNULL)
 * @param exceptfds 例外の監視を行うFD配列(必要ない場合はNULL)
 * @param timeout_p timeval構造体によるタイムアウト時間してい(NULLの場合は無期限に停止）
 * @retval 1以上 readfds/writefds/exceptfdsで入力されているFD配列の数(複数設定して同一数の場合は見分けがつかない）
 * @retval 0 タイムアウトした
 * @retval -1 失敗(errnoにエラー番号を設定)
 * @par errnoコード
 *	- EBADF 無効なFD値が含まれている
 *	- EINTR シグナルを受信した
 *	- EINVAL timeout値が不正
 *	- ENOMEM 内部テーブルにメモリを割り当てることができなかった
 *	- 上記以外また-1 その他のエラー
 *	-   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_select(int nfds, fd_set * readfds, fd_set * writefds,
			  fd_set * exceptfds, struct timeval *timeout_p)
{
#ifdef WIN32
    int status;
    status = select(nfds, readfds, writefds, exceptfds, timeout_p);
    if (status == SOCKET_ERROR) {
	errno = winsockerr2errno(WSAGetLastError());
	return -1;
    }
    return status;
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return select(nfds, readfds, writefds, exceptfds, timeout_p);
#else
#error 'not implemented s3::ips_socket_select()'
#endif
}

/**
 * @fn int s3::ips_socket_sendto( const int sockfd, const void *dat, size_t len, int flags, const struct sockaddr *dest_addr, s3::ips_socket_socklen_t addrlen)
 * @brief SOCKETライブラリsendtoのラッパ BSD,POSIX, Winsockをラッピングしますが、おもにWinsockをエラーコードをerrnoに変換します
 * @param sockfd ソケット識別子
 * @param dat 送信データバッファポインタ
 * @param len 送信データバイト数 
 * @param flags メッセージタイプフラグ MSG_????で定義
 * @param dest_addr 送信先アドレス登録 struct sockaddr構造体ポインタ
 * @param addrlen struct sockaddrのサイズ
 * @retval 0以上 送信したバイト数
 * @retval -1 エラー(errnoにエラー番号コードを設定)
 * @par errnoコード
 *	- EACCES ソケットへの書き込み許可がなかった。
 *	- EAGAIN ソケットが非停止に設定されており、 要求された操作が停止した。
 *      - EWOULDBLOCK (LINUXのみ)EAGAINと同値（ POSIX.1-2001 は、この場合にどちらのエラーを返すことも認めている)
 *	- EBADF 無効なディスクリプターが指定された。 
 *	- ECONNRESET 接続が接続相手によりリセットされた。 (MINGW及びVC++2005より前では定義がないため-1となる)
 *      - EDESTADDRREQ ソケットが接続型 (connection-mode) ではなく、 かつ送信先のアドレスが設定されていない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EFAULT ユーザー空間として不正なアドレスがパラメーターとして指定された。
 *	- EINTR データが送信される前に、シグナルが発生した。
 *	- EINVAL 不正な引き数が渡された。 
 *	- EISCONN 接続型ソケットの接続がすでに確立していたが、受信者が指定されていた。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EMSGSIZE そのソケット種別 ではソケットに渡されたままの形でメッセージを送信する必要があるが、 メッセージが大き過ぎるため送信することができない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOBUFS ネットワーク・インターフェースの出力キューが一杯である。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOMEM メモリが足りない。 
 *	- ENOTCONN ソケットが接続されておらず、接続先も指定されていない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOTSOCK 引き数 sockfd はソケットではない。 
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- EOPNOTSUPP 引き数 flags のいくつかのビットが、そのソケット種別では不適切なものである。 
 *	- EPIPE 接続指向のソケットでローカル側が閉じられている。
 *	- 上記以外また-1 その他のエラー
 *	-    ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_sendto(const int sockfd, const void *dat, size_t len,
			  int flags, const struct sockaddr *dest_addr,
			  s3::ips_socket_socklen_t addrlen)
{
#ifdef WIN32
    int retval;
    retval = sendto(sockfd, MPXX_STATIC_CAST(const char*,dat), (int) len, flags, dest_addr, addrlen);
    if (retval == SOCKET_ERROR) {
	errno = winsockerr2errno(WSAGetLastError());
	return -1;
    }
    return retval;
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return sendto(sockfd, dat, len, flags, dest_addr, addrlen);
#else
#error 'not implemented s3::ips_socket_sendto()'
#endif
}


/**
 * @fn int s3::ips_socket_recvfrom(const int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, s3::ips_socket_socklen_t *addrlen_p)
 * @brief SOCKETライブラリrecvfromのラッパ。Winsockをラッピングではエラーコードをerrnoに変換します
 * @param sockfd ソケット識別子
 * @param buf 受信データバッファポインタ
 * @param len 最大受信バイト数 
 * @param flags メッセージタイプフラグ MSG_????で定義
 * @param src_addr 送信元アドレス格納用構造体ポインタ(NULL指定可）
 * @param addrlen_p src_addrに指定された構造体サイズを入れた変数ポインタ。関数から返ると実際に使用した値が返る
 * @retval 0以上 送信バイト数
 * @retval -1 エラー(errnoにエラーコード設定)
 * @par errnoコード
 *	- EAGAIN/EWOULDBLOCK ソケットが非停止 (nonblocking) に設定されていて 受信操作が停止するような状況になったか、 受信に時間切れ (timeout) が設定されていて データを受信する前に時間切れになった。
 *	- EBADF 引き数 sockfd が不正なディスクリプタである。 
 *	- ECONNREFUSED リモートのホストでネットワーク接続が拒否された 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EFAULT 受信バッファへのポインタがプロセスのアドレス空間外を指している。
 *	- EINTR データを受信する前に、シグナルが配送されて割り込まれた。
 *	- EINVAL 不正な引き数が渡された。 
 *	- ENOTCONN ソケットに接続指向プロトコルが割り当てられており、 まだ接続されていない 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOTSOCK 引き数 sockfd がソケットを参照していない。 
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- 上記以外また-1 その他のエラー
 *	-   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_recvfrom(const int sockfd, void *buf, size_t len,
			    int flags, struct sockaddr *const src_addr,
			    s3::ips_socket_socklen_t *const addrlen_p)
{

#ifdef WIN32
    int retval;
    retval = recvfrom(sockfd, MPXX_STATIC_CAST(char*,buf), (int) len, flags, src_addr, addrlen_p);
    if (retval == SOCKET_ERROR) {
	errno = winsockerr2errno(WSAGetLastError());
	return -1;
    }
    return retval;
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen_p);
#else
#error 'not implemented s3::ips_socket_recvfrom()'
#endif
}

/**
 * @fn int s3::ips_socket_connect( const int sockfd, const struct sockaddr *addr, const s3::ips_socket_socklen_t addrlen)
 * @brief ソケットの接続を行う
 * @param sockfd ソケット識別子
 * @param addr 接続先アドレス格納 struct sockaddr構造体ポインタ
 * @param addrlen struct sockaddr構造体サイズ
 * @retval 0 成功
 * @retval -1 失敗(errnoを参照)
 * @par errnoコード
 *	- EACCES UNIX ドメインソケットはパス名で識別される。 ソケットへの書き込み許可がなかった。
 *	- EACCES, EPERM ソケットのブロードキャスト・フラグが有効になっていないのに ユーザがブロードキャストへ接続を試みた。または、ローカルのファイアウォールの規則により接続の要求が失敗した。 
 *	- EADDRINUSE ローカルアドレスが既に使用されている。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EAFNOSUPPORT 渡されたアドレスの sa_family フィールドが正しいアドレス・ファミリーではない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EAGAIN 使用可能なローカルのポートがないか、 ルーティングキャッシュに十分なエントリがない。
 *	- EALREADY ソケットが非停止 (nonblocking) に設定されており、 前の接続が完了していない。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EBADF ファイルディスクリプターがディスクリプターテーブルの 有効なインデックスではない。 
 *	- ECONNREFUSED リモートアドレスで接続を待っているプログラムがない。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EFAULT ソケット構造体のアドレスがユーザーのアドレス空間外にある。 
 *	- EINPROGRESS ソケットが非停止 (nonblocking) に設定されていて、接続をすぐに 完了することができない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- EINTR 捕捉されたシグナルによりシステムコールが中断された。
 *	- EISCONN ソケットは既に接続 (connect) されている。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENETUNREACH 到達できないネットワークである。 
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	- ENOTSOCK ファイルディスクリプターがソケットと関連付けられていない。 
 *	-  (MINGW及びVC++2005より前では定義がないためEBADFとなる)
 *	- ETIMEDOUT 接続を試みている途中で時間切れ (timeout) になった。サーバーが混雑していて 新たな接続を受け入れられないのかもしれない。
 *	-  (MINGW及びVC++2005より前では定義がないため-1となる)
 *	-   ※ MINGWやVC++2005より前の開発環境では対応していないerrnoが多数有ります
 *	-       WINDOWS系で詳細なエラーコードを評価したい場合はWSAGetLastError()を独自に使ってください
 */
int s3::ips_socket_connect(const int sockfd, const struct sockaddr *addr,
			   s3::ips_socket_socklen_t addrlen)
{
#ifdef WIN32
    int retval;
    retval = connect(sockfd, addr, addrlen);
    if (retval == SOCKET_ERROR) {
	errno = winsockerr2errno(WSAGetLastError());
	return -1;
    }
    return retval;
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    return connect(sockfd, addr, addrlen);
#else
#error 'not implemented s3::ips_socket_recvfrom()'
#endif
}

/**
 * @fn int s3::ips_socket_getaddrinfo( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) 
 * @brief ネットワークのアドレスとサービスを変換します。
 * 	nodeとserviceを渡すと、一つ以上のaddrinfo構造体を返します。IPv4とIPv6それぞれの依存関係を吸収します。
 *	 細かいことは　getaddrinfo()を参照。WIN32の場合、WINSOCKを初期化します。
 * @param node IPv4の場合はinet_aton(), IPv6の場合はinet_pton()で変換された数値アドレス又はホスト名
 *	 hints.ai_flags に AI_NUMERICHOST が設定された場合は 数値アドレスを指定する。
 * @param service サービス名 又は NULL
 * @param hints 特定のヒント又は NULL
 * @param res 変換結果を返す構造体のの領域を確保して返します
 * @retval 0 成功
 * @retval 0以外　エラーコードs3::ips_gai_strerror()で文字列に変換可能
 **/
int s3::ips_socket_getaddrinfo( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) 
{
    if (!socket_init_flag) {
#ifdef WIN32
	WSADATA wsaData;

	/* WINSOCK2.2 Socket おまじない */
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
	    perror("WSAStartup:");
	    return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
	    DMSG(stderr, "[Warning] Winsock Version\n");
	}
#endif
	socket_init_flag = -1;
    }

    return getaddrinfo( node, service, hints, res);
}

/**
 * @fn void s3::ips_socket_freeaddrinfo( struct addrinfo *res)
 * @brief s3::ips_socket_getaddrinfo()のresで返されたデータ領域を開放します
 * @param res  s3::ips_socket_getaddrinfo()のresで返されたポインタ値
 */
void s3::ips_socket_freeaddrinfo( struct addrinfo *res)
{
    freeaddrinfo(res);

    return;
}

/**
 * @fn const char *s3::ips_socket_gai_strerror( int errcode)
 * @brief s3::ips_socket_getaddrinfo()で返されたエラーコードを文字列に変換します
 * @param errcode s3::ips_socket_getaddrinfo()で返されたエラーコード値
 * @retval 文字列のポインタ
 **/
const char *s3::ips_socket_gai_strerror( int errcode)
{
    return gai_strerror(errcode);
}

/**
 * @fn int s3::ips_socket_get_tcp_mss( const int sockfd, size_t *const sizof_mss_p )
 * @brief 接続中のセッションのMSSサイズを取得します。
 *   TCPのセッションが確立する前に実行すると、実行環境の最大MSSサイズが返ります。
 * @param sockfd ソケットファイルディスクプリタ
 * @param sizof_mss_p MSS取得用変数ポインタ
 * @retval 0 成功
 * @retval EINVAL 不正な引数
 * @retval ENOSYS この環境ではサポートできない
 * @retval ENOTSOCK(POSIX) 引き数 sockfd がソケットではなくファイルである。 
 */
int s3::ips_socket_get_tcp_mss( const int sockfd, size_t *const sizof_mss_p )
{
#if !defined(TCP_MAXSEG)
    /* @note 古いMINGWとかTCP_MAXSEGに対応していない物があるのでその場合ENOTSUPを返す */
    (void)sockfd;
    (void)sizof_mss_p;
    return ENOSYS;
#else
    int reterr;
    unsigned int  get_mss = 0;
#if defined(WIN32)
    int len = sizeof(get_mss);
#else /* Unix like */
    socklen_t len = sizeof(get_mss);
#endif

    if( NULL == sizof_mss_p ) {
	return EINVAL;
    }
/**
 * @note getsockopt()/setsockopt()についてのその他のエラー
 * @par エラーコード
 *	- ENOTSOCK(POSIX) 引き数 sockfd がソケットではなくファイルである。 
 *	- EFAULT(WIN32, POSIX) optval で指定されたアドレスがプロセスのアドレス空間の有効な部分ではない。 
 *	- getsockopt() の場合、 optlen がプロセスのアドレス空間の有効な部分でない場合にもこのエラーが返される。
 *	- EINVAL(WIN32, POSIX) setsockopt() で option が不正である。
 */

#if defined(WIN32)
    reterr =
	getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, (char *) &get_mss, &len);
    reterr = winsockerr2errno(reterr);
    if (reterr) {
	return reterr;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(__AVM2__)
    reterr =
	getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, (void *) &get_mss, &len);
    if (reterr) {
	return reterr;
    }
#else
#error 'not implemented s3::ips_socket_get_tcp_mss()'
#endif

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
	
	mpxx_u64toadec( (uint64_t)get_mss, xtoa_buf_a, xtoa_bufsz);
	mpxx_u64toadec( (uint64_t)len, xtoa_buf_b, xtoa_bufsz);
	DBMS5( stderr, "s3::ips_socket_get_tcp_mss : get_mms=%s len=%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b);
    }

    if(get_mss == (unsigned int)~0 ) {
	return ENOTSUP;
    }
    *sizof_mss_p = (size_t)get_mss;

    return 0;
#endif
}

/**
 * @fn int s3::ips_socket_inet_aton(const char *const ipv4_str, struct in_addr *const ipv4_in_addr_p)
 * @breif 与えられたIPV4ドットアドレスを struct in_addr構造体に入れます
 * @param ipv4str IPV4ドットアドレス文字列ポインタ
 * @param ipv4_in_addr_p  struct in_addr構造体ポインタ
 * @retval 0 失敗
 * @retval 0以外 成功
 */
int s3::ips_socket_inet_aton(const char *const ipv4_str, struct in_addr *const ipv4_in_addr_p)
{
#if defined(WIN32)
    uint32_t tmp_ipv4_addr;
    
    if( NULL == ipv4_str ) {
	return 0;
    } else {
	tmp_ipv4_addr = inet_addr(ipv4_str);
    }

    if (NULL != ipv4_in_addr_p) {
	    ipv4_in_addr_p->s_addr = tmp_ipv4_addr;
    }

    return 1;
#else
    return inet_aton( ipv4_str, ipv4_in_addr_p);
#endif

}

/**
 * @fn int s3::ips_socket_check_accessible_ipv4_local_area( const char *const remote_or_ipv4_str, struct in_addr *const ipv4_in_addr_p, int *const eai_errno_p)
 * @brief 指定されたホスト名又はIPv4アドレスはEtherNetデバイスからアクセス可能か確認します。
 *	IPv4アドレスのみ対応しています。
 *	有効なEtherNetカードのセグメントから確認するため、ローカルエリアのみとします。
 * @param remote_or_ipv4_str IPv4アドレス又はホスト名又はIPv4ドット
 * @param ipv4_in_addr_p 確認したホストのIPv4アドレス(NULLなら戻さない）
 * @param eai_errno_p 名前変換関数 getaddrinfo()のエラー時の戻り値
 * @retval 0 成功アクセス可能
 * @retval ENXIO アドレス変換ができませんでした。eai_errno_pで取得したエラー番号を参照
 * @retval EINVAL remote_or_ipv4_strがNULLポインタを示した
 * @retval ENETDOWN 対象のネットワークが不通（リンクダウンの可能性）
 **/
int s3::ips_socket_check_accessible_ipv4_local_area( const char *const remote_or_ipv4_str, struct in_addr *const ipv4_in_addr_p, int *const eai_errno_p)
{
    int result, status;
    struct addrinfo hints, *res=NULL;
    struct in_addr addr;
    s3::ips_socket_interface_list_t ifs;
    int accessible = 0;
    size_t n;

     /* IPアドレスの指定がない場合は共有フォルダの先頭から名前を切り出してIPアドレスを求める */
    memset(&addr, 0x0, sizeof(addr));
    memset(&hints, 0x0, sizeof(hints));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */

    result = s3::ips_socket_getaddrinfo( remote_or_ipv4_str, NULL, &hints, &res);
    if(result) {
	DBMS1( stderr, "%s : s3::ips_socket_getaddrinfo() fail gai_strerror=%s\n",
	 __func__, s3::ips_socket_gai_strerror(result));
	if( NULL == eai_errno_p ) {
	    *eai_errno_p = result;
	}
	return ENXIO; 
    } 

    addr.s_addr= ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    if(NULL != ipv4_in_addr_p) {
	*ipv4_in_addr_p = addr;
    }

    result = s3::ips_socket_get_valid_interface_list(&ifs);
    if(result) {
	DBMS1( stderr, "%s : s3::ips_socket_get_valid_interface_list() fail\n", __func__);
	status = result;
	goto out;
    }

    for( n=0; n<ifs.num; n++ ) {
	    s3::ips_socket_interface_tab_t *const p = &(ifs.ifinfo[n]);
	/* 結果のIPアドレスが有効範囲にリンクアップしている範囲に入っているか調べる */
	if( (p->ipv4addr.s_addr & p->ipv4mask.s_addr) == (addr.s_addr & p->ipv4mask.s_addr)) {
	    accessible = ~0;
	    status = 0;
	    goto out;
	}
    }

    status = ENETDOWN; /* 対象のネットワークが不通（リンクダウンの可能性) */

out:

    if( NULL != res) {
	    s3::ips_socket_freeaddrinfo(res);
    }

    return status;
}



