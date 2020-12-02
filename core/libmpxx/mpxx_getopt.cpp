/**
 *	Copyright 2012 TSNｰSHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-December-31 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_getopt.c
 * @brief POSIX getopt()のフルスクラッチ版
 *	文字及び単語によるコマンドラインオプション解析関数 getopt()エミュレーション関数
 *	POSIXのgetopt()に干渉しない作りになっています。
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


/* this */
#include "mpxx_stdlib.h"
#include "mpxx_getopt.h"

#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MPXX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

#define BADCHAR ('?')
#define BADARG (':')
#define RETFINI (-1)
#define RETHYPHEN (-2)

#if 0
static const char *const null_msg = "";
#endif

typedef enum _enum_getopt_type {
    getopt_type_is_long,
    getopt_type_is_long_only
} enum_getopt_type_t;


static int own_getopt_exec(mpxx_getopt_t * const __restrict self_p,
			   const int argc, const char *const argv[],
			   const char *const ostr);
static int own_getopt_long_exec(mpxx_getopt_t * const __restrict self_p,
				const enum_getopt_type_t getopt_type,
				const int argc, const char *const argv[],
				const char *const optstring,
				const mpxx_getopt_long_option_t * const lopts,
				int *const index_p);

/**
 * @fn mpxx_getopt_t mpxx_getopt_initializer(void)
 * @brief mpxx_getopt_t構造体インスタンスを初期化する為に初期値を代入します。
 * @return mpxx_getopt_t構造体初期値
 */
mpxx_getopt_t mpxx_getopt_initializer(void)
{
    mpxx_getopt_t ret;

    ret.argc = -1;
    ret.optopt = 0;
    ret.optind = 1;
    ret.optarg = NULL;
    ret._sp = 1;

    return ret;
}

/**
 * @fn static int own_getopt_exec(mpxx_getopt_t * const self_p, int argc, char *argv[], const char *const ostr)
 * @brief コマンドライン引数解析を行う（内部での実体）
 * @param self_p mpxx_getopt_t構造体インスタンスポインた
 * @param argc 引き数数
 * @param argv 引数配列ポインタ
 *	※ argv の要素のうち '-' で始まるものはオプション要素 (option element) とみなされる。
 *	    この要素から先頭の '-' 及びオプション要素文字を除いた文字は オプション文字 (option character) とされる。
 *	    また、オプション要素は空白文字を加えて次の引数ようそとすることができる。
 * @param optstring 受け付けるるオプション文字からなる文字列 Linux等のgetopt()を参照してください。
 *	※ NULL指定の場合は '--'の検出のみおこなう
 * @retval 0以上 オプション文字コード
 * @retval -1(RETFINI) 解析の終了を示す
 * @retval -2(RETHYPHEN) '--'が検出された(ostrにNULLが指定された場合は'-'も同様)
 */
static int own_getopt_exec(mpxx_getopt_t * const __restrict self_p,
			   const int argc, const char *const argv[],
			   const char *const ostr)
{
    /**
     * This code is public document which was powered by AT&T
     * custmized by Seiichi.Takeda Vw Dep.
     */

    int c;
    const char *cp;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

	    mpxx_i64toadec( (int64_t)argc, xtoa_buf, xtoa_bufsz);
	DBMS3(stderr, "own_getopt_exec : argc=%s ostr=%s" EOL_CRLF,
	    xtoa_buf, ostr);
    }

    if ((self_p->argc < 0) || (self_p->_sp == 1)) {
	if (self_p->optind >= argc || (argv[self_p->optind][0] != '+' &&
				       argv[self_p->optind][0] != '-')
	    || argv[self_p->optind][1] == '\0') {
	    return RETFINI;
	} else if (strcmp(argv[self_p->optind], "--") == 0) {
	    DBMS3(stderr, "found --" EOL_CRLF);
	    ++(self_p->optind);
	    return RETHYPHEN;
	}
#if 0
	/* '+' for config options, '+' should not be in the opts list */
	if (argv[optind][0] == '+') {
	    optarg = argv[optind++] + 1;
	    return '+';
	}
#endif
    }

    if (NULL == ostr) {
	self_p->_sp = 1;
	return RETHYPHEN;
    }

    self_p->optopt = c = argv[self_p->optind][self_p->_sp];
    if (c == ':' || (cp = strchr(ostr, c)) == NULL) {
	DBMS5(stderr, "own_getopt_exec : illegal option -- %c" EOL_CRLF,
	      self_p->optopt);
	if (argv[self_p->optind][++self_p->_sp] == '\0') {
	    ++(self_p->optind);
	    self_p->_sp = 1;
	}
	return BADCHAR;
    }

    if (*++cp == ':') {
	if (argv[self_p->optind][(self_p->_sp) + 1] != '\0') {
	    self_p->optarg = &argv[(self_p->optind)++][(self_p->_sp) + 1];
	} else if (++(self_p->optind) >= argc) {
	    DBMS1(stdout,
		  "own_getopt_exec : '%c' option requires an argument -- " EOL_CRLF,
		  c);
	    self_p->_sp = 1;
	    return BADARG;
	} else {
	    self_p->optarg = argv[(self_p->optind)++];
	}

	self_p->_sp = 1;
    } else {
	if (argv[self_p->optind][++(self_p->_sp)] == '\0') {
	    self_p->_sp = 1;
	    self_p->optind++;
	}
	self_p->optarg = NULL;
    }

    return c;
}


