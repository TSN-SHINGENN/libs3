/**
 *      Copyright 2011 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2011-january-01 Active
 *              Last Alteration $Author: takeda $
 **/

/**
 * @file mpxx_malloc.c
 * @brief 1.アライメンを考慮したmallocのmpxx版
 *	OS毎にメモリ管理が違うので、解放には必ず専用関数を使ってください。
 *	2.MMUの無いCPU用でもlibmpxxを使用できるようにします。
 *　	これはmpxx_lite_mallocaterを使用し、STATICバッファを擬似的にHEAPにします。
 *	標準APIによるメモリ消費を軽減します。
 *	_LIBMPXX_USE_LITE_MALLOCATER定義によりmpxx_lite_mallocaterに切り替わります。
 *	切り替えた場合はmpxx_lite_mallocater_init()で必ず初期化してください。
 **/

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#ifdef WIN32
#include <malloc.h>
#endif

#if defined(_LIBMPXX_USE_LITE_MALLOCATER)
#include "mpxx_lite_mallocater.h"
#endif
#include "mpxx_stdlib.h"
#include "mpxx_string.h"
#include "mpxx_malloc.h"

#ifdef DEBUG
static int debuglevel = 2;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MPXX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/**
 * @fn int mpxx_malloc_align(void **memptr, const size_t alignment, const size_t size)
 * @brief アライメントを考慮したメモリ割り当てを行います。
 *	確保したメモリの解放にはmpxx_mfree()を使ってください
 * @param memptr 割り当てられたメモリを格納するメモリブロックポインタ
 * @param alignment アライメント(配置)の値。2 の累乗値を指定する必要があります。 
 *	もし0が指定された場合にはsizeof(int)が設定されます
 * @param size メモリブロックのサイズ
 * @retval 0 成功
 * @retval EINVAL 引数エラー
 * @retval ENOMEM メモリ確保失敗
 * @retval -1 原因不明のエラー(コードが知らないだけかも?)
 **/
int mpxx_malloc_align(void **memptr, const size_t alignment,
			 const size_t size)
{
    int result;
    const size_t a = (alignment == 0) ? sizeof(uint64_t) : alignment;

#if defined(Linux) || defined(QNX)	/* POSIX system */
    DBMS5(stderr, "mpxx_malloc_align[POSIX] : execute" EOL_CRLF);
    result = posix_memalign(memptr, a, size);
#elif defined(WIN32) && !defined(__MINGW32__)
    DBMS5(stderr, "mpxx_malloc_align[WIN32] : execute" EOL_CRLF);
    *memptr = _aligned_malloc(size, a);
    if (NULL == *memptr) {
	switch (errno) {
	case EINVAL:
	    result = EINVAL;
	    break;
	case ENOMEM:
	    result = ENOMEM;
	    break;
	default:
	    result = -1;
	}
    } else {
	result = 0;
    }
#else				/* defined(Darwin) || defined(__MINGW32__) other CRuntimeLibrary */
    void *mem = NULL;
    uint8_t *aligned_mem = NULL;

    DBMS5(stderr, "mpxx_malloc_align[Darwin/MINGW/others] : execute" EOL_CRLF);
    if ((a % 2) || (size == 0) || (NULL == memptr)) {
	result = errno = EINVAL;
    } else {
	const size_t total = size + sizeof(void *) + (a - 1);
	mem = malloc(total);
	if (NULL == mem) {
	    result = errno;
	} else {
	    uintptr_t offs;

	    IFDBG5THEN {
		const size_t xtoa_bufsz = MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
		char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
		char xtoa_buf_c[MPXX_XTOA_DEC64_BUFSZ];
		char xtoa_buf_d[MPXX_XTOA_DEC64_BUFSZ];
		mpxx_u64toahex((uintptr_t)mem, xtoa_buf_a, xtoa_bufsz, NULL);
		mpxx_u64toadec((uint64_t)total, xtoa_buf_b, xtoa_bufsz);
		mpxx_u64toadec((uint64_t)alignment, xtoa_buf_c, xtoa_bufsz);
		mpxx_u64toadec((uint64_t)size, xtoa_buf_d, xtoa_bufsz);
		DBMS5(stderr,
		  "mpxx_malloc_align : malloc = 0x%s total=%s  alignment=%s size=%s" EOL_CRLF,
		    xtoa_buf_a, xtoa_buf_b, xtoa_buf_c, xtoa_buf_d);
	    }

	    /**
	     * @note 確保されたメモリのアライメントが整ったアドレスを計算する
	     */
	    aligned_mem = ((uint8_t *) mem + sizeof(void *));
	    offs = (uintptr_t) a - ((uintptr_t) aligned_mem & (a - 1));

	    IFDBG5THEN {
		const size_t xtoa_bufsz = MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
		char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];
		mpxx_u64toahex((uintptr_t)a-1, xtoa_buf_a, xtoa_bufsz, NULL);
		mpxx_u64toahex((uint64_t)offs, xtoa_buf_b, xtoa_bufsz, NULL);
		DBMS5(stderr,
		  "mpxx_malloc_align : a-1=0x%s offs=0x%s" EOL_CRLF,
		    xtoa_buf_a, xtoa_buf_b);
	    }

	    aligned_mem += offs;

	    IFDBG5THEN {
		const size_t xtoa_bufsz = MPXX_XTOA_HEX64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_HEX64_BUFSZ];
		mpxx_u64toahex((uintptr_t)aligned_mem, xtoa_buf, xtoa_bufsz, NULL);
		DBMS5(stderr, "mpxx_malloc_align : aligned_mem = 0x%s" EOL_CRLF, xtoa_buf);
	    }

	    /* 返すアドレスの一つ前に本来mallocで返されたポインタアドレスを保存しておく */
	    ((void **) aligned_mem)[-1] = mem;

	    *memptr = aligned_mem;
	    result = 0;
	}
    }
