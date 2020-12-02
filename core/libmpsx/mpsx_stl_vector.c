/**
 *      Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2012-November-17 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file multios_stl_vector.c
 * @brief 配列ライブラリ STLのvectorクラス互換です。
 *      なのでスレッドセーフではありません。上位層で処理を行ってください。
 *      STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 */

#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4819 )
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/* POSIX */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_malloc.h"
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

/* this */
#include "mpsx_stl_vector.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef void *(*mrealloc_func_t)(void *,size_t );
typedef void (*mfree_func_t)(void *);

typedef union _stl_vector_stat {
    unsigned int flags;
    struct {
	unsigned int mem_fixed:1; /* mem_fixedを使った際の制限 */
    } f;
} stl_vector_stat_t;

typedef struct _stl_vector_ext {
    void *buf;
    size_t reserved_bytes;
    size_t num_elements;
    size_t sizof_element;

    mpsx_stl_vector_attr_t attr;
    mrealloc_func_t mrealloc_func;
    mfree_func_t mfree_func;
    stl_vector_stat_t stat;
} stl_vector_ext_t;

#define get_vector_ext(s) MPSX_STATIC_CAST(stl_vector_ext_t*,(s)->ext)
#define get_const_vector_ext(s) MPSX_STATIC_CAST(const stl_vector_ext_t*,(s)->ext)


#if defined(__GNUC__) && defined(_LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )
static void *mrealloc_std( void *const, size_t)
		__attribute__ ((optimize("Os")));
static void mfree_std( void *const)
		__attribute__ ((optimize("Os")));
static void *mrealloc_aligned16( void *const, const size_t)
		__attribute__ ((optimize("Os")));
static void *mrealloc_aligned32( void *const, const size_t)
		__attribute__ ((optimize("Os")));
