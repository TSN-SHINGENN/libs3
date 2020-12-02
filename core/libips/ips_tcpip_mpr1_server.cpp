/**
 *	Copyright 2000 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2K-April-15 Active
 *		Last Alteration $Author: takeda $
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/wait.h>
#endif

#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>
#include <time.h>

/* QNX */
#if defined(QNX) || defined(Linux) || defined(Darwin)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

/* this */
#include "multios_sys_types.h"
#include "multios_endian.h"
#include "multios_unistd.h"
#include "multios_malloc.h"
#include "multios_socket.h"
#include "multios_simd.h"
#include "multios_simd_cpu_hint.h"
#include "multios_string.h"
#include "multios_process.h"

#include "multios_tcpip_mpr1_def.h"
#include "multios_tcpip_mpr1_server.h"

#ifdef DEBUG
static int debuglevel = 5;
#else
static int debuglevel = 0;
#endif
#include "dbms.h"

static int _mpr1_msg_recv_s(multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_msg_reply_s(const multios_tcpip_mpr1_msg_t *const send_m);
static int _mpr1_open_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_close_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_cmdio_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_ioctl_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static void _mpr1_child_close_s(void);


#ifndef UDRSOCK_PAC64
static int _mpr1_memio_req_s(multios_tcpip_mpr1_msg_t *const recv_m);	/* memio �����С�¦ */
#else
static int _mpr1_memio_req_s(multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_memio_write_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_memio_read_req_s(const multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_memio_multipac_s(const char trans_dir, int memb_offs,
				    int trans_loop_count, int left_size,
				    int data_size);
static int _mpr1_memio_trans_start_req_s(const multios_tcpip_mpr1_msg_t *const recv_m,
					   int flag);
static int _mpr1_memio_read_trans_send_req_s(void);
static int _mpr1_memio_write_1pac_s(const unsigned int offs,
				      const int size);
static int _mpr1_memio_read_1pac_s(const unsigned int offs,
				     const int memb_size);
static int _mpr1_memio_trans_end_req_s(int trans_dir, int memb,
					 char *pmemb_buff,
					 int32_t memb_seg,
					 int32_t memb_offs,
					 int32_t memb_size);
#endif

static int _mpr1_readstream(const int fd, char *ptr, const int length);
static int _mpr1_writestream(const int fd, const char *ptr, const int bytelength);
static int _mpr1_hdr_certify(multios_tcpip_mpr1_msg_t *const recv_m);
static int _mpr1_msg_send(const int fd, const multios_tcpip_mpr1_msg_t *const send_m);
static int _mpr1_msg_recv(const int fd, multios_tcpip_mpr1_msg_t *const recv_m);

static void _mpr1_dump_msg(const multios_tcpip_mpr1_msg_t *const msg);

/* �����ѿ�E*/
static int master_sockfd = -1;
static int child_sockfd = -1;
static jmp_buf master_sjbuf;

static udrsock_xfer_function_table_t udrs_funcs =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void *read_stream_buff = NULL;
static void *write_stream_buff = NULL;
//static int max_packet_size = UDRSOCK_MAX_PACKET_SIZE;
static int max_packet_size = 0;
static multios_simd_processer_function_t cpuinfo;

static int mypid = -1;
static int udrsock_memiob_size = MULTIOS_TCPIP_MPR1_MEMIO_DSIZE;


/* signal function */
static void master_onintr_sigchld(int sig) MULTIOS_HINT_UNUSED;
static void onintr_sigint_child(int sig);
static void onintr_sigpipe(int sig);

#if defined(udrsock_packet_buff_clr)
#undef udrsock_packet_buff_clr
#endif

#if defined(INC_MULTIOS_STRING_H) 
#define packet_buff_clr(p) (multios_memset_path_through_level_cache((p), 0x0 , max_packet_size))
#define packet_buff_message_area_clr(p) (multios_memset_path_through_level_cache((p), 0x0 , MULTIPS_TCPIP_MPR1_MAX_PACKET_LEGACY_SIZE))
//#define packet_buff_message_area_clr(p) packet_buff_clr(p) /* for Debug */
#else
#define packet_buff_clr(p) (memset((p), 0x0 , max_packet_size))
#define packet_buff_message_area_clr(p) (memset((p), 0x0 , MULTIPS_TCPIP_MPR1_MAX_PACKET_LEGACY_SIZE))
#endif

static void master_onintr_sigchld(int sig)
{
    time_t t;
    char tmps[BUFSIZ];

    multios_sleep(1);

    time(&t);
    strcpy(tmps, ctime(&t));
    tmps[19] = '\0';

    DBMS4(stdout,
	  "master_onintr_sigchld : %s caught signal [SIGCHLD] %d\n",
	  tmps + 4, sig);
    DBMS4(stdout, "master_onintr_sigchld : mypid = %d\n", mypid);
    if (multios_waitpid(-1, NULL, MULTIOS_WAIT_WNOHANG | MULTIOS_WAIT_WUNTRACED) == -1) {
	DBMS4(stdout,
	      "master_onintr_sigchld : fail wait for kill process \n");
    } else {
	DBMS4(stdout,
	      "master_onintr_sigchld : Success kill child process \n");
    }

#ifndef WIN32
    signal(SIGCHLD, (void (*)()) master_onintr_sigchld);
#endif

    longjmp(master_sjbuf, 0);

}

static void onintr_sigint_child(int sig)
{
    const char *const msg = "foo\n";
    DBMS3(stdout, "onintr_sigint_child[%d]\n", sig);

    multios_socket_send_withtimeout(child_sockfd, msg, strlen(msg), MSG_OOB, 0);

    _mpr1_child_close_s();

    exit(0);

    return;
}



static void onintr_sigpipe(int sig)
{
    time_t t;
    char tmps[BUFSIZ];

    time(&t);
    strcpy(tmps, ctime(&t));
    tmps[19] = '\0';

    DBMS4(stdout, "onintr_sigpipe : %s caught signal [SIGPIPE(%d)]\n",
	  tmps + 4, sig);

    udrs_funcs.client_shutdown();

    exit(0);
}


int udrsock_open_s(const int SockPort,
		   const udrsock_xfer_function_table_t * funcs)
{
/*
    udrsock_open_s()
	Socket�̿��Υ����С�¦�Υ����ץ�E
	UDR�����С�¦�Υ����åȥݡ��Ȥ򥪡��ץ󤹤�E
    ������
	̵��
    �֤�E͡�
	̵��
*/

    int result;
    socklen_t clilen;
    struct sockaddr cli_addr;
    fd_set fds, rfds;
    struct timeval tv;

    DBMS1(stderr, "SockPort = %d\n", SockPort);

    /* init */
    clilen = sizeof(cli_addr);
    multios_simd_check_processor_function(&cpuinfo);

    if (cpuinfo.valid) {
	char buf[255];

	sprintf(buf, "support SIMD is");

	if (cpuinfo.simd.f.mmx) {
	    strcat(buf, " mmx");
	    if (cpuinfo.simd.f.sse) {
		strcat(buf, " sse");
		if (cpuinfo.simd.f.sse2) {
		    strcat(buf, " sse2");
		    if (cpuinfo.simd.f.sse3) {
			strcat(buf, " sse3");
			if (cpuinfo.simd.f.sse41) {
			    strcat(buf, " sse41");
			    if (cpuinfo.simd.f.sse42) {
				strcat(buf, " sse42");
				if (cpuinfo.simd.f.avx) {
				    strcat(buf, " avx");
				}
			    }
			}
		    }
		}
	    }
	} else {
	    strcat(buf, " none");
	}
	DBMS3(stderr, "%s\n", buf);
    }
#ifdef USE_CPU_HINT
    if( cpuinfo.simd.f.sse2 ) {
	DBMS3(stderr, "use cpu hint\n");
    }
#endif
#ifdef INC_MULTIOS_STRING_H
	DBMS3(stderr, "use multios_string\n");
#endif
    if (cpuinfo.valid && cpuinfo.simd.f.sse2) {
	max_packet_size = MULTIOS_TCPIP_MPR1_MAX_PACKET_SIZE;
    } else {
	max_packet_size = MULTIPS_TCPIP_MPR1_MAX_PACKET_LEGACY_SIZE;
    }

    DBMS3(stderr,"max_packet_size = %dki\n", max_packet_size / 1024);
    result = multios_malloc_align(&read_stream_buff, 512, max_packet_size);
    if (result) {
	DBMS1(stderr, "read_stream_buff malloc fail\n");
	return -1;
    }

    result =
	multios_malloc_align(&write_stream_buff, 512, max_packet_size);
    if (result) {
	DBMS1( stderr, "write_stream_buff malloc fail\n");
	return -1;
    }

    packet_buff_clr(read_stream_buff);
    packet_buff_clr(write_stream_buff);

    udrs_funcs.init = funcs->init;
    udrs_funcs.imemio = funcs->imemio;
    udrs_funcs.imemio_get_ptr = funcs->imemio_get_ptr;
    udrs_funcs.dwrite = funcs->dwrite;
    udrs_funcs.dread = funcs->dread;
    udrs_funcs.ioctl = funcs->ioctl;
    udrs_funcs.open = funcs->open;
    udrs_funcs.close = funcs->close;
    udrs_funcs.client_shutdown = funcs->client_shutdown;

    /* server socket */
    master_sockfd =
	multios_socket_server_openforIPv4(htonl(INADDR_ANY), SockPort, 5,
					  MULTIOS_TCPIP_MPR1_BIND_MAX_TRY,
					  PF_INET,
					  SOCK_STREAM, 0, NULL);
    if (master_sockfd < 0) {
	DBMS1(stdout,
	      "udrsock_open_s : multios_socket_server_open fail\n");
	return -1;
    }
    DBMS3(stderr,"bind succeeded, port = %d\n", SockPort);

#define QNX650_Socket_Window_MAX (32 * 1024 * 7)
    multios_socket_set_send_windowsize(master_sockfd,
				       QNX650_Socket_Window_MAX);
    multios_socket_set_recv_windowsize(master_sockfd,
				       QNX650_Socket_Window_MAX);

    /* �����ॢ���Ƚ��� �鴁E� */
    FD_ZERO(&fds);
    FD_SET(master_sockfd, &fds);

    /* ���饤����ȥ�E������ȴƻ�EΤ����̵�¥�E��� */
    setjmp(master_sjbuf);

#ifndef WIN32
    signal(SIGCHLD, SIG_IGN);
#endif


    DBMS4(stdout, "master_sockfd = %d\n", master_sockfd);

    while (1) {

	/* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	rfds = fds;

	result =
	    select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	if (result == 0) {
	    DBMS5(stdout, " open_s : socket listen mypid = %d\n", mypid);

	    continue;
	}

	if ((child_sockfd =
	     multios_socket_accept(master_sockfd,
				   (struct sockaddr *) &cli_addr,
				   &clilen)) == -1) {
	    DBMS1(stderr, "multios_socket_accept: %s\n", strerror(errno));

#if 0				/* for debug */
	    if (!(child_sockfd < 0)) {
		multios_socket_close(child_sockfd);
		child_sockfd = -1;
	    }

	    debug++;

	    if (debug > 15) {
		DBMS1( stderr, "socket connection, panic!\n");
		goto out;
	    }
#endif
	    continue;		/* �Ƶ�ư����E褦�˽񤭴��� */
	}

	mypid = multios_fork();

	/* for Debug */
	DBMS5(stdout, "mypid = %d\n", mypid);

	switch (mypid) {
	case -1:
#ifndef WIN32
	    signal(SIGCHLD, SIG_DFL);
#endif
	    DBMS1(stderr, "udrsock_open_s : create child process fail \n");
	    _mpr1_child_close_s();
	    break;
	case 0:
#ifndef WIN32
	    signal(SIGPIPE, (void (*)()) onintr_sigpipe);
	    signal(SIGINT, (void (*)()) onintr_sigint_child);
#endif
	    DBMS4(stderr, "udrsock_open_s : create child prosess \n");
	    multios_msleep(10);

	    udrs_funcs.init();

	    DBMS5(stdout, "connected\n");

	    /* set TCP_NODELAY option */
	    multios_socket_set_nagle(child_sockfd, 0);

	    udrsock_loop_s();

	    udrs_funcs.client_shutdown();

	    return 0;
	default:
	    multios_socket_close(child_sockfd);
	    multios_sleep(1);
#if 0
	    signal(SIGCHLD, (void (*)()) master_onintr_sigchld);
#endif
	    DBMS4(stdout, " open_s : master get child pid = %d\n", mypid);
	    DBMS5(stdout, "open_s : master is waiting next connection \n");

	    mypid = -1;
	}
    }

    multios_socket_shutdown(master_sockfd, 2);
    multios_socket_close(master_sockfd);

    return 0;
}

static int _mpr1_readstream(const int fd, char *ptr, const int length)
{
/*
	udrsock_readstream(int fd,char *ptr,int maxlength)
	���ס�
	�����å��̿��Ǽ�������E����ȥ꡼��ǡ������ɤ߹��ߤޤ���
	������
	int fd : �ե�����Eϥ�ɥ�E
	char *ptr : �����Хåե��ݥ���
	�����Хåե����礭��

	�֤�E͡�
	int �ºݤ˼�������E����ȥ꡼�ॵ����(Byte)
	-1 : �꡼�ɥ��顼 
	���͡�
		̵��
*/

    int rc;
    int get_data_size = 0;
    int need_length = 0;
    char *p_ptr;

    DBMS5(stdout, "readstream : execute\n");
    DBMS5(stdout, "readstream : need byte size = %d\n", length);

    if (length > max_packet_size) {
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;
    }

    need_length = length;
    p_ptr = ptr;

    while (1) {
	rc = multios_socket_recv_withtimeout(fd, p_ptr, need_length,
					     MSG_WAITALL, 0);
	if (rc < 1)
	    break;
	get_data_size += rc;
	if (get_data_size == length) {
	    rc = get_data_size;
	    break;
	}
	p_ptr = ptr + get_data_size;
	need_length = length - get_data_size;
    }

    if (rc == -1)		/* ��³�ڤ�E*/
	return -1;

    if (rc == 0)
	return -1;

    if (length != rc) {
	DBMS1(stdout, "readstream : recv fail \n");
	DBMS3(stdout, "readstream : need length = %d\n", length);
	DBMS3(stdout, "readstream : recb length = %d\n", rc);
    }

    DBMS5(stdout, " readstream : %x %x %x %x \n",
	  ptr[0], ptr[1], ptr[2], ptr[3]);

    return rc;
}

static int _mpr1_writestream(const int fd, const char *ptr,
			       const int bytelength)
{
/*
	udrsock_writestream(int fd,char *ptr,int nlength)
	���ס�
	�����å��̿��ǥ��ȥ꡼��ǡ�����񤭹��ߤޤ���
	������
		int fd : �ե�����Eϥ�ɥ�E
		char *ptr : �����ǡ����Хåե��ݥ���
		int nlength : �����ǡ���Ĺ�ʥХ��ȡ�
		�����ǡ������礭��

	�֤�E͡�
		int �ºݤ���������E����ȥ꡼�ॵ����(Byte)
	���͡�
		̵��
*/
    int left, written;

    struct timeval tv;
    fd_set fds;
    int result;

    DBMS5(stdout, "udrsock_writestream : execute \n");
    if (bytelength > max_packet_size) {
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;
    }

    /* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    /* �����åȳ䤁E���Select�鴁E� */
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Socket�䤁E����Ԥ� */
    result =
	select(FD_SETSIZE, (fd_set *) NULL, &fds, (fd_set *) NULL, &tv);
    if (result == 0) {
	/* �����ॢ���� ���� */
	DBMS1(stderr, "udrsock_writestream : socket read timeout \n");
	DBMS5(stderr, "udrsock_writestream : result = %d \n", result);
	return MULTIOS_TCPIP_MPR1_ERR_CONNECT;
    }

    DBMS5(stdout, "udrsock_writestream : execute\n");

    left = bytelength;
    DBMS5(stdout, "udrsock_writestream : send size =%d\n", left);

    while (left > 0) {
	written = multios_socket_send_withtimeout(fd, ptr, left, 0, 0);

	if (written <= 0)
	    return (written);	/* err, return -1 */
	left -= written;
	ptr += written;
    }

    return (bytelength - left);	/* return  >= 0 */
}

int udrsock_loop_s(void)
{
/*
    udrsock_loop_s(int sockfd)
	������¦�ǥ��饤����Ȥ��׵��ƻ�E���E
    ������
	int sockfd : �����ץ󤷤������åȥϥ�ɥ�E
	�֤�E�:
		UDRSOCK_CCLOSE : ���㥤��Eɥץ�������������
*/

    multios_tcpip_mpr1_msg_t *recv_m, *reply_m;
    int result;
    fd_set fds, rfds;
    struct timeval tv;
    int sockfd = 0;

    sockfd = child_sockfd;

    /* �鴁E� */
    recv_m = (multios_tcpip_mpr1_msg_t *) read_stream_buff;
    reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff;

    /* �����ॢ���Ƚ��� �鴁E� */
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    /* ���饤����ȥ�E������ȴƻ�EΤ����̵�¥�E��� */

    while (1) {
	packet_buff_message_area_clr(recv_m);
	packet_buff_message_area_clr(reply_m);

	/* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
	tv.tv_sec = MULTIOS_TCPIP_MPR1_CLIENT_ALIVE_CHK_INTERVAL;
	tv.tv_usec = 0;

	rfds = fds;

	result =
	    multios_socket_select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL,
		   &tv);
	if (result == 0) {
	    DBMS5(stderr, "udrsock_loop_s : result = %d \n", result);
	    DBMS4(stderr, "udrsock_loop_s : socket read timeout \n");
	    continue;
	}

	/* ��E������ȥǡ��� �꡼�� */
	result = _mpr1_msg_recv_s(recv_m);

	if (result == MULTIOS_TCPIP_MPR1_EOF) {	/* ���饤����Ȥ��Ĥ��� */
	    return MULTIOS_TCPIP_MPR1_CLIENTCLOSED;
	}

	if (result == MULTIOS_TCPIP_MPR1_ERR_DATA) {	/* �����ǡ�����̵�� */
	    continue;
	}

	/* ���饤������׵���E*/
	switch (recv_m->hdr.u.param.req) {
	case (MULTIOS_TCPIP_MPR1_OPEN_REQ):
	    DBMS5(stdout, "udrsock_loop_s : found MULTIOS_TCPIP_MPR1_OPEN_REQ \n");
	    _mpr1_open_req_s(recv_m);
	    break;
	case (MULTIOS_TCPIP_MPR1_CMDIO_REQ):
	    DBMS5(stdout, "udrsock_loop_s : found MULTIOS_TCPIP_MPR1_CMDIO_REQ\n");
	    _mpr1_cmdio_req_s(recv_m);
	    DBMS5(stdout, "udrsock_loop_s : MULTIOS_TCPIP_MPR1_CMDIO_REQ done\n");
	    break;

	case (MULTIOS_TCPIP_MPR1_IOCTL_REQ):
	    DBMS5(stdout, "udrsock_loop_s : found MULTIOS_TCPIP_MPR1_IOCTL_REQ\n");
	    _mpr1_ioctl_req_s(recv_m);
	    DBMS5(stdout, "udrsock_loop_s : MULTIOS_TCPIP_MPR1_IOCTL_REQ done\n");
	    break;

	case (MULTIOS_TCPIP_MPR1_MEMIO_REQ):
	    DBMS5(stdout, "udrsock_loop_s : found MULTIOS_TCPIP_MPR1_MEMIO_REQ\n");
	    _mpr1_memio_req_s(recv_m);
	    DBMS5(stdout, "udrsock_loop_s : MULTIOS_TCPIP_MPR1_MEMIO_REQ done\n");
	    break;
	case (MULTIOS_TCPIP_MPR1_CLOSE_REQ):
	    DBMS5(stdout, "udrsock_loop_s : found MULTIOS_TCPIP_MPR1_CLOSE_REQ\n");
	    result = _mpr1_close_req_s(recv_m);
	    DBMS5(stdout, "udrsock_loop_s : MULTIOS_TCPIP_MPR1_CLOSE_REQ done\n");
	    break;
	default:
	    DBMS5(stderr, "loop_s : unknown request = %d\n",
		  recv_m->hdr.u.param.req);
	}
    }
}

static int _mpr1_hdr_certify(multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	udrsock_msg_certify(recv_m,int length,char msg_type)
	���ס�
	 ��å��������ե����ޥåȤ�Ŭ�礹��E���ǧ���ޤ���
	������
		udrsock_msg_t *recv_m : ����������å�����
		int length : ���������ǡ���Ĺ��byte)
		char msg_type : ��å�����������

	�֤�E͡�
		UDRSOCK_ERR_NULL_DATA : ͭ���ǡ���̵��
		UDRSOCK_ERR_PACKET_DESTRY : �ѥ��åȤ��ڤ�EƤ�E
		UDRSOCK_ERR_SCC       : CRC���顼
		UDRSOCK_EOF           : EOF �ɤߤȤ�E
		UDRSOCK_NO_ERR        : ��å�������ǧ
*/

    DBMS5(stdout, "msg_certify : execute \n");

    /* UDR �ѥ��å�ID��ǧ */
    if (!(MULTIOS_GET_BE_D32(recv_m->hdr.u.param.magic_no) & MULTIOS_TCPIP_MPR1_MAGIC_NO)) {
	DBMS1(stdout, "_hdr_certify : no ID stream , ignored \n");
	return MULTIOS_TCPIP_MPR1_ERR_ID;
    }

    /* UDR �ϡ��ɥ��������� ID��ǧ */
    if (!(MULTIOS_GET_BE_D32(recv_m->hdr.u.param.magic_hd) & MULTIOS_TCPIP_MPR1_MAGIC_HDR)) {
	DBMS1(stdout,
	      "_hdr_certify : differnce HARD ID stream , ignored \n");
	return MULTIOS_TCPIP_MPR1_ERR_HD;
    }

    return MULTIOS_TCPIP_MPR1_NO_ERR;
}

static int _mpr1_msg_recv(const int fd, multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	udrsock_msg_recv(int fd,udrsock_msg_t *recv_m)
		��å�������������ޤ���
		�ᤁEͤˤơ������ǡ����ξ��֤�E
	
	������
		udrsock_msg_t *recv_m : ������å������ǡ����ݥ���
	�֤�E͡�
		UDRSOCK_ERR_DATA_NULL : ͭ���ǡ���̵��
		UDRSOCK_ERR_PACKET_DESTRY : �ѥ��åȤ��ڤ�EƤ�E
		UDRSOCK_ERR_SCC : CRC���顼
		UDRSOCK_EOF : EOF �ɤߤȤ�E
		UDRSOCK_NO_ERR : ��å�������ǧ
		
*/

    /* �إå����ȥե�E����ʬ�䤷�Ƽ�������E*/

    int bytesize;
    char *streamdata;
    int result;

    DBMS5(stdout, "_msg_recv : execute\n");

    /* �إå������� */
    bytesize = MULTIOS_TCPIP_MPR1_HDR_SIZE;
    streamdata = (char *) recv_m;

    result = _mpr1_readstream(fd, streamdata, bytesize);
    DBMS5(stdout,
	  "_msg_recv : udrsock_readstream(head) result = %d\n",
	  result);

    if (result < 0)
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;

    if (result != bytesize) {
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;
    }

    result = _mpr1_hdr_certify(recv_m) ;
    if (result != MULTIOS_TCPIP_MPR1_NO_ERR) {
	return result;
    }

    /* �ե�E��������� */
    bytesize = MULTIOS_TCPIP_MPR1_GET_FRAMESIZE(recv_m);
    if (bytesize == 0) {
	return MULTIOS_TCPIP_MPR1_NO_ERR;
    }

    streamdata = ((char *) recv_m + MULTIOS_TCPIP_MPR1_HDR_SIZE);
    result = _mpr1_readstream(fd, streamdata, bytesize);
    DBMS5(stdout,
	  "_msg_recv : _readstream(frame) result = %d\n",
	  result);

    if (result < 0) {
	return MULTIOS_TCPIP_MPR1_EOF;
    }

    if (result != bytesize) {
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;
    }

    return MULTIOS_TCPIP_MPR1_NO_ERR;
}

static int _mpr1_msg_recv_s(multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	udrsock_msg_recv_s(udrsock_msg_t *recv_m)
	��å�������������ޤ���
	�����ǡ����˥إå����ʤ��Ȥ���̵��E��ޤ���
	
	������
		udrsock_msg_t *recv_m : ������å�����
	�֤�E͡�
		UDRSOCK_ERR_DATA : ͭ���ǡ���̵��
		UDRSOCK_EOF      : EOF �ɤߤȤ�E
		UDRSOCK_NO_ERR   : �ǡ����ɤߤȤ�E
		
*/
    int result = 0;

    DBMS5(stdout, " _msg_recv_s : execut \n");
    result = _mpr1_msg_recv(child_sockfd, recv_m);

    if (recv_m->hdr.u.param.msg != MULTIOS_TCPIP_MPR1_REQUEST_MSG) {
	DBMS1(stdout,
	      "_msg_recv_s : socket message direction error \n");
	return MULTIOS_TCPIP_MPR1_ERR_DATA;
    }
#if 0				/* for Debug */
    DBMS5(stdout, "recv message infomation\n");
    udrsock_dump_msg(recv_m);
#endif

    /* ��å����������׳�ǧ */
    switch (result) {
    case (MULTIOS_TCPIP_MPR1_NO_ERR):
	DBMS5(stdout, "_msg_recv_s : succsess \n");
	return MULTIOS_TCPIP_MPR1_NO_ERR;
    case (MULTIOS_TCPIP_MPR1_EOF):
	DBMS5(stdout, "_msg_recv_s : EOF detect\n");
	return MULTIOS_TCPIP_MPR1_EOF;
    case (MULTIOS_TCPIP_MPR1_ERR_PACKET_DESTORY):
	/* ��Eȥ饤�׵�E*/
	DBMS5(stdout, "_msg_recv_s : result = %d from msg_recv \n",
	      result);
	{
	    multios_tcpip_mpr1_msg_t *const reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff;

	    MULTIOS_TCPIP_MPR1_BUF_CLEAR(reply_m);

	    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.magic_no, MULTIOS_TCPIP_MPR1_MAGIC_NO);
	    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.magic_hd, MULTIOS_TCPIP_MPR1_MAGIC_HDR);
	    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
	    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_RETRY;
	    reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_NULL_FRAME;
	    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize, 0);

	    _mpr1_msg_reply_s(reply_m);

	    DBMS5(stdout, " msg_recv_s : retry request \n");

	}

	return MULTIOS_TCPIP_MPR1_ERR_DATA;
    default:
	DBMS1(stdout,
	      "udrsock_msg_recv_s : messeage analyze fail code = %d\n",
	      result);
    }
    return MULTIOS_TCPIP_MPR1_ERR_DATA;
}

