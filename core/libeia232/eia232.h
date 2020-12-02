/*
 * シリアル通信を行うプロトタイプ
 */

#ifndef _INC_LIBEIA232_H
#define _INC_LIBEIA232_H

#include <stdint.h>
#include <stdlib.h>

#if defined(WIN32)
#define EIA232_PATH_MAX _MAX_PATH
#elif defined(Linux) || defined(Darwin) || defined(qnx)
#define EIA232_PATH_MAX PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#if defined (__cplusplus )
extern "C" {
#endif

typedef struct _eia232dev_supported_baudrate {
    int num;
    int *baudrate_table;
} eia232dev_supported_baudrate_t;

/* BAUD RATE */
int eia232dev_new_list_os_supported_baudrate(eia232dev_supported_baudrate_t *list_p);
void eia232dev_delete_list_supported_baudrate( eia232dev_supported_baudrate_t *list_p);

typedef enum _enum_eia232dev_databitstype
{
    EIA232DEV_DATA_5 = 5,
    EIA232DEV_DATA_6 = 6,
    EIA232DEV_DATA_7 = 7,
    EIA232DEV_DATA_8 = 8,
    EIA232DEV_DATA_unknown,
    EIA232DEV_DATA_eod
} enum_eia232dev_databitstype_t;

const char *eia232dev_databitstype2str(enum_eia232dev_databitstype_t databits);

typedef enum _enum_eia232dev_paritytype
{
    EIA232DEV_PAR_NONE = 10,
    EIA232DEV_PAR_ODD,
    EIA232DEV_PAR_EVEN,
    EIA232DEV_PAR_unknown,
    EIA232DEV_PAR_eod
} enum_eia232dev_paritytype_t;
const char *eia232dev_paritytype2str(enum_eia232dev_paritytype_t paritytype);

typedef enum _enum_eia232dev_stopbitstype
{
    EIA232DEV_STOP_1 = 101,
    EIA232DEV_STOP_2,
    EIA232DEV_STOP_unknown,
    EIA232DEV_STOP_eod
} enum_eia232dev_stopbitstype_t;

const char *eia232dev_stopbitstype2str(enum_eia232dev_stopbitstype_t stopbits);

typedef enum _enum_eia232dev_flowtype
{
    EIA232DEV_FLOW_OFF = 110,
    EIA232DEV_FLOW_HW,
    EIA232DEV_FLOW_XONOFF, 
    EIA232DEV_FLOW_unknown, 
    EIA232DEV_FLOW_eod
} enum_eia232dev_flowtype_t;

const char *eia232dev_flowtype2str(enum_eia232dev_flowtype_t flowtype);

typedef struct _eia232dev_table {
    char path[EIA232_PATH_MAX];
} eia232dev_table_t;

typedef struct _eia232dev_list {
    int num_tabs;
    eia232dev_table_t tabs[];
} eia232dev_list_t;

typedef struct _eia232dev_handle {
    void *ext;
} eia232dev_handle_t;

typedef struct _eia232dev_attr {
    int baudrate;
    enum_eia232dev_databitstype_t databits;
    enum_eia232dev_paritytype_t parity;
    enum_eia232dev_stopbitstype_t stopbits;
    enum_eia232dev_flowtype_t flowctrl;
    uint32_t timeout_msec;
} eia232dev_attr_t;

eia232dev_list_t *eia232dev_new_devlist(void);
void  eia232dev_delete_devlist( eia232dev_list_t *const l );

int eia232dev_open( eia232dev_handle_t *const self_p, const char *const path);
void eia232dev_close( eia232dev_handle_t *self);

int eia232dev_get_attribute( eia232dev_handle_t *const self_p, eia232dev_attr_t *const attr_p);
int eia232dev_set_attribute( eia232dev_handle_t *const self_p, const eia232dev_attr_t *const attr_p);

int eia232dev_write_stream( eia232dev_handle_t *const self_p, const void *dat, const size_t sendlen);
int eia232dev_read_stream( eia232dev_handle_t *const self_p, void *buf, const size_t bufsize, size_t *const recvlen_p);

int eia232dev_write_string( eia232dev_handle_t *const self_p, const char *const str, size_t *const retlen_p);
int eia232dev_readln_string( eia232dev_handle_t *const self_p, char *buf, const size_t bufsize, size_t *const recvlen_p);

int eia232dev_clear_recv( eia232dev_handle_t *const self_p);

int eia232dev_string_command_execute( eia232dev_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t szofrecv);

int eia232dev_initialize_buffer( eia232dev_handle_t *const self_p, size_t *const szofsendbuf_p, size_t *const szofresv_p, void *attr_p);



#if defined (__cplusplus )
} 
#endif

#endif /* end of _INC_LIBEIA232_H */
