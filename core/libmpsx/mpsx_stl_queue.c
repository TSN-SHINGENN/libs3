/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-January-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_stl_queue.cpp
 * @brief 待ち行列ライブラリ STLのqueueクラス互換です。
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
#include <stdint.h>
#include <string.h>
#include <errno.h>

/* libs3 */
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_malloc.h"
#include "mpsx_stl_deque.h"
#include "mpsx_stl_list.h"
#include "mpsx_stl_slist.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#include "mpsx_stl_queue.h"

typedef int(*front_func_t)( void *self_p, void *el_p, const size_t sizof_el);
typedef int(*back_func_t)( void *self_p, void *el_p, const size_t sizof_el);
typedef int(*push_func_t)(void *self_p, const void *el_p, const size_t sizof_el);
typedef int(*pop_func_t)( void *self_p);
typedef size_t(*get_pool_cnt_func_t)(void *self_p);
typedef int(*is_empty_func_t)(void *self_p);
typedef int(*clear_func_t)(void *self_p);
typedef int(*get_element_at_func_t)(void *self_p, size_t num, void *el_p, const size_t sizof_el);

typedef struct _stl_queue_ext {
    enum_mpsx_stl_queue_implement_type_t implement_type;
    union {
	mpsx_stl_deque_t deque;
	mpsx_stl_slist_t slist;
	mpsx_stl_list_t list;
	uint8_t ptr[1];
    } instance;

    size_t sizof_element;
    front_func_t front_func;
    back_func_t back_func;
    push_func_t push_func;
    pop_func_t pop_func;
    get_pool_cnt_func_t get_pool_cnt_func;
    is_empty_func_t is_empty_func;
    clear_func_t clear_func;
    get_element_at_func_t get_element_at_func;

    union {
	unsigned int flags;
	struct {
	    unsigned int deque:1;
	    unsigned int slist:1;
	    unsigned int list:1;
	} f;
    } init;
} stl_queue_ext_t;

#define get_stl_queue_ext(s) MPSX_STATIC_CAST(stl_queue_ext_t*,(s)->ext)
#define get_stl_const_queue_ext(s) MPSX_STATIC_CAST(const stl_queue_ext_t*,(s)->ext)

#if defined(_S3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern int mpsx_stl_queue_init( mpsx_stl_queue_t *const self_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_init_ex( mpsx_stl_queue_t *const self_p, const size_t sizof_element, const enum_mpsx_stl_queue_implement_type_t implement_type, void *const attr_p)
	__attribute__ ((optimize("Os")));

