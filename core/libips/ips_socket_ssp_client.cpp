/**
 *	Copyright 2017 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author : Seiichi Takeda '2017-Febrary-15
 *		Last Alteration $Author: takeda $
 */

/**
 * @file socket_ssp_client.c
 * @brief 文字列でメッセージパッシングを行う為のクライアント側ライブラリ。
 *	NULL文字で終端を示す、メッセージ通信を行います。
 **/

//#include "stdafx.h"

#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning(disable:4996)
#pragma warning ( disable:4819 )
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <winsock2.h>

/* this */
#include <core/libmultios/multios_sys_types.h>
#include <core/libmultios/multios_socket.h>

#include "ips_socket_ssp_client.h"

/**
 * @fn static int own_connect_option(const int fd, void *const args)
 * @brief ソケット接続時のオプション
 */
static int own_connect_option(const int fd, void *const args)
{
    return multios_socket_set_nagle(fd, 0);
}

/**
 * @fn int socket_ssp_client_connect(const char *const IpAddress, const int tcp_port, const int timeout_sec)
 * @brief 指定のIPアドレス及びTCPポート番号へ接続します
 * @retval 0以上 成功(ソケットファイルディスクプリタ)
 * @retval -1 失敗(errno参照)
 **/
int socket_ssp_client_connect(const char *const IpAddress, const int tcp_port, const int timeout_sec)
{
    return multios_socket_client_open( IpAddress, tcp_port, timeout_sec, PF_INET, SOCK_STREAM, 0,
	    own_connect_option,  NULL);
}

/**
 * @fn int socket_ssp_client_send_cmdstr(const int socket_fd, const char *const command, const int timeout_sec)
 * @brief サーバにコマンドを送ります。
 * @retval 0 サーバーとの接続が切れた
 * @retval -1 失敗(errno参照)
 **/
int socket_ssp_client_send_cmdstr(const int socket_fd, const char *const command, const int timeout_sec)
{
    const size_t Len = strlen(command);
    int result;
    size_t n;
    const char *p;

   if( Len == 0 ) {
	errno = EINVAL;
	return -1;
   }

    n = Len;
    p = command;
    do while(n!=0) {
	result = multios_socket_send_withtimeout(socket_fd, p, n, 0x0, timeout_sec);
	if(result<=0) {
	    return -1;
	}
	n -= result;
	p += result;
    } while(n!=0); 

    return 0;
}

/**
 * @fn int socket_ssp_client_recv_reply(const int socket_id, char *const recv_buf, const size_t bufsize, const int timeout_sec)
 * @brief サーバにコマンドを送ります。
 * @retval 0 サーバーとの接続が切れた
 * @retval -1 失敗(errno参照)
 **/
int socket_ssp_client_recv_reply(const int socket_id, char *const recv_buf, const size_t bufsize, const int timeout_sec)
{
    int result;
    size_t n;
    char *p = recv_buf;

    for (n = 0; n < (bufsize-1); ++n) {
	result = multios_socket_recv_withtimeout(socket_id, p, 1, MSG_WAITALL, timeout_sec);
	if(result<=0 ) {
	    *p = '\0';
	    return -1;
	} else if (*p == '\n'){
	    ++p;
	    *p = '\0';
	    return 0;
	} else if (*p == '\0' ){
	    return 0;
	}

	++p;
    }
    *p = '\0';

    return 0;
}

/**
 * @fn int socket_ssp_client_disconnect(const int socket_id);
 * @brief サーバ機器とのTCPソケット接続を切断します。
 * @retval 0 サーバーとの接続が切れた
 * @retval -1 失敗(errno参照)
 **/
int socket_ssp_client_disconnect(const int socket_id)
{
    int result;
    result = multios_socket_shutdown(socket_id, SHUT_RDWR);
    if(result) {
	return result;
    }

    return multios_socket_close(socket_id);
}



