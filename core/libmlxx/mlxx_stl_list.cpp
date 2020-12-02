/**
 *	Copyright 2014 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2014-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file multios_stl_list.c
 * @brief 双方向リンクリストライブラリ STLのlistクラス互換です。
 *	なのでスレッドセーフではありません。上位層で処理を行ってください。
 *	STL互換と言ってもCの実装です、オブジェクトの破棄の処理はしっかり実装してください
 *  ※ 現状はmallocを乱発します。機会があれば、メモリ管理マネージャに切り替えます。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* POSIX */
#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* this */
#include "multios_sys_types.h"
#include "multios_stdlib.h"
#include "multios_malloc.h"
#include "multios_string.h"
#include "multios_stl_list.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MULTIOS_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _queitem {
    struct _queitem *prev;
    struct _queitem *next;
    int item_id;
    unsigned char data[];
} queitem_t;

typedef struct _multios_stl_list_ext {
    volatile size_t sizof_element;
    volatile size_t cnt;
    int start_id;
    queitem_t base;		/* エレメントの基点。配列0の構造体があるので必ず最後にする */
} multios_stl_list_ext_t;

#define get_stl_list_ext(s) MULTIOS_STATIC_CAST(multios_stl_list_ext_t*,(s)->ext)
#define get_const_stl_list_ext(s) MULTIOS_STATIC_CAST(const multios_stl_list_ext_t*,(s)->ext)

#if defined(__GNUC__) && defined(_MULTIOS_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

static queitem_t *list_search_item(multios_stl_list_ext_t *const e,
				    const size_t num)
	__attribute__ ((optimize("Os")));

extern int multios_stl_list_pop_front(multios_stl_list_t *const)
	__attribute__ ((optimize("Os")));
extern int multios_stl_list_remove_at(multios_stl_list_t *const, const size_t)
	__attribute__ ((optimize("Os")));
extern int multios_stl_list_push_back(multios_stl_list_t *const, const void *const, const size_t)
	__attribute__ ((optimize("Os")));
extern int multios_stl_list_insert(multios_stl_list_t *const, const size_t, const void *const, const size_t)
	__attribute__ ((optimize("Os")));
extern int multios_stl_list_remove_at(multios_stl_list_t *const, const size_t)
	__attribute__ ((optimize("Os")));
extern int multios_stl_list_overwrite_element_at( multios_stl_list_t *const, const size_t, const void *const, const size_t)
	__attribute__ ((optimize("Os")));
#else /* Normal */
static queitem_t *list_search_item(multios_stl_list_ext_t *const e,
				    const size_t num);
#endif /* end of _MULTIOS_MICROCONTROLLER_OPTIMIZE_ENABLE */

/**
 * @fn int multios_stl_list_init( multios_stl_list_t *const self_p, const size_t sizof_element)
 * @brief 双方向キューオブジェクトを初期化します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param sizof_element 1以上のエレメントのサイズ
 * @retval 0 成功
 * @retval EINVAL sizof_element値が不正
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int multios_stl_list_init(multios_stl_list_t *const self_p,
			   const size_t sizof_element)
{
    multios_stl_list_ext_t *e = NULL;
    memset(self_p, 0x0, sizeof(multios_stl_list_t));

    e = (multios_stl_list_ext_t *)
	multios_malloc(sizeof(multios_stl_list_ext_t));
    if (NULL == e) {
	DBMS1(stderr, "multios_stl_list_init : multios_malloc(ext) fail" EOL_CRLF);
	return EAGAIN;
    }
    memset(e, 0x0, sizeof(multios_stl_list_ext_t));

    self_p->ext = e;
    self_p->sizeof_element = e->sizof_element = sizof_element;
    e->cnt = 0;
    e->start_id = 0;

    e->base.prev = e->base.next = &e->base;

    return 0;
}

/**
 * @fn int multios_stl_list_destroy( multios_stl_list_t *const self_p )
 * @brief 双方向キューオブジェクトを破棄します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval EBUSY リソースの解放に失敗
 * @retval -1 errno.h以外の致命的な失敗
 * @retval 0 成功
 */
int multios_stl_list_destroy(multios_stl_list_t *const self_p)
{
    int result;

    result = multios_stl_list_clear(self_p);
    if (result) {
	DBMS1(stderr, "multios_stl_list_destroy : que_clear fail" EOL_CRLF);
	return EBUSY;
    }

    multios_free(self_p->ext);
    self_p->ext = NULL;

    return 0;
}

