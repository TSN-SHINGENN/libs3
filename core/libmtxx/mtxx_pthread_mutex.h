#ifndef MTXX_PTHREAD_MUTEX_H
#define MTXX_PTHREAD_MUTEX_H

#pragma once

#include <stdint.h>

#ifndef _MPXX_MTIMESPEC_T
#define _MPXX_MTIMESPEC_T

typedef struct _mpxx_mtimespec {
    uint64_t sec;
    uint32_t msec;
} mpxx_mtimespec_t;

#endif /* end of _MPXX_MTIMESPEC_T */

namespace s3 {
typedef struct _mtxx_pthread_mutex {
    void *m_ext;
} mtxx_pthread_mutex_t;

typedef struct mtxx_pthread_cond {
    void *c_ext;
} mtxx_pthread_cond_t;

    int mtxx_pthread_mutex_init(mtxx_pthread_mutex_t *m_p, const void *attr);
    int mtxx_pthread_mutex_lock(mtxx_pthread_mutex_t *m_p );
    int mtxx_pthread_mutex_unlock(mtxx_pthread_mutex_t *m_p );
    int mtxx_pthread_mutex_destroy(mtxx_pthread_mutex_t *m_p );
    int mtxx_pthread_mutex_trylock(mtxx_pthread_mutex_t *m_p );
    int mtxx_pthread_mutex_timedlock(mtxx_pthread_mutex_t *m_p, mpxx_mtimespec_t *abstime);

    int mtxx_pthread_cond_init(mtxx_pthread_cond_t *cond_p, void *_attr);
    int mtxx_pthread_cond_timedwait(mtxx_pthread_cond_t *cond_p, mtxx_pthread_mutex_t *m_p, mpxx_mtimespec_t *abstime);
    int mtxx_pthread_cond_wait(mtxx_pthread_cond_t *cond_p, mtxx_pthread_mutex_t *m_p);
    int mtxx_pthread_cond_signal(mtxx_pthread_cond_t *cond_p);
    int mtxx_pthread_cond_broadcast(mtxx_pthread_cond_t *cond_p);
    int mtxx_pthread_cond_destroy(mtxx_pthread_cond_t *cond_p);
}

#endif /* MTXX_PTHREAD_MUTEX_H */
