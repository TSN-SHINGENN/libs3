/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2016-Nov.-07 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_system_exec.c
 * @brief シェルコマンドを実行してパイプ通信によって情報のやりとりを行う為のライブラリです。
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <windows.h>

/* this */
#include "mpxx_sys_types.h"
#include "mpxx_time.h"
#include "mpxx_unistd.h"
#include "mpxx_pipe.h"
#include "mpxx_system_execl.h"

/* dbms */
#ifdef DEBUG
static int debuglevel = 4;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* パイプの入出力を配列で取るため文字に置き換えます */
enum {PIPE_OUT = 0,PIPE_IN}; //PIPERD=0 PIPEWR=1
#define NUM_PIPE_EDGES 2

typedef struct _mpxx_system_execl_ext {
    mpxx_system_execl_attr_t attr;
    union {
	unsigned int flags;
	struct {
	    unsigned int send_only_once:1;
	} f;
    } stat;

    size_t process_wait_time_ms;

    mpxx_pipe_handle_t hPipe_PW[NUM_PIPE_EDGES];
    mpxx_pipe_handle_t hPipe_PR[NUM_PIPE_EDGES];
    mpxx_pipe_handle_t hPipe_PE[NUM_PIPE_EDGES];

    /* 子供プロセス側から見た標準パイプ入出力 */
    mpxx_pipe_handle_t hPipe_stdout;
    mpxx_pipe_handle_t hPipe_stdin;
    mpxx_pipe_handle_t hPipe_stderr;

#if defined(WIN32)
    PROCESS_INFORMATION ChildProcInfo;
#endif

} mpxx_system_execl_ext_t;

#define get_mulitos_system_execl_ext(s) (mpxx_system_execl_ext_t*)(s->ext)

/**
 * @fn int mpxx_system_execl_init( multos_system_execl_t *const self_p, const char *const commandstr, const mpxx_system_execl_attr_t *const attr_p)
 * @brief  コンソールコマンド入出力の為のパイプを作成し、オブジェクトの初期化を行います
 * @param self_p multos_system_exec_t構造体オブジェクトインスタンスポインタ
 * @param self_p コンソール起動コマンド
 * @param attr_p mpxx_system_exec_attr_t構造体ポインタ
 * @retval 0 成功
 * @retval EPIPE パイプを作成出来なかった
 * @retval ECHILD 並列動作をさせるコマンドシェルを起動出来なかった
 **/
