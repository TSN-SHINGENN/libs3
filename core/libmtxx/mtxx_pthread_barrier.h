#ifndef INC_MTXX_PTHREAD_BARRIER_H
#define INC_MTXX_PTHREAD_BARRIER_H

#pragma once

namespace s3 {

typedef struct _mtxx_pthread_barrier {
    void *m_ext;
} mtxx_pthread_barrier_t;

int mtxx_pthread_barrier_init( mtxx_pthread_barrier_t *const barrier, const void*attr, unsigned int count);
int mtxx_pthread_barrier_wait( mtxx_pthread_barrier_t *const barrier );
int mtxx_pthread_barrier_destroy( mtxx_pthread_barrier_t *const barrier );

}

#endif /* end of INC_MTXX_PTHREAD_BARRIER_H */