extern int mpsx_stl_queue_destroy( mpsx_stl_queue_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_push( mpsx_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_pop( mpsx_stl_queue_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_front( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element )
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_get_element_at( mpsx_stl_queue_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_clear( mpsx_stl_queue_t *const self_p)
	__attribute__ ((optimize("Os")));
extern size_t mpsx_stl_queue_get_pool_cnt( mpsx_stl_queue_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_is_empty( mpsx_stl_queue_t *const self_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_stl_queue_back( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t  sizof_element )
	__attribute__ ((optimize("Os")));

#endif /* end of _S3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE */

/**
 * @fn mpsx_stl_queue_t *mpsx_stl_queue_init( mpsx_stl_queue_t *const self_p, const size_t sizof_element)
 * @brief stl_queueインスタンスの初期化。属性はデフォルトとして指定される
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @param sizof_element エレメントのサイズ
 * @retval 0 成功
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mpsx_stl_queue_init(mpsx_stl_queue_t *const self_p,
			   const size_t sizof_element)
{
    return mpsx_stl_queue_init_ex( self_p, sizof_element, MPSX_STL_QUEUE_TYPE_IS_DEFAULT, NULL);
}

/**
 * @fn int mpsx_stl_queue_init_ex( mpsx_stl_queue_t *const self_p, const size_t sizof_element, const enum_mpsx_stl_queue_implement_type_t implement_type, void *const attr_p)
 * @brief stl_queueインスタンスの属性つき初期化
 * @param self_p mpsx_stl_stack_t構造体インスタンスポインタ
 * @param sizof_element 1以上のエレメントのサイズ
 * @param implement_type
 * @param attr_p 予約(NULL固定)
 * @retval 0 成功
 * @retval EAGAIN リソースを確保できなかった
 * @retval EINVAL 引数が不正
 * @retval ENOSYS サポートされていない
 */
int mpsx_stl_queue_init_ex( mpsx_stl_queue_t *const self_p, const size_t sizof_element, const enum_mpsx_stl_queue_implement_type_t type, void *const attr_p)
{
    int result, status;
    stl_queue_ext_t * __restrict e = NULL;
    const enum_mpsx_stl_queue_implement_type_t implement_type = ( type == MPSX_STL_QUEUE_TYPE_IS_DEFAULT ) ? MPSX_STL_QUEUE_TYPE_IS_SLIST : type;

    (void)attr_p;
    memset( self_p, 0x0, sizeof(mpsx_stl_queue_t));

    if(!(sizof_element > 0 )) {
	return EINVAL;
    }

    if((implement_type >= MPSX_STL_QUEUE_TYPE_IS_OTHERS) || ( implement_type < MPSX_STL_QUEUE_TYPE_IS_DEFAULT )) {
	return EINVAL;
    }

    e = (stl_queue_ext_t*)mpsx_malloc(sizeof(stl_queue_ext_t));
    if( NULL == e ) {
	return EAGAIN;
    }
    memset( e, 0x0, sizeof(stl_queue_ext_t));
    self_p->ext = e;

    self_p->sizof_element = e->sizof_element = sizof_element;

    switch(implement_type) {
    case MPSX_STL_QUEUE_TYPE_IS_SLIST:
	/* is_slist */
	result = mpsx_stl_slist_init( &e->instance.slist, sizof_element);
	if(result) {
	    DBMS1( stderr, "mpsx_stl_queue_init_ex : mpsx_stl_slist_init fail, streror:%s" EOL_CRLF, strerror(result));
	    status = result;
	    goto out;
	}

	e->front_func = (front_func_t)mpsx_stl_slist_front;
	e->push_func = (push_func_t)mpsx_stl_slist_push;
        e->pop_func = (pop_func_t)mpsx_stl_slist_pop;
        e->get_pool_cnt_func = (get_pool_cnt_func_t)mpsx_stl_slist_get_pool_cnt;
	e->is_empty_func = (is_empty_func_t)mpsx_stl_slist_is_empty;
	e->clear_func = (clear_func_t)mpsx_stl_slist_clear;
	e->get_element_at_func = (get_element_at_func_t)mpsx_stl_slist_get_element_at;

	e->init.f.slist = 1;
	break;
    case MPSX_STL_QUEUE_TYPE_IS_DEQUE:
	/* is_deque */
	result = mpsx_stl_deque_init( &e->instance.deque, sizof_element);
	if(result) {
	    DBMS1( stderr, "mpsx_stl_queue_init_ex : mpsx_stl_deque_init fail, streror:%s" EOL_CRLF, strerror(result));
	    status = result;
	    goto out;
	}

	e->front_func = (front_func_t)mpsx_stl_deque_front;
	e->push_func = (push_func_t)mpsx_stl_deque_push_back;
        e->pop_func = (pop_func_t)mpsx_stl_deque_pop_front;
        e->get_pool_cnt_func = (get_pool_cnt_func_t)mpsx_stl_deque_get_pool_cnt;
	e->is_empty_func = (is_empty_func_t)mpsx_stl_deque_is_empty;
	e->clear_func = (clear_func_t)mpsx_stl_deque_clear;
	e->get_element_at_func = (get_element_at_func_t)mpsx_stl_deque_get_element_at;
	
	e->init.f.deque = 1;
	break;
    case MPSX_STL_QUEUE_TYPE_IS_LIST:
	/* is_list */
	result = mpsx_stl_list_init( &e->instance.list, sizof_element);
	if(result) {
	    DBMS1( stderr, "mpsx_stl_list_init : mpsx_stl_list_init fail, strerror:%s" EOL_CRLF, strerror(result));
	    status = result;
	    goto out;
	}

	e->front_func = (front_func_t)mpsx_stl_list_front;
	e->push_func = (push_func_t)mpsx_stl_list_push_back;
        e->pop_func = (pop_func_t)mpsx_stl_list_pop_front;
        e->get_pool_cnt_func = (get_pool_cnt_func_t)mpsx_stl_list_get_pool_cnt;
	e->is_empty_func = (is_empty_func_t)mpsx_stl_list_is_empty;
	e->clear_func = (clear_func_t)mpsx_stl_list_clear;
	e->get_element_at_func = (get_element_at_func_t)mpsx_stl_list_get_element_at;
	
	e->init.f.list = 1;
	break;
    default:
	status = ENOSYS;
	goto out;
    }

    status = 0;

out:    
    if(status) {
        mpsx_stl_queue_destroy(self_p);
    } 
    return status;
}

/**
 * @fn int mpsx_stl_queue_destroy( mpsx_stl_queue_t *const self_p )
 * @brief キューオブジェクトを破棄します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @retval 0以外失敗
 * @retval 0 成功
 */
int mpsx_stl_queue_destroy(mpsx_stl_queue_t *const self_p)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);
    int status, result;

    if( NULL == e ) {
	return 0;
    }

    if( e->init.f.deque ) {
	/* is_deque */
	result = mpsx_stl_deque_destroy( &e->instance.deque);
	if(result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPSX_XTOA_HEX64_BUFSZ;
		char xtoa_buf[MPSX_XTOA_HEX64_BUFSZ];

		mpsx_u64toahex((uint64_t)result, xtoa_buf, xtoa_bufsz,
		    NULL);
		DBMS1( stderr, "mpsx_stl_queue_destroy : mpsx_stl_deque_destroy fail, retva=0x%s" EOL_CRLF, xtoa_buf);
	    }
	    status = result;
	    goto out;
	}
	e->init.f.deque = 0;
    }

    if( e->init.f.slist ) {
	/* is_deque */
	result = mpsx_stl_slist_destroy( &e->instance.slist);
	if(result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPSX_XTOA_HEX64_BUFSZ;
		char xtoa_buf[MPSX_XTOA_HEX64_BUFSZ];

		mpsx_u64toahex((uint64_t)result, xtoa_buf, xtoa_bufsz, NULL);
		DBMS1( stderr, "mpsx_stl_queue_destroy : mpsx_stl_slist_destroy fail, retval=0x%s" EOL_CRLF, xtoa_buf);
	    }
	    status = result;
	    goto out;
	}
	e->init.f.slist = 0;
    }

    if( e->init.f.list ) {
	/* is_deque */
	result = mpsx_stl_list_destroy( &e->instance.list);
	if(result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MPSX_XTOA_HEX64_BUFSZ;
		char xtoa_buf[MPSX_XTOA_HEX64_BUFSZ];

		mpsx_u64toahex((uint64_t)result, xtoa_buf, xtoa_bufsz, NULL);
		DBMS1( stderr, "mpsx_stl_queue_destroy : mpsx_stl_list_destroy fail, retval=0x%s" EOL_CRLF, xtoa_buf);
	    }
	    status = result;
	    goto out;
	}
	e->init.f.list = 0;
    }
    
    status = e->init.flags;

out:
    if(!status) {
	mpsx_free(e);
	self_p->ext = NULL;
    }
    return status;
}
	
/**
 * @fn int mpsx_stl_queue_push(mpsx_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief キューにエレメントを追加します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_queue_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int mpsx_stl_queue_push(mpsx_stl_queue_t *const self_p, const void *const el_p,
			   const size_t sizof_element)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->push_func(e->instance.ptr, el_p, sizof_element);
}

/**
 * @fn int mpsx_stl_queue_pop(mpsx_stl_queue_t *const self_p)
 * @brief キューの先頭の要素を削除します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @retval ENOENT 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_queue_pop(mpsx_stl_queue_t *const self_p)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->pop_func(e->instance.ptr);
}

/**
 * @fn int mpsx_stl_queue_front( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element )
 * @brief キューの先頭のエレメントデータを返します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @param el_p エレメントデータ取得用データバッファポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_queue_initで指定した以外のサイズはエラーとします)
 * @retval EINVAL  エレメントサイズが異なる
 * @retval ENOENT エレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_queue_front(mpsx_stl_queue_t *const self_p, void *const el_p,
			    const size_t sizof_element)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->front_func(e->instance.ptr, el_p, sizof_element);
}

/**
 * @fn int mpsx_stl_queue_clear( mpsx_stl_queue_t *const self_p )
 * @brief キューに貯まっているエレメントデータを全て破棄します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @retval -1 致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_queue_clear(mpsx_stl_queue_t *const self_p)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->clear_func(e->instance.ptr);
}

/**
 * @fn size_t mpsx_stl_queue_get_pool_cnt( mpsx_stl_queue_t *const self_p );
 * @brief キューにプールされているエレメント数を返します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @retval 0以上 要素数
 */
size_t mpsx_stl_queue_get_pool_cnt(mpsx_stl_queue_t *const self_p)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->get_pool_cnt_func(e->instance.ptr);
}

