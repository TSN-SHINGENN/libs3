/**
 *	Copyright 2012 TSN-SHINGENN All Rights Reserved.
 *
 *	Basic Author: Seiichi Takeda  '2012-March-01 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file mpsx_time.c
 * @brief 精度の高い時間を取得する等、時間に関係する関数のマルチOS版ライブラリコード
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
#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include <winnt.h>
#if (defined(_MSC_VER) && ( _MSC_VER >= 1400 ))
#include <sys/timeb.h>
#endif
#endif

#ifdef Darwin
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include <time.h>

#if defined(XILKERNEL) && !defined(STANDALONE)
#include <os_config.h>
#include <sys/timer.h>
#endif

/* this */
#include "mpsx_sys_types.h"
#include "mpsx_stdlib.h"
#include "mpsx_time.h"

#ifdef DEBUG
static int debuglevel = 4;
#else
static int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/**
 * @fn static int clock_realtime(mpsx_timespec_t * ts_p)
 * @brief 高精度実時間の取得起原は 1970/01/01
 * @param mts_p mpsx_mtimespec_t構造体ポインタ
 * @retval 0 成功
 * @retval EINVAL : 処理できない
 * @retval EFAULT : mts_pポインタがアクセス不可能
 * @retval ENOTSUP : サポートされていない
 */
static int clock_realtime(mpsx_timespec_t * const ts_p)
{
#if ( defined(WIN32) && defined(_MSC_VER) && ( _MSC_VER < 1400 )) || defined(__MINGW32__)
    /* VC++6.0を使用した場合 や MinGW32 */
    const uint64_t EPOCH_OFFSET = 116444736000000000LL;
    uint64_t st64;
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);

    st64 =
	(uint64_t) ft.dwHighDateTime << 32 | (uint64_t) ft.dwLowDateTime;
    st64 -= EPOCH_OFFSET;
    st64 /= 10;

    ts_p->tv_sec = st64 / 1000000;
    ts_p->tv_nsec = ((st64 / 1000) % 1000) * (1000 * 1000);

#elif defined(WIN32)
    /* VC++2005以上 */
    struct __timeb64 timeb;

    _ftime64_s(&timeb);

    ts_p->tv_sec = (time_t) timeb.time;
    ts_p->tv_nsec = (uint64_t) timeb.millitm * (1000 * 1000);

#elif defined(Darwin)
    /* MACOSX */
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);

    clock_get_time(cclock, &mts);

    mach_port_deallocate(mach_task_self(), cclock);


    ts_p->tv_sec = mts.tv_sec;
    ts_p->tv_nsec = (uint64_t) mts.tv_nsec;
#elif defined(Linux) || defined(QNX) || defined(__AVM2__)
    /* POSIX like */
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);

    ts_p->tv_sec = tp.tv_sec;
    ts_p->tv_nsec = (uint64_t) tp.tv_nsec;
#elif defined(XILKERNEL)
    DMSG( stderr, "not supported %s() function \r\n", __func__);
    abort();
    return ENOTSUP;
#else				/* others */
#error not impliment clock_realtime() in mpsx_time.c
#endif				/* others */

    return 0;
}

/**
 * @fn static int clock_realtime(mpsx_timespec_t * const ts_p)
 * @brief 高精度実時間の取得.起原は多分OS起動から 
 *	WIN32ではCPUのRDTSC命令を使う場合は正確な値が得られない
 *	とりあえず 基準クロックが200MHz以上の場合はEINVALとする。
 * @param mts_p mpsx_mtimespec_t構造体ポインタ
 * @retval 0 成功
 * @retval EINVAL : 処理できない
 * @retval EFAULT : mts_pポインタがアクセス不可能
 */
static int clock_monotinoc(mpsx_timespec_t * const ts_p)
{
#if defined(WIN32) || defined(__MINGW32__)
    /* CPUのカウンタや未サポートの場合はエラーとする */
    LARGE_INTEGER LIcounts, LIfreq;
    int64_t counts, freq;
    BOOL bresult;


    bresult = QueryPerformanceCounter(&LIcounts);
    if (bresult == 0) {		/* false */
	return EINVAL;
    }
    bresult = QueryPerformanceFrequency(&LIfreq);
    if (bresult == 0) {		/* false */
	return EINVAL;
    }

    counts = (int64_t) LIcounts.QuadPart;
    freq = (int64_t) LIfreq.QuadPart;

    IFDBG3THEN {
	const size_t xtoa_bufsz=MPSX_XTOA_DEC64_BUFSZ;
	char xtoa_buf[MPSX_XTOA_DEC64_BUFSZ];

	mpsx_i64toadec( (int64_t)counts, xtoa_buf, xtoa_bufsz);
	DBMS3(stderr, "clock_monotinoc counts=%s" EOL_CRLF, xtoa_buf);
	mpsx_i64toadec( (int64_t)freq, xtoa_buf, xtoa_bufsz);
	DBMS3(stderr, "clock_monotinoc freq=%s" EOL_CRLF, xtoa_buf);
    }

    if (freq >= 2LL * 1000LL * 1000LL * 1000LL) {
	/* 200MHz 以上の場合はCPUカウンタを使っているとしてエラーとする */
	return EINVAL;
    }
    ts_p->tv_sec = (time_t) (counts / freq);
    ts_p->tv_nsec =
	(int64_t) (counts % freq) * ((int64_t) (1000 * 1000 * 1000) /
				     freq);

#elif defined(Darwin)
    /* MACOSX */
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);

    clock_get_time(cclock, &mts);

    mach_port_deallocate(mach_task_self(), cclock);


    ts_p->tv_sec = mts.tv_sec;
    ts_p->tv_nsec = (uint64_t) mts.tv_nsec;

