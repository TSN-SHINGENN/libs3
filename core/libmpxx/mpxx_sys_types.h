/**
 * @file mpxx_sys_types.h
 * @brief なるべくPOSIX定義にあわせるための型sys/types.h 定義ラッパー
 */
#ifndef MPXX_SYS_TYPES_H
#define MPXX_SYS_TYPES_H

#pragma once

#include <sys/types.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define MPXX_ATTRIBUTE_ALIGNED(x) __declspec(align(x))
#elif defined(__GNUC__)
#define MPXX_ATTRIBUTE_ALIGNED(x) __attribute__((aligned(x)))
#else
#error 'can not define MPXX_ATRIBUTE_ALIGNED'
#endif

/**
 * @note
 *   LIKELY ･･･ 答えが成立する場合を条件に最適化する
 * UNLIKELY ･･･ 答えが非成立する場合が高いことを条件に最適化する
 */
#if defined(_MSC_VER)
    /* VC++2010ではヒント命令がないので */
#define MPXX_HINT_LIKELY(x) (x)
#define MPXX_HINT_UNLIKELY(x) (x)
#elif defined(__GNUC__)
#if ( __GNUC__ > 2 ) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define MPXX_HINT_LIKELY(x) __builtin_expect(!!(int)(x), 1)
#define MPXX_HINT_UNLIKELY(x) __builtin_expect(!!(int)(x), 0)
#else
#define MPXX_HINT_LIKELY(x) (x)
#define MPXX_HINT_UNLIKELY(x) (x)
#endif
#else
#error 'can not define MPXX_HINT_LIKELY'
#endif

/**
 * @note キャストマクロ
 **/
#ifdef __cplusplus
#define MPXX_STATIC_CAST(t, a) (static_cast<t>(a))
#else
#define MPXX_STATIC_CAST(t, a) (t)(a)
#endif


/**
 * @ note __func__定義
 **/

#if !defined(__func__)
#define __func__ __FUNCTION__
#endif

/**
 * @note スワップ処理のマクロ定義
 **/

#define MPXX_SWAP(type,a,b) do { register const type temp = a; a = b; b = temp; } while(0)


#endif /* end of MPXX_SYS_TYPES_H */
