/** 
 *      Copyright 2018 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2018-May-25 Active 
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_lite_sprintf.c
 * @brief オリジナル実装の軽い文字列処理ライブラリ。処理を軽くするために実数処理に未対応
 */
#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
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
#include "mpxx_sys_types.h"
#include "mpxx_ctype.h"
#include "mpxx_stdarg.h"
#include "mpxx_stdlib.h"
#include "mpxx_string.h"
#include "mpxx_lite_sprintf.h"

#if defined(_MPXX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef union _conversion_specifications {
    uint8_t flags;
    struct {
	uint8_t   zero_padding:1;
	uint8_t    alternative:1;
	uint8_t thousand_group:1;
	uint8_t   str_is_lower:1;
	uint8_t with_sign_char:1;
	uint8_t left_justified:1;
    } f;
} conversion_specifications_t;


#if defined(_WIN64) || (defined(__LLP64__) && (__LLP64__ == 1))
#define MY_LLP64
#elif (defined(darwin) || defined(Linux)) && (defined(__LP64__) && (__LP64__ == 1))
#define MY_LP64
#elif defined(__ILP64__) && (__ILP64__ == 1)
#define MY_ILP64
#else
#define MY_IL32P32 /* 32-bit Standard */
#endif

typedef struct _my_datamodel_table {
    enum_mpxx_stdarg_integer_type_t int_type;
    int8_t bytes;
    int16_t bits;
} my_datamodel_table_t;

const static my_datamodel_table_t datamodel_tab[] = {
/* integer type is                ,bytes, bits */
#if defined(MY_LLP64) /* IL32P64 */
{ MPXX_STDARG_INTEGER_IS_Normal,    4, 32},
{ MPXX_STDARG_INTEGER_IS_Long  ,    4, 32},
{ MPXX_STDARG_INTEGER_IS_LL    ,    8, 64},
{ MPXX_STDARG_INTEGER_IS_Ptr   ,    8, 64},
#elif defined(MY_LP64) /* I32LP64 */
/* integer type is                ,bytes, bits */
{ MPXX_STDARG_INTEGER_IS_Normal,    4, 32},
{ MPXX_STDARG_INTEGER_IS_Long  ,    4, 32},
{ MPXX_STDARG_INTEGER_IS_LL    ,    8, 64},
{ MPXX_STDARG_INTEGER_IS_Ptr   ,    8, 64},
#elif defined(MY_ILP64) /* ILP64 */
/* integer type is                ,bytes, bits */
{ MPXX_STDARG_INTEGER_IS_Normal,    8, 64},
{ MPXX_STDARG_INTEGER_IS_Long  ,    8, 64},
{ MPXX_STDARG_INTEGER_IS_LL    ,    8, 64},
{ MPXX_STDARG_INTEGER_IS_Ptr   ,    4, 64},
#elif defined(MY_IL32P32) /* IL32P32 standard 32-bit */ 
/* integer type is                ,bytes, bits */
{ MPXX_STDARG_INTEGER_IS_Normal,    4, 32},
{ MPXX_STDARG_INTEGER_IS_Long  ,    4, 32},
{ MPXX_STDARG_INTEGER_IS_LL    ,    8, 64},
{ MPXX_STDARG_INTEGER_IS_Ptr   ,    4, 32},
#else
#error undefined datamodel
#endif
{ MPXX_STDARG_INTEGER_IS_eot   ,   -1,  0}};


#define prelength_is_over(max, neederslength)  ((max) <= (neederslength))

static int _this_putchar_stdout( const int asciicode);
static int _own_put_integer(void (*_putchar_cb)(int), unsigned long long n, int radix, int length, char sign, const conversion_specifications_t *const specs_p, const int szoflimit, int *retlen_p);

static const char *const symbols_s = "0123456789abcdef";
static const char *const symbols_c = "0123456789ABCDEF";

#if defined(__GNUC__) || defined(_MPXX_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

static int _own_vsnprintf(const mpxx_xtoa_output_method_t *, const char *const, va_list)
	__attribute__ ((optimize("Os")));

static mpxx_xtoa_output_method_t set_area_method(mpxx_xtoa_output_method_t m, char *const buf, const size_t max_len)
	__attribute__ ((optimize("Os")));

