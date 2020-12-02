/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_string.c
 * @brief POSIXの文字列系メモリ制御関数ライブラリコード
 *  CPUのSIMD対応状況を調べて最適な関数を呼び出します。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#if defined(Linux)
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* this */
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_malloc.h"
#if defined(_S3_X86SIMD_ENABLE)
#include "mpsx_simd_string.h"
#endif
#include "mpsx_string.h"

/* Debug */
#if defined(__GNUC__)
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

static char *own_special_strchr( const char *const str, const int code, const char **last_char);

#if 0 && defined(_LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern void *mpsx_memcpy(void *const dest, const void *const src, const size_t n)
		__attribute__ ((optimize("Os")));
extern void *mpsx_memcpy_path_through_level_cache(void *const dst, const void *const src, const size_t n)
		__attribute__ ((optimize("Os")));
extern void *mpsx_memset_path_through_level_cache(void *const d, const int c, const size_t n)
		__attribute__ ((optimize("Os")));

extern int mpsx_strcasecmp(const char *const s1, const char *const s2)
		__attribute__ ((optimize("Os")));
extern int mpsx_strncasecmp(const char *const s1, const char *const s2, const size_t n)
		__attribute__ ((optimize("Os")));
extern int mpsx_strtoupper(const char *const before_str, char *const after_buf, const size_t after_bufsize)
		__attribute__ ((optimize("Os")));
extern int mpsx_strtolower(const char *const before_str, char *const after_buf, const size_t after_bufsize)
		__attribute__ ((optimize("Os")));
extern char *mpsx_strcasestr(const char *const haystack, const char *const needle)
		__attribute__ ((optimize("Os")));
extern char *mpsx_strtrim(char *const buf, const char *const accepts, const int c)
		__attribute__ ((optimize("Os")));

#endif /* end of _LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

/**
 * @fn static void *o32_memcpy( void *dest, const void *src, const size_t n)
 * @brief 32bitアクセスに最適化したmemcpy。先頭アドレスがアライメント４倍とになっているときに、32bit単位でアクセスする。 
 *	ポインタがややこしい場合は、通常のmemcpy()に引き渡します。
 *	Microblazeの外部メモリ接続対策用
 * @return destで指定されたポインタアドレス
 */ 
static void *o32_memcpy( void *dest, const void *src, const size_t n)
{
    if(((uintptr_t)dest % 4) || ((uintptr_t)src % 4)) {
	return memcpy( dest, src, n);
    } else {
	size_t b = n;
	uint8_t *d = MPSX_STATIC_CAST(uint8_t*, dest);
	const uint8_t *s = MPSX_STATIC_CAST(const uint8_t*, src);

	for(; b>=4; b-=4, d+=4, s+=4) {
	    *(uint32_t*)d = *(uint32_t*)s;
	}
	for(; b>0; b--, d++, s++) {
	    *d = *s;
	}
	return dest;
    }
}
		
/**
 * @fn void *mpsx_memcpy(void *const dest, const void *const src, const size_t n)
 * @brief 通常のメモリコピーです。CPUキャッシュを使用します。
 *   VC++2010はデフォルトでCPU拡張命令を判定してSSEを使用するので、そのままにします。
 * @param dest コピー先ポインタ
 * @param src コピー元ポインタ
 * @param n サイズ
 * @retval NULL 失敗
 * @retval destポインタ 成功
 */
void *mpsx_memcpy(void *const dest, const void *const src, const size_t n)
{
#if defined(_LIBS3_X86SIMD_ENABLE)
    return mpsx_simd_memcpy(dest, src, n);
#elif defined(MICROBLAZE) && defined(STANDALONE)
    return o32_memcpy( dest, src, n);
#else
    return memcpy(dest, src, n);
#endif
}

/**
 * @fn void *mpsx_memcpy_path_through_level_cache(void *const dst, const void *const src, const size_t n)
 * @brief 書き込み時にキャッシュを極力汚染しないmemcpy関数です
 * @param dst コピー先ポインタ
 * @param src コピー元ポインタ
 * @param n サイズ
 * @retval NULL 失敗
 * @retval destポインタ 成功
 */