#endif

    return result;
}

/**
 * @fn int mpxx_realloc_align(void *memblk, const size_t alignment, const size_t size)
 * @brief アライメントを考慮したメモリの再割り当てを行います。
 * @param memblk 現在のメモリブロックポインタ
 * @param size 割り当てするメモリのサイズ
 * @param alignment アライメントサイズ。2の累乗値で最大ページサイズ
 * @retval NULL 失敗
 * @retval NULL以外 再割り当てされたポインタ
 */
void *mpxx_realloc_align(void *memblk, const size_t alignment,
			    const size_t size)
{
#if defined(WIN32) && !defined(__MINGW32__)
    size_t a = (alignment == 0) ? sizeof(uint64_t) : alignment;
    return _aligned_realloc(memblk, size, a);
#else /* defined(Darwin) || defined(XILKERNEL),posix and other CRuntimeLibrary */
    int result;
    void *mem;
    result = mpxx_malloc_align(&mem, alignment, size);
    if (result) {
	return NULL;
    }
    mpxx_memcpy(mem, memblk, size);
    mpxx_mfree(memblk);
    return mem;
#endif
}

/**
 * @fn int mpxx_mfree(void *memptr)
 * @brief mpxx_malloc_align()で確保したメモリを解放します
 *   OS毎に mpxx_malloc_align()の挙動が違うので Cランタイムのfree()を使わないでください。
 * @param memptr メモリブロックポインタ
 * @retval 0 成功
 * @retval -1 このOSではサポートされていません
 */
int mpxx_mfree(void *memptr)
{
#if defined(Linux) || defined(QNX)
    free(memptr);
    return 0;
#elif defined(WIN32) && !defined(__MINGW32__)
    _aligned_free(memptr);
    return 0;
#else /* defined(Darwin) || defined(XILKERNEL) other CRuntimeLibrary */
    /**
     * @note 保存していたポインタをfreeに渡してメモリ解放 
     */
    mpxx_free(((void **) memptr)[-1]);
    return 0;
#endif
}

/**
 * @fn void *mpxx_mrealloc_align( void *memblock, const size_t alignment, const size_t size)
 * @brief メモリの再割り当てを行います。
 *	差異割り当てされたメモリへのデータコピーは行われません(C1xで未定義と決まったため）
 * @param memblock 現在メモリのブロックポインた
 * @param alignment アライメント(配置)の値。2 の累乗値を指定する必要があります。 
 *	もし0が指定された場合にはsizeof(uint64_t)が設定されます
 * @param size メモリブロックのサイズ
 * @retval NULL 失敗(errno参照)
 * @retval NULL以外 新しいメモリブロックポインタ
 */
void *mpxx_mrealloc_align(void *memblock, const size_t alignment,
			     const size_t size)
{
    size_t a = (alignment == 0) ? sizeof(uint64_t) : alignment;

#if defined(WIN32) && !defined(__MINGW32__)
    DBMS5(stderr, "mpxx_mrealloc_align[WIN32] : execute" EOL_CRLF);
    return _aligned_realloc(memblock, size, a);
#elif defined(Linux) || defined(QNX)	/* POSIX system */
    DBMS5(stderr, "mpxx_mrealloc_align[POSIX] : execute" EOL_CRLF);
    int result;
    void *memptr;
    if (NULL == memblock) {
	result = posix_memalign(&memptr, a, size);
    } else {
	result = posix_memalign(&memptr, a, size);
	if (!result) {
	    free(memblock);
	}
    }
    return (result == 0) ? memptr : NULL;
#else				/* defined(Darwin) || other CRuntimeLibrary */
    DBMS5(stderr, "mpxx_mrealloc_align[Darwin/Other] : execute" EOL_CRLF);
    int result;
    void *mem = NULL;
    uint8_t *aligned_mem = NULL;
    if (NULL == memblock) {
	void *memptr = NULL;
	result = mpxx_malloc_align(&memptr, a, size);
	return (result == 0) ? memptr : NULL;
    } else if ((a % 2) || (size == 0)) {
	errno = EINVAL;
	return NULL;
    } else {
	mem =
	    mpxx_realloc(((void **) memblock)[-1],
		    size + (a - 1) + sizeof(void *));
	if (NULL == mem) {
	    return NULL;
	} else {
	    aligned_mem = ((uint8_t *) mem + sizeof(void *));
	    aligned_mem += ((uintptr_t) aligned_mem & (a - 1));
	    ((void **) aligned_mem)[-1] = mem;
	}
    }
    return aligned_mem;
#endif
}

