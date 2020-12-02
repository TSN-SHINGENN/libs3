#ifndef INC_IPS_SOCKET_H
#define INC_IPS_SOCKET_H

#pragma once

#include <stdint.h>
#include <time.h>

#if defined(SunOS) || defined(Linux) || defined(QNX) || defined(Darwin)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if defined(WIN32)
#if defined(_WIN32_WINNT)
#if (_WIN32_WINNT < 0x0501 )
#warning _WIN32_WINNT is redefined to 0x0501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501	/* このソースコードは WindowsXP以上を対象にします。 */
#if defined(WINVER)
#undef WINVER
#define WINVER 0x0501
#endif
#endif
#else
#define _WIN32_WINNT 0x0501	/* このソースコードは WindowsXP以上を対象にします。 */
#define WINVER 0x0501
#endif
#include <Winsock2.h>
#include <ws2tcpip.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x40
#endif
#endif

#define IPS_SOCKET_IF_MAX 20

namespace s3 {

#ifdef SunOS
typedef size_t ips_socket_socklen_t;
#endif

#ifdef WIN32
typedef int ips_socket_socklen_t;
#define SHUT_RD ( SD_RECEIVE )
#define SHUT_WR ( SD_SEND )
#define SHUT_RDWR ( SD_BOTH )
#endif

#if defined(QNX) || defined(Linux) || defined(Darwin) || defined(__AVM2__)
typedef socklen_t ips_socket_socklen_t;
#endif

typedef struct _ips_socket_interface_tab {
    struct in_addr ipv4addr;
    struct in_addr ipv4mask;
} ips_socket_interface_tab_t;

typedef struct _ips_socket_interface_list {
    unsigned int num;
    ips_socket_interface_tab_t ifinfo[IPS_SOCKET_IF_MAX];
} ips_socket_interface_list_t;

typedef union _ips_socket_server_openforIPv4_attr {
    unsigned int flags;
    struct {
	unsigned int is_using_linger_option:1;
    } f;
} ips_socket_server_openforIPv4_attr_t;

int ips_socket_client_open( const char *servername, const int port, const int timeout_sec, const int socket_family, const int socket_type, const int protocol, int (*func_setsockopt)(const int fd, void*const args), void *const args);
int ips_socket_server_openforIPv4( const uint32_t nIPAddr, const int port, const int backlog, const int num_bind_retry, const int socket_famiry, const int socket_type, const int protocol, ips_socket_server_openforIPv4_attr_t *attr_p);

int ips_socket( const int socket_family, const int socket_type, const int protocol); 
int ips_socket_connect_withtimeout(const int fd, const struct sockaddr *serv_addr, const unsigned int addr_size, const int timeout);
int ips_socket_send_withtimeout(const int fd, const void *ptr, const size_t len, const int flags, const int timeout_sec);
int ips_socket_recv_withtimeout(const int fd, void *ptr, const size_t len, const int flags, const int timeout);
int ips_socket_shutdown( const int fd, const int how );
int ips_socket_close( const int fd );

int ips_socket_set_send_windowsize(const int fd, const int send_win);
int ips_socket_set_recv_windowsize(const int fd, const int recv_win);
int ips_socket_get_windowsize( const int fd, int *sendbuf_size_p, int *recvbuf_size_p);
int ips_socket_accept( const int server_open_fd, struct sockaddr *addr, ips_socket_socklen_t *addrlen);
int ips_socket_nodelay( const int fd, const int on );
int ips_socket_set_nagle(const int fd, const int flag);
int ips_socket_clear_recv( const int fd);

int ips_socket_get_valid_interface_list( ips_socket_interface_list_t *if_p);
int ips_socket_set_udp_broadcast( const int fd, int is_active);
int ips_socket_set_nonblock_mode( const int fd,  int is_active);

int ips_socket_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

int ips_socket_sendto( const int sockfd, const void *buf, size_t len, int fags,
	const struct sockaddr *dest_addr, ips_socket_socklen_t addrlen);

int ips_socket_recvfrom(const int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *const src_addr, ips_socket_socklen_t *const addrlen);

int ips_socket_connect( const int sockfd, const struct sockaddr *addr, ips_socket_socklen_t addrlen);

int ips_socket_getaddrinfo( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

void ips_socket_freeaddrinfo( struct addrinfo *res);

const char *ips_socket_gai_strerror( int errcode);

int ips_socket_get_tcp_mss( const int sockfd, size_t *const sizof_mss_p );

int ips_socket_inet_aton(const char *const ipv4_str, struct in_addr *const ipv4_in_addr_p);

int ips_socket_check_accessible_ipv4_local_area( const char *const remote_or_ipv4_str, struct in_addr *const ipv4_in_addr_p, int *const eai_errno_p);

#ifdef WIN32 /* dedicated winsock */
typedef struct _win32_socket_winsock_version {
    uint8_t major;
    uint8_t minor;
} _win32_socket_winsock_version_t;

void _win32_socket_set_winsock_startup_version( const _win32_socket_winsock_version_t ver);
int _win32_socket_get_winsock_wsadata( _win32_socket_winsock_version_t *const ver_p);
int _win32_socket_WSAStartup_once(void);
#endif

}

#endif /* end of INC_IPS_SOCKET_H */