static void mfree_aligned(void *const)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_init( mpsx_stl_vector_t *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_init_ex( mpsx_stl_vector_t *const, const size_t, mpsx_stl_vector_attr_t *const)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_attach_memory_fixed( mpsx_stl_vector_t *const, void *const, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_destroy( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_push_back( mpsx_stl_vector_t *const, const void *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_pop_back( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_resize( mpsx_stl_vector_t *const, const size_t, const void *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_reserve( mpsx_stl_vector_t *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_clear( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));

extern size_t mpsx_stl_vector_capacity( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));
extern void *mpsx_stl_vector_ptr_at( mpsx_stl_vector_t *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_is_empty( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));

extern size_t mpsx_stl_vector_size( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));
extern size_t mpsx_stl_vector_get_pool_cnt( mpsx_stl_vector_t *const)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_front( mpsx_stl_vector_t *const, void *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_back( mpsx_stl_vector_t *const, void *const, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_insert( mpsx_stl_vector_t *const, const size_t, const void *const, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_remove_at( mpsx_stl_vector_t *const, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_get_element_at( mpsx_stl_vector_t *const, const size_t, void *const, const size_t)
		__attribute__ ((optimize("Os")));
extern int mpsx_stl_vector_overwrite_element_at( mpsx_stl_vector_t *const, const size_t, const void *const, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_element_swap_at( mpsx_stl_vector_t *const, const size_t, const size_t)
		__attribute__ ((optimize("Os")));

extern int mpsx_stl_vector_shrink( mpsx_stl_vector_t *const, const size_t)
		__attribute__ ((optimize("Os")));

#else /* Normal */
static void *mrealloc_std( void *const ptr, size_t size);
static void mfree_std( void *const ptr );
static void *mrealloc_aligned16( void *const ptr, const size_t size);
static void *mrealloc_aligned32( void *const ptr, const size_t size);
static void mfree_aligned(void *const ptr );
#endif /* end of _LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

static void *mrealloc_std( void *const ptr, size_t size)
{
    return mpsx_realloc( ptr, size);
}

static void mfree_std( void *const ptr )
{
    mpsx_free(ptr);
    return;
}

static void *mrealloc_aligned16( void *const ptr, const size_t size)
{
    return mpsx_realloc_align( ptr, 16, size);
}

static void *mrealloc_aligned32( void *const ptr, const size_t size)
{
    return mpsx_realloc_align( ptr, 32, size);
}

static void mfree_aligned(void *const ptr )
{
    mpsx_mfree(ptr);
    return;
}

/**
 * @fn int mpsx_stl_vector_init( mpsx_stl_vector_t *const self_p, const size_t sizof_element)
 * @brief vectorオブジェクトを初期化します。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EAGAIN リソース獲得に失敗
 * @retval -1 エラー定義以外の致命的な失敗
 */
int mpsx_stl_vector_init( mpsx_stl_vector_t *const self_p, const size_t sizof_element)
{
    return mpsx_stl_vector_init_ex(self_p, sizof_element, NULL);
}

/**
 * @fn int mpsx_stl_vector_init_ex( mpsx_stl_vector_t *const self_p, const size_t sizof_element, mpsx_stl_vector_attr_t *const attr_p)
 * @brief vectorオブジェクトを初期化します。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param sizof_element 要素サイズ
 * @param attr_pi mpsx_stl_vector_attr_t構造体ポインタ設定値
 * @retval 0 成功
 * @retval EAGAIN リソース獲得に失敗
 * @retval EINVAL 引数が不正
 * @retval -1 エラー定義以外の致命的な失敗
 */
int mpsx_stl_vector_init_ex( mpsx_stl_vector_t *const self_p, const size_t sizof_element, mpsx_stl_vector_attr_t *const attr_p)
{
    int status;
    stl_vector_ext_t *e = NULL;
    memset(self_p, 0x0, sizeof(mpsx_stl_vector_t));

    e = (stl_vector_ext_t *)
	mpsx_malloc(sizeof(stl_vector_ext_t));
    if (NULL == e) {
	DBMS1(stderr, "mpsx_stl_vector_init_ex : mpsx_malloc(ext) fail" EOL_CRLF);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(stl_vector_ext_t));

    if(NULL != attr_p) {
	e->attr = *attr_p;
    }

    if( e->attr.f.is_aligned16 ) {
	e->mrealloc_func = mrealloc_aligned16;
	e->mfree_func = mfree_aligned;
    } else if( e->attr.f.is_aligned32) {
	e->mrealloc_func = mrealloc_aligned32;
	e->mfree_func = mfree_aligned;
    } else if( !e->attr.f.is_aligned16 && !e->attr.f.is_aligned32 ) {
	e->mrealloc_func = mrealloc_std;
	e->mfree_func = mfree_std;
    } else {
	status = EINVAL;
	goto out;
    }

    e->buf = NULL;
    e->reserved_bytes = 0;
    e->num_elements = 0;
    self_p->sizof_element = e->sizof_element = sizof_element;

    self_p->ext = e;
    status = 0;

out:

    return status;
}

/**
 * @fn int mpsx_stl_vector_destroy(mpsx_stl_vector_t *const self_p)
 * @brief vectorオブジェクトを破棄します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @return 0固定
 */
int mpsx_stl_vector_destroy(mpsx_stl_vector_t *const self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == e ) {
	return 0;
    }

    if (NULL != e->buf) {
	e->mfree_func(e->buf);
	e->buf = NULL;
    }

    mpsx_free(self_p->ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn static int resize_buffer( stl_vector_ext_t *const e, const size_t num_elements, const void *const el_p)
 * @brief vectorバッファを可変します。拡張時はel_p(NULL以外)に設定された要素で埋めることができます
 * @param e stl_vector_ext_t構造体ポインタ
 * @param num_elements 要素格納数
 * @param el_p 拡張時に設定する要素データポインタ(NULLで設定しない)
 * @retval 0 成功
 * @retval EAGAIN リソース不足　
 */
static int resize_buffer(stl_vector_ext_t *const e,
			 const size_t num_elements, const void *const el_p)
{
    const size_t new_reserve = e->sizof_element * num_elements;
    void *new_buf = NULL;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPSX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPSX_XTOA_DEC64_BUFSZ];

	mpsx_u64toadec( (uint64_t)e->reserved_bytes, xtoa_buf_a, xtoa_bufsz);
	mpsx_u64toadec( (uint64_t)new_reserve, xtoa_buf_b, xtoa_bufsz);
	DBMS3(stderr, "mpsx_stl_vector:resize_buffer: execute" EOL_CRLF);
        DBMS3(stderr,
	  "mpsx_stl_vector:resize_buffer: reserved=%s new_reserved=%s" EOL_CRLF,
	      xtoa_buf_a, xtoa_buf_b);
    }

    if (e->reserved_bytes < new_reserve) {
	new_buf = e->mrealloc_func(e->buf, new_reserve);
	if (NULL == new_buf) {
	    return EAGAIN;
	}
	e->buf = new_buf;
	e->reserved_bytes = new_reserve;
    }

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MPSX_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MPSX_XTOA_DEC64_BUFSZ];
	mpsx_u64toadec( (uint64_t)e->num_elements, xtoa_buf_a, xtoa_bufsz);
	mpsx_u64toadec( (uint64_t)num_elements, xtoa_buf_b, xtoa_bufsz);
	DBMS3(stderr,
	  "mpsx_stl_vector:resize_buffer e->num_elements=%s num_elements=%s" EOL_CRLF,
	  xtoa_buf_a, xtoa_buf_b);
    }

    if (e->num_elements < num_elements) {
	/* expand */
	if (NULL != el_p) {
	    size_t st_pos = e->num_elements;
	    size_t end_pos = num_elements;
	    size_t n;
	    if (e->sizof_element > 1) {
		IFDBG3THEN {
		    const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
		    char xtoa_buf_a[MPSX_XTOA_DEC64_BUFSZ];
		    char xtoa_buf_b[MPSX_XTOA_DEC64_BUFSZ];
		    char xtoa_buf_c[MPSX_XTOA_DEC64_BUFSZ];
		    mpsx_u64toadec( (uint64_t)st_pos, xtoa_buf_a, xtoa_bufsz);
		    mpsx_u64toadec( (uint64_t)end_pos, xtoa_buf_b, xtoa_bufsz);
		    mpsx_u64toadec( (uint64_t)e->sizof_element, xtoa_buf_c, xtoa_bufsz);
		    DBMS3(stderr,
		      "mpsx_stl_vector:resize_buffer st=%s en=%s sizof_element=%s" EOL_CRLF,
		      xtoa_buf_a, xtoa_buf_b, xtoa_buf_c);
		}
		for (n = st_pos; n < end_pos; n++) {
		    void *at =
			(uint8_t *) e->buf + (n * e->sizof_element);
		    mpsx_memcpy(at, el_p, e->sizof_element);
		}
		e->num_elements = num_elements;
	    } else {
		void *p =
		    (uint8_t *) e->buf + (st_pos * e->sizof_element);
		size_t dur = end_pos - st_pos;

		IFDBG3THEN {
		    const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
		    char xtoa_buf_a[MPSX_XTOA_DEC64_BUFSZ];
		    char xtoa_buf_b[MPSX_XTOA_DEC64_BUFSZ];
		    char xtoa_buf_c[MPSX_XTOA_DEC64_BUFSZ];

		    mpsx_u64toadec( (uint64_t)st_pos, xtoa_buf_a, xtoa_bufsz);
		    mpsx_u64toadec( (uint64_t)dur, xtoa_buf_b, xtoa_bufsz);
		    mpsx_u64toadec( (uint64_t) *(uint8_t *) el_p, xtoa_buf_c, xtoa_bufsz);
		    DBMS3(stderr,
		      "mpsx_stl_vector:resize_buffer(Byte) st=%s dur=%s d=%s" EOL_CRLF,
		      xtoa_buf_a, xtoa_buf_b, xtoa_buf_c);
		}
		memset(p, *(uint8_t *) el_p, dur);
		e->num_elements = num_elements;
	    }
	}
    } else {
	e->num_elements = num_elements;
    }

    return 0;
}

/**
 * @fn int mpsx_stl_vector_push_back( mpsx_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief 末尾に要素を追加します
 * @param self_p multios_stl_vector_t構造体インスタンスポインタ
 * @param el_p 追加する要素のポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EAGAIN リソース不足
 * @retval EINVAL 引数不正
 */
int mpsx_stl_vector_push_back(mpsx_stl_vector_t *const self_p,
				 const void *const el_p,
				 const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;

    DBMS3(stderr, "mpsx_stl_vector_push_back : execute" EOL_CRLF);

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }
    if (NULL == el_p) {
	return EINVAL;
    }

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPSX_XTOA_DEC64_BUFSZ];

	mpsx_u64toadec( (uint64_t)e->num_elements, xtoa_buf, xtoa_bufsz);
	DBMS3(stderr,
	  "mpsx_stl_vector_push_back : e->num_elements=%s, add" EOL_CRLF,
	    xtoa_buf);
    }

    result = resize_buffer(e, e->num_elements + 1, el_p);
    if (result) {
	return result;
    }

    return 0;
}

/**
 * @fn int mpsx_stl_vector_pop_back( mpsx_stl_vector_t *const self_p)
 * @brief 末尾の要素を削除します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @retval 0 成功
 * @retval ENOENT 削除する要素が存在しない
 */
int mpsx_stl_vector_pop_back(mpsx_stl_vector_t *const self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements == 0) {
	return ENOENT;
    }

    --(e->num_elements);

    return 0;
}

/**
 * @fn int mpsx_stl_vector_resize( mpsx_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element)
 * @brief 要素を後方から補充・削除します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num_elements 変更する総要素数
 * @param el_p 補充する要素データ（削除の場合はNULL指定）
 * @param sizof_element 要素サイズ（削除の場合は0指定)
 * @retval 0 成功
 * @retval EINVAL 不正な引数（オブジェクトや要素サイズ）
 * @retval EAGAIN リソース不足
 * @retval EPERM 関数の実行に許可がない
 */
int mpsx_stl_vector_resize(mpsx_stl_vector_t *const self_p,
			      const size_t num_elements, const void *const el_p,
			      const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } if (e->num_elements == num_elements) {
	return 0;
    } else if (e->num_elements < num_elements) {
	/* extpand */
	if (sizof_element != e->sizof_element) {
	    return EINVAL;
	}
    }
    result = resize_buffer(e, num_elements, el_p);
    if (result) {
	return result;
    }

    return 0;
}

/**
 * @fn int mpsx_stl_vector_reserve( mpsx_stl_vector_t *const self_p, const size_t num_elements);
 * @brief メモリの予約サイズを増やします
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num_elements メモリ割当済み総エレメント数
 * @retval 0 成功
 * @retval EINVAL 不正な引数（オブジェクトや要素サイズ）
 * @retval EBUSY 要素が使用中である
 * @retval EAGAIN リソースが獲得できなかった
 */
int mpsx_stl_vector_reserve(mpsx_stl_vector_t *const self_p,
			       const size_t num_elements)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (e->num_elements >= num_elements) {
	return EBUSY;
    }

    return resize_buffer(e, num_elements, NULL);
}

