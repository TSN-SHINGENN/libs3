#ifndef MULTIOS_TCPIP_MPR1_DEF_H
#define MULTIOS_TCPIP_MPR1_DEF_H

#include <stddef.h>
#include <stdint.h>

#define MULTIOS_TCPIP_MPR1_MAGIC_NO  (('M' << 24)|('R' << 16)|('1' << 8)| ('0'))
#define MULTIOS_TCPIP_MPR1_MAGIC_HDR (('0' << 24)|('0' << 16)|('0' << 8)|('\0'))

/* TCP/IP 通信ポート */
#define MULTIOS_TCPIP_MPR1_DEFAULT_TCP_PORT 57000

/* Socket ネットワーク 最大パケットサイズ */
#define MULTIOS_TCPIP_MPR1_MAX_PACKET_SIZE (64 * 1024)	    
#define MULTIPS_TCPIP_MPR1_MAX_PACKET_LEGACY_SIZE ( 7 * 1024 ) 
#define MULTIOS_TCPIP_MPR1_SOCKET_BUF_SIZE (MULTIPS_TCPIP_MPR1_MAX_PACKET_LEGACY_SIZE +1024)
#define MULTIOS_TCPIP_MPR1_MEMIO_DSIZE (1024 * 6)
#define MULTIOS_TCPIP_MPR1_CMDIO_DSIZE (1024 * 6)
#define MULTIOS_TCPIP_MPR1_MEMIO_FRAME_PAD 128
#define MULTIOS_TCPIP_MPR1_IOCTL_DSIZE 128

/* 通信リトライ回数定義 */

#define MULTIOS_TCPIP_MPR1_BIND_MAX_TRY 1000
#define MULTIOS_TCPIP_MPR1_MEMIO_RETRY 1
#define MULTIOS_TCPIP_MPR1_OPEN_RETRY 3
#define MULTIOS_TCPIP_MPR1_RETRY_COUNT 1

#define MULTIOS_TCPIP_MPR1_CLIENT_ALIVE_CHK_INTERVAL ( 10 * 60)
#define MULTIOS_TCPIP_MPR1_TIMEOUT_RETRY_INTERVAL 100
#define MULTIOS_TCPIP_MPR1_STREAM_READ_TIMEOUT 20

/* メッセージタイプ 定義 */
#define	MULTIOS_TCPIP_MPR1_REQUEST_MSG	0
#define	MULTIOS_TCPIP_MPR1_REPLY_MSG	1

/* リクエストタイプ 定義 */
#define MULTIOS_TCPIP_MPR1_CMDIO_REQ	0
#define MULTIOS_TCPIP_MPR1_IOCTL_REQ	2
#define MULTIOS_TCPIP_MPR1_MEMIO_REQ	3
#define MULTIOS_TCPIP_MPR1_MEMIO_END_REQ	4
#define MULTIOS_TCPIP_MPR1_OPEN_REQ	7
#define MULTIOS_TCPIP_MPR1_CLOSE_REQ	8
#define MULTIOS_TCPIP_MPR1_ALIVE_CHK_REQ	9

/* フレームタイプ定義 */
#define MULTIOS_TCPIP_MPR1_NULL_FRAME	0
#define MULTIOS_TCPIP_MPR1_CMDIO_FRAME	1
#define MULTIOS_TCPIP_MPR1_IOCTL_FRAME	2
#define MULTIOS_TCPIP_MPR1_MEMIO_FRAME	3
#define MULTIOS_TCPIP_MPR1_DATA_FRAME	5
#define MULTIOS_TCPIP_MPR1_OPEN_FRAME	4
#define MULTIOS_TCPIP_MPR1_CLOSE_FRAME     5

/* リプライメッセージ 定義 */
#define MULTIOS_TCPIP_MPR1_REPLY_SUCCESS	0 
#define MULTIOS_TCPIP_MPR1_REPLY_ERROR	1
#define MULTIOS_TCPIP_MPR1_REPLY_RETRY	2
#define MULTIOS_TCPIP_MPR1_REPLY_STOP	3

/* エラーコード定義 */
#define MULTIOS_TCPIP_MPR1_CONNECT	4
#define MULTIOS_TCPIP_MPR1_CLIENTCLOSED	3
#define MULTIOS_TCPIP_MPR1_EOF		2
#define MULTIOS_TCPIP_MPR1_RETRY	1
#define MULTIOS_TCPIP_MPR1_CHILD        0
#define MULTIOS_TCPIP_MPR1_NO_ERR	0
#define MULTIOS_TCPIP_MPR1_UNCONNECT   -1
#define MULTIOS_TCPIP_MPR1_ERR_FAIL    -1
#define MULTIOS_TCPIP_MPR1_ERR_SCC	    -3
#define MULTIOS_TCPIP_MPR1_ERR_PACKET_DESTORY -4
#define MULTIOS_TCPIP_MPR1_ERR_ID	    -6
#define MULTIOS_TCPIP_MPR1_ERR_HD      -7
#define MULTIOS_TCPIP_MPR1_ERR TIMEOUT -8
#define MULTIOS_TCPIP_MPR1_ERR_CONNECT -9
#define MULTIOS_TCPIP_MPR1_ERR_DATA    -10
#define MULTIOS_TCPIP_MPR1_ERR_UNCONNECT -11

#define MULTIOS_TCPIP_MPR1_ERR_SLENGTH 9998
#define MULTIOS_TCPIP_MPR1_ERR_ERR 9999

/* メモリI/O dir 定義 */
#define MULTIOS_TCPIP_MPR1_DIR_READ	0
#define MULTIOS_TCPIP_MPR1_DIR_WRITE	1
#define MULTIOS_TCPIP_MPR1_DIR_RW		2

