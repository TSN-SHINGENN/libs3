/** 
 *      Copyright 2015 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2015-April-24 Active 
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
#include <ctype.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

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
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_malloc.h"
#include "mpsx_rand.h"
#include "mpsx_stdlib.h"

#if defined(__GNUC__) && defined(_LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

static int8_t sign_is_minus( const char *str, const char **const vstart)
    __attribute__ ((optimize("Os")));
static uint64_t own_atox( const char *const str, const size_t bcd_len )
    __attribute__ ((optimize("Os")));
static uint64_t own_atoO( const char *const str, const size_t bcd_len )
    __attribute__ ((optimize("Os")));
static uint64_t own_atou( const char *const str, const size_t bcd_len )
    __attribute__ ((optimize("Os")));

extern char *mpsx_signed_to_string( const int64_t value, char * const buf, size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
    __attribute__ ((optimize("Os")));
extern char *mpsx_unsigned_to_string( const uint64_t value, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *const attr_p)
    __attribute__ ((optimize("Os")));
extern int mpsx_integer_to_string_with_format(const mpsx_xtoa_output_method_t *const method_p, char sign, unsigned long long value, const int radix, int field_length, const mpsx_xtoa_attr_t *const attr_p, int *const retlen_p)
    __attribute__ ((optimize("Os")));

extern int mpsx_rand(void)
    __attribute__ ((optimize("Os")));
extern void mpsx_srand(const unsigned int)
    __attribute__ ((optimize("Os")));

#endif /* end of _LIBS3_MICROCONTROLLER_OPTIMIZE_ENABLE */    


/**
 * @fn static int sign_is_minus( const char *str, const char **const vstart)
 * @brief 与えられた符号が - であることを判定します。
 * @param str 調査文字列
　* poaram vstart 次の文字のポインタ
 * @retval 0:偽 -1:真
 **/
static int8_t sign_is_minus( const char *str, const char **const vstart)
{
    int8_t sign = 0;
    switch (*str) {
    case '-':
        sign = ~0;
    case '+':
	++str;
	break;
    }

    *vstart = str;

    return sign;
}

/**
 * @fn int mpsx_ctoi(const char c)
 * @brief ASCIIコード数字を整数値に変換します。
 * @param c 数値キャラクタ
 * @retval -1 失敗
 * @retval -1以外 変換値
 **/
int mpsx_ctoi(const char c)
{
    if(( c < '0' ) ||  ('9' < c)) {
	return -1;
    }
    return (c-'0');
}


/**
 * @fn int mpsx_atoi( const char * const str)
 * @brief 10進文字列をsignd数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
int mpsx_atoi(const char *str)
{
    int tmp;
    const char *decimal_str;
    const int8_t decimal_is_minus = sign_is_minus( str, &decimal_str);

    for (tmp = 0; isdigit((unsigned char)*str); ++str) {
        tmp = tmp * 10 + *str - '0';
    } 
 
    return (decimal_is_minus) ? (int)(-1 * tmp) : (int)tmp;
}

/**
 * @fn int32_t mpsx_atoi32( const char * const str)
 * @brief 32ビット換算の10進文字列をsignd数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
int32_t mpsx_atoi32( const char * const str)
{
    const char *decimal_str;
    const int8_t decimal_is_minus = sign_is_minus( str, &decimal_str);
    uint32_t tmp = mpsx_atou32(decimal_str);

    if( (errno == 0) && (tmp & ( (uint32_t)1 << 31))) {
	errno = ERANGE;
	return 0;
    }

    return (decimal_is_minus) ? (int32_t)tmp * -1 : (int32_t)tmp;
}
/**
 * @fn int64_t mpsx_atoi64( const char * const str);
 * @brief 64ビット換算の10進文字列をsignd数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
int64_t mpsx_atoi64( const char * const str)
{
    const char *decimal_str;
    int8_t decimal_is_minus = sign_is_minus( str, &decimal_str);
    uint64_t tmp = mpsx_atou64(decimal_str);

    if( (errno == 0) && (tmp & ( (uint64_t)1 << 63))) {
	errno = ERANGE;
	return 0;
    }

    return (decimal_is_minus) ? (int64_t)tmp * -1 : (int64_t)tmp;
}

/**
 * @fn static uint64_t own_atou( const char *str, const char bcd_len )
 * @brief 符号なし10進文字列を数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @param bcd_len BCD換算の最大文字数
 * @return errno=0であれば変換が成功し、値を返します。
 **/
