#ifndef INC_MULTIOS_TCPIPV4_MPR2_SERVER_H
#define INC_MULTIOS_TCPIPV4_MPR2_SERVER_H

#include <stddef.h>
#include <stdint.h>

typedef struct _multios_tcpip_mpr2_server_function_table {
    int (*init)(void);
    void *(*imemio_get_ptr)(unsigned int offs);
    int (*imemio)( int dir, unsigned int offs, char *pdata, int size);
    int (*dwrite)( char *cmd,int size);
    int (*dread)( char *cmd,int size);
    int (*ioctl)( int cmd,char *pdata,int *char_len);
    int (*open)(char *usr_name,char *usr_pass);
    int (*close)(void);
    int (*client_shutdown)(void);
} multios_tcpip_mpr2_server_callback_function_table_t;

extern int reusraddr_flag;

#if defined(__cplusplus)
extern "C" {
#endif

int multios_tcpip_mpr2_server_attach(const char *const IPv4_ServAddr, const uint32_t SockPort, const multios_tcpip_mpr2_server_callback_function_table_t *const funcs);
void multios_tcpip_mpr2_server_close(void);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MULTIOS_TCPIP_MPR2_SERVER_H */
