/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2014-Janualy-10
 *		Last Alteration $Author: takeda $
 */

/**
 * @file ips_ping.cpp
 *	PINGプロトコルライブラリです。
 *	Socket()をRAWでオープンするために管理者権限が必要になることがあります。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "ips_socket.h"

#ifdef WIN32
#if !defined(__MINGW32__)
#pragma comment (lib,"iphlpapi.lib")
#pragma comment (lib,"Ws2_32.lib")
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#define _WIN32_PINGPROC
#else
#define _MINGW32_PINGPROC
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif
#elif defined(Darwin) || defined(Linux) || defined(QNX) || defined(__AVM2__)
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#define _POSIX_PINGPROC
#else
#error `unknown included socket api prptotype on this OS`
#endif

/* libs3 */
#include <core/libmpxx/_stdarg_fmt.h>
#include <core/libmpxx/mpxx_sys_types.h>
#include <core/libmpxx/mpxx_stdlib.h>
#include <core/libmpxx/mpxx_time.h>
#include <core/libmpxx/mpxx_unistd.h>

#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 5;
#endif
#include <core/libmpxx/dbms.h>


/* this */
#include "ips_ping.h"

typedef struct _ping_ext {
#if defined(_WIN32_PINGPROC)
    HANDLE hIcmp;
#elif defined(_POSIX_PINGPROC)
    int fd;
#endif
    struct sockaddr_in serv_addr;
    int seq;
    void *buf;
    void *send_dat;
    void *recv_buf;
} ping_ext_t;

#define get_ping_ext(s) MPXX_STATIC_CAST(ping_ext_t*, (s)->ext)

#define BUF_SIZE 2000

#if defined(_POSIX_PINGPROC)
static uint16_t in_cksum(uint16_t * addr, int len);
static int send_icmp(s3::ips_ping_t * p);
#endif

/**
 * @fn int s3::ips_ping_is_found_target_networkname(const char *tagetnetname)
 * @brief ネットワークデータベースからターゲットの名前が引けるかチェックします。
 * @param tagetnetname ターゲット名
 * @retval 0 見つけられなかった
 * @retval 0以外 見つかった
 */
int s3::ips_ping_is_found_target_networkname(const char *tagetnetname)
{
    int result;
    struct addrinfo hints, *ai;

    ai = NULL;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    result = s3::ips_socket_getaddrinfo(tagetnetname, NULL, &hints, &ai);

    if (NULL != ai) {
	s3::ips_socket_freeaddrinfo(ai);
    }

    return (result) ? -1 : 0;
}