static int _mpr1_msg_send(const int fd, const multios_tcpip_mpr1_msg_t *const send_m)
{
/*
	int udrsock_msg_send(udrsock_msg_t *send_m)
		��å��������������ޤ���
	������
		udrsock_msg_t *send_m : ������å�����
	�֤�E͡�
		����E�� UDRSOCK_NO_ERR
		���� :  UDRSOCK_ERR_FAIL
*/

    /* �إå����ȥե�E����ʬ�䤷����������E*/

    const char *streamdata;
    int bytesize;
    int result;

    /* ʬ�䤷����������E��٤��Τǡ���E����������E*/
    bytesize = MULTIOS_TCPIP_MPR1_HDR_SIZE + MULTIOS_TCPIP_MPR1_GET_FRAMESIZE(send_m);
    streamdata = (const char *) send_m;
    result = _mpr1_writestream(fd, streamdata, bytesize);
    if (result != bytesize) {
	return MULTIOS_TCPIP_MPR1_ERR_FAIL;
    }
    return MULTIOS_TCPIP_MPR1_NO_ERR;
}

static int _mpr1_msg_reply_s(const multios_tcpip_mpr1_msg_t *const send_m)
{
/*
	int udrsock_msg_reply_s(udrsock_msg_t *send_m)
	���饤����Ȥ˥�Eץ饤��å��������������ޤ���

	������
	udrsock_msg_t *send_m : ������å�����
	�֤�E͡�
	����E�� UDRSOCK_NO_ERR
	���� :  UDRSOCK_ERR_FAIL
*/

    DBMS5(stdout, "udrsock_msg_reply_s : execute \n");
#if 0				/* for Debug */
    DBMS5(stdout, "_msg_reply_s : send message infomation\n");
    udrsock_dump_msg(send_m);
#endif
    return _mpr1_msg_send(child_sockfd, send_m);
}