/**
 * @fn size_t mpsx_stl_vector_capacity( mpsx_stl_vector_t *const self_p)
 * @brief メモリの確保済みサイズを得る
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @return  確保（予約済み）要素数
 */
size_t mpsx_stl_vector_capacity(mpsx_stl_vector_t *const self_p)
{
    const stl_vector_ext_t *const e =
	get_const_vector_ext(self_p);

    return e->reserved_bytes / e->sizof_element;
}

/**
 * @fn void *mpsx_stl_vector_ptr_at(mpsx_stl_vector_t *const self_p, const size_t num)
 * @brief 要素の割当ポインタを得る
 *	※ capacityサイズを超えてコンテナが収容されるとポインタアドレスが変更されることがあります。
 *	スレッドセーフではないので、上位で排他制御してください。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num 割当の要素番号
 * @retval NULL 不正な要素番号
 * @retval NULL以外 要素のポインタ
 */
void *mpsx_stl_vector_ptr_at(mpsx_stl_vector_t *const self_p, const size_t num)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements > num) {
	return (void *) ((uint8_t *) e->buf + (num * e->sizof_element));
    }

    return NULL;
}

/**
 * @fn int mpsx_stl_vector_is_empty(mpsx_stl_vector_t *const self_p)
 * @brief コンテナに要素が空かどうかを返す
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @retval 0以外 空
 * @retval 0 要素あり
 */