extern int mpxx_lite_vprintf(const char *const fmt, va_list ap)
	__attribute__ ((optimize("Os")));
extern int mpxx_lite_vsprintf(char *const buf, const char *const fmt, va_list ap)
	__attribute__ ((optimize("Os")));
extern int mpxx_lite_vsnprintf(char *const buf, const size_t max_length, const char *const fmt, va_list ap)
	__attribute__ ((optimize("Os")));


extern int mpxx_lite_putchar_cb_sprintf(int (*const putchar_cb)(int) , const char *const fmt, ...)
	__attribute__ ((optimize("Os")))
	__attribute__((format(printf,2,3)));
extern int mpxx_lite_putchar_cb_snprintf(int (*const putchar_cb)(int), const size_t max_length, const char *const *fmt, ...)
	__attribute__ ((optimize("Os")));
//	__attribute__((format(printf,3,4)));

extern int mpxx_lite_printf_init(int (*const putchar_cb_func)(const int asicccode))
	__attribute__ ((optimize("Os")));
extern int mpxx_lite_printf_change_putchar_callback(int (*const putchar_cb_func)(const int asicccode))
	__attribute__ ((optimize("Os")));
extern int mpxx_lite_printf(const char *const fmt, ...)
	__attribute__ ((optimize("Os")))
	__attribute__((format(printf,1,2)));
extern int mpxx_lite_snprintf(char *const buf, const size_t max_length, const char *const fmt,  ...)
	__attribute__ ((optimize("Os")));

#endif /* end of _MPXX_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

/**
 * @fn static int _own_put_integer(void (*_putchar_cb)(int), unsigned long long n, int radix, int length, char sign, const conversion_specifications_t *const specs_p, const int szoflimit, int *retlen_p)
 * @brief 指定されたパラメータから数値の文字列を putchar_cb コールバック関数にASCII文字列として出力します。
 * @param _putchar_cb アスキー文字を位置文字単位で処理するコールバック関数
 * @param llv long long int型を最大とする変換前整数値（すべてキャストすること)
 * @param radix 基数　2, 8, 10, 16の対対応進数指定
 * @param length 最大文字長
 * @param sign 符号値
 * @param szoflimit 
 * @param retlen_p
 **/
static int _own_put_integer(void (*_putchar_cb)(int), unsigned long long llval, int radix, int length, char sign, const conversion_specifications_t *const specs_p, const int szoflimit, int *retlen_p)
{
    size_t ofs = 0;
    int pad = ' ';
    const char *const symbols = ((NULL != specs_p) && (specs_p->f.str_is_lower)) ? symbols_s : symbols_c;
    conversion_specifications_t specs;
    char buf[MPXX_XTOA_BIN64_BUFSZ];
    
    specs.flags = (NULL == specs_p) ? 0 : specs_p->flags;

    do {
        buf[ofs] = symbols[llval % radix];
	++ofs;
        if( (specs.f.thousand_group)) {
	    if((ofs &0x3)==0x3) {
		buf[ofs] = ',';
		++ofs;
	    }
	}
    } while (llval /= radix);

    length -= ofs;

    if (!(specs.f.left_justified)) {
        if(specs.f.zero_padding) {
	    pad = '0';
	}
        while (length > 0) {
	    --length;
	    buf[ofs] = pad;
	    ++ofs;
	}
    }

    if (sign && radix == 10) {
	buf[ofs] = sign;
	++ofs;
    }
    if(specs.f.alternative) {
	if (radix == 8) {
	   buf[++ofs] = '0';
	   ++ofs;
	} else if (radix == 16) {
           buf[ofs] = 'x';
	   ++ofs;
           buf[ofs] = '0';
	   ++ofs;
        }
    }

    while (      ofs> 0 ) {
	_putchar_cb(buf[--ofs]);
    }
    while ( length > 0 ) {
	length--;
	_putchar_cb(pad);
    }

    return 0;
}

