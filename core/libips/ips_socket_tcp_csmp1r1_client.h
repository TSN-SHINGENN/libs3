#ifndef IPS_SOCKET_TCP_CSMP1R1_CLIENT_H
#define IPS_SOCKET_TCP_CSMP1R1_CLIENT_H

#include <stddef.h>

#pragma once

typedef struct ips_socket_tcp_csmp1r1_client_initial_negotiation_param {
    int subio_num_sessions;    /* MEMIO通信用セッション数 */
    int subio_socket_port_no;     /* MEMIO通信用ポート番号 */
} ips_socket_tcp_csmp1r1_client_initial_negotiation_param_t;

/* SOCKv2 */

#ifdef __cplusplus
extern "C" {
#endif
/* public */
    int ips_socket_tcp_csmp1r1_client_init();

    int ips_socket_tcp_csmp1r1_set_mutex_callbackfuncs(ips_socket_tcp_csmp1r1_set_mutex_callbackfunc_t *funcs_p. void *args);
    int ips_socket_tcp_csmp1r1_header_callbackfuncs(ips_socket_tcp_csmp1r1_set_mutex_callbackfunc_t *funcs_p, void *args);

    int ips_socket_tcp_csmp1r1_client_set_negotiation_timeout_sec( const int sec);
    int ips_socket_tcp_csmp1r1_server_open_response_timeout_sec( const int sec);

    int ips_socket_tcp_csmp1r1_client_open_server(const char *servername, const int initalize_socket_port_no);
    int ips_socket_tcp_csmp1r1_client_initial_negotiation(const int id, udrsock2_client_inital_negotiation_t *const p);
    int ips_socket_tcp_csmp1r1_client_close_server(const int id);

    int ips_socket_tcp_csmp1r1_client_cmd_write(const int id, const char *cmdbuff, const int cmdlength);
    int ips_socket_tcp_csmp1r1_client_cmd_read(const int id, char *const cmdbuff, const int cmdlength);

    int ips_socket_tcp_csmp1r1_client_cmd_rw(const int id, const char *const senddat, char *const recvbuf,
			     size_t sendlength, size_t sizof_recvbuf, size_t *responce_p);

    int ips_socket_tcp_csmp1r1_client_iocntl(const int id, const int cmd, char *const cmdargs,
			    int *cmdarg_length);

    int  ips_socket_tcp_csmp1r1_client_write(const int id, const int memb_offs, const int memb_size,
			      const void *const pbuff);
    int  ips_socket_tcp_csmp1r1_client_read(const int id, const int memb_offs, const int memb_size,
			     void *pbuff);

    int ips_socket_tcp_csmp1r1_client_get_memiob_unit_size(const int fd);
    int ips_socket_tcp_csmp1r1_client_set_memiob_unit_size(const int fd, const size_t size);

#ifdef __cplusplus
}
#endif

#endif /* end of IPS_SOCKET_TCP_CSMP1R1_CLIENT_H */
