/**
 *	Copyright 2000 TSN・SHINGENN
 *
 *	Basic Author: Seiichi Takeda  '2000-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_endian.h
 * @brief エンディアン変換設定用マクロライブラリ及びソースコード
 *	gccで定義される __BIG_ENDIAN__ / __LITTLE_ENDIAN__ を基準に条件のエンディアンを確認します。
 */

#ifndef INC_MPSX_ENDIAN_H
#define INC_MPSX_ENDIAN_H

#pragma once

#include <stdint.h>

#if defined(WIN32)
#include <stdlib.h>
#endif

#if defined(QNX)
#include <gulliver.h>
#endif

#pragma once

/**
 * エンディア変換マクロ V2（インライン展開）
 */

#include <stdint.h>

#if defined(Linux)
 /**
  * @note GCCで一般的な定義を見るようにしていたが各ディストリビューションで定義が変わるようなのでそれを見るようにした
  */

 #if (!defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) /* standard gcc */
  #if !(defined(__BYTE_ORDER__) && ( defined(__ORDER_BIG_ENDIAN__) || defined(__ORDER_LITTLE_ENDIAN__))) /* fedora,suse, etc...*/ 
   #if !(defined(__BYTE_ORDER) && ( defined(__LITTLE_ENDIAN) || defined(__BIG_ENDIAN))) /* Fedora, CentoOS, etc */
    #include <endian.h>
   #endif
  #endif
 #endif
#endif

/* endian checker */
#if (!defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#if defined(QNX)
#if defined(__LITTLEENDIAN__)
#define __LITTLE_ENDIAN__
#elif defined(__BIGENDIAN__)
#define __BIG_ENDIAN__
#endif
#elif defined(WIN32)
#define __LITTLE_ENDIAN__
#elif (defined(__BYTE_ORDER__) && ( defined(__ORDER_BIG_ENDIAN__) || defined(__ORDER_LITTLE_ENDIAN__)))
#if ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
#define __BIG_ENDIAN__
#elif ( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ )
#define __LITTLE_ENDIAN__
#endif
#elif (defined(__BYTE_ORDER) && ( defined(__LITTLE_ENDIAN) || defined(__BIG_ENDIAN)))
#if ( __BYTE_ORDER  == __BIG_ENDIAN )
#define __BIG_ENDIAN__
#elif ( __BYTE_ORDER == __LITTLE_ENDIAN )
#define __LITTLE_ENDIAN__
#endif
#else
#error 'can not decide endian environment'
#endif
#endif

#if 0 /* for Debug */
#if defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#error (DEBUG)__BIG_ENDIAN__
#elif defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#error (DEBUG)__LITTLE_ENDIAN__
#else
#error '(DEBUG)can not decide endian environment'
#endif
#endif /* end of Debug */

#if defined(__LITTLE_ENDIAN__) && defined(__BIG_ENDIAN__)
#error '(DEBUG)can not decide endian environment'
#endif

static __inline void mpsx_set_be_d64( void * const p, const uint64_t d)
{
#if defined(QNX)
    *((uint64_t*)p) = ENDIAN_BE64(d);
#elif defined(WIN32) && !defined(__MINGW32__)
    *((__int64*)p) = _byteswap_uint64((__int64)d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *(uint64_t *)p = (uint64_t)(d);
#else 
    *((uint8_t*)p +  0) = (uint8_t)((d >> (8*7)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*6)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  5) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  6) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  7) = (uint8_t)((d         ) & 0xff);
#endif
#endif
}

/**
 * @def MPSX_SET_BE_D64(p, d)
 * @brief 64-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアンに変換してポインタpに展開します
 */
#define MPSX_SET_BE_D64(p, d) mpsx_set_be_d64(p , d)

static __inline uint64_t mpsx_get_be_d64(const void * const p)
{
    uint64_t u64;
#if defined(QNX)
    u64 = ENDIAN_BE64(*(uint64_t*)p);
#elif defined(WIN32) && !defined(__MINGW32__)
    u64 = (uint64_t)_byteswap_uint64(*(__int64*)p);
#else /* simple */
#if defined(__BIG_ENDIAN__)
    u64 = *(uint64_t*)(p);
#else
    u64 = ((uint64_t)(*((const uint8_t*)(p) +  0) & 0xff) << (8*7)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  1) & 0xff) << (8*6)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  2) & 0xff) << (8*5)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  3) & 0xff) << (8*4)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  4) & 0xff) << (8*3)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  5) & 0xff) << (8*2)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  6) & 0xff) << (8*1)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  7) & 0xff)      );
