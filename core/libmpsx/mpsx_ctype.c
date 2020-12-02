/**
 *	Copyright 2012 TSNｰSHINGENN .All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_ctype.c
 * @brief POSIX互換のASCII文字関連のライブラリ
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#if defined(__GNUC__)
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* this */
#include "mpsx_sys_types.h"
#include "mpsx_ctype.h"

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/**
 * @fn int mpsx_ascii_isprint(const int c)
 * @brief　入力コード番号をアスキーコードに照らして表示可能な文字であるか
 * 	調べる。スペースも含まれる。
 * retval 0以外 適合
 * retval 0 不適合
 **/
int mpsx_ascii_isprint(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    return isprint(c);
#elif defined(WIN32) || (defined(MICROBLAZE) && defined(STANDALONE))
    return (c >= 32 && c <= 126); // ((c >= ' ') && (c <= '^'));
#else
#error 'not impliment mpsx_ascii_isprint() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_isspace(const int c)
 * @brief　入力コード番号をアスキーコードに照らして空白文字か調べる。
 *	multosライブラリは POSIXを基準にしている。空白に次の文字が含まれる。
 *	スペース 
 *	フォームフィード ('\f') 
 *	改行(newline) ('\n')
 *	復帰(carriage return) ('\r')
 *	水平タブ ('\t')
 *	垂直タブ ('\v') 
 * retval 0以外 適合
 * retval 0 不適合
 */ 
 int mpsx_ascii_isspace(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    return isspace(c);
#elif defined(WIN32) || (defined(MICROBLAZE) && defined(STANDALONE))
    return ((int)(' ') == c || ((c <= 0x0d) && c >= 0x09));
#else
#error 'not impliment mpsx_ascii_isspace() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_isdigit(const int c)
 * @brief　入力コード番号をアスキーコードに照らして数字文字(0-9)か調べる。
 * retval 0以外 適合
 * retval 0 不適合
 **/ 
int mpsx_ascii_isdigit(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    return isdigit(c);
#elif defined(WIN32) || (defined(MICROBLAZE) && defined(STANDALONE))
    return ((c <= '9') && (c >= '0'));
#else
#error 'not impliment mpsx_ascii_isdigit() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_isalpha(const int c)
 * @brief　入力コード番号をアスキーコードに照らしてアルファベットか調べる。
 * 	"C"ロケールを基準とし、大小文字の区別はしない。
 * retval 0以外 適合
 * retval 0 不適合
 **/
int mpsx_ascii_isalpha(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    return isalpha(c);
#elif defined(WIN32) || (defined(MICROBLAZE) && defined(STANDALONE))
    return ((c <= 'Z' && c >= 'A') || (c <= 'z' && c >= 'a'));
#else
#error 'not impliment mpsx_ascii_isalpha() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_toupper(const int c)
 * @brief　入力コード番号をASCIIコードに照らしてアルファベットを大文字に変換する
 *	変換できない場合は 引数c の値をそのまま戻す
 * return 条件にあったアスキーコード
 **/
int mpsx_ascii_toupper(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin) || defined(WIN32)
    return toupper(c);
#elif defined(CUSTOM_MADE) || (defined(MICROBLAZE) && defined(STANDALONE))
    return (c <= 'z' && c >= 'a' ? c - 32 : c); 
#else
#error 'not impliment mpsx_ascii_toupper() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_tolower(const int c)
 * @brief　入力コード番号をASCIIコードに照らしてアルファベットを大文字に変換する
 *	変換できない場合は 引数c の値をそのまま戻す
 * return 条件にあったアスキーコード
 **/
int mpsx_ascii_tolower(const int c)
{
#if defined(Linux) || defined(QNX) || defined(Darwin) || defined(WIN32)
    return tolower(c) ? c : 0;
#elif defined(CUSTOM_MADE) || (defined(MICROBLAZE) && defined(STANDALONE))
    return (c <= 'Z' && c >= 'A' ? c + 32 : c); 
#else
#error 'not impliment mpsx_ascii_tolower() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_islower(const int c)
 * @brief 入力コード番号をASCIIコードに照らして小文字アルファベットであるかどうかを確認する
 * retval 0 不適合
 * retval 0以外 小文字アルファベットの場合，cの値を戻す
 **/
int mpsx_ascii_islower(const int c)
{

#if defined(Linux) || defined(QNX) || defined(Darwin) || defined(WIN32)
    return islower(c) ? c : 0;
#elif defined(CUSTOM_MADE) || (defined(MICROBLAZE) && defined(STANDALONE))
    return (c <= 'z' && c >= 'a' ? c : 0); 
#else
#error 'not impliment mpsx_ascii_islower() on target OS'
#endif
}

/**
 * @fn int mpsx_ascii_isupper(const int c)
 * @brief 入力コード番号をASCIIコードに照らして小文字アルファベットであるかどうかを確認する
 * retval 0 不適合
 * retval 0以外 小文字アルファベットの場合，cの値を戻す
 **/
int mpsx_ascii_isupper(const int c)
{

#if defined(Linux) || defined(QNX) || defined(Darwin) || defined(WIN32)
    return isupper(c) ? c : 0;
#elif defined(CUSTOM_MADE) || (defined(MICROBLAZE) && defined(STANDALONE))
    return (c <= 'Z' && c >= 'A' ? c : 0); 
#else
#error 'not impliment mpsx_ascii_isupper() on target OS'
#endif
}
