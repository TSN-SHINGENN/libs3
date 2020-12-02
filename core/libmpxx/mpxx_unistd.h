#ifndef MPXX_UNISTD_H
#define MPXX_UNISTD_H

#include <stdint.h>

#if defined(Linux) || defined(Darwin) || defined(QNX)
#define MPXX_STDIN_FILENO STDIN_FILENO
#define MPXX_STDOUT_FILENO STDOUT_FILENO
#define MPXX_STDERR_FILENO STDERR_FILENO
#elif defined(WIN32)
#define MPXX_STDIN_FILENO 0
#define MPXX_STDOUT_FILENO 1
#define MPXX_STDERR_FILENO 2
#endif

#if defined( __cplusplus)
extern "C" {
#endif

unsigned int mpxx_sleep( const unsigned int seconds );
void mpxx_msleep( const unsigned int miliseconds );

typedef enum _enum_mpxx_sysconf {
	MPXX_SYSCNF_PAGESIZE = 100,	 /* メモリのページサイズ（バイト） */
	MPXX_SYSCNF_NPROCESSORS_CONF, /* 設定されたCPUコア数 */
	MPXX_SYSCNF_NPROCESSORS_ONLN, /* 使用可能なCPUコア数 */
	MPXX_SYSCNF_PHYS_PAGES,	 /* OSが認識している物理メモリのページ数 */
	MPXX_SYSCNF_AVPHYS_PAGES,	 /* 現在利用可能な物理メモリのページ数 */
} enum_mpxx_sysconf_t;

int64_t mpxx_sysconf( const enum_mpxx_sysconf_t name );

int mpxx_is_user_an_admin(void);

int mpxx_get_username( char* name, const size_t maxlen);
int mpxx_get_hostname( char* name, const size_t maxlen);

int mpxx_fork(void);


#if defined(__cplusplus)
}
#endif

#endif /* end of MPXX_UNISTD_H */

