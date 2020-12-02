/**
 *	Copyright 2016 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2016-September-18 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_pipe.c
 * @brief スレッド・プロセス間通信で使用するパイプライブラリ
 *	POSIX系にあわせるが、関数の使用はWIN32に近い。理由はFD値として扱えないため
 */

/* WIN32 */
#if defined(WIN32)
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* WIN32 */
#if defined(WIN32)
#include <windows.h>
#endif

/* CRL */
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* POSIX */
#if defined(Linux) || defined(MACOSX) || defined(QNX) || defined(__AVM2__)
#include <unistd.h>
#include <fcntl.h>

#define POSIX_PIPE
#endif

/* this */
#include "mpxx_sys_types.h"
#include "mpxx_unistd.h"
#include "mpxx_malloc.h"
#include "mpxx_pipe.h"

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

/* パイプの入出力を配列で取るので見分ける必要の為 */
enum {PIPE_RD = 0,PIPE_WR}; //PIPERD=0 PIPEWR=1
#define NUM_PIPE_EDGES 2

/**
 * @fn int mpxx_pipe_create( mpxx_pipe_handle_t *const rd_hPipe_p, mpxx_pipe_handle_t *const wr_hPipe_p)
 * @brief パイプを作成します。Linuxは入出力を一つの引数で行いますが、WIN32にあわせます。
 * @param rd_pipe mpxx_pipe_handle_t読み出し側パイプ端ハンドル格納変数ポインタ
 * @param wr_pipe mpxx_pipe_handle_t書き込み側パイプ端ハンドル格納変数ポインタ
 * @retval 0 成功
 * @retval EFAULT 引数に無効なポインタが渡っている
 * @retval EMFILE リソースを使いすぎてパイプが作成出来ない
 * @retval ENFILE パイプの上限に達している
 */
int mpxx_pipe_create( mpxx_pipe_handle_t *const rd_hPipe_p, mpxx_pipe_handle_t *const wr_hPipe_p)
{
    int status;
    size_t n;
    mpxx_pipe_handle_t * const edge_handles[NUM_PIPE_EDGES] = { rd_hPipe_p, wr_hPipe_p };

    for( n=0; n<NUM_PIPE_EDGES; ++n) {
	if( NULL == edge_handles[n] ) {
	    status = EINVAL;
	    goto out;
	}
	*edge_handles[n] = mpxx_pipe_invalid_handle();
    }

#if defined(POSIX_PIPE)
    { /* POSIX compliance progress */
	int result;
	int edges_fd[NUM_PIPE_EDGES];
	result = pipe( edges_fd );
	if(result) {
	    status = result;
	    goto out;
	}
	for( n=0; n<NUM_PIPE_EDGES; ++n ) {
	    *edge_handles[n] = edges_fd[n];
    	    result =  fcntl( edges_fd[n], O_CLOEXEC );
	    if(result < 0) {
		status = errno;
		goto out;
	    }
	}
    }
#elif defined(WIN32)
    { /* WIN32 progress */
	HANDLE hPipes[NUM_PIPE_EDGES] = { NULL, NULL};
	BOOL bRet;
	SECURITY_ATTRIBUTES SecAtt;
	SecAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecAtt.lpSecurityDescriptor = NULL;
	SecAtt.bInheritHandle = TRUE;

//	bRet = CreatePipe(&hPipes[PIPE_RD], &hPipes[PIPE_WR], &SecAtt,0);
	bRet = CreatePipe(&hPipes[PIPE_RD], &hPipes[PIPE_WR], &SecAtt,64 * 1024); // Linuxが64Kがデフォルトなので、そっちに合わせる? 17.04.24 Takeda
	if(!bRet) {
	    return ENFILE;
	}
	for( n=0; n<NUM_PIPE_EDGES; ++n ) {
	    *edge_handles[n] = hPipes[n];
	}
    }	
#else
#error 'not impliment mpxx_pipe_create() on target OS'
#endif
    status = 0;

out:
    if(status) {
	mpxx_pipe_close(*rd_hPipe_p);
	mpxx_pipe_close(*wr_hPipe_p);
    }

    return status;
}

/**
 * @fn int mpxx_pipe_close( mpxx_pipe_handle_t hPipe)
 * @brief 指定されたパイプ端をクローズします。
 * @param self_p パイプ端オブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval EBADF 有効なインスタンスではない
 * @retval EINTR 処理が何らかの原因で割り込まれた
 * @retval EIO IOエラーが発生した
 **/
int mpxx_pipe_close(const mpxx_pipe_handle_t hPipe)
{
    int status = 0;

#if defined(POSIX_PIPE)
    {
    	if(!(hPipe < 0 )) {
	    int result = close(hPipe);
	    if(result) {
		status = result;
	 	goto out;
	    }
	} 
    }
#elif defined(WIN32) 
    {
	BOOL bRet;
	if(NULL != hPipe) {
	    bRet = CloseHandle(hPipe);
	    if(!bRet) {
	 	status = EIO;
	 	goto out;
	    }
	}
    }
#else
#error 'not impliment mpxx_pipe_close() on target OS'
#endif
    status = 0;

out:

    return status;
}



