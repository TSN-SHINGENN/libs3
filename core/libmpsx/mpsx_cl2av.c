/**
 *	Copyright 2012 TSNｰSHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-September-23 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_cl2av.cpp
 *   コマンドライン引数分割ライブラリ
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* this */
#include "mpsx_sys_types.h"
#include "mpsx_malloc.h"
#include "mpsx_stl_vector.h"

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#include "mpsx_cl2av.h"

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _cl2av_ext {
    char *delim;		/*!< デリミタのASCIIバイト集合 */
    char delim2;		/*!< 囲い込みASCII文字('\0'で無視) */
    char *work_buf;		/*!< 作業バッファ */
    mpsx_stl_vector_t argvec;

    union {
	unsigned int flags;
	struct {
	    unsigned int argvec:1;
	} f;
    } init;
} cl2av_ext_t;

typedef struct _own_strtok {
    char *strtok_pbuf;
    mpsx_cl2av_attr_t attr;

    union {
	unsigned int flags;
	struct {
	    unsigned int initial:1;
	    unsigned int final:1;
	} f;
    } stat;
} own_strtok_t;


#if defined(_LIBS3_MICROCONTROLLER_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

static own_strtok_t own_strtok_reset(const mpsx_cl2av_attr_t attr)
	__attribute__ ((optimize("Os")));

static int own_strtok(own_strtok_t * const __restrict self_p,
		      char **result_pointer, char *s, const char *delim,
		      const char delim2)
	__attribute__ ((optimize("Os")));

static int own_strtok_is_eol(own_strtok_t * const __restrict self_p, const char*const delim)
	__attribute__ ((optimize("Os")));

extern int mpsx_cl2av_init(mpsx_cl2av_t *const self_p, const char *const delim, const char delim2, const mpsx_cl2av_attr_t *const  attr_p)
	__attribute__ ((optimize("Os")));
extern int mpsx_cl2av_execute( mpsx_cl2av_t *const self_p, const char *const sentence)
	__attribute__ ((optimize("Os")));
extern void mpsx_cl2av_destroy( mpsx_cl2av_t *const self_p)
	__attribute__ ((optimize("Os")));

extern int mpsx_cl2av_lite( char * const __restrict str, const char * __restrict delim, char delim2, int *const __restrict argc_p, char ** __restrict argv_p, const unsigned int limit, mpsx_cl2av_attr_t *const attr_p )
	__attribute__ ((optimize("Os")));

#else /* standard static prototype */
static own_strtok_t own_strtok_reset(const mpsx_cl2av_attr_t attr);

static int own_strtok(own_strtok_t * const __restrict self_p,
		      char **result_pointer, char *s, const char *delim,
		      const char delim2);
static int own_strtok_is_eol(own_strtok_t * const __restrict self_p, const char*const delim);
#endif /* end of _LIBS3_MICROCONTROLLER_ENABLE_GCC_MINIMUM_OPTIMIZE */

/**
 * @fn int mpsx_cl2av_init(mpsx_cl2av_t *const self_p, const char *const delim, const char delim2, const mpsx_cl2av_attr_t *const  attr_p)
 * @brief gof_cmd_makeargvオブジェクトを初期化します
 * @param self_p mpsx_cl2av_tインスタンスポインタ
 * @param delim デリミタのASCII文字集合列( NULL で " "と等価 )
 * @param delim2 囲い文字ASCII文字('\0'で無視)
 * @param attr_p mpsx_cl2av_attr_t属性ポインタ(NULLでデフォルト動作)
 * @retval 0 成功
 * @retval EACCES デリミタ2に不正な文字が設定された
 * @retval EINVAL パラメータ不正
 * @retval ENOMEM リソース不足
 */