#endif
#endif
    return u64;
}

/**
 * @def MPSX_GET_BE_D64(p)
 * @brief ポインタpに示された64-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D64(p) mpsx_get_be_d64(p)

static __inline void mpsx_set_le_d64(void * const p, const uint64_t d)
{
#if defined(QNX)
    *((uint64_t*)p) = ENDIAN_LE64(d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *((uint8_t*)p +  7) = (uint8_t)((d >> (8*7)) & 0xff);
    *((uint8_t*)p +  6) = (uint8_t)((d >> (8*6)) & 0xff);
    *((uint8_t*)p +  5) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  0) = (uint8_t)((d         ) & 0xff); 
#else
    *(uint64_t*)p = d;
#endif
#endif
}
/**
 * @def MPSX_SET_LE_D64(p, d)
 * @brief 64-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアンにポインタpに展開します
 */
#define MPSX_SET_LE_D64(p, d) mpsx_set_le_d64( p, d)

static __inline uint64_t mpsx_get_le_d64(const void * const p)
{
    uint64_t u64;
#if defined(QNX)
    u64 = ENDIAN_LE64(*(uint64_t*)p);
#else /* simple */
#ifdef __BIG_ENDIAN__
    u64 = (((uint64_t)*((uint8_t*)p +  7) & 0xff) << (8*7)) |
          (((uint64_t)*((uint8_t*)p +  6) & 0xff) << (8*6)) |
          (((uint64_t)*((uint8_t*)p +  5) & 0xff) << (8*5)) |
          (((uint64_t)*((uint8_t*)p +  4) & 0xff) << (8*4)) |
          (((uint64_t)*((uint8_t*)p +  3) & 0xff) << (8*3)) |
          (((uint64_t)*((uint8_t*)p +  2) & 0xff) << (8*2)) |
          (((uint64_t)*((uint8_t*)p +  1) & 0xff) << (8*1)) |
          (((uint64_t)*((uint8_t*)p +  0) & 0xff)         );
#else
    u64 = *(const uint64_t*)p;
#endif
#endif
    return u64;
}

/**
 * @def MPSX_GET_LE_D64(p)
 * @brief ポインタpに示された64-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D64(p) mpsx_get_le_d64(p)

static __inline void mpsx_set_be_d56( void * const p, const uint64_t d)
{
    *((uint8_t*)p +  0) = (uint8_t)((d >> (8*6)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  5) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  6) = (uint8_t)((d         ) & 0xff);

    return;
}

/**
 * @def MPSX_SET_BE_D56(p, d)
 * @brief 56-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアンに変換してポインタpに展開します
 */
#define MPSX_SET_BE_D56(p, d) mpsx_set_be_d56(p , d)

static __inline uint64_t mpsx_get_be_d56(const void * const p)
{
    uint64_t u64;
    u64 = ((uint64_t)(*((const uint8_t*)(p) +  0) & 0xff) << (8*6)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  1) & 0xff) << (8*5)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  2) & 0xff) << (8*4)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  3) & 0xff) << (8*3)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  4) & 0xff) << (8*2)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  5) & 0xff) << (8*1)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  6) & 0xff)      );

    return u64;
}

/**
 * @def MPSX_GET_BE_D56(p)
 * @brief ポインタpに示された56-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D56(p) mpsx_get_be_d56(p)

static __inline void mpsx_set_le_d56(void * const p, const uint64_t d)
{
    *((uint8_t*)p +  6) = (uint8_t)((d >> (8*6)) & 0xff);
    *((uint8_t*)p +  5) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  0) = (uint8_t)((d         ) & 0xff); 
}

/**
 * @def MPSX_SET_LE_D56(p, d)
 * @brief 56-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアンにポインタpに展開します
 */
#define MPSX_SET_LE_D56(p, d) mpsx_set_le_d56( p, d)

