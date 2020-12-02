#ifndef INC_MULTIOS_TCPIP_MPR2_DEF_H
#define INC_MULTIOS_TCPIP_MPR2_DEF_H

#include <stddef.h>
#include <stdio.h>

#define MPR2_MAGIC_NO (('M' << 24)|('P' << 16)|('R' << 8)| ('2'))
/* UDR Socket Protocol Version */
#define UDRSOCK2_MAGIC_HD (('U' << 24)|('2' << 16)|('2' << 8)|('\0'))

/* 変数定義 */
#define int32_t int

/*	TCP/IP 通信ポート */
#define UDRSOCK2_TCP_PORT 57001
#define UDRSOCK2_SUBIO_MAX_SESSIONS 16
#define UDRSOCK2_SUBIO_NUM_PORTS 16

/*
	Socket ネットワーク 最大パケットサイズ
*/

#define UDRSOCK2_MESSAGE_PACKET_MAX_SIZE (7 * 1024)
#define UDRSOCK2_MESSAGE_PACKET_BUF_SIZE (UDRSOCK2_MESSAGE_PACKET_MAX_SIZE +1024)
#define UDRSOCK2_CMDIO_DSIZE (1024 * 6)
#define UDRSOCK2_MEMIO_FRAME_PAD 128
#define UDRSOCK2_IOCTL_DSIZE 128
#define UDRSOCK2_HDR_SIZE (sizeof(udrsock2_msg_hdr_t))

/* 通信リトライ回数定義 */

#define UDRSOCK2_BIND_MAX_TRY 1000

#define UDRSOCK2_C_ALIVE_CHK_INTERVAL ( 10 * 60)
#define UDRSOCK2_TIMEOUT_RETRY_INTERVAL 150
#define UDRSOCK2_STREAM_READ_TIMEOUT 150

/* メッセージタイプ 定義 */
#define	UDRSOCK2_REQUEST_MSG	0
#define	UDRSOCK2_REPLY_MSG	1

/* リクエストタイプ 定義 */
#define UDRSOCK2_CMDIO_REQ	0
#define UDRSOCK2_IOCTL_REQ	2
#define UDRSOCK2_MEMIO_REQ	3
#define UDRSOCK2_MEMIO_END_REQ	4
#define UDRSOCK2_INIT_REQ	5
#define UDRSOCK2_NULL		10

/* フレームタイプ定義 */
#define UDRSOCK2_NULL_FRAME	0
#define UDRSOCK2_CMDIO_FRAME	1
#define UDRSOCK2_IOCTL_FRAME	2
#define UDRSOCK2_MEMIO_FRAME	3
#define UDRSOCK2_INIT_FRAME     4

/* リプライメッセージ 定義 */
#define UDRSOCK2_REPLY_SUCCESS	0 
#define UDRSOCK2_REPLY_ERROR	1
#define UDRSOCK2_REPLY_RETRY	2
#define UDRSOCK2_REPLY_STOP	3

/* エラーコード定義 */
#define UDRSOCK2_CONNECT      4
#define UDRSOCK2_CCLOSE	     3
#define UDRSOCK2_EOF	     2
#define UDRSOCK2_RETRY	     1
#define UDRSOCK2_CHILD        0
#define UDRSOCK2_NO_ERR	     0
#define UDRSOCK2_UNCONNECT   -1
#define UDRSOCK2_ERR_FAIL    -1
#define UDRSOCK2_ERR_SCC	    -3
#define UDRSOCK2_ERR_PACKET_DESTRY -4
#define UDRSOCK2_ERR_ID	    -6
#define UDRSOCK2_ERR_HD      -7
#define UDRSOCK2_ERR TIMEOUT -8
#define UDRSOCK2_ERR_CONNECT -9
#define UDRSOCK2_ERR_DATA    -10
#define UDRSOCK2_ERR_UNCONNECT -11