/**
 * @fn int mpxx_pipe_get_pipesize( const mpxx_pipe_handle_t hPipe, size_t *const retval_p)
 * @brief パイプサイズを取得します。
 * @param obj_p パイプオブジェクトハンドルポインタ
 * @param retval_p サイズ取得変数ポインタ
 * @retval 0 成功
 **/
int mpxx_pipe_get_pipesize( const mpxx_pipe_handle_t hPipe, size_t *const retval_p)
{
    int status;

#if defined(POSIX_PIPE)

#if defined(F_GETPIPE_SZ)
    { /* POSIX compliance progress */
	int result;
	int pval;

	result = fcntl( hPipe, F_GETPIPE_SZ, 0);
	if(result<0) {
	    status = errno;
	    goto out;
	}
	if( NULL != retval_p ) {
	    *retval_p = result;
	}
    } 
    status = 0;
out:
#else
    (void)hPipe;
    (void)retval_p;

    errno = ENOSYS;
    status = -1;
#endif

#elif defined(WIN32)
    (void)hPipe;
    (void)retval_p;

    errno = ENOSYS;
    status = -1;
#else
#error 'not impliment mpxx_pipe_get_pipesize() on target OS'
#endif

    return status;
}

int mpxx_pipe_set_pipesize( const mpxx_pipe_handle_t hPipe, size_t value)
{

    int status;

#if defined(POSIX_PIPE)

#if defined(F_SETPIPE_SZ)
    { /* POSIX compliance progress */
	int result;
	int pval;

	result = fcntl( hPipe, F_SETPIPE_SZ, 0);
	if(result<0) {
	    status = errno;
	    goto out;
	}
	if( NULL != retval_p ) {
	    *retval_p = result;
	}
    } 
    status = 0;
out:
#else
    (void)hPipe;
    (void)value;
    errno = ENOSYS;
    status = -1;
#endif

#elif defined(WIN32)
    (void)hPipe;
    (void)value;
    errno = ENOSYS;
    status = -1;
#else
#error 'not impliment mpxx_pipe_set_pipesize() on target OS'
#endif

    return status;
}

/**
 * @fn int mpxx_pipe_set_mode_nonblock( mpxx_pipe_handle_t *const self_p)
 * @brief 指定されたパイプ端をノンブロック状態に設定する
 * @param self_p パイプ端オブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval EBADF 不正なパイプ端インスタンス（作成されてない？)
 **/
int mpxx_pipe_set_mode_nonblock( mpxx_pipe_handle_t hPipe)
{
    int status;

#if defined(POSIX_PIPE)
    {
	int result;
	int pval;

	result = fcntl( hPipe, F_GETFL, 0);
	if(result<0) {
	    status = errno;
	    goto out;
	} else {
	    pval = result;
	}
	pval |= O_NONBLOCK;

	result = fcntl( hPipe, pval);
	if(result<0) {
	    status = errno;
	    goto out;
	}
    } 
    status = 0;
#elif defined(WIN32)
    {
	BOOL bRet;
	DWORD mode = PIPE_NOWAIT;

	bRet = SetNamedPipeHandleState( hPipe, &mode, NULL, NULL);
	if(!bRet) {
	    status = EPERM;
	    goto out;
	}
    }
    status = 0;
#else
#error 'not impliment mpxx_pipe_set_nonblock() on target OS'
#endif

out:
    return status;
}


/**
 * @fn int mpxx_pipe_set_mode_nonblock( mpxx_pipe_handle_t *const self_p)
 * @brief 指定されたパイプ端をブロック状態に設定する
 * @param self_p パイプ端オブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval EBADF 不正なパイプ端インスタンス（作成されてない？)
 **/
int mpxx_pipe_set_mode_block( const mpxx_pipe_handle_t hPipe)
{
    int status;

#if defined(POSIX_PIPE)
    {
	int result;
	int pval;

	result = fcntl( hPipe, F_GETFL, 0);
	if(result<0) {
	    status = errno;
	    goto out;
	} else {
	    pval = result;
	}
	pval &= ~O_NONBLOCK;

	result = fcntl( hPipe, pval);
	if(result<0) {
	    status = errno;
	    goto out;
	}
    } 
    status = 0;
#elif defined(WIN32)
    {
	BOOL bRet;
	DWORD mode = PIPE_WAIT;

	bRet = SetNamedPipeHandleState( hPipe, &mode, NULL, NULL);
	if(!bRet) {
	    status = EPERM;
	    goto out;
	}
    }
    status = 0;
#else
#error 'not impliment mpxx_pipe_set_nonblock() on target OS'
#endif

out:
    return status;
}




/**
 * @fn int mpxx_pipe_duplicate(mpxx_pipe_handle_t *const old_hanlde, mpxx_pipe_handle_t * const new_pipe)
 * @brief 指定されたパイプ端オブジェクトをデュプリケートします。
 *	二つのパイプ端は同じように扱うことが出来ます。
 * @param self_p パイプ端オブジェクトインスタンスポインタ
 * @retval 0 成功
 * @retval EBADF 有効なインスタンスではない
 * @retval EINTR 処理が何らかの原因で割り込まれた
 * @retval EIO IOエラーが発生した
 **/
