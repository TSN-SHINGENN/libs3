/** 
 *      Copyright 2004 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2004-July-08 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file ips_smtp_client.cpp
 * @brief SMTPプロトコルのSendmailプロトコルライブラリ
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* POSIX */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>
#include<time.h>
#include<errno.h>

#if defined(Linux) || defined(Darwin) || defined(__AVM2__) || defined(QNX) /* posix like */
#include<netdb.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#endif

typedef struct timeval timeval_t;

#if !defined(__MINGW32__)
#pragma comment (lib,"Ws2_32.lib")
#endif

/* libs3 */
#include <core/libmpxx/mpxx_sys_types.h>

#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include <core/libmpxx/dbms.h>
#include <core/libmpxx/_stdarg_fmt.h>

/* this */
#include "ips_socket.h"
#include "ips_smtp_client.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

static int smtp_response_analyze(char *buf);
static int send_txt_cmd(int fd, char *const s_dat, char *const r_buf, const size_t r_size,
			time_t timeout);

static int sock_send_txt(int fd, char *const s_dat, const int timeout);
static int sock_recv_txt(int fd, char *const r_buf, const size_t r_size, const int timeout);

/*
   socket向けテキスト送受信コマンド
   #include <sys/types.h>
   #include <sys/socket.h>

   受信バッファには返ってくる最大文字列サイズ以上のバッファを使用すること
*/
static int
send_txt_cmd(int fd, char *const s_dat, char *const r_buf, const size_t r_size, time_t command_timeout)
{
    int result;
    const int timeout = (int)command_timeout;

    DBMS5(stdout, __func__ " : send exec" EOL_CRLF);
    result = sock_send_txt(fd, s_dat, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_send_txt fail" EOL_CRLF);
	return -1;
    }

    DBMS5(stdout, __func__ " : recv exec" EOL_CRLF);
    result = sock_recv_txt(fd, r_buf, r_size, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_recv_txt fail" EOL_CRLF);
	return -1;
    }

    return 0;
}

static int sock_send_txt(int fd, char *const s_dat, const int timeout)
{

    int result;
    int loop;
    size_t s;
    char *p;

    DBMS5(stdout, "sock_send_txt : timeout=" FMT_LLD EOL_CRLF, (long long)timeout);
    DBMS5(stdout, "sock_send_txt : sendstr = %s" EOL_CRLF, s_dat);

    loop = 1;
    p = s_dat;
    s = strlen(p);
    while (loop) {
	result = s3::ips_socket_send_withtimeout(fd, p, s, 0, timeout);
	if (result < 0) {
	    DBMS1(stdout, "send_txt_cmd : s3::ips_socket_send_withtimeout fail" EOL_CRLF);
	    return -1;
	}
	if (result != s) {
	    p += result;
	    s -= result;
	} else
	    loop = 0;
    }

    return 0;
}

static int sock_recv_txt(int fd, char *r_buf, const size_t r_size, const int timeout)
{

    int result, status;
    int loop;
    size_t s;
    char *p;

    DBMS5(stdout, __func__ " : execute" EOL_CRLF);

    loop = 1;
    p = (char*)r_buf;
    s = r_size;

    memset(r_buf, 0, r_size);
    while (loop) {
	result = s3::ips_socket_recv_withtimeout(fd, p, s, 0, timeout);
	if (result < 0) {
	    DBMS1(stdout,
		  __func__ " : sock_recv_withtimeout time="FMT_LLD"sec, fail" EOL_CRLF,
		  (long long)timeout);
	    return -1;
	}
	DBMS5(stdout, __func__ " : recv result=%d : str = %s" EOL_CRLF,
	      result, r_buf);
	if (result == 0 && (r_buf != p)) {
	    status = 0;
	    goto out;
	} else if (result == 0 && (r_buf == p)) {
	    DBMS5(stdout, __func__ " : no response" EOL_CRLF);
	    status = -1;
	    goto out;
	}

	if (!(iscntrl(*(p + result - 1)))) {
	    p += result;
	    s -= result;
	} else
	    loop = 0;
    }

    status = 0;

  out:

    DBMS5(stdout, __func__ " : recv = %s" EOL_CRLF, r_buf);

    return status;
}