int mpxx_system_execl_init( mpxx_system_execl_t *const self_p, char *const commandstr, const mpxx_system_execl_attr_t *const attr_p)
{
    int result, status;
    mpxx_system_execl_ext_t *e = NULL;

    memset( self_p, 0x0, sizeof(mpxx_system_execl_t));

    /* 拡張エリアの取得 */
    e = (mpxx_system_execl_ext_t*)malloc(sizeof(mpxx_system_execl_ext_t));
    if( NULL == e) {
	return ENOMEM;
    }
    memset( e, 0x0, sizeof(mpxx_system_execl_ext_t));
    self_p->ext = e;
    if( NULL == attr_p) {
	e->attr.flags = 0;
    } else {
	e->attr = *attr_p;
    }

    /* 初期値設定 */
    {
	size_t n;
	for( n=0; n<NUM_PIPE_EDGES; ++n) {
	    e->hPipe_PW[n] = e->hPipe_PR[n] = e->hPipe_PE[n] = mpxx_pipe_invalid_handle();
	}

	e->hPipe_stdin = e->hPipe_stdout = e->hPipe_stderr = mpxx_pipe_invalid_handle();
    }
#if 1
#if 1
    /* 1 標準出力Pipeの作成 */
    {
	/* 1 Pipe作成 */
    	result = mpxx_pipe_create( &e->hPipe_PW[PIPE_OUT], &e->hPipe_PW[PIPE_IN]);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_init : mpxx_pipe_create(hPipePW) fail\n");
	    status = result;
	    goto out;
	}

	 // 「子」プロセスの STDOUT をセット
 	SetStdHandle(STD_OUTPUT_HANDLE, e->hPipe_PW[PIPE_IN]);

	/* 1.2 Duplicate */
	result = mpxx_pipe_duplicate( e->hPipe_PW[PIPE_OUT], &e->hPipe_stdout);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_duplicate(hPipe_stdout) fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}

	/* 1.3 不必要な口をクローズ */
	result = mpxx_pipe_close( e->hPipe_PW[PIPE_OUT]);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_close(hPipePW[PIPE_RD] fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}
	e->hPipe_PW[PIPE_OUT] = mpxx_pipe_invalid_handle();
    }

    /* 標準入力Pipeの作成 */
    {
	/* 2.1 Pipe作成 */
	result = mpxx_pipe_create( &e->hPipe_PR[PIPE_OUT], &e->hPipe_PR[PIPE_IN]);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_init : mpxx_pipe_create(hPipePR) fail\n");
	    status = result;
	    goto out;
	}

	// 「子」プロセスの STDOUT をセット
 	SetStdHandle(STD_INPUT_HANDLE, e->hPipe_PW[PIPE_OUT]);

	/* 1.2 Duplicate */
	result = mpxx_pipe_duplicate( e->hPipe_PR[PIPE_IN], &e->hPipe_stdin);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_duplicate(hPipe_stdout) fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}

	/* 1.3 不必要な口をクローズ */
	result = mpxx_pipe_close( e->hPipe_PR[PIPE_IN]);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_close(hPipePW[PIPE_RD] fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}
	e->hPipe_PR[PIPE_IN] = mpxx_pipe_invalid_handle();
    }

    /* 標準エラーPipeの作成 */
    {
	/* 3.1 Pipe作成 */
	result = mpxx_pipe_create( &e->hPipe_PE[PIPE_OUT], &e->hPipe_PE[PIPE_IN]);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_init : mpxx_pipe_create(hPipePR) fail\n");
	    status = result;
	    goto out;
	}

	// 「子」プロセスの STDERR をセット
	SetStdHandle(STD_ERROR_HANDLE, e->hPipe_PW[PIPE_IN]);

	/* 3.2 Duplicate */
	result = mpxx_pipe_duplicate( e->hPipe_PE[PIPE_OUT], &e->hPipe_stderr);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_duplicate(hPipe_stderr) fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}

	/* 3.3 不必要な口をクローズ */
	result = mpxx_pipe_close( e->hPipe_PE[PIPE_OUT]);
	if(result) {
	    DBMS1( stderr, "mpxx_pipe_close(hPipe_PE[PIPE_RD] fail, stderr=%s\n", strerror(result));
	    status = result;
	    goto out;
	}
	e->hPipe_PE[PIPE_OUT] = mpxx_pipe_invalid_handle();
    }

    /* ハンドルを継承したコンソールを起動 */

#if defined(WIN32)
    {
	STARTUPINFO si;// スタートアップ情報
	PROCESS_INFORMATION pi;// プロセス情報
	BOOL bRet;

	//STARTUPINFO 構造体の内容を設定 
	ZeroMemory( &si, sizeof(STARTUPINFO) );
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW; // ハンドル別継承・Window表示フラグ
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; // ハンドル別継承・Window表示フラグ
	si.hStdInput = e->hPipe_PR[PIPE_OUT];
	si.hStdOutput = e->hPipe_PW[PIPE_IN];
	si.hStdError = e->hPipe_PE[PIPE_IN];
	si.wShowWindow = 0; //SW_HIDE;	//DOS窓が出ない

#if 1 /* ユーザー権限の継承無し */
	{
	    DWORD dwCreationFlags = CREATE_NEW_CONSOLE;

	    bRet = CreateProcess( NULL, (LPSTR)commandstr, NULL, NULL, TRUE, dwCreationFlags, NULL, NULL,&si,&pi);
	    if(!bRet) { 
	        status = ECHILD;
		goto out;
	    }
	    e->ChildProcInfo = pi;
	}
#else
	{
	    DWORD dwCreationFlags = CREATE_NEW_CONSOLE;
	    HANDLE hUserTokenDup = 

	    DuplicateTokenEx(hPToken,MAXIMUM_ALLOWED,NULL,
		SecurityIdentification,TokenPrimary,&hUserTokenDup);

	    bRet = CreateProcessAsUser( hUserTokenDup, NULL, (LPSTR)commandstr, NULL, NULL, TRUE, dwCreationFlags, NULL, NULL,&si,&pi);
	    if(!bRet) { 
	        status = ECHILD;
		goto out;
	    }
	    e->ChildProcInfo = pi;
	}
#endif

	// ノンブロッキングモードに設定
	{
	    DWORD mode=PIPE_NOWAIT;

	    if(!e->attr.f.stdout_block_by_length_of_responce) {
		SetNamedPipeHandleState(e->hPipe_stdout,&mode,NULL,NULL);
	    }
	    if(!e->attr.f.stderr_block_by_length_of_responce) {
		SetNamedPipeHandleState(e->hPipe_stderr,&mode,NULL,NULL);
	    }
	}
    }