int mpsx_stl_vector_is_empty(mpsx_stl_vector_t *const self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if (e->num_elements == 0) {
	return 1;
    }

    return 0;
}

/**
 * @fn size_t mpsx_stl_vector_size(mpsx_stl_vector_t *const  self_p)
 * @brief コンテナに入っている要素数を得る
 * @param self_p multios_stl_vector_t構造体インスタンスポインタ
 * @return 要素数
 */
size_t mpsx_stl_vector_size(mpsx_stl_vector_t *const  self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    return e->num_elements;
}

/**
 * @fn int mpsx_stl_vector_clear(mpsx_stl_vector_t *const self_p)
 * @brief コンテナの要素および内部バッファを削除します。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @retval 0 成功
 */
int mpsx_stl_vector_clear(mpsx_stl_vector_t *const self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if(NULL == self_p) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	e->num_elements = 0;
	return 0;
    }

    if (NULL != e->buf) {
	mpsx_free(e->buf);
	e->buf = NULL;
    }
    e->num_elements = 0;
    e->reserved_bytes = 0;

    return 0;
}


/**
 * @fn size_t mpsx_stl_vector_get_pool_cnt( mpsx_stl_vector_t *const self_p)
 * @brief 内包しているエレメント数を返します．push_backで保存されたエレメントに限ります。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @return 0以上 要素数
 */
