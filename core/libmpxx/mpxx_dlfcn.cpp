/**
 *      Copyright 2014 TSNｰSHINGENN .All Rights Reserved.
 *
 *      Basic Author: Seiichi Takeda  '2014-January-19 Active
 *              Last Alteration $Author: takeda $
 */

/**
 * @file mpxx_dlfcn.c
 * @brief *** 現在コーディング中。検証中です ****
 * DLL制御 API
 *     posix/Darwin系の場合, -ldl が必要です
 */

#ifdef WIN32
#if _MSC_VER >= 1400 /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
// POSIX
#if defined(WIN32)
#include <windows.h>
#define WIN32_DLFCN 1
#elif defined(Linux) || defined(Darwin) || defined(__AVM2__) || defined(QNX)
#include <dlfcn.h>
#include <sys/stat.h>
#define POSIX_DLFCN 1
#endif

/* this */
#include "mpxx_sys_types.h"
//#include "mpxx_stdlib.h"
#include "mpxx_dlfcn.h"
#ifdef DEBUG
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int debuglevel = 5;
#else
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_MPXX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

typedef struct _dlfcn_ext {
#ifdef WIN32_DLFCN
    HMODULE win32_hmodule;
    HMODULE win32_hdll;
#elif POSIX_DLFCN
    void *psx_hdll;
#else
#error
#endif
} mpxx_dlfcn_ext_t;

#define get_dlfcn_ext(s) MPXX_STATIC_CAST(mpxx_dlfcn_ext_t*,(s)->ext)

/**
 * @fn int mpxx_dlfcn_open(mpxx_dlfcn_t *self_p, const char *pathname, int flag)
 * @brief filename で指定されたファイル名の動的ライブラリ (dynamic library) をロードし、
 *	 その動的ライブラリへの内部「ハンドル」を返す。
 * @param self_p mpxx_dlfcn_t構造体ポインタ
 * @param filename dllファイル名
 * @param flag 拡張フラグ（予約）
 * @retval EAGAIN リソースの確保に失敗
 **/
int mpxx_dlfcn_open(mpxx_dlfcn_t *self_p, const char *pathname, mpxx_dlfcn_attr_t *attr_p)
{
    mpxx_dlfcn_ext_t *e = NULL;
    int status;

    (void)attr_p;

    e = (mpxx_dlfcn_ext_t*)malloc( sizeof(mpxx_dlfcn_ext_t));
    if( NULL == e ) {
	return EAGAIN;
    }
    memset( e, 0x0, sizeof(mpxx_dlfcn_ext_t));
    self_p->ext = e;

#ifdef WIN32_DLFCN

 //   e->win32_hmodule = GetModuleHandle( pathname );
 //   if( NULL == e->win32_hmodule ) {
//	DWORD w32_errno = GetLastError();
//
//	(void)w32_errno;
//
//	status = -1;
//	goto out;
//   }

    e->win32_hdll = LoadLibrary( pathname );
    if( NULL == e->win32_hdll ) {
	DWORD w32_errno = GetLastError();

	(void)w32_errno;


	status = -1;
	goto out;
    }
    status = 0;

#elif POSIX_DLFCN
    e->psx_hdll = dlopen( pathname, RTLD_NOW);
    if( NULL == e->psx_hdll ) {

	status = -1;
	goto out;
    }
    status = 0;

#else
#error 'not impliment mpxx_dlfcn_open() on target OS'
#endif

out:
    if(status) {
	mpxx_dlfcn_close(self_p);
    }

    return status;
}

/**
 * @fn void *mpxx_dlfcn_sym(mpxx_dlfcn_t *self_p, const char *symbol)
 * @brief mpxx_dlfcn_open()された動的ライブラリからsymbol関数のポインタアドレスを返す
 * @param self_p mpxx_dlfcn_t構造体ポインタ
 * @param symbol シンボル名
 * @retval NULL 失敗
 * @retval NULL以外 シンボルポインタ
 */
void *mpxx_dlfcn_sym(mpxx_dlfcn_t *self_p, const char *symbol)
{

    mpxx_dlfcn_ext_t * const e = get_dlfcn_ext(self_p);
    void * __restrict symaddr = NULL;

    if( NULL == e ) {
	errno = EINVAL;
	return NULL;
    }

#ifdef WIN32_DLFCN

    symaddr = GetProcAddress( e->win32_hdll, symbol);
    if( NULL == symaddr ) {
#if 0
	DWORD w32_errno = GetLastError();
#endif
	return NULL;
    }

#elif POSIX_DLFCN
    symaddr = dlsym( e->psx_hdll, symbol );
    if( NULL == symaddr ) {
	return NULL;
    }
#else
#error
#endif

    return symaddr;
}

/**
 * @fn int mpxx_dlfcn_close(mpxx_dlfcn_t *self_p)
 * @brief mpxx_dlfcn_tインスタンスを解放する
 * @brief 動的ライブラリへの参照カウントを１減らす。参照カウントが 0 になり、ロードされている 他のライブラリからそのライブラリ内のシンボルが使われていなければ、 その動的ライブラリをアンロードする。
 * @param self_p mpxx_dlfcn_t構造体ポインタ
 * @retval 0 成功
 * @retval 0以外 失敗
 */
int mpxx_dlfcn_close(mpxx_dlfcn_t *self_p)
{
    mpxx_dlfcn_ext_t * const e = get_dlfcn_ext(self_p);

    if( NULL == e ) {
	errno = EINVAL;
	return -1;
    }

#ifdef WIN32_DLFCN

    FreeLibrary( e->win32_hdll );

#elif POSIX_DLFCN
    dlclose( e->psx_hdll );
#else
#error 'not impliment mpxx_dlfcn_close() on target OS'
#endif

    free(self_p->ext);
    self_p->ext = NULL;

    return 0;
}