/*
   SMTPサーバの返してくるレスポンスを解析します。
　　成功ならば 0 失敗に関連することなら-1を返します
*/

static int smtp_response_analyze(char *str)
{
    int result;
    int code;
    char buf[256];

    result = sscanf(str, "%i %s", &code, buf);
    if (result != 2) {
	DBMS1(stderr, "%s : sscanf fail" EOL_CRLF, __func__ );
	DBMS1(stderr, "%s : str is %s" EOL_CRLF, __func__, str);
    }

    DBMS4(stdout, "%s : code=%d str=%s" EOL_CRLF,
	__func__, code, buf);

    if ((200 < code) && (code < 299)) {
	return 0;
    } else if ((300 < code) && (code < 399)) {
	return 0;
    } else if ((400 < code) && (code < 499)) {
	return -1;
    } else if ((500 < code) && (code < 599)) {
	return -1;
    }

    DBMS1(stdout, "smtp_response_analyze : unknown responce=%s" EOL_CRLF, str);

    return -1;
}

/**
 * @fn int s3::ips_smtp_client_connect_to_server(s3::ips_smtp_client_t *const self_p, const char *const serverip, const char *const servername, int port_no, int timeout_sec)
 * @brief インスタンスを初期化し、SMTPサーバーに接続します。
 * @param self_p インスタンス構造体
 * @param serverip サーバーのIPアドレス（NULLの場合、servernameを使用します
 * @param servername サーバー名（NULL指定できません）
 * @param port_no サーバーポート番号（-1でデフォルトの25)
 * @param timeout_sec サーバー接続時のタイムアウト時間
 * @retval 0 成功
 * @retval EINVAL 引数が不正
 * @retval EIO SOCKET通信に問題が発生した
 * @retval EACCES SMTPサーバに到達できなかった
 **/