size_t mpsx_stl_vector_get_pool_cnt( mpsx_stl_vector_t *const self_p)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    return e->num_elements;
}

/**
 * @fn int mpsx_stl_vector_front( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 先頭に保存されたエレメントを返します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 **/
int mpsx_stl_vector_front( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( e->num_elements == 0 ) {
	return ENOENT;
    } else if(e->sizof_element != sizof_element) {
	return EINVAL;
    }
    mpsx_memcpy( el_p, e->buf, e->sizof_element);

    return 0;
}

/**
 * @fn int mpsx_stl_vector_back( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 最後に保存されたエレメントを返します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 **/
int mpsx_stl_vector_back( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( e->num_elements == 0 ) {
	return ENOENT;
    } else if(e->sizof_element != sizof_element) {
	return EINVAL;
    }
    mpsx_memcpy( el_p, (uint8_t*)e->buf + (e->sizof_element * (e->num_elements -1)), e->sizof_element);

    return 0;
}

/**
 * @fn int mpsx_stl_vector_insert( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief 要素を挿入します。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num 要素番号
 * @param el_p 挿入する要素のデータバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 不正な要素番号
 * @retval EINVAL 関数実行の不許可
 */ 
int mpsx_stl_vector_insert( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;
    uint8_t *ptr;
    size_t move_size;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if( sizof_element != e->sizof_element) {
	return EINVAL;
    } else if (!(e->num_elements >= num)) {
	return ENOENT;
    }

    result = resize_buffer(e, e->num_elements + 1, NULL);
    if(result) {
	DBMS1( stderr, "mpsx_stl_vector_insert : resize_buffer fail, strerror:%s" EOL_CRLF, strerror(result));
	return result;
    }

    ptr = ((uint8_t *) e->buf + (num * e->sizof_element));
    move_size = (e->num_elements - num) * e->sizof_element;

    if( move_size ) {
	memmove( ptr + e->sizof_element, ptr, move_size);
    }
    mpsx_memcpy( ptr, el_p, e->sizof_element);
    ++(e->num_elements);

    return 0;
}


