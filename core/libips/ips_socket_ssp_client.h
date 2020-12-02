#ifndef INC_IPS_SOCKET_SSP_CLIENT_H
#define INC_IPS_SOCKET_SSP_CLIENT_H

#include <stddef.h>

#if defined (__cplusplus )
extern "C" {
#endif

int ips_socket_ssp_client_connect(const char *const IpAdress, const int tcp_port, const int timeout_sec);
int ips_socket_ssp_client_send_cmdstr(const int socket_fd, const char *const command, const int timeout_sec);
int ips_socket_ssp_client_recv_reply(const int socket_id, char *const recv_buf, const size_t bufsize, const int timeout_sec);
int ips_socket_ssp_client_disconnect(const int socket_id);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_IPS_SOCKET_SSP_CLIENT_H */