#define UDRSOCK2_ERR_SLENGTH 9998
#define UDRSOCK2_ERR_ERR 9999

/* メモリI/O dir 定義 */
#define UDRSOCK2_DIR_READ	0
#define UDRSOCK2_DIR_WRITE	1
#define UDRSOCK2_DIR_RW		2

#ifdef WIN32
#pragma pack(push, 1) /* visal c++ */
#else
#pragma pack(push, 1) /* gcc */
#endif

/*
	メッセージヘッダ 定義
*/
typedef struct _udrsock2_msg_hdr {
    uint32_t magic_no;		/* マジックナンバー */
    uint32_t magic_hd;		/* ハード識別 */
    uint8_t  msg_type;		/* メッセージタイプ */
    uint8_t  request_type;		/* リクエストタイプ */
    uint8_t  reply_type;	/* リプライタイプ */
    uint8_t  pad1;
    uint32_t result_code;	/* エラーコード */
    uint8_t  frame_type;	/* フレームタイプ */
    uint8_t  pad2[3];
    uint32_t frame_len;		/* フレームサイズ */
    uint32_t msec;		/* ヘッダーを作成したmsec秒 */
    uint32_t crc32;		/* ヘッダのCRC32C値 */
} udrsock2_msg_hdr_t;

/*
	個別フレーム定義
*/

/* メモリI/O フレーム */
typedef struct _udrsock2_memio_frame {
    uint32_t mem_type;	/* memory type */
    uint8_t  pad1[3];
    uint8_t  dir;		/* memory transfer direction */
    uint32_t data_len;	/* I/O size */
    uint32_t mem_segs;
    uint32_t mem_offs;	/* byte offset */
    uint32_t left_len;
    uint32_t num_loops;
    uint32_t memiob_len; /* add 2011.09 */
} udrsock2_memio_frame_t;

/* コマンドI/O フレーム */
typedef struct _udrsock2_cmdio_frame {
    uint32_t cmdln_no;
    uint8_t pad[3];
    uint8_t dir;		/* command I/O direction */
    uint32_t send_len;		/* command code size */
    uint32_t reply_len_max;	/* command code size */
    uint8_t data[UDRSOCK2_CMDIO_DSIZE];
} udrsock2_cmdio_frame_t;

/* initフレーム */
typedef struct _udrsock2_init_frame {
    uint8_t subio_num_sessions;    /* クライアントからサーバーへMEMIOセッション数を要求 */
    uint8_t pad[3];
    uint32_t subio_socket_port_no; /* サーバから返される接続ポート待ち受け番号 */
} udrsock2_init_frame_t;

/* I/O コントロール フレーム */
typedef struct _udrsock2_ioctl_frame {
    uint32_t cmd;
    uint8_t pad[4];
    uint32_t size;
    uint8_t data[UDRSOCK2_IOCTL_DSIZE];
} udrsock2_ioctl_frame_t;

/* サーバープログラム子プロセスステータス */
struct udrsock2_server_child_ps_status {
    uint32_t open_flag;
};

/*
	TCP/IP パケットフォーマット 定義
*/

typedef struct udrsock2_msg {
    udrsock2_msg_hdr_t hdr;
    union {
	udrsock2_init_frame_t  init;
  	udrsock2_memio_frame_t memio;
	udrsock2_ioctl_frame_t ioctl;
	udrsock2_cmdio_frame_t cmdio;
  } frame;
} udrsock2_msg_t;

#ifdef WIN32
#pragma pack(pop) /* visual c++ */
#else
#pragma pack(pop) /* gcc */
#endif

#define UDRSOCK2_GET_FRAME_SIZE(pmsg)	(ntohl((pmsg)->hdr.frame_len))
#define UDRSOCK2_GET_MSG_SIZE(pmsg)	(sizeof(udrsock2_msg_hdr_t) +\
					  ntohl((pmsg)->hdr.frame_len))

#endif /* INC_MULTIOS_TCPIP_MPR2_DEF_H */
