/**
 *	Copyright 2016 TSNｰSHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2016-August-07 Active
 *		Last Alteration $Author: takeda $
 **/

/**
 * @file  mpxx_conio.c
 * @brief *** 現在、検証中です ****
 *  標準コンソール入出力用の関数郡。ヘッダファイル名はMFCに似せてあります。
 *  WIN32の場合には、標準入力からコンソール入力をもらえるとは限らないので独立した形式を取ります。
 **/

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* CRL & POSIX */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <conio.h>
#endif

/* this */
#include "mpxx_sys_types.h"
#include "mpxx_stdlib.h"
#include "mpxx_ctype.h"
#include "mpxx_malloc.h"
#include "mpxx_conio.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/**
 * @fn int mpxx_conio_getchar(void)
 * @brief コンソール(stdin)からの次の文字をunsigned charとして返します。
 *	intにキャストして戻します。
 *	ファイルの終わりやエラーの場合はEOFを返します。
 */
int mpxx_conio_getchar(void)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    struct termios cur, alt;
    char char_cno;

	tcgetattr(STDIN_FILENO, &cur);
	alt = cur;
	alt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &alt);
	char_no = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &cur);

    return (int)cno;
#elif defined(WIN32)
    return _getch();
#else
#error 'not impliment mpxx_conio_getchar() on target OS'
#endif
}

/**
 * @fn int mpxx_conio_putchar(const int char_no)
 * @brief キャラクター番号char_noを unsigned char にキャストし、コンソール(標準出力)に出力しますす。
 * @retval EOF以外 成功（指定されたchar_no値)
 * @retval EOF 何らかの問題で失敗
 **/
int mpxx_conio_putchar(const int char_no)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    return putchar(char_no);
#elif defined(WIN32)
    return _putch(char_no);
#else
#error 'not impliment mpxx_conio_putchar() on target OS'
#endif
}

/**
 * @fn int mpxx_conio_kbhit(void)
 * @brief  コンソール入力(stdin)バッファ内のデーターの存在を確認します。
 * @retval 0 標準入力バッファ内にデータは詰まれてない
 * @retval 0以外 標準入力バッファ内にデータが積まれている
 **/
int mpxx_conio_kbhit(void)
{
#if defined(Linux) || defined(QNX) || defined(Darwin)
    int status;
    char char_no;
    int fp_stat;

    fp_stat = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, fp_stat | O_NONBLOCK);

    char_no = c_getch();

    fcntl(STDIN_FILENO, F_SETFL, fp_stat);

    if (char_no != EOF) {
	ungetc(c, stdin);
	return -1;
    }

    return 0;
#elif defined(WIN32)
    return _kbhit();
#else
#error 'not impliment mpxx_kbhit() on target OS'
#endif
}

/**
 * @fn int mpxx_conio_put_newline2stdout(void)
 * @brief 改行コードをコンソール又は標準出力に送ります。
 *	OS毎に、改行コードが異なるので、あわせて出力します。
 **/
int mpxx_conio_put_newline2stdout(void)
{
    int retval = 0;
#if defined(Linux) || defined(QNX) || defined(Darwin)
    if( EOF == mpxx_conio_putchar('\r')) { //LF
	return EOF;
    }
    return 1;
#elif defined(WIN32)
    if( EOF == mpxx_conio_putchar('\n')) { //CR
	return EOF;
    }
    if( EOF == mpxx_conio_putchar('\r')) { //LF
	return EOF;
    }
    return 2;
#else
#error 'not impliment mpxx_conio_put_newline2stdout() on target OS'
#endif
}

/**
 * @fn int mpxx_conio_puts(const char *s) 
 * @brief コンソール（標準出力）に'\0'終端文字までの文字列を出力します。
 * @param s '\0'を終端とした文字列ポインタ
 * @retval 0以上 出力した文字列
 * @retval EOF(-1) 何らかのエラーがあり出力を中断した
 **/
int mpxx_conio_puts(const char *s) 
{
    int num_chars=0;

    while('\0' != *s) {
	if(EOF == mpxx_conio_putchar(*s++) ) {
	    return EOF;
	}
	++num_chars;
    }

    return num_chars;
}