/**
 * @fn int mpxx_vsnprintf_lite(void (*_putc_cb)(int), const size_t max_len, const char *const fmt, va_list ap)
 * @brief fmt に従った文字列を生成しながら pufc_cbに指定されたコールバック関数を用いて出力します
 *	max_lenを超える場合は、処理を中止して -1 を戻します。
 * @param putc_cb 処理結果の文字列を出力するためのコールバック関数
 * @param max_len 処理結果を受ける最大長(\0)を含む。 0:最大長は無視
 * @param fmt 文字列出力表記フォーマット ANSI C/POSIX を確認してね。
 * @param ap va_list 構造体
 * @retval 0以上
 * @retval -1 失敗
 **/
int _mpxx_vsnprintf_lite(void (*_putc_cb)(int), const size_t max_len, const char *const fmt, va_list ap)
{
    int result;
    size_t lenofneeders = 0;
    const char *fptr = 0;
    int sign;

    int precision;
    int tmp;
    int length;

    conversion_specifications_t specs = { 0 };
    enum_mpxx_stdarg_integer_type_t int_type;

    for(fptr=fmt;;++fptr) {
	/* Initialize */
	sign = 0;
	length = 0;
	precision = 0;
	tmp = 0;
	specs.flags = 0;
	int_type = MPXX_STDARG_INTEGER_IS_Normal;

	/* 先方のフォーマット指定されていない部分の内容確認 */
	if(max_len) {
	    const char *t;
	    size_t prelength;

	    for( prelength=0, t=fptr; ((*t != 0) && (*t != '%')); ++t, ++prelength);
	    if( prelength_is_over(max_len, prelength) ) {
		errno = ENOSPC;
		return -1;
	    }
	}

	/* 先方のフォーマット指定されていない部分の出力 */
	for(;(*fptr != '\0') && (*fptr != '%');++fptr, ++lenofneeders) {
	    _putc_cb(*fptr);
	}
	if (*fptr == '\0') {
	    goto out;
	}
	++fptr;

	for( specs.flags = 0; (NULL != mpxx_strchr("'-+ #0", *fptr)); ++fptr) {
	    switch (*fptr) {
	    case '\'': {
		specs.f.thousand_group = 1;
		} break;
	    case  '-': {
		specs.f.left_justified = 1;
		} break;
	    case  '+': {
		specs.f.with_sign_char = 1;
		sign = '+';
		} break;
	    case  '#': {
		specs.f.alternative = 1;
		} break;
            case  '0': { 
		specs.f.zero_padding = 1;
		} break;
	    case  ' ': {
		specs.f.with_sign_char = 1;
		sign = ' ';
		} break;
            }
	}

	if(*fptr == '*') {
	    length = va_arg(ap ,int);
	    ++fptr;
	} else {
	    for(;mpxx_ascii_isdigit(*fptr); ++fptr) {
		length *= 10;
		length += mpxx_ctoi(*fptr);
	    }
	}

        if (*fptr == '.') {
            ++fptr;
            if (*fptr == '*') {
		++fptr;
		precision = va_arg(ap, int);
	    } else {
		while (mpxx_ascii_isdigit(*fptr) ) {
		    precision *= 10;
		    precision += mpxx_ctoi(*fptr);
		    ++fptr;
		}
	    }
	}

	while (mpxx_strchr("hljzt", *fptr)) {
	    switch (*fptr++) {
            case 'h': {
		int_type = MPXX_STATIC_CAST(enum_mpxx_stdarg_integer_type_t, int_type-1);
		} break;
	    case 'l' : {
		int_type = MPXX_STATIC_CAST(enum_mpxx_stdarg_integer_type_t, int_type+1);
		} break;
	    case 'j': /*intmax   : long      */
            case 'z': /*size     : long      */
            case 't': /*ptrdiff  : long      */
		int_type=MPXX_STDARG_INTEGER_IS_Long;
		break;
	    default: 
		errno = EINVAL;
		return -1;
	    }
        }

        switch (*fptr) {
	case 'd':
        case 'i': {
	    const int remain = (max_len == 0) ? 0 : (int)(max_len - lenofneeders);
	    int retlen = 0;
            const long long int lli = mpxx_get_va_list_signed(&ap, int_type);
	    unsigned long long int unni;
            if (lli < 0) {
		unni = -lli;
		sign = '-';
	    } else {
		unni = (unsigned long long int)lli;
	    }
            result = _own_put_integer(_putc_cb, unni, 10, remain, sign, &specs, remain, &retlen);
	    if(result) {
		errno = result;
		return -1;
	    }
	    lenofneeders += retlen;
	} break;

	case 'u': {
	    const int remain = (max_len == 0) ? 0 : (int)(max_len - lenofneeders);
	    int retlen = 0;
	    const unsigned int ui = (unsigned int)mpxx_get_va_list_unsigned(&ap, int_type);
	    result = _own_put_integer(_putc_cb,  ui, 10, remain, sign, &specs, remain, &retlen);
	    if(result) {
		errno = result;
		return -1;
	    }
	    lenofneeders += retlen;
	} break;

	case 'o': {
	    const int remain = (max_len == 0) ? 0 : (int)(max_len - lenofneeders);
	    int retlen = 0;
	    const unsigned int ui = (unsigned int)mpxx_get_va_list_unsigned(&ap, int_type);
            result = _own_put_integer(_putc_cb,  ui, 8, remain, sign, &specs, remain, &retlen);
	    if(result) {
		errno = result;
		return -1;
	    }
	    lenofneeders += retlen;
	} break;

	case 'p':
	    length = datamodel_tab[MPXX_STDARG_INTEGER_IS_Ptr].bytes;
	    int_type = MPXX_STDARG_INTEGER_IS_Ptr;
	    sign = 0;
	    specs.f.zero_padding = 1;
	    specs.f.alternative = 1;
	case 'x': 
	    specs.f.str_is_lower = 1;
	case 'X': {
	    int retlen = 0;
	    const int remain = (max_len == 0) ? 0 : (int)(max_len - lenofneeders);
	    const unsigned int ui = (unsigned int)mpxx_get_va_list_unsigned(&ap, int_type);
	    result = _own_put_integer(_putc_cb, ui, 16, remain, sign, &specs, remain, &retlen);
	    if(result) {
		errno = result;
		return -1;
	    }
	    lenofneeders += retlen;		    
	} break;
	case 'c': {
	    const int code = (int)mpxx_get_va_list_signed(&ap, MPXX_STDARG_INTEGER_IS_Char);
	    if( prelength_is_over(max_len, lenofneeders+1) ) {
		errno = ENOSPC;
		return -1;
	    }
	    ++lenofneeders;
	    _putc_cb(code);
	} break;
	case 's': {
	    const char *s = (char*)mpxx_get_va_list_pointer(&ap);

	    if (s==NULL) {
		s = "(null)";
	    }
	    tmp = (int)strlen(s);

	    if ( precision && (precision < tmp) ) {
		tmp = precision;
	    }
	    length -= tmp;
            if (specs.f.left_justified /* !(flags & LEFT_JUSTIFIED) */) {
		if( (max_len != 0 ) && (length > 0) && prelength_is_over(max_len, lenofneeders+length) ) {
		    errno = ENOSPC;
		    return -1;
		} else {
		    for(;length > 0; --length, ++lenofneeders ) {
			_putc_cb(' ');
		    }
		}
	    }
	    if( (max_len != 0) && ( tmp != 0) && prelength_is_over(max_len, lenofneeders+tmp) ) {
		errno = ENOSPC;
		return -1;
	    } else {
		for(;tmp; ++s, --tmp, ++lenofneeders) {
		    _putc_cb(*s);
		}
	    }
	    if( (max_len != 0) && (length != 0) && prelength_is_over(max_len, lenofneeders+length) ) {
		errno = ENOSPC;
		return -1;
	    } else {
		for(;length > 0; --length, ++lenofneeders) {
		    _putc_cb(' ');
		}
	    }
	} break;
//        case '%': {
//	    _putc_cb('%');
//	} break;
        default: 
	    if(1) {
		errno = EINVAL;
		return -1;
	    } else {
		for(;*fptr != '%';--fptr) {
		    (void)fptr;
		}
	    }
	} 
    }
out:
    return (int)lenofneeders;
}