static int _mpr1_open_req_s(const multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
    udrsock_open_req_s(udrsock_msg_t *recv_m)
    ���ס�
	���饤�����¦���饪���ץ��׵᤬���ä���E�˥桼������
	�ѥ���E��ɤ�ȹ礷����ǧ����E�
    ������
	udrsock_msg_t *recv_m : ����������å�����
	�֤�E͡�
	    ����E�� TRUE
	    ���� : FALSE
	���͡�
	    ̵����
*/

    multios_tcpip_mpr1_msg_t *const reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff; 
    int result = -1;
    int sockbuf_size;
    char usr_name[20], usr_pass[20];

    /* init */
    DBMS5(stdout, "udrsock_open_req_s : execute\n");

    /* �إå����ԡ� */
    memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_HDR_SIZE);
    /* ��Eץ饤��å����� �إå���ܮ */
    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_SUCCESS;
    reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_NULL_FRAME;
    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize, 0);

    memcpy(usr_name, recv_m->frame.open.u.param.user_name, 16);
    memcpy(usr_pass, recv_m->frame.open.u.param.user_passwd, 16);
    sockbuf_size = MULTIOS_GET_BE_D32(recv_m->frame.open.u.param.sockbufffer_size);
    /* ID�ȹ�E*/

    result = udrs_funcs.open(usr_name, usr_pass);

    /* ��Eץ饤���� */
    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);

    return _mpr1_msg_reply_s(reply_m);
}