/**
 * @fn int mpsx_stl_vector_remove_at( mpsx_stl_vector_t *const self_p, const size_t num)
 * @brief 要素を消去します。
 * 	*** 検証中です ***
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num 要素番号
 * @retval ENOENT 不正な要素番号
 * @retval EINVAL 関数実行の不許可
 */ 
int mpsx_stl_vector_remove_at( mpsx_stl_vector_t *const self_p, const size_t num)
{

    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    int result;
    uint8_t *ptr;
    size_t move_size;

    if(NULL == self_p) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (!(e->num_elements > num)) {
	return ENOENT;
    }

    ptr = ((uint8_t *) e->buf + (num * e->sizof_element));
    move_size = (e->num_elements - (num + 1)) * e->sizof_element;

    result = resize_buffer(e, e->num_elements -1, NULL);
    if(result) {
	DBMS1( stderr, "mpsx_stl_vector_insert : resize_buffer fail, strerror:%s" EOL_CRLF, strerror(result));
	return result;
    }

    if( move_size != 0 ) {
	memmove( ptr, ptr + e->sizof_element, move_size);
    }

    return 0;
}

/**
 * @fn int mpsx_stl_vector_get_element_at( mpsx_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
 * @brief 保存された要素を返します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EINVAL 不正な引数 
 **/
int mpsx_stl_vector_get_element_at( mpsx_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    const uint8_t *const ptr = MPSX_STATIC_CAST(const uint8_t*,mpsx_stl_vector_ptr_at(self_p, num));
  
    if((NULL == self_p) || (NULL == ptr) || (NULL == el_p)) {
   	return EINVAL;
    }

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }

    mpsx_memcpy( el_p, ptr, sizof_element);

    return 0;
}



/**
 * @fn int mpsx_stl_vector_overwrite_element_at( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief 保存された要素を返します
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EINVAL 不正な引数 
 **/
int mpsx_stl_vector_overwrite_element_at( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    uint8_t *const ptr = MPSX_STATIC_CAST(uint8_t*,mpsx_stl_vector_ptr_at(self_p, num));
  
    if((NULL == self_p) || (NULL == ptr) || (NULL == el_p)) {
   	return EINVAL;
    }

    if (sizof_element != e->sizof_element) {
	return EINVAL;
    }

    mpsx_memcpy( ptr, el_p, sizof_element);

    return 0;
}



/**
 * @fn int mpsx_stl_vector_element_swap_at( mpsx_stl_vector_t *const self_p, const size_t at1, const size_t at2)
 * @brief 指定された二つの要素を入れ替えます。
 * @param self_p multios_stl_vector_t構造体インスタンスポインタ
 * @param at1 要素1
 * @param at2 要素2
 * @retval 0 成功
 * @retval EINVAL 不正な要素番号
 * @retval EAGAIN リソースを獲得できなかった
 **/