int s3::ips_ping_connect(s3::ips_ping_t * self_p,
				const char *servername, const size_t len,
				s3::ips_ping_attr_t * attr_p)
{
#if defined(__MINGW32__)
    (void) self_p;
    (void) servername;
    (void) len;
    (void) attr_p;

    return ENOSYS;
#else
    int result, status;
    ping_ext_t *__restrict e;
    struct addrinfo hints, *ai = NULL;
    struct sockaddr_in serv_addr;

    memset(self_p, 0, sizeof(s3::ips_ping_t));
    memset(&hints, 0, sizeof(hints));


    if (NULL != attr_p) {
	self_p->attr = *attr_p;
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    result = s3::ips_socket_getaddrinfo(servername, NULL, &hints, &ai);
    if (result) {
	DBMS3(stderr, "sms_connect : unknown host %s(%s)" EOL_CRLF, servername,
	      gai_strerror(result));
	status = EINVAL;
	goto out;
    }

    if ((unsigned) ai->ai_addrlen > sizeof(serv_addr)) {

	IFDBG1THEN {
	    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	    char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	    char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

	    mpxx_u64toadec( (uint64_t)ai->ai_addrlen, xtoa_buf_a, xtoa_bufsz);
	    mpxx_u64toadec( (uint64_t)sizeof(serv_addr), xtoa_buf_b, xtoa_bufsz);
	    DBMS1(stdout,
	      "ping_connect : sockaddr too large (%s) > (%s)" EOL_CRLF,
	        xtoa_buf_a, xtoa_buf_b);
	}

	status = -1;
	goto out;
    }

    e = (ping_ext_t *)
	malloc(sizeof(ping_ext_t));
    if (NULL == e) {
	return ENOMEM;
    }
    self_p->ext = (void *) e;

    memcpy(&serv_addr, ai->ai_addr, ai->ai_addrlen);
    memcpy(&e->serv_addr, &serv_addr, sizeof(struct sockaddr_in));

    e->buf = (uint8_t *) malloc(BUF_SIZE);
    if (NULL == e->buf) {
	DBMS1(stdout, "ping_connect : buf mallic fail" EOL_CRLF);
	status = -1;
    }
    e->send_dat = e->buf;
    e->recv_buf = (void *) ((uint8_t *) e->buf + (BUF_SIZE / 2));
    self_p->len = len;

    status = 0;

#if defined(_WIN32_PINGPROC)
    e->hIcmp = IcmpCreateFile();
    if (e->hIcmp == INVALID_HANDLE_VALUE) {
	status = ECONNABORTED;
    }
#elif defined(_POSIX_PINGPROC)	/* POSIX and like */
    e->fd = s3::ips_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (e->fd < 0) {
	DBMS1(stdout, "ping_connect : s3::ips_socket fail" EOL_CRLF);
	status = ECONNABORTED;
    }
#endif

    status = 0;

  out:

    if (NULL != ai) {
	s3::ips_socket_freeaddrinfo(ai);
    }

    if (status) {
	DBMS1(stdout, "ping_connect : Can't connect to %s" EOL_CRLF, servername);
	s3::ips_ping_disconnect(self_p);
	return -1;
    }

    return 0;
#endif
}

void s3::ips_ping_disconnect(s3::ips_ping_t * self_p)
{
    ping_ext_t *__restrict const e = get_ping_ext(self_p);

    if (NULL == e) {
	return;
    }

#if defined(_WIN32_PINGPROC)
    IcmpCloseHandle(e->hIcmp);

#elif defined(_POSIX_PINGPROC)
    if (!(e->fd < 0)) {
	int result;

	result = s3::ips_socket_shutdown(e->fd, SHUT_RDWR);
	if (result) {
	    DBMS1(stdout, "ping_disconnect : shutdown socket fail" EOL_CRLF);
	    return;
	}

	/* close socket */
	s3::ips_socket_close(e->fd);
    }
#endif

    if (NULL != e->buf) {
	free(e->buf);
	e->buf = NULL;
    }

    if (NULL != self_p->ext) {
	free(self_p->ext);
	self_p->ext = NULL;
    }

    return;
}

#if defined(_POSIX_PINGPROC)
static uint16_t in_cksum(uint16_t * addr, int len)
{
    int nleft = len;
    int sum = 0;
    uint16_t *where = addr;

    while (nleft > 1) {
	sum += *where++;
	nleft -= 2;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (~sum);
}

static int send_icmp(s3::ips_ping_t * self_p)
{
    ping_ext_t *__restrict const e = get_ping_ext(self_p);
    struct icmp *__restrict toicmp = e->send_dat;

    DBMS4(stdout, "send_icmp : exec" EOL_CRLF);

    toicmp->icmp_type = ICMP_ECHO;
    toicmp->icmp_code = 0;
    toicmp->icmp_id = getpid();
    toicmp->icmp_seq = e->seq++;
    gettimeofday((struct timeval *) toicmp->icmp_data, NULL);
    toicmp->icmp_cksum = 0;
    toicmp->icmp_cksum = in_cksum((u_short *) toicmp, self_p->len);

    if (sendto
	(e->fd, toicmp, self_p->len, 0, (struct sockaddr *) &e->serv_addr,
	 sizeof(struct sockaddr)) <= 0) {
	DBMS4(stdout, "send_icmp : sendto" EOL_CRLF);
	return -1;
    }

    return 0;
}

static void
time_diff(struct timeval *sendval, struct timeval *recvval,
	  struct timeval *diff)
{
    DBMS4(stdout, "sendval->tv_sec="FMT_LLD" sendval->tv_usec="FMT_LLD EOL_CRLF,
	  (long long int) sendval->tv_sec,
	  (long long int) sendval->tv_usec);

    DBMS4(stdout, "recvval->tv_sec="FMT_LLD" recvval->tv_usec="FMT_LLD EOL_CRLF,
	  (long long int) recvval->tv_sec,
	  (long long int) recvval->tv_usec);

    diff->tv_sec = recvval->tv_sec - sendval->tv_sec;
    if ((diff->tv_usec = recvval->tv_usec - sendval->tv_usec) < 0) {
	--diff->tv_sec;
	diff->tv_usec += 1000000;
    }

    return;
}

static int recv_icmp(s3::ips_ping_t * self_p)
{
    ips_socket_ping_ext_t *__restrict const e =
	(ips_socket_ping_ext_t *) self_p->ext;
    struct timeval recvtime;
    struct timeval rtt = { 0, 0 };
    int hdlen;
    int save_cksum;
    int rsize;
    int icmplen;
    struct sockaddr_in fromservaddr;
    socklen_t fromaddrlen;
    struct icmp *fromicmp;
    struct icmp *toicmp;
    struct ip *ip;

    fromaddrlen = (BUF_SIZE / 2);
    if ((rsize =
	 recvfrom(e->fd, e->recv_buf, (BUF_SIZE / 2), 0,
		  (struct sockaddr *) &fromservaddr, &fromaddrlen)) < 0) {
	DBMS1(stdout, "recv_icmp : recvfrom fail" EOL_CRLF);
	self_p->last_error = s3::IPS_PING_EPACKET;
	return -1;
    }

    gettimeofday(&recvtime, NULL);

    toicmp = (struct icmp *) e->send_dat;
    ip = (struct ip *) e->recv_buf;
    hdlen = ip->ip_hl << 2;
    fromicmp = (struct icmp *) (e->recv_buf + hdlen);
    icmplen = rsize - hdlen;

    if (icmplen < 8) {
	DBMS1(stdout, "icmplen("FMT_LLD") < 8" EOL_CRLF, (long long)icmplen);
	self_p->last_error = s3::IPS_PING_EPACKET;
	return -1;
    } else {
	DBMS4(stdout, "icmplen("FMT_LLD")" EOL_CRLF, (long long)icmplen);
    }

    if (fromicmp->icmp_type != ICMP_ECHOREPLY) {
	self_p->last_error = s3::IPS_PING_EPACKET;
	return -1;
    }

    if (fromicmp->icmp_id == toicmp->icmp_id && icmplen >= 16) {
	time_diff((struct timeval *) fromicmp->icmp_data, &recvtime, &rtt);
    }

    save_cksum = fromicmp->icmp_cksum;
    fromicmp->icmp_cksum = 0;

    if (save_cksum == in_cksum((u_short *) fromicmp, icmplen)) {
	DMSG(stdout,
	      FMT_LLD" bytes from %s : type="FMT_LLD", code="FMT_LLD", cksum="FMT_LLX", id="FMT_LLD", seq="FMT_LLD", ttl="FMT_LLD", rtt=%.3f ms" EOL_CRLF,
	      (long long)icmplen, inet_ntoa(fromservaddr.sin_addr),
	      (long long)fromicmp->icmp_type, (long long)fromicmp->icmp_code,
	      (long long)save_cksum, (long long)fromicmp->icmp_id, 
	      (long long)fromicmp->icmp_seq, (long long)ip->ip_ttl,
	      (rtt.tv_sec * 1000.0) + (rtt.tv_usec / 1000.0));
    } else {
	DMSG(stdout, "Bad checksum echo reply received" EOL_CRLF);
	DMSG(stdout, "Correct checksum: "FMT_LLX EOL_CRLF,
	      (long long)in_cksum((u_short *) fromicmp, icmplen));
	DMSG(stdout,
	      FMT_LLD" bytes from %s : type="FMT_LLD", code="FMT_LLD", cksum="FMT_LLX", id="FMT_LLD", seq="FMT_LLD", ttl="FMT_LLD", rtt=%.3f ms" EOL_CRLF,
	      (long long)icmplen, inet_ntoa(fromservaddr.sin_addr),
	      (long long)fromicmp->icmp_type, (long long)fromicmp->icmp_code,
	      (long long)save_cksum, (long long)fromicmp->icmp_id,
	      (long long)fromicmp->icmp_seq, (long long)ip->ip_ttl,
	      (rtt.tv_sec * 1000.0) + (rtt.tv_usec / 1000.0));
	self_p->last_error = s3::IPS_PING_EPACKET;
	return -1;
    }

    return 0;
}
#endif				/* end of _POSIX_PINGPROC */

#if defined(_WIN32_PINGPROC)
static int
win32_socket_ping_exec_ipv4(s3::ips_ping_t * self_p,
			    size_t timeout_sec)
{
    const DWORD timeout_msec = (DWORD) (timeout_sec * 1000);
    ping_ext_t *__restrict const e = get_ping_ext(self_p);
    DWORD dwRetVal;
    PICMP_ECHO_REPLY pIcmpEchoReply = (PICMP_ECHO_REPLY) e->recv_buf;

    dwRetVal = IcmpSendEcho(e->hIcmp,
			    *(IPAddr *) & e->serv_addr.sin_addr,
			    e->send_dat, (WORD) self_p->len,
			    NULL, e->recv_buf,
			    (DWORD) self_p->len, timeout_msec);

    if (dwRetVal != 0) {
	switch (pIcmpEchoReply->Status) {
	case IP_SUCCESS: {
	    DMSG(stdout,
		  FMT_LLD" bytes from %s : ttl="FMT_LLD", rtt=%.3f ms" EOL_CRLF,
		  (long long)pIcmpEchoReply->DataSize,
		  inet_ntoa(*(struct in_addr *) pIcmpEchoReply->Address),
		  (long long)pIcmpEchoReply->Options.Ttl,
		  (double) pIcmpEchoReply->RoundTripTime);
	    return 0;
	    }
	case IP_BUF_TOO_SMALL:
	case IP_PACKET_TOO_BIG:
	case IP_BAD_REQ:
	case IP_GENERAL_FAILURE:
	    self_p->last_error = s3::IPS_PING_EPACKET;
	    return -1;
	case IP_DEST_HOST_UNREACHABLE:
	case IP_DEST_PROT_UNREACHABLE:
	case IP_DEST_PORT_UNREACHABLE:
	    self_p->last_error = s3::IPS_PING_ENOECHO;
	    return -1;
	default:
	    self_p->last_error = s3::IPS_PING_EPACKET;
	    return -1;
	}
    } else {
	DMSG( stdout, "\tCall to IcmpSendEcho failed." EOL_CRLF);
	DMSG( stdout, "\tIcmpSendEcho returned error: " FMT_LLD EOL_CRLF,
	    (long long int)GetLastError());
	return 1;
    }

    self_p->last_error = s3::IPS_PING_EPACKET;
    return -1;
}

static int
win32_socket_ping_exec_ipv6(s3::ips_ping_t * p, size_t timeout_sec)
{
    return ENOSYS;
}

#endif

int s3::ips_ping_exec(s3::ips_ping_t * self_p,
			     size_t timeout_sec)
{
#if defined(_MINGW32_PINGPROC)
    (void) self_p;
    (void) timeout_sec;

    return ENOSYS;
#elif defined(_WIN32_PINGPROC)
    if (self_p->attr.f.is_ipv6) {
	return win32_socket_ping_exec_ipv6(self_p, timeout_sec);
    }

    return win32_socket_ping_exec_ipv4(self_p, timeout_sec);

#elif defined(_POSIX_PINGPROC)
    int result;
    fd_set fds;
    struct timeval tv;
    ips_socket_ping_ext_t *__restrict const e =
	(ips_socket_ping_ext_t *) self_p->ext;

    result = send_icmp(self_p);
    if (result) {
	DBMS1(stdout, "ping_exec : send_icmp fail" EOL_CRLF);
	return result;
    }

    FD_ZERO(&fds);
    FD_SET(e->fd, &fds);

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    result =
	select(FD_SETSIZE, &fds, (fd_set *) NULL, (fd_set *) NULL, &tv);
    if (!result) {
	/* タイムアウト */
	DBMS1(stdout, "ping_exec : socket read timeout" EOL_CRLF);

	self_p->last_error = s3::IPS_PING_ENOECHO;
	return -1;
    }

    return recv_icmp(self_p);
#else
#error not impliment s3::ips_ping_exec()
#endif
}

int s3::ips_ping_ipv4(const char *servername, size_t timeout_sec)
{
    int status, result;
    s3::ips_ping_t p;

    result = s3::ips_ping_connect(&p, servername, 56, NULL);
    if (result) {
	return result;
    }

    result = s3::ips_ping_exec(&p, timeout_sec);
    if (result) {
	status = result;
	goto out;
    }

    status = 0;

  out:
    s3::ips_ping_disconnect(&p);

    return status;
}