static int _mpr1_close_req_s(const multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	udrsock_close_req_s(udrsock_msg_t *recv_m)
	���ס�
	 ���饤�����¦���饯�������׵᤬���ä��Ȥ���
	������Ԥ��ޤ���
	������
		udrsock_msg_t *recv_m : ����������å�����
	�֤�E͡�

	���͡�
		̵����
*/

    /* �ҥץ��������Ĥ��Ƥ����Ȥ���������E��ݥ󥹤��֤��� */
    int result;
    multios_tcpip_mpr1_msg_t *const reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff;

    /* init */
    DBMS5(stdout, "udrsock_close_req_s : execute\n");

    /* �إå����ԡ� */
    memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_HDR_SIZE);
    /* ��Eץ饤��å����� �إå���ܮ */
    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_SUCCESS;
    reply_m->hdr.u.param.ftype = 0;
    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize, 0);

    /* ���������׵�E*/
    result = udrs_funcs.close();

    /* ��Eץ饤���� */
    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);

    return _mpr1_msg_reply_s(reply_m);

}

static void _mpr1_child_close_s(void)
{
/*
	udrsock_close_s()
		Socket�Υ�������
	������
		̵��
	�֤�E͡�
		̵��
*/

    DBMS4(stdout,
	  "udrsock_child_close_s : Client Close , cut com connector\n");

    if (child_sockfd >= 0) {
	multios_socket_shutdown(child_sockfd, 2);
	multios_msleep(100);
	multios_socket_close(child_sockfd);
    }
}

void udrsock_master_close_s(void)
{
/*
	udrsock_close_s()
		Socket�Υ�������
	������
		̵��
	�֤�E͡�
		̵��
*/

    DBMS4(stdout, "udrsock_master_close_s : Client Close\n");

    if (child_sockfd >= 0) {
	multios_socket_shutdown(child_sockfd, 2);
	multios_msleep(100);
	multios_socket_close(child_sockfd);
    }
    if (master_sockfd >= 0) {
	multios_socket_shutdown(master_sockfd, 2);
	multios_msleep(100);
	multios_socket_close(master_sockfd);
    }
}