/**
 * @fn int multios_stl_list_push_back(multios_stl_list_t *const self_p, const void *const el_p, const int sizof_element)
 * @brief 双方向キューの後方にエレメントを追加します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(multios_stl_list_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int multios_stl_list_push_back(multios_stl_list_t *const self_p,
				const void *const el_p,
				const size_t sizof_element)
{
    int status;
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict f = NULL;

    DBMS3(stderr, "multios_stl_list_push_back : execute" EOL_CRLF);

    if (e->cnt == 0) {
	return multios_stl_list_insert(self_p, 0, el_p, sizof_element);
    }

    if (e->sizof_element != sizof_element) {
	return EINVAL;
    }

    f = (queitem_t *) multios_malloc(sizeof(queitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1(stderr, "multios_stl_list_push : multios_malloc(queitem_t) fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    }

    memset(f, 0x0, sizeof(queitem_t));
    multios_memcpy(f->data, el_p, e->sizof_element);
    f->prev = e->base.prev;
    f->next = &e->base;
    f->item_id = (e->base.prev->item_id) + 1;

    /* キューに追加 */
    e->base.prev->next = f;
    e->base.prev = f;
    e->cnt++;

    status = 0;

  out:
    if (status) {
	if (NULL != f) {
	    multios_free(f);
	}
    }
    return status;
}


/**
 * @fn int multios_stl_list_push_front(multios_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element)
 * @brief 双方向キューの前方にエレメントを追加します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param el_p エレメントポインタ
 * @param sizof_element エレメントサイズ(multios_stl_list_initで指定した以外のサイズはエラーとします)
 * @retval 0 成功
 * @retval EINVAL エレメントサイズが異なる
 * @retval EAGAIN リソースの獲得に失敗
 * @retval -1 それ以外の致命的な失敗
 */
int multios_stl_list_push_front(multios_stl_list_t *const self_p,
				 const void *const el_p,
				 const size_t sizof_element)
{
    return multios_stl_list_insert(self_p, 0, el_p, sizof_element);
}

/**
 * @fn int multios_stl_list_pop_front(multios_stl_list_t *self_p)
 * @brief 双方向キューの先頭の要素を削除します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int multios_stl_list_pop_front(multios_stl_list_t *const self_p)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return EACCES;
    }

    if (e->base.next == &e->base) {
	return -1;
    }

    /* エレメントを外す */
    tmp = e->base.next;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    --e->cnt;
    ++e->start_id;

    if (tmp != &e->base) {
	multios_free(tmp);
    }

    if (e->cnt == 0) {
	e->start_id = 0;
    }

    return 0;
}


/**
 * @fn int multios_stl_list_pop_back(multios_stl_list_t *const self_p)
 * @brief 双方向キューの後方の要素を削除します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval EACCES 削除するエレメントが存在しない
 * @retval -1 それ以外の致命的な失敗
 * @retval 0 成功
 */
int multios_stl_list_pop_back(multios_stl_list_t *const self_p)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return EACCES;
    }

    if (e->base.prev == &e->base) {
	return -1;
    }

    /* エレメントを外す */
    tmp = e->base.prev;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    (void) e->start_id;		/* 何もしない */
    e->cnt--;

    if (tmp != &e->base) {
	multios_free(tmp);
    }

    if (e->cnt == 0) {
	e->start_id = 0;
    }

    return 0;
}

/**
 * @fn int multios_stl_list_clear( multios_stl_list_t *const self_p )
 * @brief キューに貯まっているエレメントデータを全て破棄します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval -1 致命的な失敗
 * @retval 0 成功
 */
int multios_stl_list_clear(multios_stl_list_t *const self_p)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    size_t i;
    int result;

    if (e->cnt == 0) {
	return 0;
    }

    for (i = e->cnt; i != 0; --i) {
	result = multios_stl_list_pop_front(self_p);
	if (result) {
	    IFDBG1THEN {
		const size_t xtoa_bufsz=MULTIOS_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MULTIOS_XTOA_DEC64_BUFSZ];

		multios_i64toadec( (int64_t)i, xtoa_buf, xtoa_bufsz);
		DBMS1(stderr,
		  "multios_stl_list_clear : multios_stl_list_pop_back[%s] fail" EOL_CRLF, xtoa_buf);
	    }

	    return -1;
	}
    }

    return 0;
}