int mpxx_pipe_duplicate(mpxx_pipe_handle_t src_hPipe, mpxx_pipe_handle_t * const dup_hPipe_p)
{
    int status;

    if( NULL == dup_hPipe_p ) {
	return EINVAL;
    }

#if defined(POSIX_PIPE)
    {
	int result = dup(src_hPipe);
	if(result<0) {
	    status = errno;
	    goto out;
	}
	*dup_hPipe_p = result;
    }
    status = 0;
#elif defined(WIN32) 
    {
	BOOL bRet;
	HANDLE hProcess = GetCurrentProcess();

	bRet = DuplicateHandle(hProcess, src_hPipe, hProcess, dup_hPipe_p, 0, FALSE, DUPLICATE_SAME_ACCESS);
	if(!bRet) {
	    status = EPIPE;
	    goto out;
	}
    }
    status = 0;
#else
#error 'not impliment mpxx_pipe_duplicate() on target OS'
#endif

out:
    return status;
}

/**
 * @fn int mpxx_pipe_write(const void *const dat, int lenofdat, const mpxx_pipe_handle_t hPipe)
 * @brief パイプへ書き込む
 * @param dat 書き込みデータバッファ
 * @param lenofdat 書き込みデータバイト数
 * @param obj_p パイプ書き込み端オブジェクトインスタンスポインタ
 * @retval 0以上 書き込みバイト数
 * @retval -1 エラー errno参照
 **/
int mpxx_pipe_write(const void *const dat, const size_t lenofdat, const mpxx_pipe_handle_t hPipe)
{
#if defined(POSIX_PIPE)
    return write( hPipe, dat, lenofdat);
#elif defined(WIN32)
    {
	BOOL bRet;
	DWORD dwLen = (DWORD)lenofdat, dwRet;

	bRet = WriteFile( hPipe, dat, dwLen, &dwRet, 0);
	if(!bRet) {
	    errno = EIO;
	    return -1;
	}

	return (int)dwRet;
    }
#else
#error 'not impliment mpxx_pipe_write() on target OS'
#endif
}

/**
 * @fn int mpxx_pipe_read(mpxx_pipe_handle_t *const obj_p, void *const buf, size_t length)
 * @brief 指定されたパイプからデータを読み込みます。パイプは読み込み許可でなければなりません。
 * @param obj_p パイプオブジェクトハンドル
 * @param buf 読み込みバッファ(length以上のバッファを確保しておく必要があります)
 * @param length 読み込み長(データが少ない場合はブロックされます)
 * @retval 1以上 読み込みサイズ
 * @retval 0 データの終わり
 * @retval -1 エラー(errnoを参照）
 **/
int mpxx_pipe_read(const mpxx_pipe_handle_t hPipe, void *const buf, const size_t length)
{

#if defined(POSIX_PIPE)
    return read( hPipe, buf, length);
#elif defined(WIN32)
    {
	BOOL bRet;
	DWORD dwSize = (DWORD)length, dwRet;

	bRet = ReadFile( hPipe, buf, dwSize, &dwRet, 0);
	if(!bRet) {
	    errno = EIO;
	    return -1;
	}

	return (int)dwRet;
    }
#else
#error 'not impliment mpxx_pipe_write() on target OS'
	errno = ENOSYS;
	return -1;
#endif
}

/**
 * @fn int mpxx_pipe_get_std_handle( mpxx_pipe_handle_t hPipe, const int fileno)
 * @brief 標準パイプのオブジェクトハンドルを取得します。
 * @param obj_p オブジェクト取得用空ハンドルのポインタ
 * @param fileno mpxx_unistd.hで規定された MPXX_STD??_FILENO を指定 
 * @retval 0 成功
 */
int mpxx_pipe_get_std_handle(  mpxx_pipe_handle_t *const hPipe, const int fileno)
{
    (void)hPipe;

    return -1;
}

/**
 * @fn int mpxx_pipe_set_std_handle( const int file_no, const mpxx_pipe_handle_t hPipe)
 * @brief 標準パイプのオブジェクトハンドルを取得します。
 * @param obj_p オブジェクト取得用空ハンドルのポインタ
 * @param fileno mpxx_unistd.hで規定された MPXX_STD??_FILENO を指定 
 * @retval 0 成功
 */
int mpxx_pipe_set_std_handle( const int file_no, const mpxx_pipe_handle_t hPipe)
{
    (void)hPipe;

    return -1;
}

/**
 * @fn mpxx_pipe_handle_t mpxx_pipe_invalid_handle(void)
 * @brief パイプハンドルとして無効な値を戻します。
 * @return 無効な初期値
 */
mpxx_pipe_handle_t mpxx_pipe_invalid_handle(void)
{
#if defined(WIN32)
    return NULL;
#elif defined(POSIX_PIPE)
    return -1;
#else
#error 'not impliment mpxx_pipe_set_invalid_handle() on target OS'
#endif
}