int mpsx_cl2av_init(mpsx_cl2av_t *const self_p, const char *const delim, const char delim2, const mpsx_cl2av_attr_t *const  attr_p)
{
    int status, result;
    cl2av_ext_t *e;

    memset(self_p, 0x0, sizeof(mpsx_cl2av_t));

    if (NULL != attr_p) {
	self_p->attr = *attr_p;
    } else {
	self_p->attr.flags = 0;
    }

    e = (cl2av_ext_t *)
	mpsx_malloc(sizeof(cl2av_ext_t));
    if (NULL == e) {
	status = ENOMEM;
	goto out;
    }
    memset(e, 0x0, sizeof(cl2av_ext_t));
    self_p->ext = (void *) e;

    result = mpsx_stl_vector_init(&e->argvec, sizeof(char *));
    if (result) {
	status = result;
	goto out;
    } 
    e->init.f.argvec = 1;

    result = mpsx_stl_vector_reserve(&e->argvec, 16);
    if (result) {
	status = result;
	goto out;
    }

    if (delim2 != '\0') {
	if (!(isalnum((int) delim2) || ispunct((int) delim2))) {
	    DBMS3(stderr, "mpsx_cl2av_init : delim2 is not ascii=%c" EOL_CRLF, delim2);
	    status = EACCES;
	    goto out;
	}
	e->delim2 = delim2;
    }

    if (NULL == delim) {
	const char *const std_delim = " ";
	e->delim = (char *) mpsx_malloc(1 + strlen(std_delim));
	if (NULL == e->delim) {
	    status = ENOMEM;
	    goto out;
	}
	strcpy(e->delim, std_delim);
    } else {
	e->delim = (char *) mpsx_malloc(1 + strlen(delim));
	if (NULL == e->delim) {
	    status = ENOMEM;
	    goto out;
	}
	strcpy(e->delim, delim);
    }

    status = 0;
  out:
    if (status) {
	mpsx_cl2av_destroy(self_p);
    }
    return status;
}

/**
 * @fn void mpsx_cl2av_destroy(mpsx_cl2av_t *const self_p)
 * @brief gof_cmd_makeargvオブジェクトを破棄します
 * @param self_p mpsx_cl2av_tインスタンスポインタ
 */
void mpsx_cl2av_destroy(mpsx_cl2av_t *const self_p)
{
	cl2av_ext_t *const __restrict e =
		(cl2av_ext_t *)self_p->ext;;


    if (e->init.f.argvec) {
	mpsx_stl_vector_destroy(&e->argvec);
	e->init.f.argvec = 0;
    }

    if (NULL != e->delim) {
	mpsx_free(e->delim);
    }
    if (NULL != e->work_buf) {
	mpsx_free(e->work_buf);
    }

    if (e->init.flags) {
	DBMS1(stderr,
	      "mpsx_cl2av_destroy : e->init.flags=0x%llx" EOL_CRLF,
	      (long long)e->init.flags);
	return;
    }

    if (NULL != self_p->ext) {
	mpsx_free(self_p->ext);
	self_p->ext = NULL;
    }

    return;
}

/**
 * @fn static own_strtok_t own_strtok_reset( const mpsx_cl2av_attr_t attr)
 * @brief own_strtokオブジェクトを初期化します
 * @return 初期化されたown_strtok_tインスタンス
 */
static own_strtok_t own_strtok_reset( const mpsx_cl2av_attr_t attr)
{
    own_strtok_t ret;

    ret.strtok_pbuf = NULL;
    ret.attr = attr;
    ret.stat.flags = 0;

    return ret;
}

/**
 * @fn static int own_strtok( own_strtok_t *self_p, char **result_pointer, char *s, const char *delim, const char delim2)
 * @brief デリミタを認識してNULLで区切りそのトークンとします。次のトークンのポインタを返します。
 *	delim2で囲まれた部分はそれを一つのトークンとして区切ります
 * @param self_p own_strtokインスタンスポインタ
 * @param result_pointer 次のトークンへのポインタまた、 次のトークンがなければ NULL を返す。ポインタ
 * @param s 解析対象の文字列バッファポインタ(二個目以降のトークンの解析にはNULLを指定します）
 *	指定されたバッファーはトークンに分割するためデータを変更します
 * @param delim デリミタのASCII文字集合列
 * @param delim2 囲い文字ASCII文字('\0'で無視)
 * @retval 0 成功
 * @retval EINVAL 引数が不正
 * @retval EIO 囲い文字の終端が見つからない
 */
