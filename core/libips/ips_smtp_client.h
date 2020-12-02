#ifndef INC_IPS_SMTP_CLIENT
#define INC_IPS_SMTP_CLIENT

#pragma once

#define IPS_SMTP_DEFAULT_SMTPPORT 25
#define IPS_SMTP_DEFAULT_SOCKETBUFSIZE 4096
#define IPS_SMTP_SERVERNAME_MAX 256

#include <stddef.h>
#include <sys/types.h>

namespace s3 {

typedef struct _ips_smtp_client_t {
    char smtp_servername[IPS_SMTP_SERVERNAME_MAX];
    int port_no;
    int fd;
    time_t connect_timeout_sec;
    time_t command_timeout_sec;
    int send_bufsize;
    char *send_buf;
    size_t recv_bufsize;
    char *recv_buf;

    union {
	unsigned int flags;
	struct {
	    unsigned int is_opened;
	} f;
    } stat;
} ips_smtp_client_t;

int ips_smtp_client_connect_to_server(ips_smtp_client_t *const self_p, const char *const serverip, const char *const servername, int port_no, int timeout_sec);
int ips_smtp_client_disconnect( ips_smtp_client_t *const self_p);
int ips_smtp_client_seq1_preprocess( ips_smtp_client_t *const self_p, char *from_addr, char *to_addr);

int ips_smtp_client_seq2_header(ips_smtp_client_t *const self_p, char *const date, char *const from_addr, char *const to_addr, char *const subject);
int ips_smtp_client_seq3_txtline( ips_smtp_client_t *const self_p, char *linedat, int size );
int ips_smtp_client_seq4_txtend( ips_smtp_client_t *const self_p);
int ips_smtp_client_abort( ips_smtp_client_t *const self_p);
int ips_smtp_client_noopcheck( ips_smtp_client_t *const self_p);

class cips_smtp_client
{
private:
    ips_smtp_client_t obj;

public:
    cips_smtp_client();
    virtual ~cips_smtp_client();
    
    int connect_to_server(const char *const serverip, const char *const servername, int port_no, int timeout_sec);
    int disconnect();
    int seq1_preprocess(char *from_addr, char *to_addr);
    int seq2_header(char *date, char *from_addr, char *to_addr, char *subject );
    int seq3_txtline(char *linedat, int size );
    int seq4_txtend( ips_smtp_client_t *self_p);
    int abort();
    int noopcheck();
};
}

#endif /* end of INC_IPS_SMTP_CLIENT */
