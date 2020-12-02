#ifndef INC_MULTIOS_TCPIP_MPR1_SERVER_H
#define INC_MULTIOS_TCPIP_MPR1_SERVER_H

typedef struct udrsock_xfer_function_table {
    int (*init)(void);
    void *(*imemio_get_ptr)(unsigned int offs);
    int (*imemio)( int dir, unsigned int offs, char *pdata, int size);
    int (*dwrite)( char *cmd,int size);
    int (*dread)( char *cmd,int size);
    int (*ioctl)( int cmd,char *pdata,int *char_len);
    int (*open)(char *usr_name,char *usr_pass);
    int (*close)(void);
    int (*client_shutdown)(void);
} udrsock_xfer_function_table_t;

/* UDRSOCK サーバ関数 */
int udrsock_open_s(const int SockPort, const udrsock_xfer_function_table_t *funcs); /* ソケットサーバーオープンルーチン */
int udrsock_loop_s(void); /* クライアント監視用ループ */
void udrsock_master_close_s(void);

/* パラメータ変更関数 */
int udrsock_set_memiob_size( const int size );

#endif /* end of INC_MULTIOS_TCPIP_MPR1_SERVER_H */
