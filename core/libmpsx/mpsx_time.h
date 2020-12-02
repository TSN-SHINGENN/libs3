#ifndef INC_MPSX_TIME_H
#define INC_MPSX_TIME_H

#pragma once

#include <stdint.h>
#include <time.h>

#ifndef _MPSX_TM_LITE_T

typedef struct _mpsx_tm_lite {
    unsigned int tm_sec;  /* 秒 (0-60) */
    unsigned int tm_min;  /* 分 (0-59) */
    unsigned int tm_hour; /* 時間 (0-23) */
    unsigned int tm_day;  /* 日 (0- ) */
} mpsx_tm_lite_t;

#endif /* end of _MPSX_TM_LITE_T */

#ifndef _MPSX_MTIMESPEC_T
#define _MPSX_MTIMESPEC_T

typedef struct _mpsx_mtimespec {
    uint64_t sec;	/*!< 秒数 */
    uint32_t msec;	/*!< ミリ秒数 */
} mpsx_mtimespec_t;

#endif /* end of _MPSX_MTIMESPEC_T */

#ifndef _MPSX_TIMESPEC_T
#define _MPSX_TIMESPEC_T

typedef struct _mpsx_timespec {
    time_t tv_sec;	/*!< 秒数 */
    uint64_t tv_nsec;	/*!< ナノ秒数 */
} mpsx_timespec_t;

#endif /* end of _MPSX_TIMESPEC_T */

typedef enum _enum_mpsx_clockid {
    MPSX_CLOCK_REALTIME = 1000,	/*!< 高精度実時間　起原は1970/01/01 */
    MPSX_CLOCK_MONOTONIC,		/*!< ある開始時点からの時間　WIN32ではエラーを返す事がある */
} mpsx_clockid_t;

#if defined(__cplusplus)
extern "C" {
#endif

int mpsx_clock_getmtime( const mpsx_clockid_t id, mpsx_mtimespec_t * const mts_p);
int mpsx_clock_gettime( const mpsx_clockid_t id, mpsx_timespec_t * const mts_p);
struct tm *mpsx_gmtime_r( const time_t * const time_p, struct tm * const rettmi_p);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_TIME_H */