#endif
#else

#define R 0
#define W 1

#define CHR_BUF 4048
{
    HANDLE hPipeP2C[2]; // 親 → 子 のパイプ(stdin)
    HANDLE hPipeC2P[2]; // 子 → 親 のパイプ(stdout)
    HANDLE hPipeC2PE[2]; // 子 → 親 のパイプ(stderr)
    HANDLE hDupPipeP2CW; // 親 → 子 のパイプ(stdin)の複製
    HANDLE hDupPipeC2PR; // 子 → 親 のパイプ(stdout)の複製
    HANDLE hDupPipeC2PE; // 子 → 親 のパイプ(stderr)の複製

    SECURITY_ATTRIBUTES secAtt;
    STARTUPINFO startInfo;
    PROCESS_INFORMATION proInfo;

    HANDLE hParent = GetCurrentProcess();

    char processName[CHR_BUF];

    //------------------------------------------------------
    // パイプ作成(STDOUT,STDERR,STDIN の３本)

    // 親の STDOUT , STDIN ,STDERR のハンドルを保存
    HANDLE hOldIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOldOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hOldErr = GetStdHandle(STD_ERROR_HANDLE);

    // SECURITY_ATTRIBUTES の設定(パイプを作るのに必要
    secAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAtt.lpSecurityDescriptor = NULL;
    secAtt.bInheritHandle = TRUE; // ハンドル継承

    //------------------------------------------------------
    // STDOUT

     // パイプ作成
    CreatePipe(&hPipeC2P[R],&hPipeC2P[W],&secAtt,0);

     // 「子」プロセスの STDOUT をセット
    SetStdHandle(STD_OUTPUT_HANDLE,hPipeC2P[W]);

     // 「子」からくるパイプの読み側(つまり親がリードする側)は継承しない
    DuplicateHandle(hParent,hPipeC2P[R],hParent,&hDupPipeC2PR,0,FALSE,DUPLICATE_SAME_ACCESS);

     // 読み側のハンドルをクローズ
    CloseHandle(hPipeC2P[R]);

     //------------------------------------------------------
     // STDERR

     // パイプ作成
    CreatePipe(&hPipeC2PE[R],&hPipeC2PE[W],&secAtt,0);

     // 「子」プロセスの STDERR をセット
    SetStdHandle(STD_ERROR_HANDLE,hPipeC2PE[W]);

     // 「子」からくるパイプの読み側(つまり親がリードする側)は継承しない
    DuplicateHandle(hParent,hPipeC2PE[R],hParent,&hDupPipeC2PE,0,FALSE,DUPLICATE_SAME_ACCESS);

     // 読み側のハンドルをクローズ
    CloseHandle(hPipeC2PE[R]);

     //------------------------------------------------------
     // STDIN

     //パイプ作成
     CreatePipe(&hPipeP2C[R],&hPipeP2C[W],&secAtt,0); 

     // 「子」プロセスの STDIN をセット
    SetStdHandle(STD_INPUT_HANDLE,hPipeP2C[R]);

     // 「子」からくるパイプの書き込み側(つまり親がライトする側)は継承しない
    DuplicateHandle(hParent,hPipeP2C[W],hParent,&hDupPipeP2CW,0,FALSE,DUPLICATE_SAME_ACCESS);

     // 書き込み側のハンドルをクローズ
    CloseHandle(hPipeP2C[W]);

     // パイプ作成終了
     //------------------------------------------------------


     // STARTUPINFO の設定
     memset(&startInfo,0,sizeof(STARTUPINFO));
     startInfo.cb = sizeof(STARTUPINFO);
     startInfo.dwFlags = STARTF_USESHOWWINDOW;
     startInfo.wShowWindow = SW_HIDE;

     // 子プロセスでコマンドインタープリタを起動
    // STDIN,STDOUT,STDIN のハンドルが継承される(つまり親とパイプでつながる)

    //GetEnvironmentVariable("cmd.exe /K camint.exe\r\n",processName,CHR_BUF);
     GetEnvironmentVariable("ComSpec",processName,CHR_BUF);
 
     if(CreateProcess(processName,"",NULL,NULL,TRUE,
    	0,NULL,NULL,&startInfo,&proInfo)!=TRUE){
	status = ECHILD;
	goto out;
	}
     e->ChildProcInfo = proInfo;

 e->hPipe_stdin  = hDupPipeP2CW;
 e->hPipe_stdout = hDupPipeC2PR;
 e->hPipe_stderr = hDupPipeC2PE;


 // 子プロセスが起動したら親の STDIN と STDOUT を戻す
 SetStdHandle(STD_OUTPUT_HANDLE,hOldOut);
 SetStdHandle(STD_INPUT_HANDLE,hOldIn);
 SetStdHandle(STD_ERROR_HANDLE,hOldErr);
	}

    // ノンブロッキングモードに設定
    {
	DWORD mode=PIPE_NOWAIT;

	if(!e->attr.f.stdout_block_by_length_of_responce) {
	    SetNamedPipeHandleState(e->hPipe_stdout,&mode,NULL,NULL);
	}
	if(!e->attr.f.stderr_block_by_length_of_responce) {
	    SetNamedPipeHandleState(e->hPipe_stderr,&mode,NULL,NULL);
	}
    }