/**
 * @fn int mpxx_getopt(mpxx_getopt_t * self_p, int argc, char *argv[], const char *optstring)
 * @brief コマンドライン引数解析を行う
 * @param self_p mpxx_getopt_t構造体インスタンスポインた
 * @param argc 引き数数
 * @param argv 引数配列ポインタ
 *	※ argv の要素のうち '-' で始まるものはオプション要素 (option element) とみなされる。
 *	    この要素から先頭の '-' 及びオプション要素文字を除いた文字は オプション文字 (option character) とされる。
 *	    また、オプション要素は空白文字を加えて次の引数ようそとすることができる。
 * @param optstring 受け付けるるオプション文字からなる文字列 Linux等のgetopt()を参照してください。
 * @retval 0以上 オプション文字コード
 * @retval -1(RETFINI) 解析の終了を示す
 */
int mpxx_getopt(mpxx_getopt_t *const self_p, const int argc, const char *const argv[],
		   const char *optstring)
{
    int retval;

    if ((retval =
	 own_getopt_exec(self_p, argc, argv, optstring)) == RETHYPHEN) {
	retval = RETFINI;
	++(self_p->optind);
    }
    return retval;
}


/**
 * @fn static int own_getopt_long_exec(mpxx_getopt_t * const self_p, const enum_getopt_long_type_t long_type, int argc, const char * const argv[], const char * const optstring, mpxx_getopt_long_option_t * const lopts, int *index_p);
 * @brief 単語対応のコマンド引数解析をおこなう（内部処理関数)
 * @param self_p mpxx_getopt_t構造体インスタンスポインタ
 * @param is_only own_getopt_exec()を先の呼ぶか後に呼ぶかを決定する( 0:先 0以外:後）
 * @param argc 引き数数
 * @param argv 引数配列ポインタ
 *	※ argv の要素のうち '-' で始まるもの (かつ "-" 単独や "--" 単独ではないもの) は オプション要素 (option element) とみなされる。
 *	  この要素から先頭の '-' を除いた文字は オプション文字 (option character) とされる。
 * @param optstring 受け付けるオプション文字からなる文字列
 *	※ 指定無しの場合 NULL では無く ""を指定
 *	※ 文字のあとにコロン (:) が置かれている場合は、オプションには引き数が必要であることを示す。
 *	  このとき getopt() は、現在注目している argv 要素で、オプション文字に引き続くテキストへのポインタか、あるいは次の argv 要素のテキストへのポインタを optarg に代入する。 
 *	  2 個連続してコロン(:)が置かれている場合は、 そのオプションは引き数をとってもとらなくてもよい。
 *	  ?は予約されているので使えない
 * @param longopts 単語型キーワード配列ポインタ
 * @param longindex NULL以外でlongoptsで指定された長いオプションのインデックスを longopts からの相対位置として返す変数ポインタ
 * @retval 1以上 optstringのキーワード文字が見つかったらその文字番号
 * @retval 0 検出されたキーワード文字列のパラメータにflagがNULL以外の場合はval値を返す
 * @retval -1(RETFINI) 解析の終了を示す
 */