static int _mpr1_cmdio_req_s(const multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	int udrsock_cmdio_req_s(udrsock_msg_t *recv_m)
	���ס�
	���饤����Ȥ���Υ�E������Ȥ��Ф�
	���ޥ�ɥ饤�Ȥ�¹Ԥ���
	�¹Է�E̤򥯥饤����Ȥ˥�E��ݥ󥹤��ޤ���

	������
		udrsock_msg_t *recv_m
	�ᤁE�:
		���� : -1
		����E: 0
*/

    int cmdln_no = -1;
    int char_len = -1;
    int result = -1;

    multios_tcpip_mpr1_msg_t *const reply_m= (multios_tcpip_mpr1_msg_t *) write_stream_buff;
    char *pdata;

    DBMS5(stdout, "udrsock_cmdio_req_s : execute\n");

    /* init */
    pdata = NULL;

    /* �إå����ԡ� */
    memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_HDR_SIZE);

    /* ��Eץ饤��å����� �إå���ܮ */
    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_SUCCESS;

    /* dir �˽��������� */
    cmdln_no = MULTIOS_GET_BE_D32(recv_m->frame.cmdio.u.param.cmdln_no);

    switch (recv_m->frame.cmdio.u.param.dir) {
    case (MULTIOS_TCPIP_MPR1_DIR_WRITE):
	char_len = MULTIOS_GET_BE_D32(recv_m->frame.cmdio.u.param.size);
	pdata = (char *) recv_m->frame.cmdio.data;

	/* ���ޥ�ɥ�������ƤӽФ� */
	result = udrs_funcs.dwrite(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_WRITE : %s", pdata);
	fflush(stdout);
#endif
	/* ��Eץ饤�ե�E����ܮ */
	reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_NULL_FRAME;
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize, 0);
	MULTIOS_SET_BE_D32(reply_m->frame.cmdio.u.param.cmdln_no, cmdln_no);
	break;

    case (MULTIOS_TCPIP_MPR1_DIR_READ):
	reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_CMDIO_FRAME;
	pdata = (char *) reply_m->frame.cmdio.data;
	char_len = MULTIOS_GET_BE_D32(recv_m->frame.cmdio.u.param.rdsize);
	MULTIOS_SET_BE_D32(reply_m->frame.cmdio.u.param.cmdln_no, 0);

	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize,
		      sizeof(multios_tcpip_mpr1_cmdio_frame_t));

	/* ���ޥ�ɥ�������ƤӽФ� */
	result = udrs_funcs.dread(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_READ : %s", pdata);
	fflush(stdout);
#endif
	/* ��Eץ饤�ե�E����ܮ */
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
	break;

    case (MULTIOS_TCPIP_MPR1_DIR_RW):
	reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_CMDIO_FRAME;

	/* ���ޥ�ɥ饤�Ƚ��� */
	pdata = (char *) recv_m->frame.cmdio.data;
	char_len = MULTIOS_GET_BE_D32(recv_m->frame.cmdio.u.param.size);

	/* ���ޥ�ɥ�������ƤӽФ� */
	result = udrs_funcs.dwrite(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_RW(write) %d : %s", char_len, pdata);
	fflush(stdout);
#endif
	/* ���ޥ�ɥ꡼�ɽ鴁E� */
	pdata = (char *) reply_m->frame.cmdio.data;
	char_len = MULTIOS_GET_BE_D32(recv_m->frame.cmdio.u.param.rdsize);

	/* ���ޥ�ɥ�������ƤӽФ� */
	result = udrs_funcs.dread(pdata, char_len);

#if 0
	DBMS5(stdout, "DIR_RW(read) %d : %s", char_len, pdata);
	fflush(stdout);
#endif
	/* ��Eץ饤�ե�E����ܮ */
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize,
		      (sizeof(multios_tcpip_mpr1_cmdio_frame_t) -
		       MULTIOS_TCPIP_MPR1_CMDIO_DSIZE) + strlen(pdata));

	MULTIOS_SET_BE_D32(reply_m->frame.cmdio.u.param.cmdln_no, cmdln_no);
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
	break;

    default:
	DBMS1(stderr,
	      "udrsock_cmdio_req_s : unknown direction request = %d\n",
	      recv_m->frame.cmdio.u.param.dir);
	reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_ERROR;
	memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_GET_FRAMESIZE(recv_m));
	break;
    }

    /* ���饤����Ȥ˥�E��ݥ󥹤��֤� */
    return _mpr1_msg_reply_s(reply_m);
}

static int _mpr1_ioctl_req_s(const multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	int udrsock_ioctl_req_s(udrsock_msg_t *recv_m)
	���ס�
	ioctl ���ޥ�ɤ�¹Ԥ��ޤ���
	����E
		udrsock_msg_t *recv_m
*/

    int cmd;
    char *pdata;
    int result;
    int char_len;
    multios_tcpip_mpr1_msg_t *const reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff;

    /* init */
    DBMS5(stdout, "_ioctl_req_s : execute\n");

    /* �إå����ԡ� */
    memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_HDR_SIZE);
    /* ��Eץ饤��å����� �إå���ܮ */
    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_SUCCESS;

    cmd = MULTIOS_GET_BE_D32(recv_m->frame.ioctl.u.param.cmd);
    pdata = (char *) recv_m->frame.ioctl.data;
    char_len = MULTIOS_GET_BE_D32(recv_m->frame.ioctl.u.param.size);

    /* ���ޥ�ɥ�������ƤӽФ� */
    result = udrs_funcs.ioctl(cmd, pdata, &char_len);

    /* ��Eץ饤�ե�E����ܮ */
    MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
    MULTIOS_SET_BE_D32(reply_m->frame.ioctl.u.param.size, char_len);
    memcpy(reply_m->frame.ioctl.data, pdata, char_len);

    /* ���饤����Ȥ˥�E��ݥ󥹤��֤� */
    return _mpr1_msg_reply_s(reply_m);
}

#ifndef UDRSOCK_PAC64

static int _mpr1_memio_req_s(multios_tcpip_mpr1_msg_t *const recv_m)
{
/*
	udrsock_memio_req_s(udrsock_msg_t *recv_m)
	���ס�
	 ��⥁E񤭹��ߥ�å���������������Ȥ��˸ƤФ�E��ǡ����񤭹��߽���
	 ��Ԥ��ޤ���
	������
		udrsock_msg_t *recv_m : ����������å�����
		void   *memb_data : �֥��å�ž���ǡ����ݥ���
	�֤�E͡�
		����E�� TRUE
		���� : FALSE
	���͡�
*/

    int offs = 0;
    int size = 0;
    int result = -1;
    multios_tcpip_mpr1_msg_t *const reply_m = (multios_tcpip_mpr1_msg_t *) write_stream_buff;
    char *pdata;

    DBMS5(stdout, "udrsock_memio_req_s : execute \n");

    /* �鴁E� */
    offs = MULTIOS_GET_BE_D32(recv_m->frame.memio.u.param.offs);
    size = MULTIOS_GET_BE_D32(recv_m->frame.memio.u.param.size);


    /* �إå����ԡ� */
    memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_HDR_SIZE);
    /* ��Eץ饤��å����� �إå���ܮ */
    reply_m->hdr.u.param.msg = MULTIOS_TCPIP_MPR1_REPLY_MSG;
    reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_SUCCESS;

    /* ���ޥ�ɼ¹� */

    switch (recv_m->frame.memio.u.param.dir) {
    case (MULTIOS_TCPIP_MPR1_DIR_WRITE):
	pdata = (char *) recv_m->frame.memio.data;

	result = udrs_funcs.imemio(MULTIOS_TCPIP_MPR1_DIR_WRITE, offs, pdata, size);

	/* ��Eץ饤�ե�E����ܮ */
	reply_m->hdr.u.param.ftype = MULTIOS_TCPIP_MPR1_NULL_FRAME;
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize, 0);
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
	break;

    case (MULTIOS_TCPIP_MPR1_DIR_READ):
	pdata = (char *) reply_m->frame.memio.data;

	/* ��������ƤӽФ� */
	result = udrs_funcs.imemio(MULTIOS_TCPIP_MPR1_DIR_READ, offs, pdata, size);

	/* ��Eץ饤�ե�E����ܮ */
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.error, result);
	MULTIOS_SET_BE_D32(reply_m->frame.memio.u.param.size, size);
	MULTIOS_SET_BE_D32(reply_m->hdr.u.param.fsize,
		      sizeof(multios_tcpip_mpr1_memio_frame_t) -
		      MULTIOS_TCPIP_MPR1_MEMIO_DSIZE + size);
	break;

    default:
	DBMS1(stderr, " cmd_req_s : unknown direction request = %d\n",
	      recv_m->frame.cmdio.u.param.dir);
	reply_m->hdr.u.param.reply = MULTIOS_TCPIP_MPR1_REPLY_ERROR;
	memcpy(reply_m, recv_m, MULTIOS_TCPIP_MPR1_GET_FRAMESIZE(recv_m));
	break;
    }

    return _mpr1_msg_reply_s(reply_m);

}
#else

