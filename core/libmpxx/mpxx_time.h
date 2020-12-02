#ifndef INC_MPXX_TIME_H
#define INC_MPXX_TIME_H

#pragma once

#include <stdint.h>
#include <time.h>

#ifndef _MPXX_TM_LITE_T

typedef struct _mpxx_tm_lite {
    unsigned int tm_sec;  /* 秒 (0-60) */
    unsigned int tm_min;  /* 分 (0-59) */
    unsigned int tm_hour; /* 時間 (0-23) */
    unsigned int tm_day;  /* 日 (0- ) */
} mpxx_tm_lite_t;

#endif /* end of _MPXX_TM_LITE_T */

#ifndef _MPXX_MTIMESPEC_T
#define _MPXX_MTIMESPEC_T

typedef struct _mpxx_mtimespec {
    uint64_t sec;	/*!< 秒数 */
    uint32_t msec;	/*!< ミリ秒数 */
} mpxx_mtimespec_t;

#endif /* end of _MPXX_MTIMESPEC_T */

#ifndef _MPXX_TIMESPEC_T
#define _MPXX_TIMESPEC_T

typedef struct _mpxx_timespec {
    time_t tv_sec;	/*!< 秒数 */
    uint64_t tv_nsec;	/*!< ナノ秒数 */
} mpxx_timespec_t;

#endif /* end of _MPXX_TIMESPEC_T */

typedef enum _enum_mpxx_clockid {
    MPXX_CLOCK_REALTIME = 1000,	/*!< 高精度実時間　起原は1970/01/01 */
    MPXX_CLOCK_MONOTONIC,		/*!< ある開始時点からの時間　WIN32ではエラーを返す事がある */
} mpxx_clockid_t;

#ifdef __cplusplus
extern "C" {
#endif

int mpxx_clock_getmtime( const mpxx_clockid_t id, mpxx_mtimespec_t * const mts_p);
int mpxx_clock_gettime( const mpxx_clockid_t id, mpxx_timespec_t * const mts_p);
struct tm *mpxx_gmtime_r( const time_t * const time_p, struct tm * const rettmi_p);

#ifdef __cplusplus
}
#endif

#endif /* end of INC_MPXX_TIME_H */