static __inline uint64_t mpsx_get_le_d56(const void * const p)
{
    uint64_t u64;
    u64 = (((uint64_t)*((uint8_t*)p +  6) & 0xff) << (8 * 6)) |
          (((uint64_t)*((uint8_t*)p +  5) & 0xff) << (8 * 5)) |
          (((uint64_t)*((uint8_t*)p +  4) & 0xff) << (8 * 4)) |
          (((uint64_t)*((uint8_t*)p +  3) & 0xff) << (8 * 3)) |
          (((uint64_t)*((uint8_t*)p +  2) & 0xff) << (8 * 2)) |
          (((uint64_t)*((uint8_t*)p +  1) & 0xff) << (8 * 1)) |
          (((uint64_t)*((uint8_t*)p +  0) & 0xff) );

    return u64;
}

/**
 * @def MPSX_GET_LE_D56(p)
 * @brief ポインタpに示された56-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D56(p) mpsx_get_le_d56(p)


static __inline void mpsx_set_be_d48( void * const p, const uint64_t d)
{
    *((uint8_t*)p +  0) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  5) = (uint8_t)((d         ) & 0xff);

    return;
}

/**
 * @def MPSX_SET_BE_D48(p, d)
 * @brief 48-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアンに変換してポインタpに展開します
 */
#define MPSX_SET_BE_D48(p, d) mpsx_set_be_d48(p , d)

static __inline uint64_t mpsx_get_be_d48(const void * const p)
{
    uint64_t u64;
    u64 = ((uint64_t)(*((const uint8_t*)(p) +  0) & 0xff) << (8*5)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  1) & 0xff) << (8*4)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  2) & 0xff) << (8*3)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  3) & 0xff) << (8*2)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  4) & 0xff) << (8*1)) |
	  ((uint64_t)(*((const uint8_t*)(p) +  5) & 0xff)      );

    return u64;
}

/**
 * @def MPSX_GET_BE_D48(p)
 * @brief ポインタpに示された48-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D48(p) mpsx_get_be_d48(p)

static __inline void mpsx_set_le_d48(void * const p, const uint64_t d)
{
    *((uint8_t*)p +  5) = (uint8_t)((d >> (8*5)) & 0xff);
    *((uint8_t*)p +  4) = (uint8_t)((d >> (8*4)) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d >> (8*3)) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> (8*2)) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> (8*1)) & 0xff);
    *((uint8_t*)p +  0) = (uint8_t)((d         ) & 0xff); 
}

/**
 * @def MPSX_SET_LE_D48(p, d)
 * @brief 48-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアンにポインタpに展開します
 */
#define MPSX_SET_LE_D48(p, d) mpsx_set_le_d48( p, d)

static __inline uint64_t mpsx_get_le_d48(const void * const p)
{
    uint64_t u64;
    u64 = (((uint64_t)*((uint8_t*)p +  5) & 0xff) << (8 * 5)) |
          (((uint64_t)*((uint8_t*)p +  4) & 0xff) << (8 * 4)) |
          (((uint64_t)*((uint8_t*)p +  3) & 0xff) << (8 * 3)) |
          (((uint64_t)*((uint8_t*)p +  2) & 0xff) << (8 * 2)) |
          (((uint64_t)*((uint8_t*)p +  1) & 0xff) << (8 * 1)) |
          (((uint64_t)*((uint8_t*)p +  0) & 0xff) );

    return u64;
}

/**
 * @def MPSX_GET_LE_D48(p)
 * @brief ポインタpに示された48-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D48(p) mpsx_get_le_d48(p)

static __inline void mpsx_set_be_d32( void * const p, const uint32_t d)
{
#if defined(QNX)
    *((uint32_t*)p) = ENDIAN_BE32(d);
#elif defined(WIN32) && !defined(__MINGW32__)
    *((unsigned long*)p) = _byteswap_ulong((unsigned long)d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *(uint32_t *)p = (uint32_t)(d);
#else
    *((uint8_t*)p +  0) = (uint8_t)((d >> 24) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)((d >> 16) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >>  8) & 0xff);
    *((uint8_t*)p +  3) = (uint8_t)((d      ) & 0xff);
#endif
#endif
}

/**
 * @def MPSX_SET_BE_D32(p, d)
 * @brief 32-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアンに変換してポインタpに展開します
 */
