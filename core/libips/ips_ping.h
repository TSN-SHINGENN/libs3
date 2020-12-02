#ifndef INC_IPS_PING_H
#define INC_IPS_PING_H

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace s3 {

typedef enum {
    IPS_PING_EACCES = 600,
    IPS_PING_ECONNECT,
    IPS_PING_ENOECHO,
    IPS_PING_EPACKET
} enum_ips_ping_error_t;

typedef union {
    struct {
	unsigned int is_ipv6:1;
    } f;
    unsigned int flags;
} ips_ping_attr_t;

typedef struct {
    ips_ping_attr_t attr;
    size_t len;
    enum_ips_ping_error_t last_error;
    void *ext;
} ips_ping_t;


int ips_ping_connect( ips_ping_t *p, const char *const servername, const size_t len, ips_ping_attr_t *attr_p);
int ips_ping_exec(ips_ping_t * p, size_t timeout_sec);
void ips_ping_disconnect( ips_ping_t *p );
ips_ping_t ips_ping_get_lasterr( ips_ping_t *p );

int ips_ping_ipv4( const char *const servername, size_t timeout_sec );
int ips_ping_ipv6( const char *const servername, size_t timeout_sec );
int ips_ping_is_found_target_networkname(const char *tagetnetname);

}

#endif /* INC_IPS_PING_H */