/**
 * @fn int multios_stl_list_get_pool_cnt( multios_stl_list_t *const self_p );
 * @brief キューにプールされているエレメント数を返します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0以上 要素数
 */
size_t multios_stl_list_get_pool_cnt(multios_stl_list_t *const self_p)
{
    const multios_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);

    return e->cnt;
}

/**
 * @fn int multios_stl_list_is_empty( multios_stl_list_t *self_p)
 * @brief キューが空かどうかを判定します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @retval -1 失敗
 * @retval 0 キューは空ではない
 * @retval 1 キューは空である
 */
int multios_stl_list_is_empty(multios_stl_list_t *const self_p)
{
    const multios_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);

    return (e->cnt == 0) ? 1 : 0;
}

/**
 * @fn static queitem_t *list_search_item( multios_stl_list_ext_t *const e, const size_t num)
 * @brief numからqueitemのポインタを検索します
 * @param e multios_stl_list_ext_t構造体ポインタ
 * @param 0から始まるエレメント配列番号
 */
static queitem_t *list_search_item(multios_stl_list_ext_t *const e,
				    const size_t num)
{
    const int target_id = e->start_id + (int) num;
    queitem_t *__restrict item_p;

    if ((e->cnt / 2) < num) {
	/* 後方から前方検索 */
	item_p = e->base.prev;
	while (item_p != &e->base) {
	    if (item_p->item_id != target_id) {
		item_p = item_p->prev;
		continue;
	    }
	    break;
	}
    } else {
	/* 前方から後方検索 */
	item_p = e->base.next;
	while (item_p != &e->base) {
	    if (item_p->item_id != target_id) {
		item_p = item_p->next;
		continue;
	    }
	    break;
	}
    }

    if (item_p == &e->base) {
	return NULL;
    }

    return item_p;
}

/**
 * @fn int multios_stl_list_get_element_at( multios_stl_list_t *const self_p, const int num, void *const el_p, const size_t sizof_element);
 * @brief キューに保存されているエレメントを返します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int multios_stl_list_get_element_at(multios_stl_list_t *const self_p,
				     const size_t num, void *const el_p,
				     const size_t sizof_element)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p;

    if (multios_stl_list_is_empty(self_p)) {
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

    item_p = list_search_item(e, num);
    if (item_p == NULL) {
	return -1;
    }

    multios_memcpy(el_p, item_p->data, e->sizof_element);

    return 0;
}

/**
 * @fn int multios_stl_list_remove_at(multios_stl_list_t *const self_p, const size_t num)
 * @brief 双方向キューに保存されている要素を消去します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @retval 0 成功
 * @retval EACCES 双方向キューが空
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 */
int multios_stl_list_remove_at(multios_stl_list_t *const self_p,
			       const size_t num)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p = &e->base;

    if (multios_stl_list_is_empty(self_p)) {
	return EACCES;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    item_p = list_search_item(e, num);
    if (item_p == &e->base) {
	return -1;
    }

    /* itemのエレメントを排除 */
    item_p->prev->next = item_p->next;
    item_p->next->prev = item_p->prev;
    e->cnt--;

    /* start_idを書き換える */
    if (e->start_id == item_p->item_id) {
	e->start_id = item_p->next->item_id;
    } else if (item_p->next != &e->base) {
	queitem_t *__restrict tmp_p = item_p->next;
	int id = item_p->item_id;
	while (tmp_p != &e->base) {
	    tmp_p->item_id = id;
	    ++id;
	    tmp_p = tmp_p->next;
	}
    };

    /* itemのエレメントを削除 */
    multios_free(item_p);

    return 0;
}