#endif
#else
#error not impliment mpxx_system_execl_init() on Target OS
#endif


    status = 0;

out:
    if(status) {
    	mpxx_system_execl_destroy(self_p);
    }

    return status;
}

/**
 * @fn void mpxx_system_execl_destroy(mpxx_system_execl_t *const self_p)
 * @brief multos_system_exec_tオブジェクトインスタンスを破棄します
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 */
void mpxx_system_execl_destroy(mpxx_system_execl_t *const self_p)
{
    size_t n;
    int result;
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);

    /* 子供プロセスのコマンドプロンプトを解放 */
#ifdef WIN32
    if( NULL != e->ChildProcInfo.hProcess ) {
	TerminateProcess(e->ChildProcInfo.hProcess, 0);
    }
#endif

    for(n=0; n<NUM_PIPE_EDGES; ++n) {
	if( e->hPipe_PW[n] != mpxx_pipe_invalid_handle()) {
	    result = mpxx_pipe_close( e->hPipe_PW[n]);
	    if(result) {
		DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipePW[%d]) fail, strerr=%s\n",
		 (int)n, strerror(result));
	    }
	    e->hPipe_PE[n] = mpxx_pipe_invalid_handle();
	}
	if( e->hPipe_PR[n] != mpxx_pipe_invalid_handle()) {
	    result = mpxx_pipe_close( e->hPipe_PR[n]);
	    if(result) {
		DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipePR[%d]) fail, strerr=%s\n",
		 (int)n, strerror(result));
	    }
	    e->hPipe_PE[n] = mpxx_pipe_invalid_handle();
	}
	if( e->hPipe_PE[n] != mpxx_pipe_invalid_handle()) {
	    result = mpxx_pipe_close( e->hPipe_PE[n]);
	    if(result) {
		DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipePE[%d]) fail, strerr=%s\n",
		 (int)n, strerror(result));
	    }
	    e->hPipe_PE[n] = mpxx_pipe_invalid_handle();
	}
    }

    if( e->hPipe_stdout != mpxx_pipe_invalid_handle()) {
	result = mpxx_pipe_close( e->hPipe_stdout);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipe_stdout) fail, strerr=%s\n",
		strerror(errno));
	}
	e->hPipe_stdout = mpxx_pipe_invalid_handle();
    }
    if( e->hPipe_stdin != mpxx_pipe_invalid_handle()) {
	result = mpxx_pipe_close( e->hPipe_stdin);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipe_stdin) fail, strerr=%s\n",
		strerror(errno));
	}
	e->hPipe_stdin = mpxx_pipe_invalid_handle();
    }
    if(e->hPipe_stderr != mpxx_pipe_invalid_handle()) {
	result = mpxx_pipe_close( e->hPipe_stderr);
	if(result) {
	    DBMS1( stderr, "mpxx_system_execl_destroy : mpxx_pipe_close(hPipe_stderr) fail, strerr=%s\n",
		strerror(errno));
	}
	e->hPipe_stderr = mpxx_pipe_invalid_handle();
    }

	if( NULL != e) {
		free(e);
	}

    return;    
}

