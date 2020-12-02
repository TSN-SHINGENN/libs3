/** 
 *      Copyright 2018 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2018-June-04 Active 
 *              Last Alteration $Author: takeda $
 */


#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* CRL */
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int debuglevel = 1;
#else
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#include "mpxx_sys_types.h"
#include "mpxx_stdlib.h"

#include "mpxx_stdarg.h"


#if defined(__GNUC__) && defined(_LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern long long int mpxx_get_va_list_signed(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
	__attribute__ ((optimize("Os")));
extern unsigned long long int mpxx_get_va_list_unsigned(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
	__attribute__ ((optimize("Os")));
extern void *mpxx_get_va_list_pointer(va_list *const ap_p)
	__attribute__ ((optimize("Os")));

#endif /* end of _LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

/**
 * @fn long long int mpxx_get_va_list_signed(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
 * @brief va_listから指定された型に変換した符号付き数値を取り出して返します。
 * @param ap 可変数引数リスト
 * @param type 整数型
 **/
long long int mpxx_get_va_list_signed(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
{
    switch(type) {
    case MPXX_STDARG_INTEGER_IS_Ptr: {
	const intptr_t iptr = va_arg(*ap_p, intptr_t);
	return MPXX_STATIC_CAST(long long int, iptr);
      } break;
    case MPXX_STDARG_INTEGER_IS_LL: {
	const long long int lli = va_arg(*ap_p, long long);
	return MPXX_STATIC_CAST(long long int, lli);
      } break;
    case MPXX_STDARG_INTEGER_IS_Long: {
	const long int li = va_arg(*ap_p, long);
	return MPXX_STATIC_CAST(long long int, li);
      } break;
    case MPXX_STDARG_INTEGER_IS_Normal : {
	const int i = va_arg(*ap_p, int);
	return MPXX_STATIC_CAST(long long int, i);
      } break;
    case MPXX_STDARG_INTEGER_IS_Short: {
	const short si = (short) va_arg(*ap_p, signed );
	return MPXX_STATIC_CAST(long long int, si);
      } break;
    case MPXX_STDARG_INTEGER_IS_Char: {
	const char ci = (signed char) va_arg(*ap_p, signed );
	return MPXX_STATIC_CAST(long long int, ci);
      } break;
    default:
    	return ~0;
    }

    return (unsigned long long) va_arg(*ap_p, signed);
}

/**
 * @fn unsigned long long int mpxx_get_va_list_unsigned(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
 * @brief va_listから指定された型に変換した符号付き数値を取り出して返します。
 * @param ap 可変数引数リスト
 * @param type 整数型
 **/
unsigned long long int mpxx_get_va_list_unsigned(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type)
{
    switch(type) {
    case MPXX_STDARG_INTEGER_IS_Ptr: {
	const uintptr_t nptr = va_arg(*ap_p, uintptr_t);
	return MPXX_STATIC_CAST(unsigned long long,nptr);
      } break;
    case MPXX_STDARG_INTEGER_IS_LL: {
	const unsigned long long lln = va_arg(*ap_p, unsigned long long);
	return MPXX_STATIC_CAST(unsigned long long,lln);
      } break;
    case MPXX_STDARG_INTEGER_IS_Long: {
	const unsigned long ln = va_arg(*ap_p, unsigned long);
	return MPXX_STATIC_CAST(unsigned long long, ln);
      } break;
    case MPXX_STDARG_INTEGER_IS_Normal: {
	const unsigned int n = va_arg(*ap_p, unsigned int);
	return MPXX_STATIC_CAST(unsigned long long, n);
      } break;
    case MPXX_STDARG_INTEGER_IS_Short: {
	const unsigned short sn = (unsigned short) va_arg(*ap_p, unsigned );
	return MPXX_STATIC_CAST(unsigned long long, sn);
      } break;
    case MPXX_STDARG_INTEGER_IS_Char: { 
	const unsigned char cn = (unsigned char) va_arg(*ap_p, unsigned );
	return MPXX_STATIC_CAST(unsigned long long, cn);
      } break;
    default:
    	return ~0;
    }

    return (unsigned long long) va_arg(*ap_p, unsigned );
}

/**
 * @fn void *mpxx_get_va_list_pointer(va_list *const ap_p)
 * @brief va_list からポインタ値を取得します。
 * @return 指定可変長引数のポインタ
 **/
void *mpxx_get_va_list_pointer(va_list *const ap_p)
{
    void *const ptr = (void*)va_arg( *ap_p,void*);
    return ptr;
}
