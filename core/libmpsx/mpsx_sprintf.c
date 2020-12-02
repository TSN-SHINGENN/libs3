/** 
 *      Copyright 2013 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2013-August-26 Active 
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_sprintf.c
 * @brief WIN32とUNIX系で異なるsprintf()関数のラッパ
 *        mingwの%I64を%lldには変換しません。
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* CRL */
#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef __GNUC__
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* this */
#include "mpsx_sys_types.h"
#include "mpsx_ctype.h"
#include "mpsx_stdlib.h"
#include "mpsx_string.h"
#include "mpsx_sprintf.h"

typedef union _union_conversion_specifications {
    uint8_t flags;
    struct {
	uint8_t zero_padding:1;
	uint8_t alternative:1;
	uint8_t thousand_group:1;
	uint8_t capital_letter:1;
	uint8_t with_sign_char:1;
	uint8_t left_justified:1;
    } f;
} union_conversion_specifications_t;



typedef enum _enum_integer_type {
    enum_ieqLL =  2,    //Long Long
    enum_ieqL  =  1,    //Long int
    enum_ieqI  =  0,    //Integer
    enum_ieqS  = -1,    //Short
    enum_ieqC  = -2     //Char
} enum_integer_type_t;

static long long own_get_signed(va_list *const ap, const enum_integer_type_t type);
static unsigned long long own_get_unsigned(va_list *const ap, const enum_integer_type_t type);
static void own_put_integer(void (*_putchar_cb)(int), unsigned long long n, int radix, int length, char sign, const union_conversion_specifications_t *const opts_p);

/**
 * @fn static long long own_get_signed(va_list ap, const enum_integer_type_t type)
 * @brief va_listから指定された型に変換した符号付き数値を取り出して返します。
 * @param ap 可変数引数リスト
 * @param type 整数型
 **/
static long long int own_get_signed(va_list *const ap_p, const enum_integer_type_t type)
{
    enum_integer_type_t t = type;

    if(t >=  enum_ieqLL) {
	t = enum_ieqLL;
    } else if(t <= enum_ieqC) {
	t = enum_ieqC;
    }

    switch(t) {
    case enum_ieqLL: {
	    const long long int lli = va_arg(*ap_p, long long);
	    return lli;
	} break;
    case enum_ieqL: {
	    const long int li = va_arg(*ap_p, long);
	    return li;
	} break;
    case enum_ieqI : {
	    const int i = va_arg(*ap_p, int);
	    return i;
	} break;
    case enum_ieqS: {
	    const short si = (short) va_arg(*ap_p, signed );
	    return si;
	} break;
    case enum_ieqC: {
	    const char ci = (signed char) va_arg(*ap_p, signed );
	    return ci;
	} break;
    }

    return (signed char) va_arg(*ap_p, signed );
}

static unsigned long long int own_get_unsigned(va_list *const ap_p, const enum_integer_type_t type)
{
    enum_integer_type_t t = type;

    if(t >=  enum_ieqLL) {
	t = enum_ieqLL;
    } else if(t <= enum_ieqC) {
	t = enum_ieqC;
    }

    switch(t) {
    case enum_ieqLL: {
	const unsigned long long lln = va_arg(*ap_p, unsigned long long);
	return lln;
	} break;
    case enum_ieqL: {
	const unsigned long ln = va_arg(*ap_p, unsigned long);
	return ln;
	} break;
    case enum_ieqI: {
	const unsigned int n = va_arg(*ap_p, unsigned int);
	return n;
	} break;
    case enum_ieqS: {
	const unsigned short sn = (unsigned short) va_arg(*ap_p, unsigned );
	return sn;
	} break;
    case enum_ieqC: { 
	const unsigned char cn = (unsigned char) va_arg(*ap_p, unsigned );
	return cn;
        } break;
    }

    return (unsigned char) va_arg(*ap_p, unsigned );
}

/**
 * @fn void _mpsx_vprintf(void (*_putc_cb)(int), const char *fmt, va_list ap)
 * @brief
 *
 **/