#define MPSX_SET_BE_D32(p, d) mpsx_set_be_d32(p , d)

static __inline uint32_t mpsx_get_be_d32(const void * const p)
{
    uint32_t u32;
#if defined(QNX)
    u32 = ENDIAN_BE32(*(uint32_t*)p);
#elif defined(WIN32) && !defined(__MINGW32__)
    u32 = (uint32_t)_byteswap_ulong(*(unsigned long*)p);
#else /* simple */
#ifdef __BIG_ENDIAN__
    u32 = *(uint32_t*)(p);
#else
    u32 = ((uint32_t)(*((const uint8_t*)(p) +  0) & 0xff) << 24) |
	  ((uint32_t)(*((const uint8_t*)(p) +  1) & 0xff) << 16) |
	  ((uint32_t)(*((const uint8_t*)(p) +  2) & 0xff) <<  8) |
	  ((uint32_t)(*((const uint8_t*)(p) +  3) & 0xff)      );
#endif
#endif
    return u32;
}

/**
 * @def MPSX_GET_BE_D32(p)
 * @brief ポインタpに示された32-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D32(p) mpsx_get_be_d32(p)

static __inline void mpsx_set_le_d32( void *p, const uint32_t d)
{
#if defined(QNX)
    *((uint32_t*)p) = ENDIAN_LE32(d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *((uint8_t*)p +  3) = (uint8_t)((d >> 24) & 0xff);
    *((uint8_t*)p +  2) = (uint8_t)((d >> 16) & 0xff); 
    *((uint8_t*)p +  1) = (uint8_t)((d >>  8) & 0xff);
    *((uint8_t*)p +  0) = (uint8_t)((d      ) & 0xff);
#else
    *(uint32_t*)p = d;
#endif
#endif
}

/**
 * @def MPSX_SET_LE_D32(p, d)
 * @brief 32-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアン形式に変換してポインタpに展開します
 */
#define MPSX_SET_LE_D32(p, d) mpsx_set_le_d32(p, d)

static __inline uint32_t mpsx_get_le_d32(const void * const p)
{
    uint32_t r32;

#if defined(QNX)
    r32 = ENDIAN_LE32(*(uint32_t*)p);
#else /* simple */
#ifdef __BIG_ENDIAN__
    r32 = (((uint32_t)(*((uint8_t*)(p) +  3) & 0xff) << 24) | 
           ((uint32_t)(*((uint8_t*)(p) +  2) & 0xff) << 16) | 
           ((uint32_t)(*((uint8_t*)(p) +  1) & 0xff) <<  8) | 
           ((uint32_t)(*((uint8_t*)(p) +  0) & 0xff)       ));
#else
    r32 = ((uint32_t)*(uint32_t*)p);
#endif
#endif

    return r32;
}

/**
 * @def MPSX_GET_LE_D32(p)
 * @brief ポインタpに示された32-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D32(d) mpsx_get_le_d32(d)

/**
 * @def MPSX_SET_BE_D24(p, d)
 * @brief 24-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアン形式に変換してポインタpに展開します
 */
#define MPSX_SET_BE_D24(p, d) \
		do { \
		    *((uint8_t*)(p) +  0) = (uint8_t)(((uint32_t)(d) >> 16) & 0xff);\
		    *((uint8_t*)(p) +  1) = (uint8_t)(((uint32_t)(d) >>  8) & 0xff); \
		    *((uint8_t*)(p) +  2) = (uint8_t)(((uint32_t)(d)      ) & 0xff);     \
		} while (0)

/**
 * @def MPSX_GET_BE_D24(p)
 * @brief ポインタpに示された24-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D24(p) \
		(\
		    (((uint32_t)*((uint8_t*)p +  0) & 0xff) << 16) | \
		    (((uint32_t)*((uint8_t*)p +  1) & 0xff) << 8) | \
		     ((uint32_t)*((uint8_t*)p +  2) & 0xff)\
		)

/**
 * @def MPSX_SET_LE_D24(p, d)
 * @brief 24-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアン形式に変換してポインタpに展開します
 */