/**
 * @fn int mpxx_conio_puts_with_newline(const char *s) 
 * @brief 文字列sに強制的に改行を付けてコンソール(標準出力)に出力します。
 * @retval １以上 成功した文字数(強制追加する最後の改行は抜き）
 * @retval EOF エラー発生
 **/
int mpxx_conio_puts_with_newline(const char *s) 
{
    int num_chars;
	
    if( EOF == (num_chars = mpxx_conio_puts_without_newline(s)) ) {
	return EOF;
    }

    if( EOF == mpxx_conio_set_newline2stdout() ) {
	return EOF;
    }

    return num_chars;
}

#if defined(LINUX)
#define CONSOLE_KEY_CODE_ENTER 10
#elif defined(WIN32)
#define CONSOLE_KEY_CODE_ENTER 13
#endif

/**
 * @fn int mpxx_conio_gets_with_echoback( char *const strbuf, const int maxlen, mpxx_conio_gets_attr_t * const attr_p)
 * @brief コンソール(標準入力)から最大長length-1の文字列入力を読み込みます。
 *	手入力時のバックスペースDEL等のESC/Pコードの処理も属性オプションで指定します。NULLでは処理しません。
 *	改行文字やEOFを読み込んだ場合には、読み込みを中断する。
 * @param strbuf 文字列バッファポインタ
 * @param maxlen バッファ長(文字と終端を設定するので必ず2以上でバッファを確保してください）
 * @retval -1 読み込み中にエラーが発生した。
 * @retval 0以上 読み込んだ文字列数(最大length -1)
 **/
int mpxx_conio_gets_with_echoback( char *const strbuf, const int maxlen, mpxx_conio_gets_attr_t * const attr_p)
{
    int result, status;
    char *buf_p = (char *)strbuf;
    char loop_flag=~0;
    int read_len = 0;

    mpxx_conio_gets_attr_t attr;

    if( (NULL == strbuf) || (maxlen < 2)) {
	return -1;
    } 

    *buf_p = '\0';

    if( NULL == attr_p ) {
	attr.flags = 0;
    } else {
	attr = *attr_p;
    }

    while(loop_flag) {
	char input_code;

	input_code = mpxx_conio_getchar();
	// fprintf( stderr, "Keycode=0x%02x(%d)\n", input_code, input_code); 
	if( (input_code != EOF) && ( input_code != CONSOLE_KEY_CODE_ENTER ) ) {
	    if( attr.f.TABkey_is_exchange_to_space && ( input_code == '\t' )) {
		input_code = ' ';
	    } else if( !attr.f.BSkey_is_not_manipulated_bs && (input_code == '\b' )) {
		if( read_len > 0 ) {
		    --read_len;
		    --buf_p;
		    result = mpxx_conio_putchar('\b');
		    if( EOF == result ) { status = -1; goto out; }
		    result = mpxx_conio_putchar(' ');
		    if( EOF == result ) { status = -1; goto out; }
		    result = mpxx_conio_putchar('\b');
		    if( EOF == result ) { status = -1; goto out; }
		}
		continue;
	    } else if( !attr.f.DELkey_is_not_manipulated_bs && (input_code == 127 /* DEL */ )) {
		if( read_len > 0 ) {
		    --read_len;
		    --buf_p;
		    result = mpxx_conio_putchar('\b');
		    if( EOF == result ) { status = -1; goto out; }
		    result = mpxx_conio_putchar(' ');
		    if( EOF == result ) { status = -1; goto out; }
		    result = mpxx_conio_putchar('\b');
		    if( EOF == result ) { status = -1; goto out; }
		}
		continue;
	    }

	    if( !(attr.f.nocheck_ascii_isprint_condition) && !mpxx_ascii_isprint(input_code)) {
		continue;
	    }

	    if( read_len < (maxlen - 1 )) {
		*buf_p = input_code;
		++buf_p;
		++read_len;
		result = mpxx_conio_putchar((int)input_code);
	        if( EOF == result ) { status = -1; goto out; }
	    }
	} else {
	    loop_flag = 0;
	}
    }
    result = mpxx_conio_set_newline2stdout();
    if( EOF == result ) { status = -1; goto out; }
    *buf_p = '\0'; /* Set EOL */
    status = read_len;

out:
    // fprintf( stderr, "read_len=%d strbuf=%s", read_len, strbuf);
    return status;
}