void _mpsx_vprintf(void (*_putc_cb)(int), const char *fmt, va_list ap)
{
    unsigned long long ui;
    long long i;
    char *s = NULL;
    double d = 0.0;
    int sign = 0;

    int length = 0;
    int precision = 0;
    int tmp = 0;

    union_conversion_specifications_t opts = { 0 };
    enum_integer_type_t int_type = enum_ieqI;

    for (;;++fmt) {
	/* Initialize */
	s = NULL;
	d = 0.0;
	sign = 0;
	length = 0;
	precision = 0;
	tmp = 0;
	int_type = enum_ieqI;

	while ((*fmt != 0) && (*fmt != '%')) {
	    _putc_cb(*fmt);
	    ++fmt;
	}
	if (*fmt++ == '\0') {
//	    va_end(ap);
	    goto out;
	}
        /* ++fmt; 上記if分内でやってる */

	for( opts.flags = 0; (NULL != mpsx_strchr("'-+ #0", *fmt)); ++fmt) {
	    switch (*fmt) {
	    case '\'': {
		opts.f.thousand_group = 1;
//		flags |= THOUSAND_GROUP;
		} break;
	    case  '-': {
		opts.f.left_justified = 1;
//		flags |= LEFT_JUSTIFIED;
		} break;
	    case  '+': {
		opts.f.with_sign_char = 1;
//		flags |= WITH_SIGN_CHAR;
		sign = '+';
		} break;
	    case  '#': {
		opts.f.alternative = 1;
//		flags |= ALTERNATIVE;
		} break;
            case  '0': { 
		opts.f.zero_padding = 1;
//		flags |= ZERO_PADDING;
		} break;
	    case  ' ': {
		opts.f.with_sign_char = 1;
//		flags |= WITH_SIGN_CHAR;
		sign = ' ';
		} break;
            }
	}

	if(*fmt == '*') {
	    length = va_arg(ap ,int);
	    ++fmt;
	} else {
	    for(;mpsx_ascii_isdigit(*fmt); ++fmt) {
		length *= 10;
		length += mpsx_ctoi(*fmt);
	    }
	}

        if (*fmt == '.') {
            ++fmt;
            if (*fmt == '*') {
		++fmt;
		precision = va_arg(ap, int);
	    } else {
		while (mpsx_ascii_isdigit(*fmt) ) {
		    precision *= 10;
		    precision += mpsx_ctoi(*fmt);
		    ++fmt;
		}
	    }
	}

	while (mpsx_strchr("hljzt", *fmt)) {
	    switch (*fmt++) {
            case 'h': {
		int_type = MPSX_STATIC_CAST( enum_integer_type_t, int_type-1);
		} break;
	    case 'l': {
		int_type = MPSX_STATIC_CAST( enum_integer_type_t, int_type-1);
		} break;
	    case 'j': /*intmax   : long      */
            case 'z': /*size     : long      */
            case 't': /*ptrdiff  : long      */
		int_type=enum_ieqL;
	    }
        }

        switch (*fmt) {
	case 'd':
        case 'i': {
            i = own_get_signed(&ap, int_type);
            if (i < 0) {
		i = -i;
		sign = '-';
	    }
            own_put_integer(_putc_cb, i, 10, length, sign, &opts);
	} break;

	case 'u': {
	    ui = own_get_unsigned(&ap, int_type);
	    own_put_integer(_putc_cb,  ui, 10, length, sign, &opts);
	} break;

	case 'o': {
	    ui = own_get_unsigned(&ap, int_type);
            own_put_integer(_putc_cb,  ui, 8, length, sign, &opts);
	} break;

	case 'p':
	    length = sizeof(long) * 2;
	    int_type = enum_ieqL;
	    sign = 0;
	    opts.f.zero_padding = 1;
	    opts.f.alternative = 1;
//	    flags = ZERO_PADDING | ALTERNATIVE;
	case 'X': 
	    opts.f.capital_letter = 1;
//	    flags |= CAPITAL_LETTER;
	case 'x':
	    ui = own_get_unsigned(&ap, int_type);
	    own_put_integer(_putc_cb, ui, 16, length, sign, &opts);
            break;

	case 'c': {
	    const int code = (int)own_get_signed(&ap, enum_ieqC);
	    _putc_cb(code);
	} break;
	case 's': {
	    s = va_arg(ap, char *);

	    if (s==NULL) {
		s = "(null)";
	    }
	    tmp = strlen(s);

	    if (precision && (precision < tmp)) {
		tmp = precision;
	    }
	    length -= tmp;
            if (opts.f.left_justified     /* !(flags & LEFT_JUSTIFIED) */) {
		while ( length > 0   ) {
		    _putc_cb(' ');
		    --length;
		}
	    }
	    for(;tmp; ++s, --tmp) {
		_putc_cb(*s);
	    }
	    for(;length > 0; --length) {
		_putc_cb(' ');
	    }
	} break;
        case '%': {
	    _putc_cb('%');
	} break;

        default:
            for(;*fmt != '%';--fmt) {
		(void)fmt;
	    }
        }
    }

out:
    return;
}