int mpsx_stl_vector_element_swap_at( mpsx_stl_vector_t *const self_p, const size_t at1, const size_t at2)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    void *ptr1 = mpsx_stl_vector_ptr_at(self_p, at1);
    void *ptr2 = mpsx_stl_vector_ptr_at(self_p, at2);
    void *buf;

    if((NULL == self_p) || (NULL == ptr1) || (NULL == ptr2)) {
	return EINVAL;
    }

    buf = mpsx_malloc( e->sizof_element );
    if( NULL == buf ) {
	return EAGAIN;
    }
    mpsx_memcpy( buf, ptr1, e->sizof_element);
    mpsx_memcpy( ptr1, ptr2, e->sizof_element);
    mpsx_memcpy( ptr2, buf, e->sizof_element);

    if( NULL != buf ) {
	mpsx_free(buf);
    }

    return 0;
}


/**
 * @fn int mpsx_stl_vector_shrink( mpsx_stl_vector_t *const self_p, const size_t num_elements)
 * @brief メモリの予約サイズを縮小します。
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param num_elements メモリ割当済み総エレメント数
 * @retval 0 成功
 * @retval EINVAL 引数(self_p)が不正
 * @retval EBUSY 要素が使用中である
 * @retval EAGAIN リソースが獲得できなかった
 */
int mpsx_stl_vector_shrink( mpsx_stl_vector_t *const self_p, const size_t num_elements)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);
    const size_t new_reserve = e->sizof_element * num_elements;

    if( NULL == self_p ) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    } else if (e->num_elements > num_elements) {
	return EBUSY;
    }

    if( num_elements == 0 ) {
	if( NULL != e->buf ) {
	    e->mfree_func(e->buf);
	    e->buf = NULL;
	}
	e->reserved_bytes = 0;
    } else {
	void *new_buf = e->mrealloc_func(e->buf, new_reserve);
	if (NULL == new_buf) {
	    return EAGAIN;
	}
	e->buf = new_buf;
	e->reserved_bytes = new_reserve;
    }

    return 0;
}

/**
 * @fn int mpsx_stl_vector_attach_memory_fixed( mpsx_stl_vector_t *const self_p, void *const mem_ptr, const size_t len)
 * @brief エレメントを保存するエリアを固定します。指定されたメモリポインタの領域は、インスタンスが破棄されるまで使用されます。
 * 割り当てた後はインスタンスを破棄するまで指定された領域の中はいじらないでください。
 *
 * 次の関数はエラーになります。エリアの変更・拡張・縮小に関係するものです。
 *  EPERMをエラーコードで戻します。 
 * ・mpsx_stl_vector_attach_memory_fixed()
 * ・mpsx_stl_vector_resize()
 * ・mpsx_stl_vector_reserve()
 * ・mpsx_stl_vector_shrink()
 * 次の関数は成功しますが、指定されたメモリを開放することはしません。
 * ・mpsx_stl_vector_clear()
 * 
 * @param self_p mpsx_stl_vector_t構造体インスタンスポインタ
 * @param mem_ptr メモリ領域の開始ポインタ(仮想・物理エリア対応
 * @param len メモリエリアサイズ
 * @retval 0 成功
 * @retval EPERM すでに割り当て済み、エレメントが存在する
 * @retval EINVAL 引数のどれかが不正
 **/
int mpsx_stl_vector_attach_memory_fixed( mpsx_stl_vector_t *const self_p, void *const mem_ptr, const size_t len)
{
    stl_vector_ext_t *const e =
	get_vector_ext(self_p);

    if( NULL == self_p ) {
	return EINVAL;
    } else if( NULL != e->buf ) {
	return EPERM;
    } else if((NULL == mem_ptr) || !(e->sizof_element < len)) {
	return EINVAL;
    } else if( e->stat.f.mem_fixed ) {
	return EPERM;
    }

    e->buf = mem_ptr;
    e->reserved_bytes = len - (len % e->sizof_element);
    e->stat.f.mem_fixed = 1;

    // mbmcs_printf("mpsx_stl_vector_attach_memory_fixed : e->reserved_bytes = %u" EOL_CRLF, (unsigned int)e->reserved_bytes);

    return 0;
}