/**
 * @fn void mpxx_system_execl_set_command_progress_waiting_time(mpxx_system_execl_t *const self_p, const size_t time_msec)
 * @brief 外部コマンド実行中の処理の待ち時間を設定します。非同期運転等でstdout/stderrで受け取るデータがパイプ入る待ち時間です。
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 * @param time_msec 待ち時間(ミリ秒)
 */
void mpxx_system_execl_set_command_progress_waiting_time(mpxx_system_execl_t *const self_p, const size_t time_msec)
{
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);

    e->process_wait_time_ms = time_msec;

    return;
}

/**
 * @fn int mpxx_system_execl_writeto_stdin(mpxx_system_execl_t *const self_p, const char *const dat, size_t writelen)
 * @brief stdinにデータを書き込みます。
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 * @param dat データバッファポインタ
 * @param writelen 書き込みデータバイト
 * @retval 0以上 書き込まれたバイト長
 * @retval 0未満 失敗
 */
int mpxx_system_execl_writeto_stdin(mpxx_system_execl_t *const self_p, const char *const dat, const size_t writelen)
{
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);
#if defined(WIN32)
    BOOL bRet = FALSE;
    DWORD dwByte;
    DWORD remain = (DWORD)writelen;
    const  char *dat_p = dat;
    const HANDLE hWrite = e->hPipe_stdin;
    const HANDLE hRead  = e->hPipe_stdout;
    const HANDLE herrRead  = e->hPipe_stderr;

    for( dwByte = 0; remain != 0; remain -= dwByte) {
	bRet = WriteFile(hWrite, dat_p, remain, &dwByte,NULL);
	if(!bRet) {
	    break;
	} else if( dwByte == remain ) {
	    break;
	} else {
	    dat_p += dwByte;
	    remain -= dwByte;
	}
    }
    // バッファのフラッシュ
    FlushFileBuffers(hWrite);
    FlushFileBuffers(hRead);
    FlushFileBuffers(herrRead);

    return (bRet ? (int)(writelen - remain) : -1);
#else
#error 'not impliment mpxx_system_execl_writeto_stdin() on target OS'
#endif
}

/**
 * @fn int mpxx_system_execl_readfrom_stdout(mpxx_system_execl_t *const self_p, char *const buf, const size_t readlen)
 * @brief stdoutパイプからデータを読み込みます
 *  ノンブロッキング状態で動作しますのでPIPE場のデータが無くなった時点で戻ります。
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 * @param buf 読み込みデータバッファポインタ
 * @param readlen データバッファバイト長
 * @retval 0以上 読み込まれたバイト長
 * @retval 0未満 失敗
 */
int mpxx_system_execl_readfrom_stdout(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen)
{
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);
#if defined(WIN32)

#if 1
    return mpxx_system_execl_readfrom_stdout_withtimeout( self_p, buf, readlen, 0);