/**
 * @fn static void own_put_integer(void (*_putchar_cb)(int), unsigned long long n, int radix, int length, char sign, const union_conversion_specifications_t *const opts_p)
 * @brief 引数の情報から整数値を変換してコールバックにASCIIコードで送ります。
 * @param n
 * @param radix 進数
 * @param length
 * @param sign 符号
 * @param slags オプションフラグ
 */
static void own_put_integer(void (*_putchar_cb)(int), unsigned long long n, int radix, int length, char sign, const union_conversion_specifications_t *const opts_p)
{
    static const char *const symbols_s = "0123456789abcdef";
    static const char *const symbols_c = "0123456789ABCDEF";
    const char *const symbols = (opts_p->f.capital_letter) ? symbols_c : symbols_s;
    int pad = ' ';
    int strpoint = 0;
    char buf[80];
    
    do {
        buf[strpoint++] = symbols[n % radix];
        if( (opts_p->f.thousand_group) && ((strpoint & 0x3) == 3)) {
	    buf[strpoint++] = ',';
	}
    } while ( n /= radix );
    length -= strpoint;

    if (!(opts_p->f.left_justified)) {
        if(opts_p->f.zero_padding) {
	    pad = '0'; /* pad 変更 */
	}
        for(;length > 0; --length) {
	    buf[strpoint++] = pad;
	}
    }

    if (( sign != 0 ) && (radix == 10)) { 
	buf[strpoint++] = sign;
    }

    if (opts_p->f.alternative) {
	if (radix == 8) {
	    buf[strpoint++] = '0';
	} else if (radix == 16) {
           buf[strpoint++] = 'x';
           buf[strpoint++] = '0';
        }
    }

    while(strpoint > 0) {
	_putchar_cb(buf[--strpoint]);
    }
    while( length > 0 ) {
	length--;
	_putchar_cb(pad);
    }
    return;
}

static void own_putc(int c)
{
    putc(c, stdout);
}

void _mpsx_printf(const char *const fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _mpsx_vprintf(own_putc,fmt, ap);
    va_end(ap);

    return;
}


/**
 * @fn int mpsx_snprintf( char * __restrict buf, size_t size, const char * __restrict fmt, ...)
 * @brief Cランタイム snprintf()のラッパーです. WIN32仕様に近い動作をします。
 * @param buf 文字列を挿入する為のバッファ
 * @param size bufのサイズ
 * @param fmt 文字列フォーマット
 * @retval -1 失敗（errnoがセットされます）
 * 	EINVAL buf又はfmtにNULLが設定された. sizeに0が設定された
 *	ERANGE bufにfmt指定された文字列が収まらなかった
 */
int mpsx_snprintf(char *__restrict buf, size_t size,
		     const char *__restrict fmt, ...)
{
    va_list arg;
    int retval;

    va_start(arg, fmt);
    retval = mpsx_vsnprintf(buf, size, fmt, arg);
    va_end(arg);

    return retval;
}

/**
 * @fn int mpsx_vsnprintf( char * __restrict buf, size_t size, const char * __restrict fmt, va_list ap)
 * @brief va_listを使ったsprintf() WIN32に近い動作をします。
 *	VC++6.0仕様の%I64には対応しません
 * @param buf 文字列を挿入する為のバッファ
 * @param size bufのサイズ
 * @param fmt 文字列フォーマット
 * @param ap va_list
 * @retval 0以上 書き込まれた文字列のバイト数（NULLを含まない）
 * @retval -1 失敗（errnoがセットされます）
 * 	EINVAL buf又はfmtにNULLが設定された. sizeに0が設定された
 *	ERANGE bufにfmt指定された文字列が収まらなかった
 */
int mpsx_vsnprintf(char *__restrict buf, size_t size,
		      const char *__restrict fmt, va_list ap)
{
    if ((NULL == buf) || (NULL == fmt)) {
	errno = EINVAL;
	return -1;
    } else if (size == 0) {
	errno = EINVAL;
	return -1;
    }
#if defined(Linux) || defined(Darwin) || defined(QNX) || defined(SunOS) || defined(__AVM2__) || defined(XILKERNEL) || defined(H8300)
    {
	int retlen;

	retlen = vsnprintf(buf, size, fmt, ap);

	if (retlen >= (int) size) {
	    errno = ERANGE;
	    return -1;
	}
	return retlen;
    }
#elif defined(WIN32)
#if _MSC_VER >= 1400		/* VC++2005 */
    return vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
#else
    return vsnprintf(buf, size, fmt, ap);
#endif				/* end of _MSC_VER */

#else
#error not implemented mpsx_vsnprintf()
#endif
}