static int own_strtok(own_strtok_t * const __restrict self_p,
		      char **result_pointer, char *s, const char *delim,
		      const char delim2)
{
    int status;
    enum _enum_fence_mode {
	IN_FENCE,
	NOT_FENCE
    } mode;
    size_t n, remain_length;
    char *delim_point, *delim2_point;

    /* inital */
    delim_point = delim2_point = NULL;

    if (!self_p->stat.f.initial) {
	if (NULL == s) {
	    return EINVAL;
	}
	self_p->strtok_pbuf = s;
	self_p->stat.f.initial = 1;
	DBMS5(stderr, "own_strtok : now = %p" EOL_CRLF, self_p->strtok_pbuf);
	DBMS5(stderr, "own_strtok : inital = '%s'" EOL_CRLF, self_p->strtok_pbuf);
	if((delim2 != '\0') && (NULL!=strchr(delim, delim2))) {
	    /* 囲い文字とデリミタが重なっている */
  	    return EINVAL;
	}
    } else {
	DBMS5(stderr, "own_strtok : now(ptr:%p) = '%s'" EOL_CRLF, self_p->strtok_pbuf, self_p->strtok_pbuf);
    }

    /* 囲い文字が設定されていて、先頭から囲い文字が出てきた場合は先方の区切り文字飛ばしは行わない */
    if(('\0' != *self_p->strtok_pbuf) && (delim2 != *self_p->strtok_pbuf)) {
	char * __restrict p = self_p->strtok_pbuf;
	int a, b, c;
	DBMS5(stderr, "own_strtok : sentence is detected delimiter, str=%s" EOL_CRLF, p);

	/* 先頭から終端文字・囲い文字 ではなく、区切り文字の場合はインクリメントして飛ばす */
	while(((a=('\0' != *p)) && (b=( delim2 != *p ))) && (c=(NULL != strchr(delim, *p)))) {
	    /* 空白文字の扱いが特殊なので検出時は脱出する */
	    if (!(self_p->attr.f.untrim_space) && (' ' == *p) ) {
		break;
	    }
	    ++p;
	}
	self_p->strtok_pbuf = p;

	/* すでに終端なら抜ける */
	if( '\0' == *(self_p->strtok_pbuf) ) {
	    DBMS5(stderr, "own_strtok : detected eod" EOL_CRLF);
	    *result_pointer = NULL;
	    status = 0;
	    goto out;
	}
    }

    /* 先頭のデリミタを飛ばしたら残りの文字数を数える */
    remain_length = strlen(self_p->strtok_pbuf);
    if (remain_length == 0) {
	DBMS5(stderr, "own_strtok : nothing reamin word" EOL_CRLF);
	*result_pointer = NULL;
	status = 0;
	goto out;
    }

    /* フラグが立ってた場合スペースを除外する */
    if (!(self_p->attr.f.untrim_space)) {
	for (n = 0; n < remain_length; ++n) {
	    if (delim2 == *self_p->strtok_pbuf) {
		break;
	    } else if (' ' != *(self_p->strtok_pbuf)) {
		break;
	    }

	    ++(self_p->strtok_pbuf);
	}
    } else {
	n = 0;
    }

    DBMS5(stderr, "own_strtok : p1='%s'" EOL_CRLF, self_p->strtok_pbuf);

    /* 次のデリミタの位置(トークンの終わり)を探す */
    delim_point = strpbrk(self_p->strtok_pbuf, delim);
    if ('\0' != delim2) {
	delim2_point = strchr(self_p->strtok_pbuf, delim2);
    }
    if ((NULL == delim_point) && (NULL == delim2_point)) {
	*result_pointer = self_p->strtok_pbuf;
	self_p->strtok_pbuf += (remain_length - n);	/* Trimした分を引く */
	status = 0;
	goto out;
    } else if ((delim_point == NULL)
	       || (delim_point == self_p->strtok_pbuf)
	       || ('\0' == *(self_p->strtok_pbuf))) {
	/* これ以上、トークンは無い */
	*result_pointer = NULL;
	status = 0;
	goto out;
    }

    if (NULL == delim2_point) {
	mode = NOT_FENCE;
    } else if (delim_point < delim2_point) {
	mode = NOT_FENCE;
    } else {
	mode = IN_FENCE;
    }

    DBMS5(stderr, "own_strtok : mode = %d" EOL_CRLF, (int) mode);

    switch (mode) {
    case NOT_FENCE:
	/* 囲い文字で囲まれてなかった場合 */
	*delim_point = '\0';
	*result_pointer = self_p->strtok_pbuf;
	self_p->strtok_pbuf = delim_point + 1;
	break;

    case IN_FENCE:
	/* 囲い文字で囲まれた場合 */

	/* 1.囲い文字の先頭を飛ばす */
	self_p->strtok_pbuf = delim2_point + 1;

	/* 2.囲い文字の終了を探す */
	delim2_point = strchr(self_p->strtok_pbuf, delim2);

	/* 3a. 見つからない場合 */
	if (NULL == delim2_point) {
	    /* エラーリターン */
	    status = EIO;
	    goto out;
	}

	/* 3b. 見つかった場合囲い文字を消す */

	*result_pointer = self_p->strtok_pbuf;
	*delim2_point = '\0';
	self_p->strtok_pbuf = delim2_point + 1;

	/* さらにトークンが続く場合は1文字トークンを飛ばす */
	if( '\0' != *self_p->strtok_pbuf ) {
	    delim_point = strchr((char *) delim, *self_p->strtok_pbuf);
	    if (NULL != delim_point) {
		++(self_p->strtok_pbuf);
	    }
	}
    }

    /* success */
    status = 0;
  out:
    if (status) {
	*result_pointer = NULL;
    }

    return status;
}

