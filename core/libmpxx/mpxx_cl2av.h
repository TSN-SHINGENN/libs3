#ifndef INC_MPXX_CL2AV_H
#define INC_MPXX_CL2AV_H

#pragma once

#include <errno.h>
#include <stdint.h>

/**
 * @typedef mpxx_cl2av_attr_t
 * @brief mpxx_cl2avの属性フラグ集合を設定するunion共同体　
 */
typedef union _mpxx_cl2av_attr {
    unsigned int flags;			/*!< ビットフラグを0クリアする時に使用 */
    struct {
	unsigned int untrim_space:1;	/*!< 空白文字を削除します */
    } f;
} mpxx_cl2av_attr_t; 

/**
 * @typedef multios_gof_cmd_makeargv_t
 * @brief multios_gof_cmd_makeargv_tクラス構造体定義です。
 *	文字列を解析して要素に分割します
 */
typedef struct _mpxx_cl2av {
    void *ext;
    mpxx_cl2av_attr_t attr; /*! 属性フラグ集合 */
    size_t argc;				  /*!< 分解された要素数 */
    char **argv;			  /*!< 分解された要素文字列のポインタ配列 */
} mpxx_cl2av_t;

int mpxx_cl2av_init(mpxx_cl2av_t *const self_p, const char *const delim, const char delim2, const mpxx_cl2av_attr_t *const  attr_p);
int mpxx_cl2av_execute( mpxx_cl2av_t *const self_p, const char *const sentence);
void mpxx_cl2av_destroy( mpxx_cl2av_t *const self_p);

int mpxx_cl2av_lite( char * const __restrict str, const char * __restrict delim, char delim2, int *const __restrict argc_p, char ** __restrict argv_p, const unsigned int limit, mpxx_cl2av_attr_t *const attr_p );

#endif /* end of INC_MPXX_CL2AV */