static uint64_t own_atou( const char * const str, const size_t bcd_len )
{
    const char * __restrict p = str;
    size_t len = strlen(p);
    size_t n;
    uint64_t retval=0;

    if( len > bcd_len ) {
	errno = ERANGE;
	return 0;
    } 

    errno = 0;

    for( n=0; n < len; ++n, ++p) {
	const char c = *p;
	unsigned char a=0;
	if( (c>= 0) && (c<= 9)  ) {
	    a = c - '0';
	} else {
	    errno = ERANGE;
	    return 0;
	}

	retval *= 10;
	retval += a;

 	if((bcd_len ==10) && ( 0xffffffff & retval )) { /* 32-bit */
	    errno = ERANGE;
	    return 0;
	} else if((bcd_len == 20)) { /* 64-bit */
	    const uint64_t tmp64 = (retval * 10) / 10;
	    if( tmp64 < retval) {
		errno = ERANGE;
		return 0;
	    }
        }
    }

    return retval;
}

/**
 * @fn uint32_t mpsx_atou32( const char * const str)
 * @brief 32ビット換算の10進文字列をunsignd数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint32_t mpsx_atou32( const char * const str)
{
    return (uint32_t)own_atou( str, 10);
}

/**
 * @fn uint32_t mpsx_atou32( const char * const str)
 * @brief 32ビット換算の10進文字列をundigned数値に変換します。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint64_t mpsx_atou64( const char * const str)
{
    return (uint32_t)own_atou( str, 20);
}

/**
 * @fn static uint64_t own_atox( const char *const str, const size_t bcd_len )
 * @brief 16進文字列を数値に変換します。前置詞の0xは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @param bcd_len BCD換算の最大文字数
 * @return errno=0であれば変換が成功し、値を返します。
 **/
static uint64_t own_atox( const char *const str, const size_t bcd_len )
{
    size_t len;
    const char * __restrict p=str;
    unsigned int n;
    uint64_t retval=0;

    if( !strncmp("0x", str, 2) || !strncmp("0X", str, 2 ) ) {
	p+=2; len=strlen(p);
    } else {
	len=strlen(p);
    }

    if( len > bcd_len ) {
	errno = ERANGE;
	return 0;
    } 

    errno = 0;

    for( n=0; n < len; ++n, ++p) {
	const char c = *p;
	unsigned char a=0;

	if( (c>= '0') && (c<= '9')  ) {
	    a = c - '0';
	} else if ( (c>='a') && (c<='f') ) {
	    a = (c - 'a') + 10;
	} else if ( (c>='A') && (c<='F') ) {
	    a = (c - 'A') + 10;
	} else {
	    errno = ERANGE;
//	    retval=0;
//	    goto out;
	    return 0;
	}

#if 0
	switch(c) {
	    case 'a':
	    case 'b':
	    case 'c':
	    case 'd':
	    case 'e':
	    case 'f':
		a = (c - 'a') + 10;
	        break;
	    case 'A':
	    case 'B':
	    case 'C':
	    case 'D':
	    case 'E':
	    case 'F':
		a = (c - 'A') + 10;
		break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		a = c - '0';
		break;
	    default:
		errno = ERANGE;
		retval=0;
		goto out;
	}
#endif
	retval <<= 4;
	retval |= (0xf & a);
    }

// out:

    return retval;
}


/**
 * @fn static uint64_t own_atoO( const char *str, const char bcd_len )
 * @brief 8進文字列を数値に変換します。前置詞の0oは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @param bcd_len BCD換算の最大文字数
 * @return errno=0であれば変換が成功し、値を返します。
 **/