#else
    BOOL bRet;
    DWORD dwByte;
    DWORD remain = readlen;
    uint8_t *buf_p = (uint8_t*)buf;

    memset( buf, 0x0, readlen);
    // 読み込みバッファのフラッシュ

    FlushFileBuffers(e->hPipe_stdout);
    // 隠しコンソールから標準出力の読み出し

    for(dwByte = 0;remain!=0; remain -= dwByte, buf_p += dwByte) {
	bRet = ReadFile(e->hPipe_stdout, buf_p, remain, &dwByte, NULL);
	if(!bRet) {
	    /* PIPEの持ち分が空になっている */
	    break;
	} 
	
	if( bRet && (dwByte == remain)) {
	    break;
	}
    }

    return ( !bRet && (readlen == remain) ? -1 : (int)(readlen - remain));
#endif /* end of mpxx_system_execl_readfrom_stdout_withtimeout() */
#else
#error 'not impliment mpxx_system_execl_readfrom_stdout() on target OS'
#endif
}

/**
 * @fn int mpxx_system_execl_readfrom_stdout_withtimeout(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen, const int timeout_msec)
 * @brief stdoutパイプからデータを読み込みます。但し、読み込みデータが無い場合timeoutミリ秒間読み込み可能になるまで続行します。
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 * @param buf 読み込みデータバッファポインタ
 * @param readlen データバッファバイト長
 * @param timeout_msec ミリ秒でタイムアウトを設定します。0以下で状態を変化させます。
 *    -1:読み込めるまでブロックします 0:ノンブロックとして動作1以上:タイムアウト値(ミリ秒）
 * @retval 0以上　読み込まれたバイト長
 * @retval -1 致命的な失敗 
 */
int mpxx_system_execl_readfrom_stdout_withtimeout(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen, const int timeout_msec)
{
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);
    int result;
#if defined(WIN32)
    BOOL bRet;
    DWORD dwByte;
    DWORD remain = (DWORD)readlen;
    uint8_t *buf_p = (uint8_t*)buf;
    char *tmp = (char*)buf;
    mpxx_mtimespec_t mtime;
    uint64_t mtimeout_v;
    const HANDLE hWrite = e->hPipe_stdin;
    const HANDLE hRead = e->hPipe_stdout;

    FlushFileBuffers(hWrite);

    result = mpxx_clock_getmtime(MPXX_CLOCK_MONOTONIC, &mtime);
    if(result) {
	DBMS1( stderr, "mpxx_system_execl_readfrom_stdout_withtimeout : mpxx_clock_getmtime(MPXX_CLOCK_MONOTONIC) fail\n");
	return -1;
    }
    mtimeout_v = (timeout_msec >= 0 ) ? ((mtime.sec * 1000 + mtime.msec) + timeout_msec) : 0;

    /* バッファクリア */
    memset( buf, 0x0, readlen);

    if(timeout_msec != 0) {
	DWORD prev_total=0;
	for(;;) {
	    DWORD totalLen;
	    if (PeekNamedPipe(hRead, NULL, 0, NULL, &totalLen, NULL) == 0) {
		/* 致命的なエラー */
		return -1;
	    }
	    if((timeout_msec == -1) && (readlen == totalLen)) {
		break;
	    } else if(timeout_msec > 0 ) {
	 	uint64_t nowmtime_v;
	 	result = mpxx_clock_getmtime(MPXX_CLOCK_MONOTONIC, &mtime);
	 	if(result) {
		    DBMS1( stderr, "mpxx_system_execl_readfrom_stdout_withtimeout : mpxx_clock_getmtime(MPXX_CLOCK_MONOTONIC) fail\n");
		    return -1;
		}		
		nowmtime_v = (mtime.sec * 1000 + mtime.msec);
		if( mtimeout_v < nowmtime_v ) {
		    /* タイムアウトしたので読み出し開始 */
			DBMS1( stdout, "timeout recalc=%f\n", (double)(nowmtime_v - mtimeout_v) / 1000.0);
		    break;
		}
	
	        if((totalLen !=0 ) && (totalLen == prev_total)) {
		    /* ポーリング中に値に変化がなくなったので読み出し開始 */
		    break;
		} 
	 	prev_total = totalLen;
		mpxx_msleep(10);
		FlushFileBuffers(hRead);
	    FlushFileBuffers(hWrite);
	    }
	}
    }

    // 隠しコンソールから標準出力の読み出し
    for(dwByte = 0; remain != 0; remain -= dwByte, buf_p += dwByte) {
        // 読み込みバッファのフラッシュ
        FlushFileBuffers(hRead);
	    FlushFileBuffers(hWrite);


	bRet = ReadFile(hRead, buf_p, remain, &dwByte, NULL);
	if(!bRet) {
	    DWORD dwError = GetLastError();
	    if(dwError==ERROR_HANDLE_EOF || dwError==ERROR_NO_DATA ) {
		/* ERROR_HANDLE_EOF : 終端に達してPIPEの持ち分が空になった */
		/* ERROR_NO_DATA    : データが入っていない */
		break;
	    } else if(dwError == ERROR_IO_PENDING) {
		/* 何かやっているので時間を取ってリトライ */
		mpxx_msleep(10);
	    } else {
	    	/* ERROR_BROKEN_PIPE */
		/* ERROR_INVALID_USER_BUFFER */
		/* ERROR_NOT_ENOUGH_MEMORY */
		return -1;
	    }
	} else if( dwByte == remain ) {
	    /* 何となく必要なバイト数を読み込んだ */
	    return (int)readlen;
	}
    }

    return (int)(readlen - remain);