#define MPSX_SET_LE_D24(p, d)   \
		do { \
		    *((uint8_t*)(p) +  2) = (uint8_t)(((uint32_t)(d) >> 16) & 0xff); \
		    *((uint8_t*)(p) +  1) = (uint8_t)(((uint32_t)(d) >> 8) & 0xff); \
		    *((uint8_t*)(p) +  0) = (uint8_t)((uint32_t)(d) & 0xff); \
		} while (0)

/**
 * @def MPSX_GET_LE_D24(p)
 * @brief ポインタpに示された24-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D24(p) \
		(\
		    (((uint32_t)*((uint8_t*)(p) +  2) & 0xff) << 16) | \
		    (((uint32_t)*((uint8_t*)(p) +  1) & 0xff) << 8) | \
		     ((uint32_t)*((uint8_t*)(p) +  0) & 0xff)\
		)

static __inline void mpsx_set_be_d16( void * const p, const uint16_t d)
{
#if defined(QNX)
    *((uint16_t*)p) = ENDIAN_BE16(d);
#elif defined(WIN32) && !defined(__MINGW32__)
    *((uint16_t*)p) = _byteswap_ushort(d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *(uint16_t*)p = d; 
#else
    *((uint8_t*)p +  0) = (uint8_t)((d >> 8) & 0xff);
    *((uint8_t*)p +  1) = (uint8_t)(d        & 0xff);
#endif
#endif
}

/**
 * @def MPSX_SET_BE_D16(p, d)
 * @brief 16-bitパック変数dをのプラットフォーム固有エンディアン形式からビックエンディアン形式に変換してポインタpに展開します
 */
#define MPSX_SET_BE_D16(p, d) mpsx_set_be_d16( p, d)


static __inline void mpsx_set_le_d16( void * const p, const uint32_t d)
{
#if defined(QNX)
    *((uint16_t*)p) = ENDIAN_LE16(d);
#else /* simple */
#ifdef __BIG_ENDIAN__
    *((uint8_t*)(p) +  1) = (uint8_t)((d >> 8) & 0xff);
    *((uint8_t*)(p) +  0) = (uint8_t)((d     ) & 0xff);
#else
    *(uint16_t*)(p) = ( d & 0xffff );
#endif
#endif
}

/**
 * @def MPSX_SET_LE_D16(p, d)
 * @brief 16-bitパック変数dをのプラットフォーム固有エンディアン形式からリトルエンディアン形式に変換してポインタpに展開します
 */
#define MPSX_SET_LE_D16(p, d) mpsx_set_le_d16( p, d)

static __inline uint16_t mpsx_get_be_d16(const void * const p)
{
    uint16_t u16;

#if defined(QNX)
    u16 = ENDIAN_BE16(*(uint16_t*)p);
#elif defined(WIN32) && !defined(__MINGW32__)
    u16 = (uint16_t)_byteswap_ushort(*(unsigned short*)p);
#else /* simple */
#ifdef __BIG_ENDIAN__
    u16 = *(uint16_t*)p;
#else
    u16 = (uint16_t)(((*((uint8_t*)p +  0) & 0xff) << 8) | 
	             ((*((uint8_t*)p +  1) & 0xff)     )  );
#endif
#endif
    return u16;
}

/**
 * @def MPSX_GET_BE_D16(p)
 * @brief ポインタpに示された16-bitパックのビックエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_BE_D16(p) mpsx_get_be_d16(p)

static __inline uint16_t mpsx_get_le_d16(const void * const p)
{
    uint16_t u16;
#if defined(QNX)
    u16 = ENDIAN_LE16(*(uint16_t*)p);
#else /* simple */
#ifdef __BIG_ENDIAN__
    u16 = (uint16_t)(((*((uint8_t*)(p) +  1) & 0xff) << 8) |
                     ((*((uint8_t*)(p) +  0) & 0xff)     )  );
#else
    u16 = *(uint16_t*)(p);
#endif
#endif
    return u16;
}

/**
 * @def MPSX_GET_LE_D16(p)
 * @brief ポインタpに示された16-bitパックのリトルエンディアン値をプラットフォーム固有の形式に展開して返します。
 */
#define MPSX_GET_LE_D16(p) mpsx_get_le_d16(p)

#endif /* end of INC_MPSX_ENDIAN */