static int udrsock_memio_req_s(udrsock_msg_t * recv_m)
{
    DBMS5(stdout, "udrsock_memio_req_s : execute \n");

    switch (recv_m->frame.memio.dir) {
    case (UDRSOCK_DIR_READ):
	DBMS5(stdout, "udrsock_memio_req_s : dir type read\n");
	return udrsock_memio_read_req_s(recv_m);
    case (UDRSOCK_DIR_WRITE):
	DBMS5(stdout, "udrsock_memio_req_s : dir type write\n");
	return udrsock_memio_write_req_s(recv_m);
    default:
	DBMS1(stdout, " memio_req_s : unknown direction type \n");
	DBMS1(stdout, " memio_req_s : type = %d\n",
	      recv_m->frame.memio.dir);
	return UDRSOCK_ERR_FAIL;
    }
}

static int udrsock_memio_write_req_s(const udrsock_msg_t * recv_m)
{
/*
	udrsock_memio_write_req_s(udrsock_msg_t *recv_m)
	���ס�
	 ��⥁E񤭹��ߥ�å���������������Ȥ��˸ƤФ�E��ǡ����񤭹��߽���
	 ��Ԥ��ޤ���
	������
		udrsock_msg_t *recv_m : ����������å�����
		void   *memb_data : �֥��å�ž���ǡ����ݥ���
	�֤�E͡�
		����E�� TRUE
		���� : FALSE
	���͡�
		ioctrl�ǡ�ž���оݤΥ�⥁E�����ɥ������ꤷ�Ƥ���ɬ�פ�����E�
*/

    int memb_type;
    int32_t memb_seg;
    int32_t memb_offs;
    int32_t memb_size;
    int trans_loop_count;
    int left_size;
    int data_size;
    int result;

    /* �饤�ȥǡ��� �׵᾵ǧ ��Eץ饤 */
    if (udrsock_memio_trans_start_req_s(recv_m, UDRSOCK_NO_ERR)
	== UDRSOCK_ERR_FAIL)
	return UDRSOCK_ERR_FAIL;

    /* ž��E*/
    memb_type = WORD_DATA_GET(recv_m->frame.memio.mem_type);
    memb_seg = WORD_DATA_GET(recv_m->frame.memio.segs);
    memb_offs = WORD_DATA_GET(recv_m->frame.memio.offs);
    memb_size = WORD_DATA_GET(recv_m->frame.memio.size);

    trans_loop_count = WORD_DATA_GET(recv_m->frame.memio.remain_cnt);
    left_size = WORD_DATA_GET(recv_m->frame.memio.left_size);
    data_size = udrsock_memiob_size;

    DBMS4(stdout, "udrsock_memio_write_req_s : trans_loop_count = %d\n",
	  trans_loop_count);
    DBMS4(stdout, "udrsock_memio_write_req_s : left_size = %d\n",
	  left_size);
    DBMS4(stdout, "udrsock_memio_write_req_s : data_size = %d\n",
	  data_size);

    result =
	udrsock_memio_multipac_s(UDRSOCK_DIR_WRITE, memb_offs,
				 trans_loop_count, left_size, data_size);

    /* ž����λ */
    if (result == UDRSOCK_NO_ERR) {
	result = udrsock_memio_trans_end_req_s(UDRSOCK_DIR_WRITE,
					       memb_type, NULL, memb_seg,
					       memb_offs, memb_size);
    }

    return result;
}

static int udrsock_memio_read_req_s(const udrsock_msg_t * recv_m)
{
/*
	udrsock_memio_read_req_s(udrsock_msg_t *recv_m)
	���ס�
	 ��⥁E񤭹��ߥ�å���������������Ȥ��˸ƤФ�E��ǡ����񤭹��߽���
	 ��Ԥ��ޤ���
	������
		udrsock_msg_t *recv_m : ����������å�����
		void   *memb_data : �֥��å�ž���ǡ����ݥ���
	�֤�E͡�
		����E�� TRUE
		���� : FALSE
	���͡�
		ioctrl�ǡ�ž���оݤΥ�⥁E�����ɥ������ꤷ�Ƥ���ɬ�פ�����E�
*/

    int memb_type;
    int32_t memb_seg;
    int32_t memb_offs;
    int32_t memb_size;
    int trans_loop_count;
    int left_size;
    int data_size;
    int result;
    int imem_result = 0;

    DBMS5(stdout, "udrsock_memio_read_req_s : execute \n");

    /* �鴁E� */
    memb_type = WORD_DATA_GET(recv_m->frame.memio.mem_type);
    memb_seg = WORD_DATA_GET(recv_m->frame.memio.segs);
    memb_offs = WORD_DATA_GET(recv_m->frame.memio.offs);
    memb_size = WORD_DATA_GET(recv_m->frame.memio.size);

    trans_loop_count = WORD_DATA_GET(recv_m->frame.memio.remain_cnt);
    left_size = WORD_DATA_GET(recv_m->frame.memio.left_size);
    data_size = udrsock_memiob_size;

    /* �饤�ȥǡ��� �׵᾵ǧ ��Eץ饤 */
    if (udrsock_memio_trans_start_req_s(recv_m, imem_result)
	== UDRSOCK_ERR_FAIL)
	return UDRSOCK_ERR_FAIL;

    /* ž���¹� */
    result = udrsock_memio_multipac_s(UDRSOCK_DIR_READ,
				      memb_offs, trans_loop_count,
				      left_size, data_size);

    /* ž����λ */
    if (result == UDRSOCK_NO_ERR) {
	result = udrsock_memio_trans_end_req_s(UDRSOCK_DIR_READ,
					       memb_type, NULL, memb_seg,
					       memb_offs, memb_size);
    }

    return result;
}

static int udrsock_memio_write_1pac_s(const unsigned int offs,
				      const int size)
{
/*
	udrsock_memio_write_1pac_s(char *memb_data,udrsock_msg_t *recv_msg)
	���ס�
	 imem_write�¹Ԥΰ١��֥��å��ǡ�����������ޤ���
	 ��E��ݥ󥹤��֤��Τϡ�ž�����֤��Ф�����ˤ��ޤ�����(^^;)
	 ����ž���������� udrsock_memiob_size (Byte)�Ǥ���
	������
		char *memb_data : ������¦�ΥХåե��ݥ���
		int size : ��E����֥�å�����������
	�֤�E͡�
		UDRSOCK_ERR_CONNECT : ����
		UDRSOCK_NO_ERR : ����E
	���͡�
		ioctrl�ǡ�ž���оݤΥ�⥁E�����ɥ������ꤷ�Ƥ���ɬ�פ�����E�
*/

    udrsock_memiob_data_t *recv_m;
    timeval_t tv;
    fd_set fds;
    int result;
    int use_readbuf = 0;

    DBMS5(stdout, "udrsock_memio_write_1pac_s : execute \n");

    if (NULL == udrs_funcs.imemio_get_ptr) {
	use_readbuf = 1;
    } else if (udrsock_memiob_size != size) {
	use_readbuf = 1;
    } else {
	use_readbuf = 0;
    }

    /* �Хåե��鴁E� */
    if (use_readbuf) {
	recv_m = (udrsock_memiob_data_t *) read_stream_buff;
    } else {
	recv_m = (udrsock_memiob_data_t *) udrs_funcs.imemio_get_ptr(offs);
    }

    /* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
    tv.tv_sec = UDRSOCK_TIMEOUT_RETRY_INTERVAL;
    tv.tv_usec = 0;

    /* �����åȳ䤁E���Select�鴁E� */
    FD_ZERO(&fds);
    FD_SET(child_sockfd, &fds);

    /* Socket�䤁E����Ԥ� */
    result =
	multios_socket_select(FD_SETSIZE, &fds, (fd_set *) NULL, (fd_set *) NULL, &tv);
    if (result == 0) {
	/* �����ॢ���� ���� */
	DBMS3(stderr, "udrsock_memio_write_1pac_s : result = %d \n",
	      result);
	DBMS1(stderr,
	      "udrsock_memio_write_1pac_s : socket read timeout \n");
	return UDRSOCK_ERR_CONNECT;
    }

    /* ���� */
    result = _readstream(child_sockfd, (char *) recv_m->data,
				udrsock_memiob_size);
    if (result != udrsock_memiob_size)
	return UDRSOCK_ERR_FAIL;

    /* �Хåե���ž��E*/
    if (use_readbuf) {
	result =
	    udrs_funcs.imemio(UDR_DIR_WRITE, offs, recv_m->data, size);
    }

#ifdef USE_CPU_HINT
    /* CPU����å���˾�äƤ���Eǡ������˴� */
    if( cpuinfo.simd.f.sse2 ) {
	multios_simd_cpu_hint_fence_store_to_memory();
	multios_simd_cpu_hint_invalidates_level_cache( recv_m->data, udrsock_memiob_size, notcall_memory_fence);
    }
#endif

    return UDRSOCK_NO_ERR;
}

