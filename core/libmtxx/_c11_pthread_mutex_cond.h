#ifndef INC_C11_PTHREAD_MUTEX_COND_H
#define INC_C11_PTHREAD_MUTEX_COND_H

#pragma once

#include <mutex>
#include <libmpxx/mpxx_time_h>

namespace s3 {

typedef struct _c11_pthread_mutex {
    std::mutex *m_;
} c11_pthread_mutex_t;

typedef struct _c11_pthread_cond {
    std::condition_variable *cv_;
} c11_pthread_cond_t;

int c11_pthread_mutex_init( c11_pthread_mutex_t *m_p, const void *attr);
int c11_pthread_mutex_destroy( c11_pthread_mutex_t*m_p );
int c11_pthread_mutex_lock( c11_pthread_mutex_t *m_p);
int c11_pthread_mutex_trylock( c11_pthread_mutex_t *m_p);
int c11_pthread_mutex_unlock( c11_pthread_mutex_t *m_p);
int c11_pthread_mutex_timedlock( c11_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts);

int c11_pthread_cond_init(c11_pthread_cond_t * cv, void *attr);
int c11_pthread_cond_wait(c11_pthread_cond_t * cv,
			    c11_pthread_mutex_t * external_mutex);
int c11_pthread_cond_signal(c11_pthread_cond_t * cv);
int c11_pthread_cond_broadcast(c11_pthread_cond_t * cv);
int c11_pthread_cond_destroy(c11_pthread_cond_t * cv);
int c11_pthread_cond_timedwait(c11_pthread_cond_t * cv,
	 c11_pthread_mutex_t* external_mutex, mpxx_mtimespec_t *abstime_p);

}

#endif /* end of INC_C11_PTHREAD_MUTEX_COND_H */