/**
 * @fn static int own_strtok_is_eol(own_strtok_t * const __restrict self_p, const char*const delim)
 * @brief 解析処理中の後方にトークンで分割する必要があるかどうかを確認します。
 * @param self_p own_strtokインスタンスポインタ
 * @param トークン分割するための文字種
 * @retval 0 後方も解析する必要有り
 * @retval 0以外 後方はすでに解析する必要なし(終端に達している)
 */
static int own_strtok_is_eol(own_strtok_t * const __restrict self_p, const char*const delim)
{

    if(NULL != self_p->strtok_pbuf) {
	const size_t len = strlen(self_p->strtok_pbuf);
	if(!len) {
	     return 1;
	} else { 
	     char *p = self_p->strtok_pbuf;
	     size_t l;
	     for(l=0; l<len; ++p, ++l) {
	 	if(NULL == strrchr(delim, (int)*p)) {
		      break;
		}
	     }
	     return ( l==len ) ? 1 : 0;
	}
    }

    return 0;
}

/**
 * @fn int mpsx_cl2av_execute( mpsx_cl2av_t *const self_p, const char *const sentence)
 * @brief sentenceをトークンに分割してインスタンスに保存します。
 * @param self_p mpsx_cl2av_tインスタンスポインタ
 * @param sentence 解析元文字列
 * @retval 0 成功
 * @retval ENOMEM リソース不足
 * @retval EINVAL 不正な引数
 * @retval EIO sentence文字列の構文エラー
 * @retval -1 その他の失敗
 */