/**
 * @fn char mpxx_conio_get_enter_code(void)
 * @brief ターゲットプラットフォームでコンソール(標準入力)でEnterを判定するためのコード番号を返します。
 * @return Enter判定コード
 **/
char mpxx_conio_get_enter_code(void)
{
    return CONSOLE_KEY_CODE_ENTER;
}

/**
 * @fn int mpxx_conio_readline_with_new_buffer( mpxx_conio_readline_t *const rd_p, const char eol_char, size_t maxszofline, FILE *frp)
 * @brief 指定されたファイルポインタからeol_charまで読み込んだ文字列データをバッファと一緒に返します。
 * @param rd mpxx_conio_readline_t構造体ポインタ
 *	読み込んだ文字列と文字数を返します
 * @param eol_char 改行文字
 * @param maxszofline ライン入力を打ち切るサイズ(確保するバッファのサイズ)
 * @param frp データリード用ファイルポインタ
 * @retval 0 new_datbufの戻り値で変化します。
 *	NULL のときは、EOFまで読んだ
 *	NULL以外 読み込んだデータバッファポインタ値
 * @retval EINVAL 引数が不正
 * @retval ENOMEM バッファを確保できなかった
 * @retval ERANGE szofmaxlineを超えてバッファを確保しようとした
 **/
int mpxx_conio_readline_with_new_buffer( mpxx_conio_readline_t *const rd_p, const char eol_char, size_t maxszofline, FILE *frp)
{
    int status;
    size_t szoflinebuf, cno=0;
    char *buf;

    if((NULL == rd_p) || (eol_char == '\0') || (NULL == frp)) {
	return EINVAL;
    }

    /* initialize */
    buf = NULL;
    szoflinebuf = 2;
    memset(rd_p, 0x0, sizeof(mpxx_conio_readline_t));

    if(feof(frp)) {
	/* 既にファイルの最後まで読まれている */
	status = 0;
	goto out;
    }

    /* 初期バッファを確保 */
    buf = (char*)mpxx_malloc(szoflinebuf);
    if(NULL == buf) {
	status = ENOMEM;
	goto out;
    }

    for( cno=0; cno < szoflinebuf; ++cno) {
	size_t retsz;
	char *bptr = &buf[cno];
	retsz = fread( bptr, 1, 1, frp);
	if(retsz == 0) {
	    if( cno != 0 ) {
		/* ファイルの終端まで来たので読み込んだものを返す */
		*(bptr+1) = '\0'; /* 終端文字を追加 */
		status = 0;
		goto out;
	    }
	    /* 最初から eof だった */
	    status = 0;
	    goto out;
	}

	if(eol_char == *bptr) {
	    *(bptr+1) = '\0'; /* 終端文字を追加 */
	    status = 0;
	    goto out;
	}

	if((cno+2) == szoflinebuf) {
	    /* 次の文字と終端文字を入れられなければバッファを拡張する */
	    void *newbuf = NULL;
	    szoflinebuf *= 2;
	    if( szoflinebuf >= maxszofline) {
	 	status = ERANGE;
		goto out;
	    }

	    newbuf = mpxx_realloc(buf, szoflinebuf);
	    if(NULL == newbuf) {
		status = ENOMEM;
		goto out;
	    }
	    buf = MPXX_STATIC_CAST( char*, newbuf);
	}
    }

    /* for文は抜けることが無いので */
    status = EIO;

out:
    if(status) {
	mpxx_free(buf);
    } else {
	rd_p->readstr = buf;
	rd_p->length  = cno+1;
    }

    return status;
}

/**
 * @fn void mpxx_conio_readline_delete_buffer(mpxx_conio_readline_t *const s_p)
 * @brief csvfile_readline_with_buffer()で確保したバッファを破棄します。
 * @param s_p mpxx_conio_readline_t構造体ポインタ
 **/
void mpxx_conio_readline_with_buffer(mpxx_conio_readline_t *const s_p)
{
    if((NULL != s_p) && (NULL != s_p->readstr)) {
	mpxx_free(s_p->readstr);
    }
    return;
}