static uint64_t own_atoO( const char * const str, const size_t bcd_len )
{
    size_t len;
    const char * __restrict p = str;
    unsigned int n, retval=0;

    if( !strncmp("0o", str, 2) || !strncmp("0o", str, 2 ) ) {
	p+=2; len=strlen(p);
    } else {
	len=strlen(p);
    }

    if( len > bcd_len ) {
	errno = ERANGE;
	return 0;
    } 

    errno = 0;

    for( n=0; n < len; ++n, ++p) {
	const char c = *p;
	unsigned char a=0;
	if( (c>= 0) && (c<= 7)  ) {
	    a = c - '0';
	} else {
	    errno = ERANGE;
//	    retval=0;
//	    goto out;
	    return 0;
	}
#if 0	
	switch(c) {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
		a = c - '0';
		break;
	    default:
		errno = ERANGE;
		retval=0;
		goto out;
	}
#endif
	if( n==0 ) {
	    if((bcd_len == 11 ) && (a > 3)  ) { /* 32bit */
		errno = ERANGE;
//	        retval = 0;
//	        goto out;
		return 0;
	    } else if((bcd_len == 22 ) && ( a > 1 ) ) { /* 64bit */
		errno = ERANGE;
//	    retval = 0;
//	    goto out;
	        return 0;
	    }
	}

	retval <<= 3;
	retval |= (0x7 & a);
    }

//out:

    return retval;
}

/**
 * @fn uint32_t mpsx_atox32( const char *str )
 * @brief 32ビット換算の16進文字列を数値に変換します。前置詞の0xは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint32_t mpsx_atox32( const char * const str )
{
    return (uint32_t)own_atox( str, 8 );
}

/**
 * @fn uint32_t mpsx_atox32( const char *str )
 * @brief 32ビット換算の16進文字列を数値に変換します。前置詞の0xは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint64_t mpsx_atox64( const char * const str )
{
    return own_atox( str, 16 );
}


/**
 * @fn uint64_t mpsx_atoo32( const char *str )
 * @brief 32ビット換算の8進文字列を数値に変換します。前置詞の0oは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint32_t mpsx_atoo32( const char * const str )
{
    return (uint32_t)own_atoO( str, 11 );
}

/**
 * @fn uint64_t mpsx_atoo64( const char *str )
 * @brief 32ビット換算の8進文字列を数値に変換します。前置詞の0oは無視します。
 *	大文字・小文字は問いません。
 *	エラーの場合は0を戻し、errnoにERANGEを設定します。
 * @param str 変換する文字列
 * @return errno=0であれば変換が成功し、値を返します。
 **/
uint64_t mpsx_atoo64( const char * const str )
{
    return own_atoO( str, 22 );
}

static char *own_utoa( const uint64_t value, char *const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p);
static char *own_itoa( const int64_t value, char *const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p);

static const char * const own_x2a_lower_str = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char * const own_x2a_upper_str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


