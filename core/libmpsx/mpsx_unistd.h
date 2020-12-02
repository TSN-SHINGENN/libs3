#ifndef INC_MPSX_UNISTD_H
#define INC_MPSX_UNISTD_H

#pragma once

#include <stdint.h>

#if defined(Linux) || defined(Darwin) || defined(QNX)
#define MPSX_STDIN_FILENO STDIN_FILENO
#define MPSX_STDOUT_FILENO STDOUT_FILENO
#define MPSX_STDERR_FILENO STDERR_FILENO
#elif defined(WIN32)
#define MPSX_STDIN_FILENO 0
#define MPSX_STDOUT_FILENO 1
#define MPSX_STDERR_FILENO 2
#endif

#if defined( __cplusplus)
extern "C" {
#endif

unsigned int mpsx_sleep( const unsigned int seconds );
void mpsx_msleep( const unsigned int miliseconds );

typedef enum _enum_mpsx_sysconf {
    MPSX_SYSCNF_PAGESIZE = 100,	 /* メモリのページサイズ（バイト） */
    MPSX_SYSCNF_NPROCESSORS_CONF, /* 設定されたCPUコア数 */
    MPSX_SYSCNF_NPROCESSORS_ONLN, /* 使用可能なCPUコア数 */
    MPSX_SYSCNF_PHYS_PAGES,	 /* OSが認識している物理メモリのページ数 */
    MPSX_SYSCNF_AVPHYS_PAGES,	 /* 現在利用可能な物理メモリのページ数 */
} enum_mpsx_sysconf_t;

int64_t mpsx_sysconf( const enum_mpsx_sysconf_t name );

int mpsx_is_user_an_admin(void);

int mpsx_get_username( char* name, const size_t maxlen);
int mpsx_get_hostname( char* name, const size_t maxlen);

int mpsx_fork(void);


#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_UNISTD_H */