/**
 * @fn void multios_stl_list_dump_element_region_list( multios_stl_list_t *const self_p)
 * @brief 双方向キューに保存されている要素の情報をダンプします
 *	主にこのデバッグ用
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 **/
void multios_stl_list_dump_element_region_list(multios_stl_list_t *const
						self_p)
{
    const multios_stl_list_ext_t *const e = get_const_stl_list_ext(self_p);
    const queitem_t *__restrict item_p = &e->base;
    size_t n;
#ifdef __GNUC__
__attribute__ ((unused))
#endif
	size_t p;

    const size_t xtoa_bufsz=MULTIOS_XTOA_DEC64_BUFSZ;
    char xtoa_buf[MULTIOS_XTOA_DEC64_BUFSZ];

    DMSG(stdout,
	    "multios_stl_list_dump_element_region_list : execute\n");
    multios_u64toadec( (uint64_t)e->sizof_element, xtoa_buf, xtoa_bufsz);
    DMSG(stderr, "sizof_element = %s" EOL_CRLF, xtoa_buf);
    multios_u64toadec( (uint64_t)e->cnt, xtoa_buf, xtoa_bufsz);
    DMSG(stderr, "numof_elements = %s" EOL_CRLF, xtoa_buf);
    multios_i64toadec( (int64_t)e->start_id, xtoa_buf, xtoa_bufsz);
    DMSG(stderr, "start_id = %s" EOL_CRLF, xtoa_buf);

    item_p = e->base.next;
    for (n = 0; n < e->cnt; ++n) {
#if !defined(XILKERNEL)
    if(1) {
  	char buf[512];
	buf[0] = '\0';
	p = 0;

	p += sprintf(&buf[p], "[%03d(%03d)]: ",
		     (item_p->item_id - e->start_id), item_p->item_id);
	p += sprintf(&buf[p], "%p : prev[%03d]=%p next[%03d]=%p ", item_p,
		     item_p->prev->item_id, item_p->prev,
		     item_p->next->item_id, item_p->next);
	if (item_p->prev == &e->base) {
	    p += sprintf(&buf[p], "(prev is base) ");
	}
	if (item_p->next == &e->base) {
	    p += sprintf(&buf[p], "(next is base) ");
	}
	DMSG(stdout, "%s" EOL_CRLF, buf);
	}
#else /* XILINX */

	xil_printf("[%03d(%03d)]: ",
		     (int)(item_p->item_id - e->start_id), (int)item_p->item_id);
	xil_printf("%08x : prev[%03d]=%08x next[%03d]=%08x ", item_p,
		     (int)(item_p->prev->item_id), item_p->prev,
		     (int)(item_p->next->item_id), item_p->next);
	if (item_p->prev == &e->base) {
	    xil_printf( "(prev is base) ");
	}
	if (item_p->next == &e->base) {
	    xil_printf("(next is base) ");
	}
	xil_printf(EOL_CRLF);

#endif

	if (n > 16) {
	    return;
	}
	item_p = item_p->next;
    }

    return;
}

/**
 * @fn int multios_stl_list_insert( multios_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element )
 * @brief 双方向キューの指定されたエレメント番号の直前に、データを挿入します.
 *	numに0を指定した場合は、multios_listue_push_front()と同等の動作をします。
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EFAULT el_pに指定されたアドレスがNULLだった
 * @retval EINVAL sizof_elementのサイズが異なる(小さい）
 * @retval EAGAIN リソースの獲得に失敗
 */