/** 
 * @fn static char *own_utoa( const uint64_t value, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗(errnoに番号設定）
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
#if 0
static char *own_utoa( const uint64_t value, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
{
    mpsx_xtoa_attr_t zeroattr = {0};

    if( NULL == attr_p ) {
	attr_p = &zeroattr;
    }

    if( ( NULL == buf ) || (radix < 2) || !(strlen(own_x2a_lower_str) > radix) ) {
	errno = EINVAL;
        return NULL;
     }

     if( value < radix) {
	if( !(attr_p->f.no_assign_terminate) && (bufsz < 2)) {
	    errno = ERANGE;
	    return NULL;
	}
        buf[0] = (attr_p->f.str_is_lower) ? own_x2a_lower_str[value] : own_x2a_upper_str[value];
	if(!(attr_p->f.no_assign_terminate) ) {
	    buf[1] = '\0';
	}
     } else {
         size_t remain_sz = bufsz-1;
         char *p;

	 if(!remain_sz) return NULL;
         /* 処理を行うための再帰for-loop */
         for(p = own_utoa((value / radix), buf, remain_sz, radix, attr_p);
			 (p !=NULL) && ( *p!=0 ); ++p, --remain_sz); 
	 if(NULL==p) {
	    errno = ERANGE;
	    return NULL;
	 }
	 p = own_utoa(value % radix, p, remain_sz, radix, attr_p);
	 if(NULL==p) {
	    errno = ERANGE;
	    return NULL;
	 }
     }

     return buf;
}
#else
static char *own_utoa( const uint64_t value, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
{
    const mpsx_xtoa_output_method_t o =  mpsx_xtoa_output_method_initializer_set_buffer( buf, bufsz);
    const mpsx_xtoa_attr_t attr = {0};
    const int field_length = 0;

    char *retptr; 
    int reterr, retlen;

    if(NULL == attr_p) {
	attr_p = &attr;
    }

    reterr = mpsx_integer_to_string_with_format(&o, 0, (unsigned long long)value, radix, field_length, &attr, &retlen);

    retptr = (reterr == 0 ) ? buf : NULL;

    return retptr;
}
#endif


/** 
 * @fn static char *own_itoa( const int64_t value, char * const buf, size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。マイナス値は基数に関係なく先頭に'-'を付けます。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
#if 0
static char *own_itoa( const int64_t i64v, char * const buf, size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
{
    char *p = (char*)buf;
    uint64_t abs_value;
    unsigned int min_field_length = 1;

    if( i64v < 0) {
	++min_field_length; /* aigned flag */
    }
    if(!attr_p->f.no_assign_terminate) {
	++min_field_length; /* CR */
    }
    if( bufsz < min_field_length) {
	errno = ERANGE;
	return NULL;
    }

     if(i64v < 0) {
         *p = '-';
	 ++p;
         abs_value = -i64v;
	 --bufsz;
     } else {
         abs_value = i64v;
     }
     
     return own_utoa( abs_value, p, bufsz, radix, attr_p);
}
#else
static char *own_itoa( const int64_t i64v, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
{
    const mpsx_xtoa_output_method_t o =  mpsx_xtoa_output_method_initializer_set_buffer( buf, bufsz);
    const mpsx_xtoa_attr_t attr = {0};
    const int field_length = 0;

    uint64_t abs_value;
    int reterr, retlen;
    int sign;
    char *retptr; 

    if(NULL == attr_p) {
	attr_p = &attr;
    }

    if(i64v < 0) {
	sign = '-';
	abs_value = -i64v;
    } else {
	sign = 0;
        abs_value = i64v;
     }

    reterr = mpsx_integer_to_string_with_format(&o, sign, (unsigned long long)abs_value, radix, field_length, &attr, &retlen);

    retptr = (reterr == 0 ) ? buf : NULL;

    return retptr;

}
#endif

/** 
 * @fn char *mpsx_i32toa( const int32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。マイナス値は基数に関係なく先頭に'-'を付けます。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_i32toa( const int32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
{
    return own_itoa( (int64_t)value, (char*)buf, bufsz, radix, attr_p);
}

/** 
 * @fn char *mpsx_i64toa( const int64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。マイナス値は基数に関係なく先頭に'-'を付けます。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_i64toa( const int64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
{
    return own_itoa( value, (char*)buf, bufsz, radix, attr_p);
}

/** 
 * @fn char *mpsx_u32toa( const uint32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u32toa( const uint32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
{
    return own_utoa( (uint64_t)value, (char*)buf, bufsz, radix, attr_p);
}

/** 
 * @fn char *mpsx_u64toa( const uint64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値をradixで指定された文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsize バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u64toa( const uint64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p)
{
    return own_utoa( value, (char*)buf, bufsz, radix, attr_p);
}

/** 
 * @fn char *mpsx_u32toahex( const uint32_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t *attr_p)
 * @brief value値を符号なし16進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @param radix 基数値(2- ) 
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u32toahex( const uint32_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t * const attr_p)
{
    return mpsx_u32toa( value, buf, bufsz, MPSX_XTOA_RADIX_HEX, attr_p);
}

/** 
 * @fn char *mpsx_u64toahex( const uint64_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t * const attr_p)
 * @brief value値を符号なし16進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @param radix 基数値(2- )
 * @retval attr_p mpsx_xtoa_attr_t属性ポインタ（NULLでデフォルト)
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u64toahex( const uint64_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t * const attr_p)
{
    return mpsx_u64toa( value, buf, bufsz, MPSX_XTOA_RADIX_HEX, attr_p);
}


/** 
 * @fn char *mpsx_u64toadec( const uint64_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号なし10進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u64toadec( const uint64_t value, void * const buf, const size_t bufsz)
{
    return mpsx_u64toa( value, buf, bufsz, MPSX_XTOA_RADIX_DEC, NULL);
}


/** 
 * @fn char *mpsx_u32toadec( const uint32_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号なし10進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u32toadec( const uint32_t value, void * const buf, const size_t bufsz)
{
    return mpsx_u32toa( value, buf, bufsz, MPSX_XTOA_RADIX_DEC, NULL);
}


/** 
 * @fn char *mpsx_i64toadec( const int64_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号あり10進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_i64toadec( const int64_t value, void * const buf, const size_t bufsz)
{
    return mpsx_i64toa( value, buf, bufsz, MPSX_XTOA_RADIX_DEC, NULL);
}


/** 
 * @fn char *mpsx_i32toadec( const int32_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号あり10進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_i32toadec( const int32_t value, void * const buf, const size_t bufsz)
{
    return mpsx_i32toa( value, buf, bufsz, MPSX_XTOA_RADIX_DEC, NULL);
}

/** 
 * @fn char *mpsx_u64toabin( const uint64_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号なし2進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u64toabin( const uint64_t value, void * const buf, const size_t bufsz)
{
    return mpsx_u64toa( value, buf, bufsz, MPSX_XTOA_RADIX_BIN, NULL);
}

/** 
 * @fn char *mpsx_u32toabin( const uint32_t value, void * const buf, const size_t bufsz)
 * @brief value値を符号なし2進文字列に変換します。
 * @param value 変換値
 * @param buf 変換した文字列を格納するバッファ(マルチスレッド対策)
 * @param bufsz バッファサイズ
 * @retval NULL 失敗
 *     ERANGE : 文字列変換時にバッファに収まりきらない
 *     EINVAL : BUFが無効又はradixが無効
 * @retval NULL以外 成功（buf値が戻る）
 **/
char *mpsx_u32toabin( const uint32_t value, void * const buf, const size_t bufsz)
{
    return mpsx_u32toa( value, buf, bufsz, MPSX_XTOA_RADIX_BIN, NULL);
}

/**
 * @fn int mpsx_integer_to_string_with_format(mpsx_xtoa_output_method_t method_p, char sign, unsigned long long value, const int radix, int length, const mpsx_xtoa_attr_t *const opts_p, const int szoflimit, int *retlen_p)
 * @brief 引数の情報から整数値を変換してコールバックにASCIIコードで送ります。
 * @param method_p 文字出力（出力先コールバック・バッファ・最大文字長)
 * @param sign 符号(ASCIIコードで、0は符号なし）
 * @param value 文字列変換値
 * @param radix 基数
 * @param field_length パディング文字を含めたフィールド文字数
 * @param opts_p 文字変換時のオプション属性フラグポインタ
 * @param retlen_p 書き出し文字数取得用変数用ポインタ
 * @retval 0 成功
 * @retval 0以外　エラー番号
 */
int mpsx_integer_to_string_with_format(const mpsx_xtoa_output_method_t *const method_p, char sign, unsigned long long value, const int radix, int field_length, const mpsx_xtoa_attr_t *const attr_p, int *const retlen_p)
{
    const char *const symbols = (attr_p->f.str_is_lower) ? own_x2a_lower_str : own_x2a_upper_str;
    const size_t szoflimit = method_p->maxlenofstring;
    const putchar_callback_ptr_t _putchar_cb =  method_p->_putchar_cb;
    const int estimate_field_length = field_length;

    char pad = ' ';
    int strpoint = 0;
    char stack_buf[MPSX_XTOA_BIN64_BUFSZ];
    int lenofneeders = (attr_p->f.no_assign_terminate) ? 0 : 1;  /* \0のため */
    char *bufp = method_p->buf;

    if( NULL == method_p ) {
	DBMS1( stderr, "%s : methos_p is NULL" EOL_CRLF, __func__);
	return EINVAL;
    }

    /* value値の文字列変換および,のスタック追加 */
    do {
	const uint8_t digi = (uint8_t)(value % radix);
#if 1
	stack_buf[strpoint] = symbols[digi];
	++strpoint;
#else
	if( NULL == own_utoa( digi, &stack_buf[strpoint], 1, radix, attr_p)) {
	    DBMS1( stderr, "%s : own_utoa fail" EOL_CRLF, __func__);
	    return EOVERFLOW;
	}
	++strpoint;
#endif
	if( (attr_p->f.thousand_group) && ((strpoint & 0x3) == 0x3)) {
	    stack_buf[strpoint] = ',';
	    ++strpoint;
	}
    } while ( value /= radix );
    if( field_length > 0 ) {
	field_length -= strpoint;
    }

    /* 右寄せの際のPadding設定 */
    if (attr_p->f.left_justified) {
        if(attr_p->f.zero_padding) {
	    pad = '0'; /* pad 変更 */
	}

        for(;field_length > 0; --field_length) {
	    stack_buf[strpoint] = pad;
	    ++strpoint;
	}
    }

    if (( sign != 0 ) && (radix == 10)) { 
	stack_buf[strpoint] = sign;
	++strpoint;
	--field_length;
    } else if (attr_p->f.alternative) {
	/* 基数ヘッドの処理 */
	if (radix == 8) {
	    stack_buf[strpoint] = '0';
	    ++strpoint;
	    --field_length;
	} else if (radix == 16) {
           stack_buf[strpoint] = 'x';
	    ++strpoint;
           stack_buf[strpoint] = '0';
	    ++strpoint;
	    field_length -= 2;
        }
    }

    if(1) {
  	const unsigned int total_length = (field_length > 0 ) ? (strpoint + field_length) : strpoint;
	if( retlen_p != NULL ) {
	    *retlen_p = total_length;
	}
	if((szoflimit != 0) && !( szoflimit > total_length)) {
	    return ENOSPC;
	}
    }

    while(strpoint > 0) {
	const char c = stack_buf[--strpoint];
	if( NULL != _putchar_cb ) {
	    _putchar_cb(c);
	}
	if( NULL !=  bufp ) {
	    *bufp = c;
	    ++bufp;
	}
    }
    while(field_length > 0) {
	if( NULL != _putchar_cb ) {
	    _putchar_cb(pad);
	}
	if( NULL !=  bufp ) {
	    *bufp = pad;
	    ++bufp;
	}
	--field_length;
    }

    if(!(attr_p->f.no_assign_terminate)) {
	if( NULL !=  bufp ) {
	    *bufp = '\0';
	}
    }

    return 0;
}

/**
 * @fn int mpsx_rand(void)
 * @brief ANSI/ISO/IEC C/ POSIXに対応する乱数発生を行う
 * @retval乱数値
 */
int mpsx_rand(void)
{
    return mpsx_rand_ansi();
}

/** 
 * @fn void mpsx_srand(const unsigned int seed)
 * @brief mpsx_rand()が使用する、新しい種を設定する
 * @param 新しい種
 */
 void mpsx_srand(const unsigned int seed)
{
    mpsx_rand_ansi_set_seed(seed);
    return;
}