#else
#error 'not impliment mpxx_system_execl_readfrom_stdout() on target OS'
#endif

}

/**
 * @fn int mpxx_system_execl_readfrom_stderr(mpxx_system_execl_t *const self_p, char *const buf, const size_t readlen)
 * @brief stderrパイプからデータを読み込みます
 * @param self_p multos_system_exec_tオブジェクトインスタンスポインタ
 * @param buf 読み込みデータバッファポインタ
 * @param readlen データバッファバイト長
 * @retval 0以上 読み込まれたバイト長
 * @retval 0未満 失敗
 */
int mpxx_system_execl_readfrom_stderr(mpxx_system_execl_t *const self_p, char *const buf, const size_t readlen)
{
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);
#if defined(WIN32)
    BOOL bRet;
    DWORD dwByte;
    DWORD remain = (DWORD)readlen;
    char *buf_p = buf;
    const HANDLE hOut = e->hPipe_stderr;

    memset( buf, 0x0, readlen);
    // 読み込みバッファのフラッシュ
    FlushFileBuffers(hOut);

    // 隠しコンソールから標準出力の読み出し
    // オーバーラップの場合は考慮されていない
    for(dwByte = 0;remain!=0; remain -= dwByte, remain -= dwByte, buf_p += dwByte ) {
	bRet = ReadFile(hOut, buf_p, remain, &dwByte, NULL);
	if(bRet && ( dwByte == 0 )) {
	   /* EOFになっている */
	   bRet = 0;
	   break;
	} else if(!bRet) {
	    if( e->attr.f.stderr_block_by_length_of_responce && (dwByte == remain )) {
		/* 全て読み込んだ */
		bRet = ~0;
		remain -= dwByte;
		break;
	    } else if( !e->attr.f.stderr_block_by_length_of_responce ) {
		DWORD dwError = GetLastError();
		if(dwError == ERROR_NO_DATA) {
		   /* ブロックしなくてよいので成功として脱出する */
		   bRet = ~0;
		   break;
		}
	    }
	    mpxx_sleep(10);
	} 
    }

    return (bRet ? (int)(readlen - remain) : -1);
#else
#error 'not impliment mpxx_system_execl_readfrom_stderr() on target OS'
#endif
}

