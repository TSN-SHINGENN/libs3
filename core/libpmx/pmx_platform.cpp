/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-May-18 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file pmx_platform.c
 * @brief OSプラットフォーム判定ライブラリ
 */

#ifdef WIN32
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#if defined(Linux) || defined(SunOS) || defined(QNX)
#include <sys/utsname.h>
#include <stdbool.h>
#endif

#if defined(Darwin)
#if 0				/* old */
#include <Carbon/Carbon.h>
#else
#include <stdbool.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <string.h>
#include <stdlib.h>
#endif
#endif

#ifdef DEBUG
static int debuglevel = 1;
#else
static int debuglevel = 0;
#endif
#include <libmpxx/dbms.h>

/* this */
#include <libmpxx/mpxx_string.h>
#if defined(WIN32)
#include <libmpxx/mpxx_dlfcn.h>
#endif
#include <libmlxx/mlxx_gof_cmd.h>
#include <libmlxx/mlxx_platform.h>

/**
 * @fn int libs3::pmx_platform_is_ms_windows(void)
 * @brief 使用中のOSがMS-Windowsであることを判定します。
 * @retval 1 MS-Windowsシリーズである
 * @retval 0 その他のOS
 */
int libs3::pmx_platform_is_ms_windows(void)
{
#ifdef WIN32
    DBMS5(stderr, "libs3::pmx_platform_is_ms_windows : execute\n");
    return 1;
#else
    DBMS5(stderr, "libs3::pmx_platform_is_ms_windows : execute\n");
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_get_ms_windows_nt_version(int *majver_p, int *minver_p)
 * @brief MS-WindowsNT系のバージョンを取得します。
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @retval -1 失敗
 * @retval 0 成功
 */
int libs3::pmx_platform_get_ms_windows_nt_version(int *majver_p,
					       int *minver_p)
{
#ifdef WIN32
#if defined(_WIN32_WINNT)
    typedef void (WINAPI *RtlGetVersionFunc_t)(OSVERSIONINFOEXW* );

    mpxx_dlfcn_t dl_ntdll;
    int result;
    RtlGetVersionFunc_t dlf_RtlGetVersionFunc;

    OSVERSIONINFOEX os;
#ifdef UNICODE
    OSVERSIONINFOEXW* osw=&os;
#else
    OSVERSIONINFOEXW o;
    OSVERSIONINFOEXW* osw=&o;
#endif

    result = mpxx_dlfcn_open(&dl_ntdll, "ntdll.dll", NULL);
    if (result) {
	DBMS1(stderr,
	      "mpxx_platform_get_ms_windows_nt_version : mpxx_dlfcn_open(ntdll.dll) fail\n");
	return -1;
    }

    dlf_RtlGetVersionFunc =
	(void(WINAPI *) (OSVERSIONINFOEXW*)) mpxx_dlfcn_sym(&dl_ntdll,
							   "RtlGetVersion");
    if (!dlf_RtlGetVersionFunc) {
	return -1;
    }

    ZeroMemory(osw,sizeof(*osw));
    osw->dwOSVersionInfoSize=sizeof(*osw);

    dlf_RtlGetVersionFunc(osw);

#ifndef	UNICODE
	os.dwOSVersionInfoSize=sizeof(os);
	os.dwBuildNumber=osw->dwBuildNumber;
	os.dwMajorVersion=osw->dwMajorVersion;
	os.dwMinorVersion=osw->dwMinorVersion;
	os.dwPlatformId=osw->dwPlatformId;
#endif

    if( NULL != majver_p ) {
	*majver_p = os.dwMajorVersion;
    }
    if (NULL != minver_p) {
	*minver_p = os.dwMinorVersion;
    }

    mpxx_dlfcn_close(&dl_ntdll);

    return 0;
#else
    OSVERSIONINFOA osVerInfo;

    /* OSの取得 */
    osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
    if (GetVersionEx(&osVerInfo) == 0) {
	DBMS1(stderr,
	      "pmx_platform_get_ms_windows_version : GetVersionEx fail\n");
	return -1;
    }
    if (osVerInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) {
	return -1;
    }

    if (NULL != majver_p) {
	*majver_p = osVerInfo.dwMajorVersion;
    }

    if (NULL != minver_p) {
	*minver_p = osVerInfo.dwMinorVersion;
    }

    return 0;
#endif
#else
    if (NULL != majver_p) {
	*majver_p = -1;
    }

    if (NULL != minver_p) {
	*minver_p = -1;
    }

    return -1;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_9x(void)
 * @brief OSがMS-Windows9xであるか評価します
 * @retval 1 MS-Windows9xシリーズである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_9x(void)
{
#ifdef WIN32
    OSVERSIONINFOA osVerInfo;

    /* OSの取得 */
    osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
    if (GetVersionEx(&osVerInfo) == 0) {
	DBMS1(stderr,
	      "libs3::pmx_platform_get_ms_windows_version : GetVersionEx fail\n");
	return -1;
    }
    if (osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif
}
int libs3::pmx_platform_is_ms_windows_10(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_10;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif

}

int libs3::pmx_platform_is_ms_windows_8_1(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_8_1;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_8(void)
 * @brief OSがMS-Windows8であるか評価します
 * @retval 1 MS-Windows8である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_8(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_8;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_7(void)
 * @brief OSがMS-Windows7であるか評価します
 * @retval 1 MS-Windows7である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_7(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_7;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}


/**
 * @fn int libs3::pmx_platform_is_ms_windows_vista(void);
 * @brief OSがMS-WindowsVistaであるか評価します
 * @retval 1 MS-WindowsVistaである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_vista(void)
{
#ifdef WIN32
    const int ms_ver[2] = pmx_PLATFORM_VER_WINDOWS_VISTA;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_xp(void);
 * @brief OSがMS-WindowsXPであるか評価します
 * @retval 1 MS-WindowsXPである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_xp(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_XP;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}


/**
 * @fn int libs3::pmx_platform_is_ms_windows_xp64(void);
 * @brief OSがMS-WindowsXPであるか評価します
 * @retval 1 MS-WindowsXPである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_xp64(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_XP64;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_2k(void)
 * @brief OSがMS-Windows2Kであるか評価します
 * @retval 1 MS-Windows2Kである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_2k(void)
{
#ifdef WIN32
    const int ms_ver[2] = libs3::PMX_PLATFORM_VER_WINDOWS_2k;
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == ms_ver[0]) && (minver == ms_ver[1])) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_2003Server(void)
 * @brief OSがMS-Windows2Kであるか評価します
 * @retval 1 MS-Windows2Kである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_2003Server(void)
{
#ifdef WIN32
    int majver, minver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, &minver)) {
	return -1;
    }

    if ((majver == 5) && (minver == 2)) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_ms_windows_nt(void)
 * @brief OSがMS-Windows2Kであるか評価します
 * @retval 1 MS-Windows2Kである
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_ms_windows_nt(void)
{
#ifdef WIN32
    int majver;

    if (libs3::pmx_platform_get_ms_windows_nt_version(&majver, NULL)) {
	return -1;
    }

    if (majver <= 4) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

#if defined(Linux) || defined(SunOS) || defined(QNX)
/**
 * @fn static int posix_get_version( int *majver_p, int *minver_p)
 * @brief バージョン情報を取得します
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @param rev_p リビジョン取得変数ポインタ(リビジョン番号が内場合は-1がセットされます)
 * @retval 0 成功
 * @retval -1 失敗
 */
static int posix_get_version(int *majver_p, int *minver_p, int *rev_p)
{
    struct utsname myos_info;
    int majver, minver;
    int rev;

    majver = minver = rev = -1;

    if (uname(&myos_info) != 0) {
	DBMS1(stderr, "posix_get_version : uname fail\n");
	return -1;
    }

    if (sscanf(myos_info.release, "%d.%d", &majver, &minver) != 2) {
	return -1;
    }

    if (sscanf(myos_info.release, "%*d.%*d.%d", &rev) != 1) {
	rev = -1;
    }

    if (NULL != majver_p) {
	*majver_p = majver;
    }

    if (NULL != minver_p) {
	*minver_p = minver;
    }

    if (NULL != rev_p) {
	*rev_p = rev;
    }

    return 0;
}
#endif

/**
 * @fn int libs3::pmx_platform_is_linux(void)
 * @brief 使用中のOSがLinux系であることを判定します。
 * @retval 1 Linux系である
 * @retval 0 その他のOS
 */
int libs3::pmx_platform_is_linux(void)
{
#ifdef Linux
    return 1;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_get_linux_version(int *majver_p, int *minver_p, int *rev_p)
 * @brief LINUXのバージョン情報を返します。
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @param rev_p リビジョン取得変数ポインタ(リビジョン番号が内場合は-1がセットされます)
 * @retval 0 成功
 * @retval -1 失敗
 */
int libs3::pmx_platform_get_linux_version(int *majver_p, int *minver_p,
				       int *rev_p)
{
#ifdef Linux
    return posix_get_version(majver_p, minver_p, rev_p);
#else
    if (NULL != majver_p) {
	*majver_p = -1;
    }

    if (NULL != minver_p) {
	*minver_p = -1;
    }

    if (NULL != rev_p) {
	*rev_p = -1;
    }

    return -1;
#endif
}


/**
 * @fn int libs3::pmx_platofrm_is_qnx(void)
 * @brief 使用中のOSがQNX系であることを判定します。
 * @retval 1 QNX系である
 * @retval 0 その他のOS
 */
int libs3::pmx_platofrm_is_qnx(void)
{
#ifdef QNX
    return 1;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_get_qnx_version(int *majver_p, int *minver_p, int *rev_p)
 * @brief QNXのバージョン情報を返します。
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @param rev_p リビジョン取得変数ポインタ(リビジョン番号が内場合は-1がセットされます)
 * @retval 0 成功
 * @retval -1 失敗
 */
int libs3::pmx_platform_get_qnx_version(int *majver_p, int *minver_p,
				     int *rev_p)
{
#ifdef QNX
    return posix_get_version(majver_p, minver_p, rev_p);
#else

    if (NULL != majver_p) {
	*majver_p = -1;
    }

    if (NULL != minver_p) {
	*minver_p = -1;
    }

    if (NULL != rev_p) {
	*rev_p = -1;
    }

    return -1;
#endif
}


/**
 * @fn int libs3::pmx_platform_is_sunos(void)
 * @brief 使用中のOSがSunOS系であることを判定します。
 * @retval 1 SunOS系である
 * @retval 0 その他のOS
 */
int libs3::pmx_platform_is_sunos(void)
{
#ifdef SunOS
    return 1;
#else
    return 0;
#endif
}


/**
 * @fn int libs3::pmx_platform_get_sunos_version(int *majver_p, int *minver_p)
 * @brief SunOS系のバージョン情報を取得します
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @retval 0 成功
 * @retval -1 失敗
 */
int libs3::pmx_platform_get_sunos_version(int *majver_p, int *minver_p)
{
#ifdef SunOS
    return posix_get_version(majver_p, minver_p, NULL);
#else
    if (NULL != majver_p) {
	*majver_p = -1;
    }

    if (NULL != minver_p) {
	*minver_p = -1;
    }

    return -1;
#endif
}


/**
 * @fn int libs3::pmx_platform_is_macosx(void)
 * @brief 使用中のOSがMACOSX系であることを判定します。
 * @retval 1 MACOSX系である
 * @retval 0 その他のOS
 */
int libs3::pmx_platform_is_macosx(void)
{
#ifdef Darwin
    return 1;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_get_macosx_version( int *majver_p, int *minver_p, int *kernmin_p)
 * @brief MACOSX系のバージョン情報を取得します
 * @param majver_p メジャーバージョン取得用変数ポインタ
 * @param minver_p マイナーバージョン取得用変数ポインタ
 * @param kernmin_p カーネルのマイナーバージョンを取得用変数ポインタ
 *  ※ リビジョン番号とほぼ同値ですが、Appleの都合で異なる場合があります(不明な場合は-1がセットされます)
 * @retval 0 成功
 * @retval -1 失敗
 */
int libs3::pmx_platform_get_macosx_version(int *majver_p, int *minver_p,
					int *kernmin_p)
{
#ifdef Darwin
#if 0				/* Carbon */
    int majver, minver, rev;
    char buf[16];

    SInt32 ver;
    Gestalt(gestaltSystemVersion, &ver);

    majver = (0xff & (ver >> 8));

    sprintf(buf, "%x", majver);
    sscanf(buf, "%u", &majver);

    minver = 0xf & (ver >> 4);
    rev = 0xf & ver;

    if (NULL != majver_p) {
	*majver_p = majver;
    }

    if (NULL != minver_p) {
	*minver_p = minver;
    }

    if (NULL != kernmin_p) {
	*kernmin_p = rev;
    }

    return 0;
#else				/* sysctrl */

/* *INDENT-OFF* */
    const int kknown_last = 13;
    const struct {
	int kern_maj;
	int os_majver;
	int os_minver;
    } verlist[] = {
/* kern_maj, os_majver, os_minver */
{          5,        10,         1},
{          6,        10,         2},
{          7,        10,         3},
{          8,        10,         4},
{          9,        10,         5},
{         10,        10,         6},
{         11,        10,         7},
{         12,        10,         8},
{kknown_last,        10,         9},
{         -1,        -1,        -1}};
/* *INDENT-ON* */
    size_t n;
    char buf[PATH_MAX];
    size_t size = sizeof(buf);
    int result, status;
    int majver = -1, minver = -1, rev = -1;
    int kern_maj, kern_min;
    libs3::mlxx_gof_cmd_makeargv_t marg;

    DBMS1(stderr,
	  "libs3::mlxx_platform_get_macosx_version(sysctl) : execute\n");

    result = sysctlbyname("kern.osrelease", buf, &size, NULL, 0);
    if (result) {
	DBMS1(stderr,
	      "libs3::mlxx_platform_get_macosx_version : sysctlbyname(%d:%s) fail\n",
	      result, strerror(result));
	return -1;
    }

    result = libs3::mlxx_gof_cmd_makeargv_init(&marg, ".", '\0', NULL);
    if (result) {
	DBMS1(stderr,
	      "mpxx_platform_get_macosx_version : libs3::mlxx_gof_cmd_makeargv_init(%d:%s) fail\n",
	      result, strerror(result));
	return -1;
    }

    result = libs3::mlxx_gof_cmd_makeargv(&marg, buf);
    if (result) {
	DBMS1(stderr,
	      "libs3::mlxx_platform_get_macosx_version : libs3::mlxx_gof_cmd_makeargv(%d:%s) fail\n",
	      result, strerror(result));
	status = -1;
	goto out;
    }

    if (marg.argc < 2) {
	DBMS1(stderr,
	      "libs3::mlxx_platform_get_macosx_version : ignore param(%s)\n",
	      buf);
	status = -1;
	goto out;
    }
#if 0 /* for Debug */
    for(n=0; n<marg.argc; ++n) {
	fprintf( stderr, "%s ac[%u]=%s\n", __func__, n, marg.argv[n]);
    }
#endif

    kern_maj = -1;
    kern_min = atoi(marg.argv[1]);
    for (n = 0; verlist[n].kern_maj != -1; ++n) {
	if (verlist[n].kern_maj == (kern_maj = atoi(marg.argv[0]))) {
	    break;
	}
    }

    if (verlist[n].kern_maj == -1) {
	if (kern_maj > kknown_last) {
	    for (n = 0; verlist[n].kern_maj != -1; ++n) {
		if (verlist[n].kern_maj == kknown_last) {
		    break;
		}
	    }

	    majver = verlist[n].os_majver;
	    minver = verlist[n].os_minver;
	    rev = -1;		/* 分からないので最後のバージョン */
	} else if ((kern_maj >= 0) && (kern_maj < 5)) {
	    status = -1;
	    goto out;
	}
    } else {
	majver = verlist[n].os_majver;
	minver = verlist[n].os_minver;
	rev = kern_min;
    }

    DBMS5( stderr, "kern_maj = %d majver=%d minver=%d rev=%d\n", kern_maj, majver, minver, rev);

    status = 0;
  out:
    libs3::mlxx_gof_cmd_makeargv_destroy(&marg);

    if (NULL != majver_p) {
	*majver_p = majver;
    }

    if (NULL != minver_p) {
	*minver_p = minver;
    }

    if (NULL != kernmin_p) {
	*kernmin_p = rev;
    }

    return 0;

#endif

#else
    if (NULL != majver_p) {
	*majver_p = -1;
    }

    if (NULL != minver_p) {
	*minver_p = -1;
    }

    if (NULL != kernmin_p) {
	*kernmin_p = -1;
    }

    return -1;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_macosx_snowleopard(void)
 * @brief OSがMACOSX SnowLeopard(10.6.x)であるか評価します
 * @retval 1 MACOSX SnowLeopard(10.6.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_snowleopard(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 6)) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_macosx_leopard(void)
 * @brief OSがMACOSX Leopard(10.5.x)であるか評価します
 * @retval 1 MACOSX Leopard(10.5.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_leopard(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 5)) {
	return 1;
    }
    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_macosx_lion(void)
 * @brief OSがMACOSX Lion(10.7.x)であるか評価します
 * @retval 1 MACOSX Lion(10.7.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_lion(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 7)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_macosx_mountainlion(void)
 * @brief OSがMACOSX Mountain Lion(10.8.x)であるか評価します
 * @retval 1 MACOSX Mountain Lion(10.8.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_mountainlion(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 8)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::pmx_platform_is_macosx_mavericks(void)
 * @brief OSがMACOSX Mountain Mavericks(10. 9.x)であるか評価します
 * @retval 1 MACOSX Mountain Mavericks(10. 9.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_mavericks(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 9)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif
}


/**
 * @fn int libs3::pmx_platform_is_macosx_yosemite(void);
 * @brief OSがMACOSX Mountain Yosemite(10.10.x)であるか評価します
 * @retval 1 MACOSX Mountain Yosemite(10.10.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_yosemite(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 10)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif

}


/**
 * @fn int libs3::pmx_platform_is_macosx_eicaptain(void);
 * @brief OSがMACOSX Mountain Eicaptain(10.11.x)であるか評価します
 * @retval 1 MACOSX Mountain Eicaptain(10.11.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macosx_eicaptain(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 11)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif

}



/**
 * @fn int libs3::pmx_platform_is_macos_sierra(void);
 * @brief OSがMACOS Sierra(10.12.x)であるか評価します
 * @retval 1 MACOS Sierra(10.12.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macos_sierra(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 12)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif

}


/**
 * @fn int libs3::pmx_platform_is_macos_highsierra(void);
 * @brief OSがMACOS High Sierra(10.13.x)であるか評価します
 * @retval 1 MACOS High Sierra(10.13.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macos_highsierra(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 13)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif

}

/**
 * @fn int libs3::pmx_platform_is_macos_mojave(void)
 * @brief OSがMACOS High Sierra(10.14.x)であるか評価します
 * @retval 1 MACOS High Sierra(10.14.x)である
 * @retval 0 その他のOS
 * @retval -1 不明なエラー
 */
int libs3::pmx_platform_is_macos_mojave(void)
{
#ifdef Darwin
    int result;
    int majver, minver;

    result = libs3::pmx_platform_get_macosx_version(&majver, &minver, NULL);
    if (result) {
	return -1;
    }

    if ((majver == 10) && (minver == 14)) {
	return 1;
    }

    return 0;
#else
    return 0;
#endif

}

/**
 * @fn int libs3::pmx_platform_is_emulate_32bit_on_64bit_kernel(void)
 * @brief 64bitカーネル上で32bitエミュレータを使った動作をしているか判定します。
 *	カーネル及び実行ファイルの判定は、別にやってください
 * @retval 0 32bitエミュレータを使っておらず、たぶんネイティブ
 * @retval 1以上 62bitエミュレータをつかって32bit実行ファイルを動かしている
 * @retval -1 検出に失敗
 */
int libs3::pmx_platform_is_emulate_32bit_on_64bit_kernel(void)
{
#if defined(WIN32)
#if !defined(_WIN64)
    int result;
    BOOL wow64 = FALSE;
    BOOL(WINAPI * dlf_IsWow64ProcessFunc) (HANDLE, PBOOL);
    mpxx_dlfcn_t dl_kernel32;

    result = libs3::mpxx_dlfcn_open(&dl_kernel32, "kernel32.dll", NULL);
    if (result) {
	DBMS1(stderr,
	      "mpxx_platform_is_emulate_32bit_on_64bit_kernel : mpxx_dlfcn_open fail\n");
	return -1;
    }

    dlf_IsWow64ProcessFunc =
	(BOOL(WINAPI *) (HANDLE, PBOOL)) mpxx_dlfcn_sym(&dl_kernel32,
							   "IsWow64Process");
    if (!dlf_IsWow64ProcessFunc) {
	return -1;
    }
    if (!dlf_IsWow64ProcessFunc(GetCurrentProcess(), &wow64)) {
	return 0;
    }

    mpxx_dlfcn_close(&dl_kernel32);

    return (wow64 == TRUE) ? 1 : 0;
#else				/* 32bit */
    return 0;
#endif
#elif defined(Darwin) || defined(Linux)
#if !defined(__x86_64__)
    FILE *fp = NULL;
    char *const cmdline = "/bin/uname -a";
    char buf[BUFSIZ] = { 0 };
    const char *const ident = "x86_64";
    int n = 0;
    bool on64 = false;

    if ((fp = popen(cmdline, "r")) == NULL) {
	return -1;
    }

    while (fgets(buf, BUFSIZ - 1, fp) != NULL) {
	if (NULL != mpxx_strcasestr(buf, ident)) {
	    on64 = true;
	    goto out;
	}
	memset(buf, 0x0, BUFSIZ);
	++n;
    }
  out:
    (void) pclose(fp);

    return on64;
#else
    return 0;
#endif
#elif defined(__AVM2__) || defined(QNX) || defined(XILINX) || defined(H8300)
    return 0;
#else
#error foo
#error `not impliment mpx_is_emulate_32bit_on_64bit_kernel() on this os`
#endif
}

/**
 * @fn int libs3::mpx_platform_is_xilinx_standalone(void)
 * @brief 使用中のカーネルがXilinx Standaine系であることを判定します。
 * @retval 1 Xilinx Xilkernel Standalone系である
 * @retval 0 その他のOS
 */
int libs3::mpx_platform_is_xilinx_standalone(void)
{
#if defined(XILKERNEL) && defined(STANDALONE)
    return 1;
#else
    return 0;
#endif
}

/**
 * @fn int libs3::mpx_platform_is_xilinx_xilkernel(void)
 * @brief 使用中のカーネルがXilinx xilkernel系であることを判定します。
 * @retval 1 Xilinx Xilkernel系である
 * @retval 0 その他のOS
 */
int mpx_platform_is_xilinx_xilkernel(void)
{
#if defined(XILNIX) && !defined(XILKERNEL)
    return 1;
#else
    return 0;
#endif
}