void *mpsx_memcpy_path_through_level_cache(void *const dst, const void *const src,
					      const size_t n)
{
#if defined(_LIBS3_X86SIMD_ENABLE)
    return mpsx_simd_memcpy_path_through_level_cache(dst, src, n);
#else
    return memcpy(dst, src, n);
#endif
}

/**
 * @fn void *mpsx_memset_path_through_level_cache(void *const d, const int c, const size_t n)
 * @brief 書き込み時にキャッシュを極力汚染しないmemset関数です
 * @param d メモリの先頭ポインタ
 * @param c Octetデータ
 * @param n サイズ
 * @retval NULL 失敗
 * @retval d値 成功
 */
void *mpsx_memset_path_through_level_cache(void *const d, const int c,
					      const size_t n)
{
#if defined(_LIBS3_X86SIMD_ENABLE)
    return mpsx_simd_memset_path_through_level_cache(d, c, n);
#else
    return memset(d, c, n);
#endif

}

/**
 * @fn int mpsx_strcasecmp(const char *const s1, const char *const s2)
 * @brief 二つの文字列 s1 と s2 を、 大文字小文字を区別せずに比較します。
 *	s1 が s2 よりも小さいか、同じか、大きいかによってそれぞれ 負の整数、0、正の整数を返します。
 *	POSIX strcasecmpのラッパー
 * @param s1 '\0'で終端された文字列バッファポインタその1
 * @param s2 '\0'で終端された文字列バッファポインタその2
 * @return s1 が s2 よりも小さいか、同じか、大きいかによってそれぞれ 負の整数、0、正の整数を返します。
 */
int mpsx_strcasecmp(const char *const s1, const char *const s2)
{
#if defined(Darwin) || defined(QNX) || defined(Linux) || defined(__AVM2__) || (defined(MICROBLAZE) && defined(STANDALONE)) || defined(H8300)
    return strcasecmp(s1, s2);
#elif defined(WIN32)
    return _stricmp(s1, s2);
#else
#error 'can not define mpsx_strncasecmp()'
#endif
}


/**
 * @fn int mpsx_strncasecmp(const char *const s1, const char *const s2, const size_t n)
 * @brief 二つの文字列 s1 と s2 を、 大文字小文字を区別せずに比較します。
 *	但し、s1 の最初の n 文字又は、それ以下だけを比較します。
 *	s1 が s2 よりも小さいか、同じか、大きいかによってそれぞれ 負の整数、0、正の整数を返します。
 *	POSIX strncasecmpのラッパー
 * @param s1 '\0'で終端された文字列バッファポインタその1
 * @param s2 '\0'で終端された文字列バッファポインタその2
 * @param n  比較文字数(終端文字'\0'以降も含む事が可能)
 * @return s1 が s2 よりも小さいか、同じか、大きいかによってそれぞれ 負の整数、0、正の整数を返します。
 */
int mpsx_strncasecmp(const char *const s1, const char *const s2, const size_t n)
{
#if defined(QNX)
    int result, status;
    char *__restrict ref1 = NULL;
    char *__restrict ref2 = NULL;

    const size_t ref1_len = n + 1;
    const size_t ref2_len = n + 1;

    ref1 = mpsx_malloca(ref1_len);
    memset(ref1, 0x0, ref1_len);
    ref2 = mpsx_malloca(ref2_len);
    memset(ref2, 0x0, ref1_len);

    result = mpsx_strtolower(s1, ref1, ref1_len);
    if (result) {
	status = -1;
	goto out;
    }
    result = mpsx_strtolower(s2, ref2, ref2_len);
    if (result) {
	status = -1;
	goto out;
    }

    status = strncmp(ref1, ref2, n);

  out:

    if (NULL != ref1) {
	mpsx_freea(ref1);
    }
    if (NULL != ref2) {
	mpsx_freea(ref2);
    }

    return status;
#elif defined(Darwin) || defined(Linux) || defined(__AVM2__) || (defined(MICROBLAZE) && defined(STANDALONE)) || defined(H8300)
    return strncasecmp(s1, s2, n);
#elif defined(WIN32)
    return _strnicmp(s1, s2, n);
#else
#error 'can not define mpsx_strncasecmp()'
#endif
}