/**
 * @fn static int udrsock_memio_read_1pac_s(int offs, int memb_size)
 * @brief �֥��å��ǡ������������ޤ�
 *	 ����ž���������� udrsock_memiob_size (Byte)�Ǥ���
 * @param offs ���Ѥ��Ƥ���E�⥁E֥��å��Υ��ե��å�
 * @param memb_size ����������
 * @retval UDRSOCK_NO_ERR(0) ����E
 * @retval 0�ʳ� ����
 */
static int udrsock_memio_read_1pac_s(const unsigned int offs,
				     const int memb_size)
{
    udrsock_memiob_data_t *reply_m;
    int result;
    int use_writebuf;

    /* init */
    if (NULL == udrs_funcs.imemio_get_ptr) {
	use_writebuf = 1;
    } else if (udrsock_memiob_size != memb_size) {
	use_writebuf = 1;
    } else {
	use_writebuf = 0;
    }

    if (use_writebuf) {
	reply_m = (udrsock_memiob_data_t *) write_stream_buff;

	/* �饤�ȥǡ����Ѱ� */
	result =
	    udrs_funcs.imemio(UDR_DIR_READ, offs, reply_m->data,
			      memb_size);

	/* ���ޤ�ˣ���ͤᤁE*/
	if (memb_size < udrsock_memiob_size) {
	    memset(write_stream_buff + memb_size, 0,
		   udrsock_memiob_size - memb_size);
	}
    } else {
	reply_m =
	    (udrsock_memiob_data_t *) udrs_funcs.imemio_get_ptr(offs);
    }

#ifdef USE_CPU_HINT
    if( cpuinfo.simd.f.sse2 ) {
	multios_simd_cpu_hint_prefetch_line(reply_m->data, udrsock_memiob_size, MULTIOS_MM_HINT_T2);
    }
#endif
    /* �ǡ������� */
    result = udrsock_writestream(child_sockfd, reply_m->data,
				 udrsock_memiob_size);
    if (result == -1 || result != udrsock_memiob_size) {
	return UDRSOCK_ERR_FAIL;
    }

#ifdef USE_CPU_HINT
    /* CPU����å���˾�äƤ���Eǡ������˴� */
    if( cpuinfo.simd.f.sse2 ) {
	multios_simd_cpu_hint_fence_store_to_memory();
	multios_simd_cpu_hint_invalidates_level_cache( reply_m->data, udrsock_memiob_size, notcall_memory_fence);
    }
#endif


    return UDRSOCK_NO_ERR;
}

/**
 * @fn static int udrsock_memio_multipac_s(const char trans_dir, int offs, int trans_loop_count, int left_size, int data_size)
 * @brief ��⥁E֥��å��ǡ�����E��Eudrsock_memiob_size (Byte) ��ʬ�䤷��Socket���Ϥ��ޤ�
 * @param trans_dir ���饤����Ȥ��鸫��ž������
 * @param offs ������Хåե��Υ��ե��å�
 * @param trans_loop_count ž����E��ץ������
 * @param left_size �Ǹ��write�ǡ���������
 * @param data_size �̾�E�write�ǡ���������
 * @retval UDRSOCK_NO_ERR(0) ����E
 * @retval 0�ʳ� ����
 */
static int
udrsock_memio_multipac_s(char trans_dir, int offs, int trans_loop_count,
			 int left_size, int data_size)
{
    int result, status;

    DBMS5(stdout, "udrsock_memio_multipac_s : execute \n");

    /* ž��E*/

    if (trans_dir == UDRSOCK_DIR_READ) {
	result = udrsock_memio_read_trans_send_req_s();
	if (result != UDRSOCK_NO_ERR) {
	    DBMS1(stderr, "udrsock_memio_multipacpac_s : start req err\n");
	    return UDRSOCK_ERR_FAIL;
	}

	multios_socket_set_nagle(child_sockfd, 1);
    }

    while (1) {
	DBMS5(stdout, "udrsock_memio_multipac_s : trans_loop_count =%d\n",
	      trans_loop_count);
	if ((trans_loop_count == 1) && (left_size > 0))
	    data_size = left_size;
	switch (trans_dir) {
	case (UDRSOCK_DIR_READ):
	    result = udrsock_memio_read_1pac_s(offs, data_size);
	    break;
	case (UDRSOCK_DIR_WRITE):
	    result = udrsock_memio_write_1pac_s(offs, data_size);
	    break;
	default:
	    status = UDRSOCK_ERR_FAIL;
	    goto out;
	}

	switch (result) {
	case (UDRSOCK_NO_ERR):
	    break;
	case (UDRSOCK_ERR_FAIL):
	    DBMS5(stdout,
		  "udrsock_memio_multipac_s : 1pac transfer fail\n");
	    status = UDRSOCK_ERR_FAIL;
	    goto out;
	default:
	    DBMS1(stdout,
		  "udrsock_memio_multipac_s : unknown result code = %d\n",
		  result);
	    status = UDRSOCK_ERR_FAIL;
	    goto out;
	}

	if (trans_loop_count-- == 1) {
	    DBMS5(stdout,
		  "udrsock_memio_multipac_s : transfer end,loop out\n");
	    status = UDRSOCK_NO_ERR;
	    goto out;
	}

	DBMS5(stdout,
	      "udrsock_memio_multipac_s : transfer pointer move\n");
	offs += data_size;
    }

  out:
    multios_socket_set_nagle(child_sockfd, 0);

    return status;
}

/**
 * @fn static int udrsock_memio_read_trans_send_req_s(void)
 * @brief ���饤����Ȥ���Υ꡼��ž�������׵���Ԥ��ޤ�
 * @retval UDRSOCK_NO_ERR(0) ����E
 * @retval 0�ʳ� ����
 */
static int udrsock_memio_read_trans_send_req_s(void)
{
    udrsock_msg_t *recv_m;
    int result;
    fd_set fds, rfds;
    timeval_t tv;

    recv_m = (udrsock_msg_t *) read_stream_buff;
    packet_buff_message_area_clr(recv_m);

    /* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
    tv.tv_sec = UDRSOCK_TIMEOUT_RETRY_INTERVAL;
    tv.tv_usec = 0;

    /* �����ॢ���Ƚ��� �鴁E� */
    FD_ZERO(&fds);
    FD_SET(child_sockfd, &fds);

    /* ���饤����ȥ�E������ȴƻ�EΤ����̵�¥�E��� */

    rfds = fds;

    result =
	select(FD_SETSIZE, &rfds, (fd_set *) NULL, (fd_set *) NULL, &tv);
    if (result == 0) {
	DBMS1(stderr,
	      "udrsock_memio_read_trans_send_req_s : socket read timeout \n");
	return UDRSOCK_ERR_FAIL;
    }

    /* ��E������ȥǡ��� �꡼�� */
    DBMS5(stdout,
	  "udrsock_memio_read_trans_send_req_s : recv_m pointer= 0x%p\n",
	  recv_m);
    result = udrsock_msg_recv_s(recv_m);

    if (recv_m->hdr.req != UDRSOCK_MEMIO_REQ) {
	DBMS1(stderr, " request fail id = %d\n", recv_m->hdr.req);
	return UDRSOCK_ERR_FAIL;
    }

    return UDRSOCK_NO_ERR;
}

/**
 * @fn static int udrsock_memio_trans_start_req_s(const udrsock_msg_t * recv_m, int flag)
 * @brief ���饤����Ȥ���ν񤭹����׵���Ф���E�Eץ饤���֤��ޤ���
 * @param recv_m ���饤����Ȥ���μ����ѥ��å�
 * @param flag ��Eץ饤��å������˺ܤ���Eե饰(error)
 * @retval 0 ����E
 * @retval 0�ʳ� ����
 */