static int own_getopt_long_exec(mpxx_getopt_t * const __restrict self_p,
				const enum_getopt_type_t getopt_type,
				const int argc, const char *const argv[],
				const char *const optstring,
				const mpxx_getopt_long_option_t * const lopts,
				int *const index_p)
{
    int retval;

    if (!(self_p->optind < argc)) {
	IFDBG5THEN {
	    const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
	    char xtoa_buf_a[MPXX_XTOA_DEC64_BUFSZ];
	    char xtoa_buf_b[MPXX_XTOA_DEC64_BUFSZ];

	    mpxx_i64toadec( (int64_t)self_p->optind, xtoa_buf_a, xtoa_bufsz);
	    mpxx_i64toadec( (int64_t)argc, xtoa_buf_b, xtoa_bufsz);
	    DBMS5(stderr, "s->optind=%s argc=%s, argment is finish" EOL_CRLF,
		xtoa_buf_a, xtoa_buf_b);
	}

	self_p->_sp = 1;
	return RETFINI;
    }

    if (getopt_type == getopt_type_is_long) {
	retval = own_getopt_exec(self_p, argc, argv, optstring);
    } else {
	if (*argv[self_p->optind] != '-') {
	    return RETFINI;
	}
	retval = -1;
    }
    if ((retval == RETHYPHEN) || (getopt_type == getopt_type_is_long_only)) {
	const char *cur_argv = argv[self_p->optind], *has_equal;
	int match;
	size_t i;
	size_t cur_argv_len;

	DBMS3(stderr, "getopt_long_exec : cur_argv = %s" EOL_CRLF, cur_argv);

	for (i = 0; i < 2; ++i) {
	    if (*cur_argv == '-') {
		++cur_argv;
	    } else {
		break;
	    }
	}

	DBMS3(stderr, "cur_argv = %s" EOL_CRLF, cur_argv);
	if (*cur_argv == '\0') {
	    return RETFINI;
	}
	if ((has_equal = strchr(cur_argv, '=')) != NULL) {
	    cur_argv_len =
		(size_t) ((uintptr_t) has_equal - (uintptr_t) cur_argv);
	    ++has_equal;
	} else {
	    cur_argv_len = strlen(cur_argv);
	}

	if ((NULL != lopts) && (NULL != lopts[0].name)) {
	    for (match = 0; NULL != lopts[match].name; ++match) {
		if (strncmp(cur_argv, lopts[match].name, cur_argv_len)) {
		    continue;
		}
		if (strlen(lopts[match].name) == cur_argv_len) {
		    break;
		}
	    }
	    if (NULL == lopts[match].name) {
		match = -1;
	    }
	} else {
	    match = -1;
	}
	if (match != -1) {
	    const mpxx_getopt_long_option_t *t = &lopts[match];
	    IFDBG3THEN {
		const size_t xtoa_bufsz=MPXX_XTOA_DEC64_BUFSZ;
		char xtoa_buf[MPXX_XTOA_DEC64_BUFSZ];

		mpxx_i64toadec( (int64_t)match, xtoa_buf, xtoa_bufsz);
		DBMS3(stderr, "match : %s %s"EOL_CRLF,
		    xtoa_buf, t->name);
	    }
	    ++(self_p->optind);
	    switch (t->has_arg) {
	    case MPXX_GETOPT_NO_ARGUMENT:
		self_p->optarg = NULL;
		break;
	    case MPXX_GETOPT_OPTIONAL_ARGUMENT:
	    case MPXX_GETOPT_REQUIRED_ARGUMENT:
		if (has_equal) {
		    self_p->optarg = has_equal;
		} else {
		    self_p->optarg = argv[self_p->optind];
		    ++(self_p->optind);
		}

		if (t->has_arg == MPXX_GETOPT_REQUIRED_ARGUMENT) {
		    if (self_p->optarg == NULL) {
			if (*optstring != ':') {
			    DBMS5(stderr,
				  "option requires an argument -- %s" EOL_CRLF,
				  cur_argv);
			    return BADARG;
			}
		    }
		}
		break;
	    case MPXX_GETOPT_eot:
		return BADARG;
	    }
	} else {
	    /* was not matched */
	    if (getopt_type == getopt_type_is_long_only) {
		if ((retval =
		     own_getopt_exec(self_p, argc, argv,
				     optstring)) == RETHYPHEN) {
		    retval = RETFINI;
		}
		++(self_p->optind);
		return retval;
	    } else {
		if (*optstring != ':') {
		    DBMS5(stderr, "illegal option -- %s" EOL_CRLF, cur_argv);
		}
		++(self_p->optind);
		return BADCHAR;
	    }
	}
	if (NULL != lopts[match].flag_p) {
	    *lopts[match].flag_p = lopts[match].val;
	    retval = 0;
	} else {
	    retval = lopts[match].val;
	}
	if (NULL != index_p) {
	    *index_p = match;
	}
    }
    return retval;
}

