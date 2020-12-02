#ifndef INC_MPSX_CL2AV_H
#define INC_MPSX_CL2AV_H

#pragma once

#include <errno.h>
#include <stdint.h>

/**
 * @typedef mpsx_cl2av_attr_t
 * @brief mpsx_cl2avの属性フラグ集合を設定するunion共同体　
 */
typedef union _mpsx_cl2av_attr {
    unsigned int flags;			/*!< ビットフラグを0クリアする時に使用 */
    struct {
	unsigned int untrim_space:1;	/*!< 空白文字を削除します */
    } f;
} mpsx_cl2av_attr_t; 

/**
 * @typedef mpsx_gof_cmd_makeargv_t
 * @brief mpsx_gof_cmd_makeargv_tクラス構造体定義です。
 *	文字列を解析して要素に分割します
 */
typedef struct _mpsx_cl2av {
    void *ext;
    mpsx_cl2av_attr_t attr; /*! 属性フラグ集合 */
    size_t argc;				  /*!< 分解された要素数 */
    char **argv;			  /*!< 分解された要素文字列のポインタ配列 */
} mpsx_cl2av_t;

int mpsx_cl2av_init(mpsx_cl2av_t *const self_p, const char *const delim, const char delim2, const mpsx_cl2av_attr_t *const  attr_p);
int mpsx_cl2av_execute( mpsx_cl2av_t *const self_p, const char *const sentence);
void mpsx_cl2av_destroy( mpsx_cl2av_t *const self_p);

int mpsx_cl2av_lite( char * const __restrict str, const char * __restrict delim, char delim2, int *const __restrict argc_p, char ** __restrict argv_p, const unsigned int limit, mpsx_cl2av_attr_t *const attr_p );

#endif /* end of INC_MPSX_CL2AV */