static int
udrsock_memio_trans_start_req_s(const udrsock_msg_t * recv_m, int flag)
{
    udrsock_msg_t *reply_m;
    int err = 0;

    DBMS5(stdout, "udrsock_memio_trans_start_req_s : execute \n");

    reply_m = (udrsock_msg_t *) write_stream_buff;

    /* init */
    memcpy(reply_m, recv_m, UDRSOCK_HDR_SIZE);
    reply_m->hdr.msg = UDRSOCK_REPLY_MSG;
    reply_m->hdr.reply = UDRSOCK_REPLY_SUCCESS;
    WORD_DATA_PUT(reply_m->hdr.error, flag);
    reply_m->hdr.ftype = UDRSOCK_MEMIO_FRAME;
    WORD_DATA_PUT(reply_m->hdr.fsize, sizeof(struct udrsock_memio_frame));
    memcpy(&reply_m->frame.memio, &recv_m->frame.memio,
	   sizeof(struct udrsock_memio_frame));
    WORD_DATA_PUT(reply_m->frame.memio.memiob_size, udrsock_memiob_size);

    udrsock_msg_reply_s(reply_m);

    return err;
}

static int
udrsock_memio_trans_end_req_s(int trans_dir, int memb, char *pmemb_buff,
			      int32_t memb_seg, int32_t memb_offs,
			      int32_t memb_size)
{
/*
	udrsock_memio_end_req_s()
	���ס�
	 �Х��ʥ�E֥��å�ž����λ�׵��������ޤ���
	 ���饤����Ȥˤϥ�Eץ饤���֤��ޤ���
	������
		int reply_flag : ž����λ 0
						-1 ����
	�֤�E͡�
		-1 : ����
		 0 : ����E
	���͡�
		ioctrl�ǡ�ž���оݤΥ�⥁E�����ɥ������ꤷ�Ƥ���ɬ�פ�����E�
*/

    udrsock_msg_t *recv_m, *reply_m;
    int err = UDRSOCK_NO_ERR;
    int result;
    timeval_t tv;
    fd_set fds;


    DBMS5(stdout, "udrsock_memio_trans_end_req_s : execute \n");
    DBMS4(stdout, "udrsock_memio_trans_end_req_s : memb_offs = %d\n",
	  memb_offs);
    DBMS4(stdout, "udrsock_memio_trans_end_req_s : memb_size = %d\n",
	  memb_size);

    recv_m = (udrsock_msg_t *) read_stream_buff;
    reply_m = (udrsock_msg_t *) write_stream_buff;

    /* �����åȥ�å����������ॢ���Ȼ��� ��āE*/
    tv.tv_sec = UDRSOCK_TIMEOUT_RETRY_INTERVAL;
    tv.tv_usec = 0;

    /* �����åȳ䤁E���Select�鴁E� */
    FD_ZERO(&fds);
    FD_SET(child_sockfd, &fds);

    /* �Хåե��鴁E� */
    packet_buff_message_area_clr(recv_m);
    packet_buff_message_area_clr(reply_m);

    /* Socket�䤁E����Ԥ� */
    result =
	select(FD_SETSIZE, &fds, (fd_set *) NULL, (fd_set *) NULL, &tv);
    if (result == 0) {
	/* �����ॢ���� ���� */
	DBMS1(stderr,
	      "udrsock_memio_trans_end_req_s : socket read timeout \n");
	return UDRSOCK_ERR_FAIL;
    }

    result = udrsock_msg_recv_s(recv_m);
    if (result != UDRSOCK_NO_ERR)
	return result;

    /* ��E��ݥ󥹥�å�������ܮ */
    memcpy(reply_m, recv_m, UDRSOCK_GET_MSG_SIZE(recv_m));
    reply_m->hdr.msg = UDRSOCK_REPLY_MSG;
    reply_m->hdr.ftype = UDRSOCK_NULL_FRAME;
    WORD_DATA_PUT(reply_m->hdr.fsize, 0);

    if (recv_m->hdr.req == UDRSOCK_MEMIO_END_REQ) {
	reply_m->hdr.reply = UDRSOCK_REPLY_SUCCESS;

	result = 0;
	WORD_DATA_PUT(reply_m->hdr.error, result);

    } else {

	reply_m->hdr.reply = UDRSOCK_REPLY_ERROR;
	err = UDRSOCK_ERR_FAIL;
    }

    udrsock_msg_reply_s(reply_m);

    return err;
}
#endif

void _dump_msg(const multios_tcpip_mpr1_msg_t *const msg)
{
/*
	udrsock_dump_msg(udrsock_msg_t msg)

	�ǥХå��ѥ�����
	��å������ǡ�����ɽ��

	������
		udrsock_msg_t msg : ��å�������¤��
	�ᤁE͡�
		̵��
*/

    fprintf(stdout, " socket message infomation \n");
    fprintf(stdout, " messgae header part \n");
    fprintf(stdout, " hdr.magic_no = %s \n", msg->hdr.u.param.magic_no);
    fprintf(stdout, " hdr.msg = %d \n", msg->hdr.u.param.msg);
    fprintf(stdout, " hdr.req = %d \n", msg->hdr.u.param.req);
    fprintf(stdout, " hdr.reply = %d \n", msg->hdr.u.param.reply);
    fprintf(stdout, " hdr.error = %d \n", MULTIOS_GET_BE_D32(msg->hdr.u.param.error));
    fprintf(stdout, " hdr.ftype = %d \n", msg->hdr.u.param.ftype);
    fprintf(stdout, " hdr.fsize = %d \n", MULTIOS_GET_BE_D32(msg->hdr.u.param.fsize));

    switch (msg->hdr.u.param.ftype) {
    case (MULTIOS_TCPIP_MPR1_NULL_FRAME):
	fprintf(stdout, " frame type = NULL_FRAME \n");
	break;

    case (MULTIOS_TCPIP_MPR1_CMDIO_FRAME):
	fprintf(stdout, " frame type = CMD_FRAME \n");
	fprintf(stdout, " cmdio.size = %d\n",
		MULTIOS_GET_BE_D32(msg->frame.cmdio.u.param.size));

	fprintf(stdout, " cmdio.data = %s\n", msg->frame.cmdio.data);
	break;

    case (MULTIOS_TCPIP_MPR1_IOCTL_FRAME):
	fprintf(stdout, " frame type = IOCTL_FRAME \n");
	fprintf(stdout, " ioctl.size = %d\n",
		MULTIOS_GET_BE_D32(msg->frame.ioctl.u.param.size));
	fprintf(stdout, " ioctl.data = %s\n", msg->frame.ioctl.data);
	break;
#if 0
    case (MULTIOS_TCPIP_MPR1_MEMIO_FRAME):
	fprintf(stdout, " frame type = MEM_FRAME \n");
	fprintf(stdout, " memio.dir = %d\n", msg->frame.memio.dir);
	fprintf(stdout, " memio.size = %d\n",
		WORD_DATA_GET(msg->frame.memio.size));
	fprintf(stdout, " memio.size(high bit)= %x\n",
		LOW_OFFS_GET(msg->frame.memio.size));
	fprintf(stdout, " memio.offs = %d\n",
		WORD_DATA_GET(msg->frame.memio.offs));
	fprintf(stdout, " memio.offs(high bit)= %x\n",
		LOW_OFFS_GET(msg->frame.memio.offs));
	fprintf(stdout, " memio.remain_cnt = %d\n",
		msg->frame.memio.remain_cnt);
	fprintf(stdout, " memio.left_size = %d\n",
		WORD_DATA_GET(msg->frame.memio.left_size));
	fprintf(stdout, " memio.memiob_scc = %x\n",
		msg->frame.memio.memiob_scc);
	break;
#endif

    case (MULTIOS_TCPIP_MPR1_OPEN_FRAME):
	fprintf(stdout, " frame type = IOCTL_FRAME \n");
	fprintf(stdout, " dev_id = %d", msg->frame.open.u.param.dev_id);
	fprintf(stdout, " user_name = %s\n", msg->frame.open.u.param.user_name);
	fprintf(stdout, " user_passwd = %s\n",
		msg->frame.open.u.param.user_passwd);
	break;
    }
}

/**
 * @fn int udrsock_set_memiob_size(int size)
 * @brief MEMIOž������SocketIO�˰����Ϥ��ǡ����κ��祵���������ꤷ�ޤ�
 * @param size ������
 * @retval -1 ����
 * @retval 0 ����E
 **/
int udrsock_set_memiob_size(const int size)
{
    if (max_packet_size < size) {
	return -1;
    }
    udrsock_memiob_size = size;

    return 0;
}
