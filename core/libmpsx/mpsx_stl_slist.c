/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2014-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_stl_slist.c
 * @brief 単方向待ち行列ライブラリ STLのslist(SGI)クラス互換です。
 *	なのでスレッドセーフではありません。上位層で処理を行ってください。
 *	STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* POSIX */
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_malloc.h"
#include "mpsx_string.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#include "mpsx_stl_slist.h"

typedef struct _fifoitem {
    struct _fifoitem *next;
    uint8_t data[0];  // c++では可変長配列が使えない
} fifoitem_t;

typedef struct _stl_slist_ext {
    fifoitem_t *r_p, *w_p;	/* カレント参照のポインタ */
    size_t sizof_element;
    size_t cnt;
    fifoitem_t base;		/* 配列0の構造体があるので必ず最後にする */
} stl_slist_ext_t;

#define get_stl_slist_ext(s) MPSX_STATIC_CAST(stl_slist_ext_t*,(s)->ext)
#define get_const_stl_slist_ext(s) MPSX_STATIC_CAST(const stl_slist_ext_t*,(s)->ext)

#if defined(_LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )
static fifoitem_t *slist_get_foward_element_ptr( const stl_slist_ext_t *const e, void * const element_ptr)
	__attribute__ ((optimize("Os")));

extern int mpsx_stl_slist_init( mpsx_stl_slist_t *const self_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_destroy( mpsx_stl_slist_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_push( mpsx_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_pop( mpsx_stl_slist_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_front( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element )
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_get_element_at( mpsx_stl_slist_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_clear( mpsx_stl_slist_t *const self_p)
	__attribute__ ((optimize("Os")));
extern size_t mpsx_stl_slist_get_pool_cnt( mpsx_stl_slist_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_is_empty( mpsx_stl_slist_t *const self_p)
	__attribute__ ((optimize("Os")));

extern int mpsx_stl_slist_back( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element )
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_slist_insert_at( mpsx_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element )
	__attribute__ ((optimize("Os")));
int mpsx_stl_slist_erase_at( mpsx_stl_slist_t *const self_p, const size_t no)
	__attribute__ ((optimize("Os")));

#else /* Normal */

static fifoitem_t *slist_get_foward_element_ptr( const stl_slist_ext_t *const e, void * const element_ptr);

#endif /* _LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */


/**
 * @fn static fifoitem_t *slist_get_foward_element_ptr( const stl_slist_ext_t *const e, void * const element_ptr)
 * @brief 指定されたエレメントバッファの前方のエレメントバッファのポインタを返します。
 *	外部関数でinsert, eraseを処理する為に使用します。
 * @param stl_slist_ext_t構造体ポインタ
 * @param element_ptr 基準とするエレメントポインタ
 * @retval NULL 前方のポインタは無い
 * @retval NULL以外 前方のfifoエレメントポインタ
 */
static fifoitem_t *slist_get_foward_element_ptr( const stl_slist_ext_t *const e, void * const element_ptr)
{
    fifoitem_t *cur;

    if( (e->cnt == 0) || (e->w_p == &e->base) ) {
	return NULL;
    }

    for( cur=e->w_p; cur->next != NULL; cur=cur->next) {
	if( cur->next == element_ptr ) {
	    return cur;
	}
    }

    return NULL;
}


/**
 * @fn mpsx_stl_slist_t *mpsx_stl_slist_init( mpsx_stl_slist_t *const self_p, const size_t sizof_element)
 * @brief キューオブジェクトを初期化します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param sizof_element エレメントのサイズ
 * @retval 0 成功
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mpsx_stl_slist_init(mpsx_stl_slist_t *const self_p,
			   const size_t sizof_element)
{
    stl_slist_ext_t *e = NULL;
    memset(self_p, 0x0, sizeof(mpsx_stl_slist_t));

    e = (stl_slist_ext_t *)
	mpsx_malloc(sizeof(stl_slist_ext_t));
    if (NULL == e) {
	DBMS1(stderr, "mpsx_stl_slist_init : mpsx_malloc(ext) fail" EOL_CRLF);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(stl_slist_ext_t));

    self_p->ext = e;
    self_p->sizof_element = e->sizof_element = sizof_element;

    e->r_p = e->w_p = &e->base;

    return 0;
}

/**
 * @fn int mpsx_stl_slist_destroy( mpsx_stl_slist_t *const self_p )
 * @brief キューオブジェクトを破棄します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @retval EBUSY リソースの解放に失敗
 * @retval -1 errno.h以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_destroy(mpsx_stl_slist_t *const self_p)
{
    int result;
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);

    result = mpsx_stl_slist_clear(self_p);
    if (result) {
	DBMS1(stderr, "mpsx_stl_slist_destroy : que_clear fail" EOL_CRLF);
	return EBUSY;
    }

    /* 最後のエレメントがbaseで無ければ削除 */
    if (e->r_p != &e->base) {
	mpsx_free(e->r_p);
	e->r_p = NULL;
    }

    mpsx_free(self_p->ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn int mpsx_stl_slist_push(mpsx_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief キューにエレメントを追加します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_slist_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mpsx_stl_slist_push(mpsx_stl_slist_t *const self_p, const void *const el_p,
			   const size_t sizof_element)
{
    int status;
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict f = NULL;

    if (e->sizof_element != sizof_element) {
	return EINVAL;
    }

    f = (fifoitem_t *) mpsx_malloc(sizeof(fifoitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1(stderr,
	      "mpsx_stl_slist_push : mpsx_malloc(fifoitem_t) fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    }

    memset(f, 0x0, sizeof(fifoitem_t));
    //memcpy(f->data, el_p, e->sizof_element);
    mpsx_memcpy(f->data, el_p, e->sizof_element);
    f->next = NULL;

    /* キューに追加 */
    e->w_p->next = f;
    e->w_p = f;
    ++(e->cnt);

    status = 0;

  out:
    if (status) {
	if (NULL != f) {
	    mpsx_free(f);
	}
    }
    return status;
}

/**
 * @fn int mpsx_stl_slist_pop(mpsx_stl_slist_t *const self_p)
 * @brief キューの先頭の要素を削除します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @retval ENOENT 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_pop(mpsx_stl_slist_t *const self_p)
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0 ) {
	return ENOENT;
    }

    if((e->r_p == &e->base) && ( e->r_p->next == NULL)) { // ここの検査がよく分からないので調べる必要有り
	return -1;
    }

    tmp = e->r_p;
    e->r_p = e->r_p->next;
    --(e->cnt);

    if (tmp != &e->base) {
	mpsx_free(tmp);
    }

    if (e->cnt == 0) {
	if (e->r_p != &e->base) {
	    mpsx_free(e->r_p);
	}
	e->r_p = e->w_p = &e->base;
    }

    return 0;
}

/**
 * @fn int mpsx_stl_slist_front( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element )
 * @brief キューの先頭のエレメントデータを返します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param el_p エレメントデータ取得用データバッファポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_slist_initで指定した以外のサイズはエラーとします)
 * @retval EINVAL  エレメントサイズが異なる
 * @retval ENOENT エレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_front(mpsx_stl_slist_t *const self_p, void *const el_p,
			    const size_t sizof_element)
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict f;

    if (e->sizof_element != sizof_element) {
	return EINVAL;
    }

    if (e->cnt == 0) {
	/* キューにエレメントが無い場合 */
	return ENOENT;
    }
    f = e->r_p->next;

    // memcpy(el_p, f->data, e->sizof_element);
    mpsx_memcpy(el_p, f->data, e->sizof_element);

    return 0;
}

/**
 * @fn int mpsx_stl_slist_clear( mpsx_stl_slist_t *const self_p )
 * @brief キューに貯まっているエレメントデータを全て破棄します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @retval -1 致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_clear(mpsx_stl_slist_t *const self_p)
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    size_t n;
    int result;

    if (e->cnt == 0) {
	return 0;
    }

    for (n = e->cnt; n != 0; --n) {
	result = mpsx_stl_slist_pop(self_p);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPSX_XTOA_DEC64_BUFSZ];

		mpsx_u64toadec( (uint64_t)n, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		  "mpsx_stl_slist_clear : mpsx_stl_slist_pop[%s] fail" EOL_CRLF, xtoa_buf);
	    }
	    return -1;
	}
    }

    return 0;
}

/**
 * @fn int mpsx_stl_slist_get_pool_cnt( mpsx_stl_slist_t *const self_p );
 * @brief キューにプールされているエレメント数を返します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0以上 要素数
 */
size_t mpsx_stl_slist_get_pool_cnt(mpsx_stl_slist_t *const self_p)
{
    const stl_slist_ext_t * const e = get_const_stl_slist_ext(self_p);

    return e->cnt;
}

/**
 * @fn int mpsx_stl_slist_is_empty( mpsx_stl_slist_t *const self_p)
 * @brief キューが空かどうかを判定します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0 キューは空ではない
 * @retval 1 キューは空である
 */
int mpsx_stl_slist_is_empty(mpsx_stl_slist_t *const self_p)
{
    const stl_slist_ext_t * const e = get_const_stl_slist_ext(self_p);

    return (e->cnt == 0) ? 1 : 0;
}

/**
 * @fn int mpsx_stl_slist_get_element_at( mpsx_stl_slist_t *const self_p, const size_t num, void *const el_p, const int sizof_element);
 * @brief キューに保存されているエレメントを返します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功 失敗
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int mpsx_stl_slist_get_element_at(mpsx_stl_slist_t *const self_p,
				       const size_t num, void *const el_p,
				       const size_t sizof_element)
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict item_p = &e->base;
    size_t n;

    if (mpsx_stl_slist_is_empty(self_p)) {
	return EACCES;
    }

    if (NULL == el_p) {
	return EFAULT;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    for (n = 0; n < num; ++n) {
	item_p = item_p->next;
    }

    memcpy(el_p, item_p->next->data, e->sizof_element);

    return 0;
}


/**
 * @fn int mpsx_stl_slist_back( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element )
 * @brief キューの最後尾のエレメントデータを返します
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param el_p エレメントデータ取得用データバッファポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_slist_initで指定した以外のサイズはエラーとします)
 * @retval EINVAL  エレメントサイズが異なる
 * @retval ENOENT エレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_back( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element )
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }

    if (e->w_p == NULL) {
	return -1;
    }

    tmp = e->w_p;
    // memcpy( el_p, tmp->data, e->sizof_element);
    mpsx_memcpy( el_p, tmp->data, e->sizof_element);

    return 0;
}

/**
 * @fn int mpsx_stl_slist_insert_at( mpsx_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element )
 * @brief 指定されたエレメント番号の前に、エレメントデータを挿入します。
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param no オブジェクト内の0～のリストエレメント番号
 * @param el_p エレメントデータ取得用データバッファポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_slist_initで指定した以外のサイズはエラーとします)
 * @retval EINVAL  エレメントサイズが異なる
 * @retval ENOENT 指定された番号のエレメントが存在しない
 * @retval EAGAIN メモリ不足で追加不可能
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_slist_insert_at( mpsx_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element )
{
    int status;
    size_t n;
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict cur_ticket_p = &e->base;
    fifoitem_t *__restrict fwd_ticket_p;
    fifoitem_t *__restrict f;

    if (mpsx_stl_slist_is_empty(self_p)) {
	return ENOENT;
    }

    if (NULL == el_p) {
	return EFAULT;
    }

    if ((e->sizof_element != sizof_element) || ( no < e->cnt) ) {
	return EINVAL;
    }

    for (n = 0; n <= no; ++n) {
	cur_ticket_p = cur_ticket_p->next;
    }

    fwd_ticket_p = slist_get_foward_element_ptr( e, cur_ticket_p);
    if( NULL == fwd_ticket_p ) {
	return EFAULT;
    }

    f = (fifoitem_t *) mpsx_malloc(sizeof(fifoitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1(stderr,
	      "mpsx_stl_slist_insert_at : mpsx_malloc(fifoitem_t) fail" EOL_CRLF);
	return EAGAIN;
    }

    memset(f, 0x0, sizeof(fifoitem_t));
    //memcpy(f->data, el_p, e->sizof_element);
    mpsx_memcpy(f->data, el_p, e->sizof_element);
    f->next = NULL;

    /* キューに追加 */
    f->next = cur_ticket_p;
    fwd_ticket_p->next = f;
    ++(e->cnt);

    status = 0;

// out:
    if (status) {
	if (NULL != f) {
	    mpsx_free(f);
	}
    }
    return status;
}

/**
 * @fn int mpsx_stl_slist_erase_at( mpsx_stl_slist *const self_p, const size_t no)
 * @brief 指定されたエレメントを消去します。
 *      制御確認中
 * @param self_p mpsx_stl_slist_t構造体インスタンスポインタ
 * @param no オブジェクト内の0～のリストエレメント番号
 * @retval 0 成功
 * @retval ENOENT 指定された番号のエレメントが存在しない
 **/
int mpsx_stl_slist_erase_at( mpsx_stl_slist_t *const self_p, const size_t no)
{
    stl_slist_ext_t * const e = get_stl_slist_ext(self_p);
    fifoitem_t *__restrict tmp = NULL;
    fifoitem_t *__restrict fwd_ticket_p;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0 || !(no < e->cnt) ) {
	return ENOENT;
    }

    if((e->r_p == &e->base) || ( e->r_p->next == NULL)) {
	return ENOENT;
    }

    if( no == 0 ) {
	tmp = e->r_p;
	e->r_p = e->r_p->next;
	--(e->cnt);
    } else {
	size_t n;
	fifoitem_t *__restrict cur_ticket_p = &e->base;

	for (n = 0; n <= no; ++n) {
            cur_ticket_p = cur_ticket_p->next;
	}

        fwd_ticket_p = slist_get_foward_element_ptr( e, cur_ticket_p);
        if( NULL == fwd_ticket_p ) {
	    return EFAULT;
        }
    }


    if (tmp != &e->base) {
	mpsx_free(tmp);
    }

    if (e->cnt == 0) {
	if (e->r_p != &e->base) {
	    mpsx_free(e->r_p);
	}
	e->r_p = e->w_p = &e->base;
    }

    return 0;

}

