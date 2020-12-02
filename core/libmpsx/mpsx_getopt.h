#ifndef INC_MPSX_GETOPT_H
#define INC_MPSX_GETOPT_H

#pragma once

/**
 * @var enum_mpsx_getopt_long_has_arg_t
 * @brief オプションの引数を表す要素
 */
typedef enum _enum_mpsx_getopt_long_has_arg {
    MPSX_GETOPT_NO_ARGUMENT = 0,			/*!< オプションは引数をとらない */
    MPSX_GETOPT_REQUIRED_ARGUMENT = 1,		/*!< オプションは引数を必要とする */
    MPSX_GETOPT_OPTIONAL_ARGUMENT = 2,		/*!< オプションは引数をとってもとらなくても良い */
    MPSX_GETOPT_eot = -1				/*!< オプション配列終端用 */
} enum_mpsx_getopt_long_has_arg_t;

/**
 * @var mpsx_getopt_long_option_t
 * @brief mpsx_getopt_long()に渡すオプションキー
 * 	※ 配列の最後の要素は、全て NULLまたは0 で埋める
 */
typedef struct _mpsx_getopt_long_option {
    const char *name;				/*!< 長いオプションの名前 */
    enum_mpsx_getopt_long_has_arg_t has_arg; /*!< オプションの取り方 */
    int *flag_p;				/*!< NULLの場合val値を返す。
    						     NULL以外は関数に0を返し,このポインタの変数にval値を入する */
    int val;					/*!< flag がしめすポインタに代入する値 */
    const char *memo;				/*!< 使い方とかメモ(関数挙動に特に意味はない) */
} mpsx_getopt_long_option_t;

/**
 * @var mpsx_getopt_t
 * @brief mpsx_getopt????を利用する為のオブジェクト構造体
 */
typedef struct _mpsx_getopt {
    /* public */
    int argc;		/*!< 引数の数 */
    int optind;		/*!< 次に処理される要素のインデックス */
    int optopt;		/*!< エラーになったオプション文字コード */
    const char *optarg; /*!< オプションに引数がある場合の文字列の先頭ポインタ(ない場合はNULL) */

    /* private */
    int _sp;
} mpsx_getopt_t;

#if defined(__cplusplus)
extern "C" {
#endif

mpsx_getopt_t mpsx_getopt_initializer(void);

int mpsx_getopt(mpsx_getopt_t *const self_p, const int argc, const char *const argv[], const char *optstring);

int mpsx_getopt_long(mpsx_getopt_t *const self_p, const int argc, const char *const argv[], const char *const optstring, const mpsx_getopt_long_option_t *const opts, int *const index_p);

int mpsx_getopt_long_only(mpsx_getopt_t *const self_p, const int argc, const char *const argv[], const char *const optstring, const mpsx_getopt_long_option_t *const lopts, int *const index_p);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_GETOPT_H */