int _mpxx_vsprintf_lite(void (*_putc_cb)(int), const char *const fmt, va_list ap)
{
    const int ret = _mpxx_vsnprintf_lite(_putc_cb, 0, fmt, ap);
    return ret;
}

static mpxx_xtoa_output_method_t set_area_method(mpxx_xtoa_output_method_t m, char *const buf, const size_t max_len)
{
    if(NULL == m._putchar_cb) {
	m.maxlenofstring = max_len;
	m.buf = buf;
    }
    return m;
}

/**
 * @fn int _own_vsnprintf(const mpxx_xtoa_output_method_t *const method_p, const char *const fmt, va_list ap)
 * @brief fmt に従った文字列を生成しながら pufc_cbに指定されたコールバック関数を用いて出力します
 *	max_lenを超える場合は、処理を中止して -1 を戻します。
 * @param putc_cb 処理結果の文字列を出力するためのコールバック関数
 * @param max_len 処理結果を受ける最大長(\0)を含む。 0:最大長は無視
 * @param fmt 文字列出力表記フォーマット ANSI C/POSIX を確認してね。
 * @param ap va_list 構造体
 * @retval 0以上
 * @retval -1 失敗
 **/
static int _own_vsnprintf(const mpxx_xtoa_output_method_t *const method_p, const char *const fmt, va_list ap)
{
    int result;
    int lenofneeders = 0;
    const char *fptr;
    int sign;

    int precision;
    int tmp;
    int length;
    const putchar_callback_ptr_t _putc_cb = method_p->_putchar_cb;
    const size_t max_len = method_p->maxlenofstring;
    char *bufp = method_p->buf;

    mpxx_xtoa_attr_t specs = { 0 };
    enum_mpxx_stdarg_integer_type_t int_type;

#if 0
    DBMS5( stderr, "%s : method_p=%p _putchar_cb=%p max_len=%d bufp=%p fmt=%s" EOL_CRLF, __func__,
		    method_p, _putc_cb, max_len, bufp, fmt);
#endif

    if( NULL == method_p ) {
	errno = EINVAL;
	return -1;
    }

    for(fptr=fmt;;++fptr) {
	/* Initialize */
	sign = 0;
	length = 0;
	precision = 0;
	tmp = 0;
	specs.flags = 0;
	int_type = MPXX_STDARG_INTEGER_IS_Normal;

	/* 先方のフォーマット指定されていない部分の内容確認 */
	if(max_len) {
	    const char *t;
	    size_t prelength;

	    for( prelength=0, t=fptr; ((*t != 0) && (*t != '%')); ++t, ++prelength); // 1 line loop
	    if( prelength_is_over(max_len, prelength) ) {
//		DBMS5( stderr, "%s : max_len=%d prelength=%d is over" EOL_CRLF, __func__, (int)max_len,  (int)prelength);
		errno = ENOSPC;
		return -1;
	    }
//	    DBMS5( stderr, "%s : max_len=%d prelength=%d" EOL_CRLF, __func__, (int)max_len,  (int)prelength);
	}

	/* 先方のフォーマット指定されていない部分の出力 */
	for(;(*fptr != '\0') && (*fptr != '%');++fptr, ++lenofneeders) {
//	    DBMS5(stderr, "%s : lenofneeders=%d *fptr=%d bufp=0x%p" EOL_CRLF, __func__, (int)lenofneeders, (int)*fptr, bufp);
	    if( NULL != _putc_cb) {
		_putc_cb(*fptr);
	    }
	    if( NULL != method_p->buf) {
		*bufp = *fptr;
		++bufp;
	    }
	}
	if (*fptr == '\0') {
//	    xil_printf("%s : pre strings fini." EOL_CRLF, __func__);
		if( NULL != method_p->buf) {
		if( prelength_is_over(max_len, (size_t)(lenofneeders)) ) {
		    errno = ENOSPC;
		    return -1;
		}
		*bufp = '\0';
		++bufp;
	    }
	    goto out;
	} else {
	    ++fptr;
	}

	if(1) { 
	    const char *const printf_flag_chars = "'-+ #0";

	    for( specs.flags = 0; (NULL != mpxx_strchr( printf_flag_chars, *fptr)); ++fptr) {
		switch (*fptr) {
		    case '\'': 
			specs.f.thousand_group = 1;
			break;
		    case  '-': 
			specs.f.left_justified = 1;
			break;
		    case  '+': 
			specs.f.with_sign_char = 1;
			sign = '+';
			break;
		    case  '#': 
			specs.f.alternative = 1;
			break;
 		    case  '0': 
			specs.f.left_justified = 1;
			specs.f.zero_padding = 1;
			break;
		    case  ' ': 
			specs.f.with_sign_char = 1;
			sign = ' ';
			break;
		}
            }
	    specs.f.no_assign_terminate = 1;
	}

	if(*fptr == '*') {
	    length = (int)mpxx_get_va_list_signed( &ap, MPXX_STDARG_INTEGER_IS_Normal);
	    ++fptr;
	} else {
	    for(;mpxx_ascii_isdigit(*fptr); ++fptr) {
		length *= 10;
		length += mpxx_ctoi(*fptr);
	    }
	}

        if (*fptr == '.') {
            ++fptr;
            if (*fptr == '*') {
		++fptr;
		precision = (int)mpxx_get_va_list_signed(&ap, MPXX_STDARG_INTEGER_IS_Normal);
	    } else {
		while (mpxx_ascii_isdigit(*fptr) ) {
		    precision *= 10;
		    precision += mpxx_ctoi(*fptr);
		    ++fptr;
		}
	    }
	}
	if(1) {
	   const char *const fmt_length_modifier="hlz";

	   while (mpxx_strchr(fmt_length_modifier,*fptr)) {
		switch (*fptr++) {
		case 'h': 
		    int_type = MPXX_STATIC_CAST(enum_mpxx_stdarg_integer_type_t, int_type-1);
		    break;
		case 'l': 
		    int_type = MPXX_STATIC_CAST(enum_mpxx_stdarg_integer_type_t, int_type+1);
		    break;
		case 'z': /* size : long long     */
		    int_type=MPXX_STDARG_INTEGER_IS_LL;
		    break;
		default: 
		    errno = EINVAL;
		    return -1;
		}
	    }

	    switch (*fptr) {
	    case 'd':
	    case 'i': {
		const int remain = (int)(max_len - lenofneeders);
		const long long int lli = mpxx_get_va_list_signed(&ap, int_type);
		unsigned long long int ulli = 0;
		mpxx_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		if (lli < 0) {
		    ulli = -lli;
		    sign = '-';
		} else {
		    ulli = (unsigned long long int)lli;
		}

		result = mpxx_integer_to_string_with_format(&m, sign, ulli, 10, length, &specs, &retlen);
		if(result) {
		    errno = result;
		    return -1;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : d i lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'u': {
		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = mpxx_get_va_list_unsigned(&ap, int_type);
		mpxx_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		result = mpxx_integer_to_string_with_format(&m, sign, ulli, 10, length, &specs, &retlen);
		if(result) {
		    errno = result;
		    return -1;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : u lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'o': {
		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = mpxx_get_va_list_unsigned(&ap, int_type);
		mpxx_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		result = mpxx_integer_to_string_with_format(&m, sign, ulli, 8, length, &specs, &retlen);
		if(result) {
		    errno = result;
		    return -1;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
	
//		xil_printf("%s : o lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'p':
		length = datamodel_tab[MPXX_STDARG_INTEGER_IS_Ptr].bytes;
		int_type = MPXX_STDARG_INTEGER_IS_Ptr;
		specs.f.zero_padding = 1;
		specs.f.alternative = 1;
		specs.f.str_is_lower = 1;
	    case 'x': 
		specs.f.str_is_lower = 1;
	    case 'X': {
		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = mpxx_get_va_list_unsigned(&ap, int_type);
		mpxx_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		result = mpxx_integer_to_string_with_format( &m, 0, ulli, 16, length, &specs, &retlen);
		if(result) {
		    errno = result;
		    return -1;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : pxX lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;
	    case 'c': {
		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = mpxx_get_va_list_unsigned(&ap, int_type);
		const char asc = (char)(0xff & ulli);
		mpxx_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		if( (max_len != 0) && prelength_is_over(max_len, (size_t)(lenofneeders+1)) ) {
		    errno = ENOSPC;
		    return -1;
		}


		if( NULL != _putc_cb ) {
		    _putc_cb(asc);
		}
		if( NULL != method_p->buf) {
		    *bufp = asc;
		    ++bufp;
		}
	    } break;
	    case 's': {
		const char *s = (char*)mpxx_get_va_list_pointer(&ap);

		if (NULL == s) {
		    s = "(null)";
		}
		tmp = (int)strlen(s);

		if ( precision && (precision < tmp) ) {
		    tmp = precision;
		}
		length -= tmp;
//		xil_printf(" specs.flags=%x specs.f.zero_padding=%d specs.f.left_justified=%d" EOL_CRLF, specs.flags, specs.f.zero_padding, specs.f.left_justified);
		if (specs.f.left_justified /* !(flags & LEFT_JUSTIFIED) */) {
		    if( (max_len != 0 ) && (length > 0) && prelength_is_over(max_len, (size_t)(lenofneeders+length)) ) {
			errno = ENOSPC;
			return -1;
		    } else {
			const char strpad = specs.f.zero_padding ? '0' : ' ';
			for(;length > 0; --length, ++lenofneeders ) {
			    if( NULL != _putc_cb ) {
				_putc_cb(strpad);
			    }
			    if( NULL != method_p->buf) {
				*bufp = strpad;
				++bufp;
			    }
			}
		    }
		}
		if( (max_len != 0) && ( tmp != 0) && prelength_is_over(max_len, (size_t)(lenofneeders+tmp)) ) {
		    errno = ENOSPC;
		    return -1;
		} else {
		    for(;tmp; ++s, --tmp, ++lenofneeders) {
			if( NULL != _putc_cb ) {
			    _putc_cb(*s);
			}
			if( NULL != method_p->buf) {
			    *bufp = *s;
			    ++bufp;
			}
		    }
		}
		if( (max_len != 0) && (length != 0) && prelength_is_over(max_len, (size_t)(lenofneeders+length)) ) {
		    errno = ENOSPC;
		    return -1;
		} else {
		    const char strpad = ' ';
		    for(;length > 0; --length, ++lenofneeders) {
			if( NULL != _putc_cb ) {
			    _putc_cb(strpad);
			}
			if( NULL != method_p->buf) {
			    *bufp = strpad;
			    ++bufp;
			}
		    }
		}
//		xil_printf("%s : s lenofneeders=%d" EOL_CRLF, __func__, lenofneeders); 
	    } break;
//          case '%': {
//              _putc_cb('%');
//          } break;
	    default: 
		if(1) {
		    errno = EINVAL;
		    return -1;
		} else {
		    for(;*fptr != '%';--fptr) {
			(void)fptr;
		    }
		}
	    }
	} 
    }
out:
    // xil_printf( " %s : function final lenofneeders=%d" EOL_CRLF, __func__,lenofneeders);

    return lenofneeders;
}


static mpxx_xtoa_output_method_t _this_out_method =
    mpxx_xtoa_output_method_initializer_set_callback_func(NULL, 0); 

int mpxx_lite_printf_init(int (*const putchar_cb_func)(const int asicccode))
{
    if( NULL == putchar_cb_func ) {
	return EINVAL;
    } else {
	const mpxx_xtoa_output_method_t out_method = 
	    mpxx_xtoa_output_method_initializer_set_callback_func(putchar_cb_func, 0);

#if 0
	xil_printf("%s : out_method=0x%p _putchar_cb=0x%p maxlenofstring=%d buf=0x%p" EOL_CRLF, __func__,
	out_method , out_method._putchar_cb, (int)out_method.maxlenofstring, out_method.buf);
#endif

	_this_out_method = out_method;
    }

#if 0
    xil_printf("%s : _this_out_method=0x%p _putchar_cb=0x%p maxlenofstring=%d buf=0x%p" EOL_CRLF, __func__,
	_this_out_method , _this_out_method._putchar_cb, (int)_this_out_method.maxlenofstring,
	_this_out_method.buf);
#endif

    return 0;
}

/**
 * @fn static int _this_putchar_stdout( const int asciicode)
 * @brief mpxx_integer_to_string_with_fomat()のための標準出力関数
 * @param asciicode アスキーコード
 * @return 1固定（出力文字数)
 */
static int _this_putchar_stdout( const int asciicode)
{
    putc( asciicode, stdout);
    return 1;
}


/**
 * @fn int mpxx_lite_printf_change_putchar_callback(const putchar_callback_ptr_t _putchar_cb);
 * @brief mpxx_lite_printf()が生成した文字列を指定された _putchar_cv で指定された関数に変更する
 * @param _putchar_cb 文字出力コールバック(NULLで デフォルトを設定
 * @retval 0 成功
 * @retval EINVAL 引数無効
 * */
int mpxx_lite_printf_change_putchar_callback(int (*const putchar_cb_func)(const int asicccode))
{
    if( NULL == putchar_cb_func ) {
	const mpxx_xtoa_output_method_t out_method = 
	    mpxx_xtoa_output_method_initializer_set_callback_func( _this_putchar_stdout, 0); 
	_this_out_method = out_method;
    } else { 
	const mpxx_xtoa_output_method_t out_method = 
	    mpxx_xtoa_output_method_initializer_set_callback_func( putchar_cb_func, 0); 
	_this_out_method = out_method;
    }

    return 0;
}

int mpxx_lite_vprintf(const char *const fmt, va_list ap)
{
    const int retval = _own_vsnprintf( &_this_out_method, fmt, ap);

    return retval;
}


int mpxx_lite_vsnprintf(char *const buf, const size_t max_length, const char *const fmt, va_list ap)
{
    const mpxx_xtoa_output_method_t _method = 
	mpxx_xtoa_output_method_initializer_set_buffer( buf, max_length);

    const int retval = _own_vsnprintf( &_method, fmt, ap);

    return retval;
}

/**
 * @fn int mpxx_lite_vsprintf(char *const buf, const char *const fmt, va_list ap)
 * @brief 可変長幕をを使ったbufの配列長を指定しない機能限定版vsprintf()
 * @param buf 生成文字列を書き出すバッファ
 * @param fmt 文字出力フォーマット
 * @param ap va_listマクロ構造体
 * @retval -1 失敗(errno参照)
 * @retval 0以上 bufに設定した文字数
 **/
int mpxx_lite_vsprintf(char *const buf, const char *const fmt, va_list ap)
{
    const mpxx_xtoa_output_method_t _method = 
	mpxx_xtoa_output_method_initializer_set_buffer( buf, 0);

    const int retval = _own_vsnprintf( &_method, fmt, ap);

    return retval;
}

/**
 * @fn int mpxx_lite_printf(const char *const fmt, ...)
 * @brief printf()のオリジナルソースコードカスタマイズ版です。コード削減のために浮動小数点は対応していません。
 * 	出力は mpxx_lite_printf_change_putchar_callback()で変更します。
 * @param fmt 出力フォーマット
 * @param ... 可変引数
 * @retval -1 失敗
 * @retval 0以上　出力文字数
 **/
int mpxx_lite_printf(const char *const fmt, ...)
{
    int retval;
    va_list ap;

    DBMS5( stderr, "%s : fmt=%s" EOL_CRLF,__func__, fmt);

    va_start(ap, fmt);
    retval = mpxx_lite_vprintf(fmt, ap);
    va_end(ap);

    return retval;
}

int mpxx_lite_sprintf(char *const buf, const char *const fmt, ...)
{
    int retval;
    va_list ap;

    DBMS5( stderr, "%s : fmt=%s" EOL_CRLF,__func__, fmt);

    va_start(ap, fmt);
    retval = mpxx_lite_vsprintf( buf, fmt, ap);
    va_end(ap);

    return retval;
}

int mpxx_lite_snprintf(char *const buf, const size_t max_length, const char *const fmt, ...)
{
    int retval;
    va_list ap;

    DBMS5( stderr, "%s : fmt=%s" EOL_CRLF,__func__, fmt);

    va_start(ap, fmt);
    retval = mpxx_lite_vsnprintf( buf, max_length, fmt, ap);
    va_end(ap);

    return retval;
}