/**
 * @fn int mpxx_getopt_long(mpxx_getopt_t *self_p, int argc, char *argv[], const char *optstring, mpxx_getopt_long_option_t *lopts, int *index_p)
 * @brief 単語対応のコマンド引数解析をおこなう
 * @param self_p mpxx_getopt_t構造体インスタンスポインタ
 * @param argc 引き数数
 * @param argv 引数配列ポインタ
 *	※ argv の要素のうち '-' で始まるもの (かつ "-" 単独や "--" 単独ではないもの) は オプション要素 (option element) とみなされる。
 *	    この要素から先頭の '-' を除いた文字は オプション文字 (option character) とされる。
 * @param optstring 受け付けるオプション文字からなる文字列
 *	※ 指定無しの場合 NULL では無く ""を指定
 *	※ 文字のあとにコロン (:) が置かれている場合は、オプションには引き数が必要であることを示す。
 *	  このとき getopt() は、現在注目している argv 要素で、オプション文字に引き続くテキストへのポインタか、あるいは次の argv 要素のテキストへのポインタを optarg に代入する。 
 *	  2 個連続してコロン(:)が置かれている場合は、 そのオプションは引き数をとってもとらなくてもよい。
 *	  ?は予約されているので使えない
 * @param lopts 単語型キーワード配列ポインタ
 * @param index_p NULL以外でlongoptsで指定された長いオプションのインデックスを longopts からの相対位置として返す変数ポインタ
 * @retval 1以上 optstringのキーワード文字が見つかったらその文字番号
 * @retval 0 検出されたキーワード文字列のパラメータにflagがNULL以外の場合はval値を返す
 * @retval -1 解析の終了を示す
 */
int mpxx_getopt_long(mpxx_getopt_t *const self_p, const int argc, const char *const argv[],
			const char *const optstring,
			const mpxx_getopt_long_option_t *const lopts, int *const index_p)
{
    return own_getopt_long_exec(self_p, getopt_type_is_long, argc, argv,
				optstring, lopts, index_p);
}

/**
 * @fn int mpxx_getopt_long_only(mpxx_getopt_t *self_p, int argc, char *argv[], const char *optstring, mpxx_getopt_long_option_t *lopts, int *index_p)
 * @brief 単語対応のコマンド引数解析をおこなう
 *	基本的にmpxx_getopt_long()と同じなので、 詳細はmpxx_getopt_longを参照。
 *	異なる点は '--'と'-'が同じに扱われること。長いオプションは先に評価され、次に短いオプションを評価する.
 * @param self_p mpxx_getopt_t構造体インスタンスポインタ
 * @param argc 引き数数
 * @param argv 引数配列ポインタ
 *	※ argv の要素のうち '-' で始まるもの (かつ "-" 単独や "--" 単独ではないもの) は オプション要素 (option element) とみなされる。
 *	   この要素から先頭の '-' を除いた文字は オプション文字 (option character) とされる。
 * @param optstring 受け付けるオプション文字からなる文字列
 *	※ 指定無しの場合 NULL では無く ""を指定
 *	※ 文字のあとにコロン (:) が置かれている場合は、オプションには引き数が必要であることを示す。
 *	  このとき getopt() は、現在注目している argv 要素で、オプション文字に引き続くテキストへのポインタか、あるいは次の argv 要素のテキストへのポインタを optarg に代入する。 
 *	  2 個連続してコロン(:)が置かれている場合は、 そのオプションは引き数をとってもとらなくてもよい。
 *	  ?は予約されているので使えない
 * @param lopts 単語型キーワード配列ポインタ
 * @param index_p NULL以外でlongoptsで指定された長いオプションのインデックスを longopts からの相対位置として返す変数ポインタ
 * @retval 1以上 optstringのキーワード文字が見つかったらその文字番号
 * @retval 0 検出されたキーワード文字列のパラメータにflagがNULL以外の場合はval値を返す
 * @retval -1 解析の終了を示す
 */
int mpxx_getopt_long_only(mpxx_getopt_t *const self_p, const int argc,
			     const char *const argv[], const char *const optstring,
			     const mpxx_getopt_long_option_t *const lopts,
			     int *const index_p)
{
    return own_getopt_long_exec(self_p, getopt_type_is_long_only, argc,
				argv, optstring, lopts, index_p);
}