/* MEMIO 方式 設定 */
#define MULTIOS_TCPIP_MPR1_PAC64
#define MULTIOS_TCPIP_MPR1_PACCUNT 88

/*
	メッセージヘッダ 定義
*/
typedef struct _multios_tcpip_mpr1_msg_hdr {
    union {
	uint32_t pad[20];
	struct {
	    uint8_t magic_no[4]; /* マジックナンバー */
	    uint8_t magic_hd[4]; /* ハード識別 */
	    uint8_t msg;         /* メッセージタイプ */
	    uint8_t req;         /* リクエストタイプ */
	    uint8_t reply;       /* リプライタイプ */
	    uint8_t error[4];    /* エラーコード */
	    uint8_t ftype;       /* フレームタイプ */
	    uint8_t fsize[4];    /* フレームサイズ */
	} param; 
    } u;
} multios_tcpip_mpr1_msg_hdr_t;

#define MULTIOS_TCPIP_MPR1_HDR_SIZE (sizeof(multios_tcpip_mpr1_msg_hdr_t))

/*
	個別フレーム定義
*/

/* メモリI/O フレーム */
typedef struct _multios_tcpip_mpr1_memio_frame {
    union {
	uint32_t pad[32];
	struct {
	    uint8_t mem_type[4];	/* memory type */
	    uint8_t pad1[3];
	    uint8_t dir;		/* memory transfer direction */
	    uint8_t size[4];	/* I/O size */
	    uint8_t segs[4];
	    uint8_t offs[4];	/* byte offset */
	    uint8_t left_size[4];
	    uint8_t remain_cnt[4];
	    uint8_t memiob_size[4]; /* add 2011.09 */
	} param;
    } u;
    uint8_t data[MULTIOS_TCPIP_MPR1_MEMIO_FRAME_PAD];
} multios_tcpip_mpr1_memio_frame_t;

/* コマンドI/O フレーム */
typedef struct _multios_tcpip_mpr1_cmdio_frame {
    union {
	uint32_t pad[16];
	struct {
	    uint8_t cmdln_no[4];
	    uint8_t pad[3];
	    uint8_t dir;		/* command I/O direction */
	    uint8_t size[4];	/* command code size */
	    uint8_t rdsize[4];	/* command code size */
	} param;
    } u;
    uint8_t data[MULTIOS_TCPIP_MPR1_CMDIO_DSIZE];
} multios_tcpip_mpr1_cmdio_frame_t;

/* I/O コントロール フレーム */
typedef struct _multios_tcpip_mpr1_ioctl_frame {
    union {
	uint32_t pad[12];
	struct {
	    uint8_t cmd[4];
	    uint8_t pad[4];
	    uint8_t size[4];
	} param;
    } u;
    uint8_t data[MULTIOS_TCPIP_MPR1_CMDIO_DSIZE];
} multios_tcpip_mpr1_ioctl_frame_t;

/* オープンフレーム */
typedef struct _multios_tcpip_mpr1_open_frame {
    union {
	uint32_t pad[12];
	struct {
	    char pad1[7];
	    char dev_id;
	    char pad2[4];
	    char user_name[16];
	    char user_passwd[16];
	    char sockbufffer_size[4];
	} param;
    } u;
} multios_tcpip_mpr1_open_frame_t;

/* クローズフレーム */
typedef struct _multios_tcpip_mpr1_close_frame {
    union {
	uint32_t pad[12];
	struct {
	    char pad[7];
	    char dev_id;
	} param;
    } u;
} multios_tcpip_mpr1_close_frame_t;

/* サーバープログラム子プロセスステータス */
typedef struct _multios_tcpip_mpr1_server_child_ps_status {
    int open_flag;
} multios_tcpip_mpr1_server_child_ps_status_t;

/*
	TCP/IP パケットフォーマット 定義
*/

typedef struct _multios_tcpip_mpr1_msg {
    multios_tcpip_mpr1_msg_hdr_t hdr;
    union {
	multios_tcpip_mpr1_memio_frame_t memio;
	multios_tcpip_mpr1_ioctl_frame_t ioctl;
	multios_tcpip_mpr1_cmdio_frame_t cmdio;
	multios_tcpip_mpr1_close_frame_t close;
	multios_tcpip_mpr1_open_frame_t open;
  } frame;
} multios_tcpip_mpr1_msg_t;

typedef struct _multios_tcpip_mpr1_memiob_data {
    char data[MULTIOS_TCPIP_MPR1_MAX_PACKET_SIZE];
} multios_tcpip_mpr1_memiob_data_t;

/* ライブラリ内メッセージ定義 */
#define MULTIOS_TCPIP_MPR1_SCC_OFF 0
#define MULTIOS_TCPIP_MPR1_SCC_ON 1

#define MULTIOS_TCPIP_MPR1_GET_FRAMESIZE(pmsg)	(MULTIOS_GET_BE_D32((pmsg)->hdr.u.param.fsize))
#define MULTIOS_TCPIP_MPR1_GET_MSGSIZE(pmsg)	(MULTIOS_TCPIP_MPR1_HDR_SIZE +\
					  (MULTIOS_GET_BE_D32((pmsg)->hdr.u.param.fsize)))
#define MULTIOS_TCPIP_MPR1_BUF_CLEAR(p)  memset((void *)(p),0,MULTIOS_TCPIP_MPR1_MAX_PACKET_SIZE)

#endif /* end of MULTIOS_TCPIP_MPR1_DEF_H */
