/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-August-07 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_memareacpy.c
 *  二次元バッファの領域から矩形領域を指定してコピーします。
 * CPUがコア数に関わらずメインメモリコントロールエンジンが一つしかないので並列化できない
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

/* this */
#include "mpxx_stdlib.h"
#include "mpxx_string.h"
#include "mpxx_memareacpy.h"

#ifdef DEBUG
static int debuglevel = 5;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MPXX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _memareacpy_param {
    size_t line_length;
    size_t num_total_lines;
    size_t valid_line_length;
    size_t num_valid_lines;

    size_t total_length;

    void *(*func_memcpy) (void *, const void *, size_t);

    const void *src;
    void *dest;

    mpxx_memareacpy_ext_t ext;
    mpxx_memareacpy_attr_t attr;
} memareacpy_param_t;

static void memareacpy_exec(int thread_no, void *args, size_t start_line,
			    size_t num_lines);

/**
 * @fn static void memareacpy_exec( int thread_no, void *args, size_t start_line, size_t num_lines)
 * @brief メモリコピー本体コールバック関数
 * @param thread_no スレッド番号
 * @param args 引数ポインタ
 * @param start_line 開始ポイント番号
 * @param lines 担当ライン数
 */
static void memareacpy_exec(int thread_no, void *args, size_t start_line,
			    size_t num_lines)
{
    memareacpy_param_t *p = (memareacpy_param_t *) args;
    const uint8_t *__restrict psrc =
	(const uint8_t *) p->src + (start_line * p->line_length);
    uint8_t *__restrict pdest =
	(uint8_t *) p->dest + (start_line * p->line_length);
    size_t l;

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	mpxx_i64toadec( (int64_t)thread_no, xtoa_buf, xtoa_bufsz);
	DBMS5(stderr, "memareacpy_exec : thread_no = %s" EOL_CRLF,
	    xtoa_buf);
    }

    if (p->ext.stat.f.src_parameter_is_valid) {
	psrc +=
	    (p->ext.src_start_valid_line * p->ext.src_start_ofs_in_line);
	psrc += p->ext.src_start_ofs_in_line;
    }

    if (p->ext.stat.f.dest_parameter_is_valid) {
	pdest +=
	    (p->ext.dest_start_valid_line * p->ext.dest_start_ofs_in_line);
	pdest += p->ext.dest_start_ofs_in_line;
    }

    for (l = start_line; l < num_lines; l++) {
	p->func_memcpy(pdest, psrc, p->valid_line_length);

	psrc += p->line_length;
	pdest += p->line_length;
    }

    return;
}

/**
 * @fn int mpxx_memareacpy( void *dest, const void *src, size_t line_length, size_t num_total_lines, size_t valid_line_length, size_t num_valid_lines, mpxx_memareacpy_ext_t *ext_p)
 * @brief 指定された二次元バッファ領域から矩形領域をコピーします。
 * @param dst_p コピー先バッファポインタ
 * @param src_p コピー元バッファポインタ
 * @param line_length 水平ライン長
 * @param num_total_lines 水平ライン数
 * @param valid_line_length 有効ライン長
 * @param num_valid_lines 比較水平ライン数
 * @param ext 拡張パラメータポインタ(有効矩形領域にオフセットを付けたい場合に使用)
 *	  ※   extは現在実装していません
 * @retval 0 成功
 * @retval EINVAL 不正なパラメータが与えられた
 */
int mpxx_memareacpy(void *dest, const void *src, size_t line_length,
		       size_t num_total_lines, size_t valid_line_length,
		       size_t num_valid_lines,
		       mpxx_memareacpy_ext_t * ext_p,
		       mpxx_memareacpy_attr_t * attr_p)
{
    memareacpy_param_t cpyparam;

    DBMS1(stderr, "mpxx_memareacpy : execute" EOL_CRLF);

    if ((NULL == dest) || (NULL == src)) {
	return EINVAL;
    }

    memset(&cpyparam, 0x0, sizeof(cpyparam));
    cpyparam.src = src;
    cpyparam.dest = dest;
    cpyparam.line_length = line_length;
    cpyparam.num_total_lines = num_total_lines;
    cpyparam.total_length = num_total_lines * line_length;
    cpyparam.valid_line_length = valid_line_length;
    cpyparam.num_valid_lines = num_valid_lines;

    if (NULL != ext_p) {
	cpyparam.ext = *ext_p;
    }

    if (NULL != attr_p) {
	cpyparam.attr = *attr_p;
    }

    if (cpyparam.valid_line_length > cpyparam.line_length) {
	return EINVAL;
    }

    if (cpyparam.num_total_lines > cpyparam.num_valid_lines) {
	return EINVAL;
    }

    if (cpyparam.attr.f.use_func_is_stdmemcpy) {
	cpyparam.func_memcpy = memcpy;
    } else {
	cpyparam.func_memcpy =
	    (cpyparam.attr.f.
	     use_func_is_path_through_level_cache) ? mpxx_memcpy :
	    mpxx_memcpy_path_through_level_cache;
    }

    if (cpyparam.attr.f.dest_is_cleared_to_zero) {
	mpxx_memset_path_through_level_cache(cpyparam.dest, 0x0,
						cpyparam.total_length);
    }

    memareacpy_exec(-1, &cpyparam, 0, cpyparam.num_valid_lines);

    return 0;
}
