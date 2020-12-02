#ifndef INC_MPXX_MALLOC_H
#define INC_MPXX_MALLOC_H

#include <stddef.h>
#include <stdint.h>
#if defined(WIN32)
#include <malloc.h>
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(XILINX)
#include <alloca.h>
#endif

#pragma once

typedef struct _mpxx_mem_alignment_partition {
   size_t foward_sz; /* アライメントされていない前方サイズ */
   size_t middle_sz; /* アライメントされている中間サイズ   */
   size_t bottom_sz; /* アライメントされていない後方サイズ */
   size_t total_sz;  /* 総サイズ */
   size_t alignment_sz; /* 指定されたアライメントサイズ */
   union {
	unsigned int flags;
	struct {
	    unsigned int is_aligned:1;		/* ポインタ先頭がアライメントがととなっている場合を示す */
	    unsigned int size_is_multiple_alignment:1;  /* サイズはアライメントの倍数である場合を示す */
	} f;
   } ext;
} mpxx_mem_alignment_partition_t;

#ifdef __cplusplus
extern "C" {
#endif

int mpxx_malloc_align(void **memptr, const size_t alignment, const size_t size);
void *mpxx_realloc_align( void *memblk, const size_t algnment, const size_t size);
int mpxx_mfree(void *memptr);
void *mpxx_mrealloc_align( void *memblock, const size_t alignment, const size_t size);

mpxx_mem_alignment_partition_t mpxx_mem_alignment_partition(int * const resilt_p, const void * const p, size_t size, size_t alignment);



#ifdef __cplusplus
}
#endif

/**
 * @note スタックからメモリを割り当てる関数のラッパー定義です。
 * mpxx_malloca(s)
 *	スタックからメモリを割り当てる関数のマルチOSラッパー
 * 	WIN32はスタック・ヒープを切り替えて割り当てます
 * 	関数の終わりでmpxx_freea()を呼んでください
 * mpxx_freea(p)
 *	mpxx_malloca()で確保したメモリを開放します
 *	WIN32用の対策です
 */

#ifdef WIN32 /* mainly VC++ */
#if defined(_MSC_VER)
#if _MSC_VER >= 1400 /* VC++2005 */
#define mpxx_malloca(s) _malloca(s)
#define mpxx_freea(p) _freea(p)
#else /* old */
/**
 * @def mpxx_malloca(s)
 * @brief スタックからメモリを割り当てる関数のマルチOSラッパー
 * 	WIN32はスタック・ヒープを切り替えて割り当てます
 * 	関数の終わりでmpxx_freea()を呼んでください
 * @def mpxx_freea(p)
 * @brief mpxx_malloca()で確保したメモリを開放します
 *	WIN32用の対策です
 */
#define mpxx_malloca(s) _alloca(s)
#define mpxx_freea(p) (void)(p)
#endif
#elif defined(__AVM2__)  /* AVMには無いみたいなので */
#define mpxx_malloca(s) malloc(s)
#define mpxx_freea(p) free(p)
#elif defined(__MINGW32__)
/** gccと同じ */
#define mpxx_malloca(s) alloca(s)
#define mpxx_freea(p) (void)(p)
#else
#error 'need to research allocate of memory space in stack frame'
#endif
#elif __GNUC__ /* GCC */
#define mpxx_malloca(s) alloca(s)
#define mpxx_freea(p) (void)(p)
#else /* other */
#error 'need to research allocate of memory space in stack frame'
#endif

#if defined(__cplusplus )
extern "C" {
#endif

void *mpxx_malloc(const size_t size);
void mpxx_free( void *const ptr);
void *mpxx_realloc( void *const ptr, const size_t size);

#if defined(__cplusplus )
}
#endif

#endif /* end of INC_MPXX_MALLOC_H */