int mpsx_cl2av_execute( mpsx_cl2av_t *const self_p, const char *const sentence)
{
    cl2av_ext_t *const __restrict e =
	(cl2av_ext_t *) self_p->ext;
    int result;
    own_strtok_t s = own_strtok_reset(self_p->attr);
    void *new_buf;

    if ((NULL == self_p) || (NULL == sentence)) {
	return EINVAL;
    }

    DBMS3(stderr, "mpsx_cl2av_execute : sentence =%s" EOL_CRLF, sentence);

    /* initalize */
    mpsx_stl_vector_clear(&e->argvec);
    mpsx_stl_vector_reserve(&e->argvec, 16);
    self_p->argc = 0;

    new_buf = (char *) mpsx_realloc(e->work_buf, strlen(sentence) + 1);
    if (NULL == new_buf) {
	return ENOMEM;
    }
    e->work_buf = (char *) new_buf;
    strcpy(e->work_buf, sentence);

    for (;;) {
	char *argp;
	result = own_strtok(&s, &argp, e->work_buf, e->delim, e->delim2);
	if (result) {
	    DBMS1(stderr, "mpsx_cl2av_execute : own_strtok fail" EOL_CRLF);
	    return result;
	}

	if (argp == NULL) {
	    break;
	}

	DBMS5(stderr, "mpsx_cl2av_execute : argp(%p)=%s" EOL_CRLF, argp,
	      argp);

	result =
	    mpsx_stl_vector_push_back(&e->argvec, &argp, sizeof(argp));
	if (result) {
	    DBMS1(stderr,
		  "mpsx_cl2av_execute : mpsx_stl_vector_push_back fail" EOL_CRLF);
	    return result;
	}

    }

    self_p->argc = mpsx_stl_vector_size(&e->argvec);
    self_p->argv = (char **)mpsx_stl_vector_ptr_at(&e->argvec, 0);

    return 0;
}

/**
 * @fn int mpsx_cl2av_lite( char * const __restrict str, const char * __restrict delim, char delim2, int *const __restrict argc_p, char ** __restrict argv_p, const unsigned int limit, mpsx_cl2av_attr_t *const attr_p )
 * @brief 文字列を解析して要素に分割します。オブジェクトのいらないlite版です
 *	第一引数strの文字列は解析時に書き換えられます。
 * @param str 要素ごとに分割する文字列バッファです。分割後にバッファ内容を書き換えます
 * @param delim デリミタのASCII文字集合列( NULL で " "と等価 )
 * @param delim2 囲い文字ASCII文字('\0'で無視)
 * @param argc 要素数を返すための変数ポインタ
 * @param argv 要素に分割された文字列のポインタ集合を返します。配列数は次の引数limit分確保してください
 * @param limit 要素分割の最大数を返します。 argv配列を超えて、バッファーオーバランを起こさないためのリミッタです。
 * @retval 0 成功
 * @retval EACCES デリミタ2に不正な文字が設定された
 * @retval EINVAL 引数が不正
 * @retval EOVERFLOW limitを超えて引数解析をしようとした
 * @retval EIO 囲い文字の終端が見つからない
 **/
int mpsx_cl2av_lite( char * const __restrict str, const char * __restrict delim, char delim2, int *const __restrict argc_p, char ** __restrict argv_p, const unsigned int limit, mpsx_cl2av_attr_t *const attr_p )
{
    int result;
    unsigned int n;    
    mpsx_cl2av_attr_t attr;
    own_strtok_t s; 
    char *buf = str;

    if( (NULL == str) || ( NULL == argv_p ) || ( NULL == argc_p ) || ( limit == 0 ) ) {
	return EINVAL;
    }

    if (delim2 != '\0') {
	if (!(isalnum((int) delim2) || ispunct((int) delim2))) {
	    DBMS3(stderr, "mpsx_cl2av_lite : delim2 is not ascii=%c" EOL_CRLF, delim2);
	    return EACCES;
	}
    }

    if (NULL == delim) {
	    char *const std_delim = " ";
	delim = std_delim;
    }

    if( NULL != attr_p ) {
	attr = *attr_p;
    } else {
	attr.flags = 0;
    }
    s = own_strtok_reset(attr);
    
    for( n=0; n<limit; ++n) {
	char *argp;
	result = own_strtok( &s, &argp, buf, delim, delim2);
	if(result) {
	    DBMS1( stderr, "mpsx_cl2av_lite : own_strtok fail, strerror:%s" EOL_CRLF, strerror(result));
	    return result;
	}
	if( argp == NULL ) {
	    break;
	} else {
	    argv_p[n] = argp;
	}
    }

    if( n==limit && own_strtok_is_eol(&s, delim)) {
	return EOVERFLOW;
    }

    *argc_p = n;

    return 0;
}

