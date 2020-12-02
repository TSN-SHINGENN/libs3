#ifndef INC_WIN32E_PTHREAD_MUTEX_COND_H
#define INC_WIN32E_PTHREAD_MUTEX_COND_H

#pragma once

#include <windows.h>
#include <libmpxx/mpxx_time.h>

typedef struct _win32e_pthread_mutex {
//    CRITICAL_SECTION g_section;
    HANDLE hSem;
    win32e_pthread_cond_t *cond_ext;
} win32e_pthread_mutex_t;

/**
 * @struct win32_pthread_cond_t
 * @brief pthread_condのWin32エミュレータクラスオブジェクト構造体
 **/
 typedef struct win32e_pthread_cond {
    int m_waiters_count;
    int m_was_broadcast;

    HANDLE m_Sem;
    HANDLE m_waiters_done;
    CRITICAL_SECTION m_waiters_count_lock;

} win32e_pthread_cond_t;

#ifdef __cplusplus
extern "C" {
#endif

int win32e_pthread_mutex_init( win32e_pthread_mutex_t *m_p, const void *attr);
int win32e_pthread_mutex_destroy( win32e_pthread_mutex_t*m_p );
int win32e_pthread_mutex_lock( win32e_pthread_mutex_t *m_p);
int win32e_pthread_mutex_trylock( win32e_pthread_mutex_t *m_p);
int win32e_pthread_mutex_unlock( win32e_pthread_mutex_t *m_p);
int win32e_pthread_mutex_timedlock( win32e_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts);

int win32e_pthread_cond_init(win32e_pthread_cond_t * cv, void *attr);
int win32e_pthread_cond_wait(win32e_pthread_cond_t * cv,
			    win32e_pthread_mutex_t * external_mutex);
int win32e_pthread_cond_signal(win32e_pthread_cond_t * cv);
int win32e_pthread_cond_broadcast(win32e_pthread_cond_t * cv);
int win32e_pthread_cond_destroy(win32e_pthread_cond_t * cv);
int win32e_pthread_cond_timedwait(win32e_pthread_cond_t * cv,
	 win32e_pthread_mutex_t* external_mutex, mpxx_mtimespec_t *abstime);

#ifdef __cplusplus
}
#endif

#endif /* end of INC_WIN32E_PTHREAD_MUTEX_COND */