/**
 * @fn int mpsx_strtoupper(const char *const before_str, char *const after_buf, const size_t after_bufsize)
 * @brief ASCII文字を大文字に変換します。
 * @param before_str 対象文字列バッファ
 * @param after_buf 変換後文字列バッファ
 * @param after_bufsize 変換後バッファサイズ
 * @retval EINVAL 不正なパラメータ
 * @retval 0 成功
 */
int mpsx_strtoupper(const char *const before_str, char *const after_buf,
		       const size_t after_bufsize)
{
    const size_t before_len = strlen(before_str);
    size_t i;
    char *__restrict pa = after_buf;
    const char *__restrict pb = before_str;

    if (before_len > after_bufsize) {
	return EINVAL;
    }

    memset(pa, 0, after_bufsize);

    for (i = 0; i < before_len; ++i, ++pa, ++pb) {
	char chk_char = *pb;

	if ('\0' == chk_char) {
	    break;
	}

	if (!(chk_char < 'a' || chk_char > 'z')) {
	    chk_char = chk_char - 'a' + 'A';
	}

	*pa = chk_char;
    }

    return 0;
}


/**
 * @fn int mpsx_strtolower(const char *const before_str, char *const after_buf, const size_t after_bufsize)
 * @brief ASCII文字を小文字に変換します。
 * @param before_str 対象文字列バッファ
 * @param after_buf 変換後文字列バッファ
 * @param after_bufsize 変換後バッファサイズ
 * @retval EINVAL 不正なパラメータ
 * @retval 0 成功
 */
int mpsx_strtolower(const char *const before_str, char *const after_buf,
		       const size_t after_bufsize)
{
    const size_t before_len = strlen(before_str);
    size_t i;
    char *__restrict pa = after_buf;
    const char *__restrict pb = before_str;

    if (before_len > after_bufsize) {
	return EINVAL;
    }

    IFDBG5THEN {
	const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPSX_XTOA_DEC64_BUFSZ];

	mpsx_i64toadec( (int64_t)before_len, xtoa_buf, xtoa_bufsz);
	DBMS5(stderr, "before=%s:%s" EOL_CRLF, xtoa_buf, before_str);
    }

    memset(pa, 0x0, after_bufsize);

    for (i = 0; i < before_len; ++i, ++pa, ++pb) {
	char chk_char = *pb;

	if ('\0' == chk_char) {
	    break;
	}

	if (!(chk_char < 'A' || chk_char > 'Z')) {
	    chk_char = chk_char - 'A' + 'a';
	}

	*pa = chk_char;
    }

    DBMS5(stderr, "after=%s" EOL_CRLF, after_buf);

    return 0;
}

/**
 * @fn char *mpsx_strcasestr(const char *const haystack, const char *const needle)
 * @brief 部分文字列 needle が文字列 haystack 中 で最初に現れる位置を見つける。
 *         大文字、小文字は無視する 文字列を終端 NULL バイト ('\0') は比較されない。
 *	   needle文字列数が0の場合、heystackポインタを返します。
 * @param haystack 対象文字列
 * @param needle 検索文字列
 * @retval NULL 見つからなかった
 * @retval NULL以外 検出された位置の先頭
 */
char *mpsx_strcasestr(const char *const haystack, const char *const needle)
{
#if defined(WIN32) || defined(Darwin) || defined(QNX)
    int result;
    char *datbuf = NULL;
    char *target = NULL;
    char *retp = NULL;

    const size_t datbuf_len = strlen(haystack) + 1;
    const size_t target_len = strlen(needle) + 1;

    if (target_len == 1) {
	return (char *) haystack;
    }

    datbuf = (char *) mpsx_malloc(datbuf_len);
    if (NULL == datbuf) {
	errno = ENOMEM;
	return NULL;
    }
    target = (char *) mpsx_malloc(target_len);
    if (NULL == target) {
	errno = ENOMEM;
	return NULL;
    }
    result = mpsx_strtolower(haystack, datbuf, datbuf_len);
    if (result) {
	retp = NULL;
	goto out;
    }
    result = mpsx_strtolower(needle, target, target_len);
    if (result) {
	retp = NULL;
	goto out;
    }

    retp = strstr(datbuf, target);
    if (NULL != retp) {
	size_t pos = (size_t) (retp - datbuf);
	retp = (char *) haystack + pos;
    }

  out:

    if (NULL != datbuf) {
	mpsx_free(datbuf);
    }
    if (NULL != target) {
	mpsx_free(target);
    }

    return retp;
#elif defined(Linux) || defined(__AVM2__) || (defined(MICROBLAZE) && defined(STANDALONE)) || defined(H8300)
    return strcasestr(haystack, needle);
#else
#error 'can not define mpsx_strcasestr()'
#endif
}