/**
 * @fn int mpxx_system_execl_command_exec( mpxx_system_execl_t *const self_p, const char *const cmd, char *const resbuf, size_t *reslen_p, char *const errbuf, size_t *errlen_p)
 * @brief 起動しているプロセスから標準入出力・エラー出力の送受信を行います。
 *	標準入力へ書き込んだ後にmpxx_system_execl_set_command_process_wait_time()で指定した遅延時間を待ってから読み込みを開始します。
 * @param self_p インスタンスポインタ
 * @param cmd コンソールに渡す実行コマンド文字列ポインタ
 * @param timeout_msec コマンドに対応するレスポンスタイムアウト値 -1:ブロック状態 0:ノンブロック状態 1以上:タイムアウト値(ミリ秒）
 * @param resbuf 標準出力から戻ってくる文字列を保持する文字列バッファポインタ。NULLを指定すると無視します(errlen_pもNULLxしていします）
 * @param reslen_p resbufのサイズ入力してある変数のポインタ(NULLは無視）。関数から戻ると読み込んだ文字列数が返します
 * @param errbuf 標準エラー出力から読み込んだ文字列を保持する文字列バッファポインタ。NULLを指定すると無視します(errlen_pもNULLxしていします）
 * @param errlen_p resbufのサイズ入力してある変数のポインタ(NULLは無視）。関数から戻ると読み込んだ文字列数が返します
 * @retval 0 成功 
 * @retval EINVAL 引数が無効
 * @retval EACCES process_terminated_after_only_onceが１に指定されているにも関わらず二度目を実行した
 * @retval EIO プロセス間通信に失敗した
 **/
int mpxx_system_execl_command_exec( mpxx_system_execl_t *const self_p, const char *const cmd, const int timeout_msec, char *const resbuf, size_t *reslen_p, char *const errbuf, size_t *errlen_p)
{
    int retval;
    mpxx_system_execl_ext_t *const e = get_mulitos_system_execl_ext(self_p);

    if(e->stat.f.send_only_once) {
	return EACCES;
    }

    if( ( NULL != resbuf ) && ( NULL == reslen_p) ) {
	return EINVAL;
    } else if( ( NULL != errbuf ) && ( NULL == errlen_p) ) {
	return EINVAL;
    }

#if 0
    if(NULL != cmd ) {
	//コマンドをコンソールに書き込み
	const char *cmd_p = cmd;
	DWORD remain = strlen(cmd);

	for(dwByte = 0; remain!=0; remain -= dwByte) {
	    bRet = WriteFile(e->hPipe_stdin, cmd_p, remain, &dwByte,NULL);
	    if( dwByte == remain ) {
		break;
	    } else {
	    	cmd_p += dwByte;
		remain -= dwByte;
	    }
	}
	// 書き込みバッファのフラッシュ
	FlushFileBuffers(e->hPipe_stdin);
    }
#else
    if( NULL != cmd ) {
	size_t cmdlen = strlen(cmd);

        retval = mpxx_system_execl_writeto_stdin(self_p, cmd, cmdlen);
        if(retval <0 ) {
	    return EIO;
	}
    }
#endif

    //コマンドが処理されるまで待機するウエイト
    if(e->process_wait_time_ms) {
	mpxx_msleep(MPXX_STATIC_CAST(const unsigned int,e->process_wait_time_ms));
    }

    if(e->attr.f.process_terminated_after_only_once) {
	//子プロセスが終了するまで待機
	WaitForSingleObject(e->ChildProcInfo.hProcess, INFINITE);
	e->stat.f.send_only_once = 1;
    }

    if( NULL != resbuf ) {
	retval = mpxx_system_execl_readfrom_stdout_withtimeout(self_p, resbuf, *reslen_p, timeout_msec);	
	if(retval<0) {
	    DBMS1( stderr, "mpxx_system_execl_command : mpxx_stytem_execl_readfrom_stdout fail\n");
	    return EIO;
	}
	*reslen_p = retval;
    }

    if( NULL != errbuf ) {
	retval = mpxx_system_execl_readfrom_stderr( self_p, errbuf, *errlen_p);
	if(retval<0) {
	    DBMS1( stderr, "mpxx_system_execl_command : mpxx_stytem_execl_readfrom_stderr fail\n");
	    return EIO;
	}
	*reslen_p = retval;
    }

    return 0;
}