int multios_stl_list_insert(multios_stl_list_t *const self_p,
			     const size_t num, const void *const el_p,
			     const size_t sizof_element)
{
    int status;
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict f, *__restrict front;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MULTIOS_XTOA_DEC64_BUFSZ;
	char xtoa_buf_a[MULTIOS_XTOA_DEC64_BUFSZ];
	char xtoa_buf_b[MULTIOS_XTOA_DEC64_BUFSZ];

	multios_u64toadec( (uint64_t)num, xtoa_buf_a, xtoa_bufsz);
	multios_u64toadec( (uint64_t)e->cnt, xtoa_buf_b, xtoa_bufsz);
	DBMS3(stderr, "multios_stl_list_insert : execute" EOL_CRLF);
	DBMS3(stderr, "multios_stl_list_insert : num=%s e->cnt=%s" EOL_CRLF,
	    xtoa_buf_a, xtoa_buf_b);
    }

    if (e->sizof_element != sizof_element) {

	IFDBG3THEN {
	    const size_t xtoa_bufsz=MULTIOS_XTOA_DEC64_BUFSZ;
	    char xtoa_buf_a[MULTIOS_XTOA_DEC64_BUFSZ];
	    char xtoa_buf_b[MULTIOS_XTOA_DEC64_BUFSZ];

	    multios_u64toadec( (uint64_t)e->sizof_element, xtoa_buf_a, xtoa_bufsz);
	    multios_u64toadec( (uint64_t)sizof_element, xtoa_buf_b, xtoa_bufsz);
	    DBMS3(stderr,
	      "multios_stl_list_insert : e->sizof_element=%s sizof_element=%s" EOL_CRLF, xtoa_buf_a, xtoa_buf_b);
	}
	return EINVAL;
    }

    front = list_search_item(e, num);
    if (NULL == front) {
	if ((num == 0) && (e->cnt == 0)) {
	    front = &e->base;
	} else {
	    return ENOENT;
	}
    }

    DBMS3(stderr, "multios_stl_list_insert : front=0x%p e->base=0x%p" EOL_CRLF,
	  front, &e->base);

    f = (queitem_t *) multios_malloc(sizeof(queitem_t) + e->sizof_element);
    if (NULL == f) {
	DBMS1(stderr, "multios_stl_list_push : multios_malloc(queitem_t) fail" EOL_CRLF);
	status = EAGAIN;
	goto out;
    }

    memset(f, 0x0, sizeof(queitem_t));
    multios_memcpy(f->data, el_p, e->sizof_element);

    f->prev = front->prev;
    f->next = front;

    if ((num == 0) && (e->cnt != 0)) {
	f->item_id = (front->item_id) - 1;
	e->start_id = f->item_id;
    } else {
	queitem_t *__restrict p = front;
	f->item_id = front->item_id;
	/* 挿入したエレメント以降のidの再割当 */
	while (p != &e->base) {
	    ++(p->item_id);
	    p = p->next;
	}
    }

    /* 挿入処理 */
    f->prev->next = f;
    f->next->prev = f;
    e->cnt++;

    status = 0;

  out:
    if (status) {
	if (NULL != f) {
	    multios_free(f);
	}
    }
    return status;
}


/**
 * @fn int multios_stl_list_overwrite_element_at( multios_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
 * @brief キューに保存されているエレメントを上書きしますす
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param num 0から始まるキュー先頭からのエレメント配列番号
 * @param el_p エレメントデータポインタ
 * @param sizof_element エレメントサイズ(主にエレメントサイズ検証向け)
 * @retval 0 成功
 * @retval EACCES キューにエレメントがない
 * @retval ENOENT 指定されたエレメントが無い
 * @retval EINVAL 引数が不正
 */

int multios_stl_list_overwrite_element_at( multios_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict item_p;

    if (multios_stl_list_is_empty(self_p)) {
	return EACCES;
    }

    if (NULL == el_p) {
	return EINVAL;
    }

    if (sizof_element < e->sizof_element) {
	return EINVAL;
    }

    if (!(num < e->cnt)) {
	return ENOENT;
    }

    item_p = list_search_item(e, num);
    if (item_p == NULL) {
	return -1;
    }

    multios_memcpy(item_p->data, el_p, e->sizof_element);

    return 0;
}

/**
 * @fn int multios_stl_list_front( multios_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element)
 * @brief 先頭に保存されたエレメントを返します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int multios_stl_list_front( multios_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }
    tmp = e->base.next;

    multios_memcpy(el_p, tmp->data, e->sizof_element);

    return 0;
}


/**
 * @fn int multios_stl_list_back( multios_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
 * @brief 最後尾に保存されたエレメントを返します
 * @param self_p multios_stl_list_t構造体インスタンスポインタ
 * @param el_p 取得する要素のバッファポインタ
 * @param sizof_element 要素サイズ
 * @retval 0 成功
 * @retval EINVAL 要素サイズが異なる
 * @retval ENOENT 先頭に保存された要素がない
 * @retval -1 その他の致命的なエラー
 **/
int multios_stl_list_back( multios_stl_list_t *const self_p, void *const el_p, const size_t sizof_element)
{
    multios_stl_list_ext_t *const e = get_stl_list_ext(self_p);
    queitem_t *__restrict tmp = NULL;

    /* エレメントがあるかどうか二重チェック */
    if (e->cnt == 0) {
	return ENOENT;
    } else if( e->sizof_element != sizof_element ) {
	return EINVAL;
    }
    tmp = e->base.prev;

    multios_memcpy(el_p, tmp->data, e->sizof_element);

    return 0;
}