int s3::ips_smtp_client_connect_to_server(s3::ips_smtp_client_t *const self_p, const char *const serverip, const char *const servername, int port_no, int timeout_sec)
{
    ips_smtp_client_t *const s = self_p;
    int result;
    int status;
    int fd;
    int sockport;
    time_t stime;
    struct sockaddr_in serv_addr;
    struct addrinfo hints, *ai;
#if 0
    struct in_addr in;
#endif
    struct hostent *he;

    he = NULL;
    ai = NULL;

    if(NULL == servername ) {
	return EINVAL;
    } 

    if( NULL != serverip ) {
	if ( !(strlen(serverip) < IPS_SMTP_SERVERNAME_MAX ) ) {
	    DBMS1(stdout, __func__ " : serverip is long -> name = %s" EOL_CRLF,
	      serverip);
	    return EINVAL;
	}
    }
    if ( !(strlen(servername) < IPS_SMTP_SERVERNAME_MAX ) ) {
	DBMS1(stdout, __func__ " : servername is long -> name = %s" EOL_CRLF,
	      servername);
	return EINVAL;
    }

    /* sms構造体初期化 */
    memset(s, 0, sizeof(ips_smtp_client_t));
    s->fd = -1;
    if (NULL == serverip) {
	strcpy(s->smtp_servername, servername);
    } else {
	strcpy(s->smtp_servername, serverip);
    }

    sockport = (port_no < 0) ? IPS_SMTP_DEFAULT_SMTPPORT : port_no;
    s->port_no = sockport;

    s->connect_timeout_sec = timeout_sec;

    DBMS5(stdout, __func__ " : servername is %s" EOL_CRLF, s->smtp_servername);
    DBMS5(stdout, __func__ " : smtp port no = %d" EOL_CRLF, s->port_no);
    DBMS5(stdout, __func__ " : timeout time is %d" EOL_CRLF, (int)s->connect_timeout_sec);

  loop:
    stime = time(NULL);

    /* socket 接続 */
    fd = s3::ips_socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
	DBMS1(stderr, __func__ " : socket fail" EOL_CRLF);
	return EIO;
    }
    s->fd = fd;
    DBMS4(stdout, __func__ " : open socket fd = %d" EOL_CRLF, s->fd);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    result = getaddrinfo(s->smtp_servername, NULL, &hints, &ai);
    if (result) {
	DBMS3(stderr, __func__ " : unknown host %s(%s)" EOL_CRLF,
	      s->smtp_servername, gai_strerror(result));
	status = EACCES;
	goto out;
    }

    if (ai->ai_addrlen > sizeof(serv_addr)) {
	DBMS1(stdout, __func__ " : sockaddr too large (" FMT_LLU ") > (" FMT_LLU ")" EOL_CRLF, (long long)ai->ai_addrlen,
	      (long long)sizeof(serv_addr));
	status = EIO;
	goto out;
    }

    memcpy(&serv_addr, ai->ai_addr, ai->ai_addrlen);
    serv_addr.sin_port = htons((short) sockport);

    /* タイムアウトつきconnect を利用する */

    DBMS3(stdout, __func__ " : s->smtp_servername = %s s->connect_timeout_sec = %d" EOL_CRLF,
	  s->smtp_servername, (int)s->connect_timeout_sec);
    result =
	s3::ips_socket_connect_withtimeout(fd, (struct sockaddr *) &serv_addr,
				 sizeof(serv_addr), (int)s->connect_timeout_sec);
    if(result) {
	DBMS1(stdout,
	      __func__ " : connect_withtimeout detect connection timeout" EOL_CRLF);
	status = errno;
	goto out;
    }

    /* バッファ確保 */
    s->send_bufsize = IPS_SMTP_DEFAULT_SOCKETBUFSIZE;
    s->send_buf = (char*)malloc(s->send_bufsize);
    if (NULL == s->send_buf) {
	DBMS1(stdout, __func__ " : malloc send_buf fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    }
    memset(s->send_buf, 0x0, s->send_bufsize);

    s->recv_bufsize = IPS_SMTP_DEFAULT_SOCKETBUFSIZE;
    s->recv_buf = (char*)malloc(s->recv_bufsize);
    if (NULL == s->recv_buf) {
	DBMS1(stdout, __func__ " : malloc recv_buf fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    }
    memset(s->recv_buf, 0x0, s->recv_bufsize);
    s->command_timeout_sec = 10;

    /* WELCOME messageを待ってみる */
    result = sock_recv_txt(fd, s->recv_buf, s->recv_bufsize, (int)s->command_timeout_sec);
    if (result) {
	time_t t;

	if ((t = time(NULL) - stime) < s->command_timeout_sec) {
	    /* TIMEOUT時間経っていないのにタイムアウトしたのでやり直し */
	    s->command_timeout_sec -= t;

	    DBMS3(stdout, __func__ " : remain time = %d" EOL_CRLF, (int)s->command_timeout_sec);

	    /* socket shutdown */
	    result = s3::ips_socket_shutdown(fd, SHUT_RDWR);
	    if (result && (ENOTCONN != result)) {
		DBMS1(stdout, __func__ " : shutdown socket fail, strerror=%s" EOL_CRLF, strerror(result));
		status = -1;
		goto out;
	    }

	    /* close socket */
	    result = s3::ips_socket_close(fd);
	    if (result) {
		DBMS1(stdour, __func__ " : s3::ips_socket_close, strerror=%s" EOL_CRLF, strerror(errno));
	    }

	    s->fd = -1;
	    goto loop;
	} else {
	    DBMS3(stdout, __func__ " : time=" FMT_LLD EOL_CRLF,
		  (long long)(time(NULL) - stime));
	    DBMS1(stsout,
		  __func__ " : sock_recv_txt(Welcome mess.) fail" EOL_CRLF);
	    status = -1;
	    goto out;
	}
    }

    DBMS5(stdout, __func__ " : welcome message = %s" EOL_CRLF, s->recv_buf);

    /* 応答をチェックする */
    result = s3::ips_smtp_client_noopcheck(s);
    if (result) {
	DBMS1(stsout, __func__ " : sms_sendmailnoopcheck fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    /* サーバに呼びかける */
    sprintf(s->send_buf, "HELO %s\r\n", s->smtp_servername);
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, s->command_timeout_sec);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
	DBMS1(stdout, __func__ " : smtp_response_analyze fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    if (result > 0) {
	DBMS2(stdout, __func__ " : responce of smtp_server is %s " EOL_CRLF,
	      self_p->recv_buf);

	DBMS2(stdout, __func__ " : so disconnect server" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

  out:

    if (NULL != ai) {
	freeaddrinfo(ai);
	ai = NULL;
    }

    if (status) {
	DBMS1(stdout, __func__ " : Can't connect to SMTP Server ->%s "EOL_CRLF,
	      servername);
	s3::ips_smtp_client_disconnect(self_p);
    }

    return status;
}


/**
 * @fn int s3::ips_smtp_client_disconnect( s3::ips_smtp_client_t *const self_p)
 * @brief サーバーに切断することを伝えて切断処理します
 * @param s3::ips_smtp_client_tインスタンスポインタ
 * @retval  0 成功
 * @retval -1  失敗
 **/
int s3::ips_smtp_client_disconnect( s3::ips_smtp_client_t *const self_p)
{
    int result, status;
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;

    if (s->stat.f.is_opened) {
	/* サーバに呼びかける */
	sprintf(s->send_buf, "QUIT\r\n");
	result =
	    send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
	if (result) {
	    DBMS1(stdout, __func__ " : smtp_txt_cmd fail, strerror=%s" EOL_CRLF,strerror(result));
	    status = -1;
	    goto out;
	}

	result = smtp_response_analyze(s->recv_buf);
	if (result < 0) {
	    DBMS1(stdout, __func__ " : smtp_response_analyze fail, strerror=%s" EOL_CRLF, strerror(result));
	    status = -1;
	    goto out;
	}
    }

  out:

    if (!(s->fd < 0)) {
	/* socket shutdown */
	result = s3::ips_socket_shutdown(s->fd, SHUT_RDWR);
	if (result && (ENOTCONN != result)) {
	    DBMS1(stdout,
		  __func__ " : shutdown socket(fd:%d) fail %d" EOL_CRLF, s->fd,
		  result);
	}

	/* close socket */
	result = s3::ips_socket_close(s->fd);
	if (result) {
	    DBMS1(stdout, __func__ " : s3::ips_socket_close(fd:%d) fail %d" EOL_CRLF, s->fd,
		  result);
	}
	s->fd = -1;
    }

    /* バッファ開放 */
    if (NULL != s->send_buf) {
	free(s->send_buf);
	s->send_buf = NULL;
    }

    if (NULL != s->recv_buf) {
	free(s->recv_buf);
	s->recv_buf = NULL;
    }

    return 0;
}

/**
 * @fn int s3::ips_smtp_client_seq1_preprocess( s3::ips_smtp_client_t *const self_p, char *from_addr, char *to_addr)
 * @brief メールの送信開始サーバーとの前処理
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @param from_addr 送信元アドレス
 * @param to_addr 送信先アドレス
 * @retval 0 成功
 * @retval -1 失敗
 **/
int s3::ips_smtp_client_seq1_preprocess( s3::ips_smtp_client_t *const self_p, char *from_addr, char *to_addr)
{
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;
    int result, status;

    /* 送信元 */
    sprintf(s->send_buf, "MAIL FROM:%s\r\n", from_addr);
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    DBMS5(stdout, __func__ " : smtp_response_analyze(FROM) = %d" EOL_CRLF, result);
    if (result < 0) {
	DBMS1(stdout,
	      __func__ " : smtp_response_analyze(FROM) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    /* 送信先 */
    sprintf(s->send_buf, "RCPT TO:%s\r\n", to_addr);
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd(%s) fail" EOL_CRLF, s->send_buf);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
	DBMS1(stdout,
	      __func__ " : smtp_response_analyze(%s) fail" EOL_CRLF, s->recv_buf);
	status = -1;
	goto out;
    }

    /* DATA打ち込み */
    sprintf(s->send_buf, "DATA\r\n");
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd(%s) fail" EOL_CRLF, s->send_buf);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
	DBMS1(stdout,
	      __func__ " : smtp_response_analyze(%s) fail" EOL_CRLF, s->recv_buf);
	status = -1;
	goto out;
    }

    status = 0;
  out:

    if (status) {
	/* 現在発行中の送信状態をリセット */
	result = s3::ips_smtp_client_abort(self_p);
	if (result) {
	    DBMS1(stdout, __func__ " : s3::ips__smtp_client_abort fail" EOL_CRLF);
	    return -1;
	}
    }

    return status;
}


/**
 * @fn int s3::ips_smtp_client_abort(s3::ips_smtp_client_t *const self_p)
 * @brief メールの送信を中断して、送信したパラメータを破棄させます。
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 **/
int s3::ips_smtp_client_abort(s3::ips_smtp_client_t *const self_p)
{
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;
    int result, status;

    /* 送信先 */
    sprintf(s->send_buf, "RSET\r\n");
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
	DBMS1(stdout,
	      __func__ " : smtp_response_analyze(TO) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

  out:

    return status;
}


/**
 * @fn int s3::ips_smtp_client_noopcheck(s3::ips_smtp_client_t *const self_p)
 * @brief サーバのOKを要求します
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int s3::ips_smtp_client_noopcheck(s3::ips_smtp_client_t *const self_p)
{
    s3::ips_smtp_client_t *const s = self_p;
    int result, status;

    /* NOP */
    sprintf(s->send_buf, "NOOP\r\n");
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, s->command_timeout_sec);
    if (result) {
	DBMS1(stdout, __func__ " : smtp_txt_cmd fail, strerror=%s" EOL_CRLF, strerror(result));
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
	DBMS1(stdout,
	      __func__ " : smtp_response_analyze(TO) fail, strerror=%s" EOL_CRLF, strerror(result));
	status = -1;
	goto out;
    }

    status = 0;

  out:

    return status;
}


/**
 * @fn int s3::ips_smtp_client_seq2_header(s3::ips_smtp_client_t *const self_p, char *const date, char *const from_addr, char *const to_addr, char *const subject)
 * @brief サーバーにメール送信のためのヘッダ情報を送信します
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @param date 送信日
 * @param from_addr 送信元アドレス
 * @param to_addr 送信先アドレス
 * @param subject 表題
 * @retval 0 成功
 * @retval -1 失敗
 **/
int s3::ips_smtp_client_seq2_header(s3::ips_smtp_client_t *const self_p, char *const date, char *const from_addr, char *const to_addr, char *const subject)
{
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;
    int result, status;

    /* date */
    sprintf(s->send_buf, "Date: %s\r\n", date);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_send_txt(Date) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
#if 0
    DBMS3(stdout, "%s", send_buf);
#endif

    /* form */
    sprintf(s->send_buf, "From: %s\r\n", from_addr);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_send_txt(From) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    /* to */
    /* 複数送信時は , で区切る */
    sprintf(s->send_buf, "To: %s\r\n", from_addr);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_send_txt(To) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    /* Subject */
#if 0
#ifdef USE_JPN
    sjis2jis(&tmp[0][0], mailctrl.subject);	// SJIS -> JIS
    encBase64(&tmp[1][0], &tmp[0][0], strlen(tmp[0][0]));	// BASE64エンコード
    strcpy(mail_buf, "Subject: =?ISO-2022-JP?B?");	// 文字コードとBASE64エンコード指定
    strcat(mail_buf, &tmp[1][0]);	// エンコードされたサブジェクト
    strcat(mail_buf, "?=\r\n");
#else
    strcpy(mail_buf, "Subject: ");
    strcat(mail_buf, mailctrl.subject);
    strcat(mail_buf, "\r\n");
#endif
#endif

    sprintf(s->send_buf, "Subject: %s\r\n", subject);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout, __func__ " : sock_send_txt(To) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    /* MIME */
    sprintf(s->send_buf, "MIME-Version: 1.0\r\n");
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME Ver) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
#if 1
#define MIME_PART "unique-boundary-1"
    sprintf(s->send_buf, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n",
	    MIME_PART);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    sprintf(s->send_buf, "--%s\r\n", MIME_PART);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }
#endif

    sprintf(s->send_buf, "Content-Type: Text/Plain; charset=US-ASCII\r\n");
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    sprintf(s->send_buf, "Content-Transfer-Encoding: 7bit\r\n");
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    sprintf(s->send_buf, "\r\n");
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

out:

    return status;
}


/**
 * @fn int s3::ips_smtp_client_seq4_txtend( s3::ips_smtp_client_t *const self_p)
 * @brief メール本体送信終了のフッタ情報を送信します
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @retval 0 成功
 * @retval 0以外 失敗
 **/
int s3::ips_smtp_client_seq4_txtend( s3::ips_smtp_client_t *const self_p)
{
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;
    int result, status;

#if 1
    /* end MIME PART */
    sprintf( s->send_buf, "--%s--\r\n", MIME_PART);
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	// DBMS1( stdout, __func__ " : smtp_txt_cmd(%s) fail" EOF_CRLF, s->send_buf);
	status = -1;
	goto out;
    }
#endif

    /* . */
    sprintf(s->send_buf, ".\r\n");
    result = send_txt_cmd(s->fd, s->send_buf, s->recv_buf, s->recv_bufsize, timeout);
    if (result) {
//	DBMS1(stdout, __func__ " : smtp_txt_cmd fail" EOF_CRLF);
	status = -1;
	goto out;
    }

    result = smtp_response_analyze(s->recv_buf);
    if (result < 0) {
//	DBMS1(stdout, __func__ " : smtp_response_analyze fail" EOF_CRLF);
	status = -1;
	goto out;
    }

    status = 0;

  out:

    return status;
}

/**
 * @fn int s3::ips_smtp_client_seq3_txtline( s3::ips_smtp_client_t *const self_p, char *linedat, int size )
 * @brief メールの本文を一行送信します
 * @param self_p s3::ips_smtp_client_tインスタンスポインタ
 * @param linedat メール本文の文字列データ
 * @param linedatサイズ
 * @retval 0 成功
 * @retval 0以外 失敗
 **/
int s3::ips_smtp_client_seq3_txtline( s3::ips_smtp_client_t *const self_p, char *linedat, int size )
{
    s3::ips_smtp_client_t *const s = self_p;
    const int timeout = (int)self_p->command_timeout_sec;
    char *pl;
    int result, status;
    size_t cnt;
    char *dat = NULL;

    dat = (char*)malloc(size);
    if(NULL == dat) {
	DBMS1(stderr, __func__ " : malloc(dat) fail" EOL_CRLF);
	return ENOMEM;
    }

    /* linedatに複数の\nが出たらエラー */
    pl = linedat;
    cnt = 0;
    while (NULL != (pl = strpbrk(pl, "\n"))) {
	cnt++;
	pl++;
    }
    DBMS5(stdout, __func__ " : cnt =" FMT_LLU EOL_CRLF, (long long)cnt);

    if (cnt > 1) {
#if 0
	DBMS2(stdout, "sms_sendmailtxtline : \\n cnt = %d" EOF_CRLF, cnt);
	return -1;
#endif
    }

    /* linedatが254を越えたらエラー */
    if (strlen(linedat) > 254) {
	DBMS1(stdout, __func__ " : linedat length=" FMT_LLU EOL_CRLF,
	      (long long)strlen(linedat));
	status = -1;
	goto out;
    }

    strcpy(dat, linedat);

    /* .だけになったら、変更 */
    if (!strcmp(dat, ".")) {
	strcpy(dat, "..");
    }

    /* \r\n付きの文字列を作成 */
    sprintf(s->send_buf, "%s\r\n", dat);

    /* 行を送信 */
    result = sock_send_txt(s->fd, s->send_buf, timeout);
    if (result) {
	DBMS1(stdout,
	      __func__ " : sock_send_txt(MIME ContType) fail" EOL_CRLF);
	status = -1;
	goto out;
    }

    status = 0;
out:
    if( NULL == dat ) {
	    free(dat);
    }

    return  status;
}