/**
 * @fn int mpsx_stl_queue_is_empty( mpsx_stl_queue_t *const self_p)
 * @brief キューが空かどうかを判定します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0 キューは空ではない
 * @retval 1 キューは空である
 */
int mpsx_stl_queue_is_empty(mpsx_stl_queue_t *const self_p)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->is_empty_func(e->instance.ptr);
}

/**
 * @fn int mpsx_stl_queue_get_element_at( mpsx_stl_queue_t *const self_p, const int num, void *const el_p, const int sizof_element);
 * @brief キューに保存されているエレメントを返します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @param num 0空始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータコピー用エレメント構造体ポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功 失敗
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int mpsx_stl_queue_get_element_at(mpsx_stl_queue_t *const self_p,
				       const size_t num, void *const el_p,
				       const size_t sizof_element)
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->get_element_at_func(e->instance.ptr, num, el_p, sizof_element);
}


/**
 * @fn int mpsx_stl_queue_back( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element )
 * @brief キューの最後尾のエレメントデータを返します
 * @param self_p mpsx_stl_queue_t構造体インスタンスポインタ
 * @param el_p エレメントデータ取得用データバッファポインタ
 * @param sizof_element エレメントサイズ(mpsx_stl_queue_initで指定した以外のサイズはエラーとします)
 * @retval EINVAL  エレメントサイズが異なる
 * @retval ENOENT エレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int mpsx_stl_queue_back( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element )
{
    stl_queue_ext_t *const e = get_stl_queue_ext(self_p);

    return e->back_func(e->instance.ptr, el_p, sizof_element);
}