/**
 * @fn char *mpsx_strtrim(char *const buf, const char *const accepts, const int c)
 * @brief bufに含まれる文字列からacceptsに含まれる文字列をcに置き換えます
 *	*** 現在。検証中です ****
 *	bufの内容を書き換えます。
 * 	戻り値には取り除かれた文字列の先頭のポインタが戻ります。
 *      '\0'で置き換えられた場合には'\0'以外が出現するポインタになります。
 * @param buf 文字列バッファ
 * @param accepts 取り除く文字集合
 * @param c 書き換える文字
 * @return 文字列の先頭のポインタ
 */
char *mpsx_strtrim(char *const buf, const char *const accepts, const int c)
{
    const size_t before = strlen(buf);
    char *ptr = buf + before - 1;
    size_t n;

    if (0 == before) {
	return buf;
    }

    while ((0 < strlen(buf)) && (NULL != strchr(accepts, *ptr))) {
	*ptr = (char) (0xff & c);
	--ptr;
    }

    ptr = buf;
    for (n = 0; n < before; ++n) {
	if (*ptr != '\0') {
	    break;
	}
	++ptr;
    }

    return ptr;
}

/**
 * @fn char *own_special_strchr( const char *const str, const int code, char **last_char)
 * @brief 文字列 str 中に最初にASCII文字code が現れた位置へのポインターを返す。 
 * @param s ASCII文字列ポインタ
 * @param c 検索コード( 符号なしにも一応対応）
 * @param last_code 最後に見たコード
 * @retval NULL cで指定された文字が見つからない
 * @retval NULL以外
 **/
static char *own_special_strchr( const char *const str, const int code, const char **last_char)
{
    if( code > 0xff ) {
	if( NULL != last_char) {
	    *last_char = NULL;
	}
	return NULL;
    } else if(code < INT8_MIN) {
	if( NULL != last_char) {
	    *last_char = NULL;
	}
	return NULL;
    } else {
	const char c = ( code > INT8_MAX ) ? ( code - INT8_MAX ) : code;
	const char *s = str;

	while (*s != '\0') {
	    if (*s == c) {
		if(NULL != last_char) {
		   *last_char = s;
		}
		return (char *) s;
	    }
	    ++s;
	}
	if( NULL != last_char) {
	   *last_char = s;
	}
    }
    return NULL;
}

/**
 * @fn char *mpsx_strchr(const char *const str, const int code)
 * @brief 文字列 str 中に最初にASCII文字code が現れた位置へのポインターを返す。 
 * @param s ASCII文字列ポインタ
 * @param c 検索コード
 * @retval NULL cで指定された文字が見つからない
 * @retval NULL以外 見つけた文字位置のポインタ
 **/
char *mpsx_strchr(const char *const str, const int code)
{
    return own_special_strchr( str, code, NULL);
}

/**
 * @fn char *mpsx_strrchr(const char *const str, const int code)
 * @brief 文字列 str 中に最後にASCII文字code が現れた位置へのポインターを返す。 
 * @param s ASCII文字列ポインタ
 * @param c 検索コード
 * @retval NULL cで指定された文字が見つからない
 * @retval NULL以外 見つけた文字位置のポインタ
 **/
char *mpsx_strrchr(const char *const str, const int code)
{
    const char* code_p= NULL;
    const char *p, *sp = str;
    const char *np = str;

    for(;;) {
	p = own_special_strchr( np, code, &code_p);
	if((NULL == p) && ( *code_p == '\0') ) {
	    if(np == str) {
		return NULL;
	    }
	    return (char*)sp;
	}

	if( *code_p != '\0' ) {
	    sp = p;
	    np = p+1;
	    continue;
	} 
    }
}