#elif defined(Linux) || defined(QNX) || defined(__AVM2__)    /* POSIX like */
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    ts_p->tv_sec = tp.tv_sec;
    ts_p->tv_nsec = (uint64_t) tp.tv_nsec;
#elif (defined(XILKERNEL) && !defined(STANDALONE))

#define SYS_MSPERTICK (SYSTMR_INTERVAL/SYSTMR_CLK_FREQ_KHZ)
#define TICKS_TO_MS(x) ((x) * SYS_MSPERTICK)

    unsigned int ticks = xget_clock_ticks();
    unsigned int ms = TICKS_TO_MS(ticks);

    ts_p->tv_sec = ms / 1000;
    ts_p->tv_nsec = (ms % 1000) * 1000;

#elif defined(XILKERNEL) && defined(STANDALONE)
    DMSG( stderr, "not supported %s() function \r\n", __func__);
    abort();
    return ENOTSUP;
#else				/* others */
#error not impliment clock_monotinoc() in mpsx_time.c
#endif

    return 0;
}

/**
 * @fn int mpsx_clock_getmtime( mpsx_clockid_t id, mpsx_mtimespec_t *mts_p)
 * @brief POSIX clock_gettime()のマルチOS版, 精度はちょっと劣る
 * @param id mpsx_clockid_t値
 * @param mts_p mpsx_mtimespec_t構造体ポインタ
 *	sec : 秒
 *	msec: ミリ秒
 * @retval 0 成功
 * @retval EINVAL : サポートしてないID
 * @retval EFAULT : mts_pポインタがアクセス不可能
 */
int mpsx_clock_getmtime(const mpsx_clockid_t id,
			   mpsx_mtimespec_t * const mts_p)
{
    int status;
    mpsx_timespec_t ts;

    DBMS5(stderr, "mpsx_clock_getmtime : execute" EOL_CRLF);

    if (NULL == mts_p) {
	return EFAULT;
    }

    switch (id) {
    case MPSX_CLOCK_REALTIME:
	status = clock_realtime(&ts);
	if (status) {
	    goto out;
	}
	break;
    case MPSX_CLOCK_MONOTONIC:
	status = clock_monotinoc(&ts);
	if (status) {
	    goto out;
	}
	break;
    default:
	return EINVAL;
    }

    mts_p->sec = (uint64_t) ts.tv_sec;
    mts_p->msec = (uint32_t) (ts.tv_nsec / (1000 * 1000));
    status = 0;
  out:

    if (status) {
	errno = status;
    }

    return status;
}


/**
 * @fn int mpsx_clock_getmtime( mpsx_clockid_t id, mpsx_mtimespec_t *mts_p)
 * @brief POSIX clock_gettime()のマルチOS版, 精度はWIN32基準のためちょっと劣る
 * @param id mpsx_clockid_t値
 * @param mts_p mpsx_mtimespec_t構造体ポインタ
 *	sec : 秒
 *	msec: ミリ秒
 * @retval 0 成功
 * @retval EINVAL : サポートしてないID
 * @retval EFAULT : mts_pポインタがアクセス不可能
 */
int mpsx_clock_gettime(const mpsx_clockid_t id,
			  mpsx_timespec_t * const mts_p)
{
    int status;

    if (NULL == mts_p) {
	return EFAULT;
    }

    switch (id) {
    case MPSX_CLOCK_REALTIME:
	status = clock_realtime(mts_p);
	if (status) {
	    goto out;
	}
	break;
    case MPSX_CLOCK_MONOTONIC:
	status = clock_monotinoc(mts_p);
	if (status) {
	    goto out;
	}
	break;
    default:
	status = EINVAL;
	goto out;
    }
    status = 0;
  out:
    if (status) {
	errno = status;
    }
    return status;
}

/**
 * @fn struct tm *mpsx_gmtime_r( const time_t * const time_p, struct tm * const rettm_p)
 * @brief time_t型のカレンダー時刻をstruct tm構造体型に変換します。カレンダー時刻はEpoch時刻期限（TUC)からの経過秒数を返します。
 * 	POSXIで規定されている gmtime_r()のマルチOS版です。
 *	スレッドセーフです。
 * @param time_p time_t型入力変数ポインタ
 * @param rettm_p 戻り値として返すstruct tm構造体型変数ポインタ
 * @retval NULL 失敗
 * @retval NULL以外 成功(retmp_pで指定されたポインタ値）
 */
struct tm *mpsx_gmtime_r(const time_t * const time_p,
			    struct tm *const rettm_p)
{
    if (NULL == rettm_p) {
	return NULL;
    }
#if defined(WIN32)
    /**
     * @note WIN32にでは gmtime()自信がスレッドセーフ(TLS使用）のため戻り値をコピーする
     **/
    {
	const struct tm *const r = gmtime(time_p);
	if (MPSX_HINT_UNLIKELY(NULL == r)) {
	    return NULL;
	}
	*rettm_p = *r;
	return rettm_p;
    }
#elif defined(Linux) || defined(Darwin) || defined(QNX) || defined(XILKERNEL)
    /* POSIX like */
    return gmtime_r(time_p, rettm_p);
#else
#error not impliment mpsx_gmtime_r()
#endif
}