/**
 * @fn mpxx_mem_alignment_partition_t mpxx_mem_alignment_partition(int *result_p, const void *p, size_t size, size_t alignment)
 * @brief 指定されたメモリ空間のアライメントを確認し、前方・中間・後方に分割します
 * @param result_p 結果返却用変数ポインタ(0:成功 0以外:失敗)
 * @param p メモリの先頭ポインタ
 * @param size メモリのサイズ
 * @param alignment アライメントあわせの為のサイズ
 * @return mpxx_mem_alignment_partition_t構造体
 */
mpxx_mem_alignment_partition_t mpxx_mem_alignment_partition(int
								  *const
								  result_p,
								  const
								  void
								  *const p,
								  size_t
								  size,
								  size_t
								  alignment)
{
    mpxx_mem_alignment_partition_t part;
    uintptr_t a = (uintptr_t) p;
    const size_t align_mask = (alignment - 1);

    memset(&part, 0x0, sizeof(mpxx_mem_alignment_partition_t));
    part.total_sz = size;
    part.alignment_sz = alignment;

    if (size < alignment) {
	part.foward_sz = size;
    } else {
	part.ext.f.size_is_multiple_alignment =
	    (size & align_mask) ? 0 : 1;
	part.foward_sz = (a & align_mask);
	if (!part.foward_sz) {
	    part.ext.f.is_aligned = 1;
	} else {
	    part.foward_sz = alignment - part.foward_sz;
	    size -= part.foward_sz;
	    part.ext.f.is_aligned = 0;
	}

	if (size != 0) {
	    part.bottom_sz = (size & align_mask);
	    part.middle_sz = size - part.bottom_sz;
	}
    }

    if (NULL != result_p) {
	*result_p = 0;
    }

    return part;
}

/**
 * @fn void *mpxx_malloc(size_t size);
 * @breif malloc()のラッパーです。
 *	size バイトを割り当て、 割り当てられたメモリーに対する ポインターを返す。
 *	メモリーの内容は初期化されない。 size が 0 の場合、NULL)を返す。 
 * @param size 割り当ててほしいメモリバイトサイズ
 * @retval NULL 割り当て失敗。errnoにENOMEMを設定する
 **/
void *mpxx_malloc(const size_t size)
{
#if defined(_LIBMPXX_USE_LITE_MALLOCATER)
    DBMS5( stderr, "mpxx_malloc[LITE_MALLOC] : execute" EOL_CRLF);
    return mpxx_lite_mallocater_alloc(size);
#else
    DBMS5( stderr, "mpxx_malloc[STD] : execute" EOL_CRLF);
    return malloc(size);
#endif
}

/**
 * @fn void mpxx_free( void *const ptr)
 * @brief free()のラッパーです。mpxx_malloc()で取得したメモリを開放します。
 * @param ptr mpxx_malloc()で取得したメモリのポインタを指定。NULLを指定すると。何も処理せず抜けます。
 **/
void mpxx_free( void *const ptr)
{
#if defined(_LIBMPXX_USE_LITE_MALLOCATER)
    mpxx_lite_mallocater_free(ptr);
    return;
#else
    free(ptr);
    return;
#endif
}


/**
 * @fn void *mpxx_realloc( void *ptr, size_t size)
 * @breif realloc()のラッパーです。
 *	size バイトを再割り当てを行います。
 *	元のメモリエリアで処理できない場合は戻り値はptrと異なるポインタを戻します。
 *	このとき、内部のデータは保障されます。
 * @param size 割り当ててほしいメモリバイトサイズ
 * @retval NULL 割り当て失敗。errnoにENOMEMを設定する
 * @retval NULL以外 再割り当て成功。ptr指定と異なるポインタの場合はptrは開放済
 **/
void *mpxx_realloc( void *const ptr, const size_t size)
{
#if defined(_LIBMPXX_USE_LITE_MALLOCATER)
    return mpxx_lite_mallocater_realloc(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

