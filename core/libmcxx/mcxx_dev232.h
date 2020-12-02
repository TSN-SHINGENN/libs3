/*
 * シリアル通信を行うプロトタイプ
 */

#ifndef INC_MCXX_DEV232_H
#define INC_MCXX_DEV232_H

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(WIN32)
#define MCXX_DEV232_PATH_MAX _MAX_PATH
#elif defined(Linux) || defined(Darwin) || defined(qnx)
#define MCXX_DEV232_PATH_MAX PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

namespace s3 {

typedef struct _mcxx_dev232_supported_baudrate {
    size_t num;
    int *baudrate_table;
} mcxx_dev232_supported_baudrate_t;

/* BAUD RATE */
int mcxx_dev232_new_list_os_supported_baudrate(mcxx_dev232_supported_baudrate_t *list_p);
void mcxx_dev232_delete_list_supported_baudrate( mcxx_dev232_supported_baudrate_t *list_p);

typedef enum _enum_mcxx_dev232_databitstype
{
    MCXX_DEV232_DATA_5 = 5,
    MCXX_DEV232_DATA_6 = 6,
    MCXX_DEV232_DATA_7 = 7,
    MCXX_DEV232_DATA_8 = 8,
    MCXX_DEV232_DATA_unknown,
    MCXX_DEV232_DATA_eod
} enum_mcxx_dev232_databitstype_t;

const char *mcxx_dev232_databitstype2str(enum_mcxx_dev232_databitstype_t databits);

typedef enum _enum_mcxx_dev232_paritytype
{
    MCXX_DEV232_PAR_NONE = 10,
    MCXX_DEV232_PAR_ODD,
    MCXX_DEV232_PAR_EVEN,
    MCXX_DEV232_PAR_unknown,
    MCXX_DEV232_PAR_eod
} enum_mcxx_dev232_paritytype_t;
const char *mcxx_dev232_paritytype2str(enum_mcxx_dev232_paritytype_t paritytype);

typedef enum _enum_mcxx_dev232_stopbitstype
{
    MCXX_DEV232_STOP_1 = 101,
    MCXX_DEV232_STOP_2,
    MCXX_DEV232_STOP_unknown,
    MCXX_DEV232_STOP_eod
} enum_mcxx_dev232_stopbitstype_t;

const char *mcxx_dev232_stopbitstype2str(enum_mcxx_dev232_stopbitstype_t stopbits);

typedef enum _enum_mcxx_dev232_flowtype
{
    MCXX_DEV232_FLOW_OFF = 110,
    MCXX_DEV232_FLOW_HW,
    MCXX_DEV232_FLOW_XONOFF, 
    MCXX_DEV232_FLOW_unknown, 
    MCXX_DEV232_FLOW_eod
} enum_mcxx_dev232_flowtype_t;

const char *mcxx_dev232_flowtype2str(enum_mcxx_dev232_flowtype_t flowtype);

typedef struct _mcxx_dev232_table {
    char path[MCXX_DEV232_PATH_MAX];
} mcxx_dev232_table_t;

typedef struct _mcxx_dev232_list {
    int num_tabs;
    mcxx_dev232_table_t tabs[1];
} mcxx_dev232_list_t;

typedef struct _mcxx_dev232_handle {
    void *ext;
} mcxx_dev232_handle_t;

typedef struct _mcxx_dev232_attr {
    int baudrate;
    enum_mcxx_dev232_databitstype_t databits;
    enum_mcxx_dev232_paritytype_t parity;
    enum_mcxx_dev232_stopbitstype_t stopbits;
    enum_mcxx_dev232_flowtype_t flowctrl;
    uint32_t timeout_msec;
} mcxx_dev232_attr_t;

mcxx_dev232_list_t *mcxx_dev232_new_devlist(void);
void  mcxx_dev232_delete_devlist( mcxx_dev232_list_t *const l );

int mcxx_dev232_open( mcxx_dev232_handle_t *const self_p, const char *const path);
void mcxx_dev232_close( mcxx_dev232_handle_t *self);

int mcxx_dev232_get_attr( mcxx_dev232_handle_t *const self_p, mcxx_dev232_attr_t *const attr_p);
int mcxx_dev232_set_attr( mcxx_dev232_handle_t *const self_p, const mcxx_dev232_attr_t *const attr_p);

int mcxx_dev232_write_stream_withtimeout( mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen, size_t *const rlen_p, int timeout_msec);
int mcxx_dev232_write_stream( mcxx_dev232_handle_t *const self_p, const void *dat, const size_t sendlen);

int mcxx_dev232_read_stream_withtimeout( mcxx_dev232_handle_t *const self_p, void *const buf, const int length, size_t *const rlen_p, const int timeout_msec);
int mcxx_dev232_read_stream( mcxx_dev232_handle_t *const self_p, void *buf, const size_t bufsize, size_t *const recvlen_p);

int mcxx_dev232_write_string( mcxx_dev232_handle_t *const self_p, const char *const str, size_t *const retlen_p);
int mcxx_dev232_readln_string( mcxx_dev232_handle_t *const self_p, char *buf, const size_t bufsize, size_t *const recvlen_p);

int mcxx_dev232_clear_recv( mcxx_dev232_handle_t *const self_p);

int mcxx_dev232_string_command_execute( mcxx_dev232_handle_t *const self_p, const char *send_cmd, char *recv_str, const size_t szofrecv);

int mcxx_dev232_initialize_buffer( mcxx_dev232_handle_t *const self_p, size_t *const szofsendbuf_p, size_t *const szofresv_p, void *attr_p);

}

#endif /* end of INC_MCXX_DEV232_H */
