#ifndef INC_MTXX_PTHREAD_H
#define INX_MTXX_PTHREAD_H

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace s3 {

#ifdef WIN32
    /* Microsoft Windows Series */
    typedef HANDLE mtxx_pthread_t;
#else
    /* UNIX Series */
    typedef pthread_t mtxx_pthread_t;
#endif

	int mtxx_pthread_create(mtxx_pthread_t * pt, void *attr,
			    void *(*start_routine) (void *), void *arg);
	int mtxx_pthread_join(mtxx_pthread_t *const pt, void **const value_ptr);
	void mtxx_pthread_exit(void *value_ptr);
	void mtxx_sched_yield(void);
	int mtxx_pthread_cancel(mtxx_pthread_t *const pt );
	int mtxx_pthread_detach(mtxx_pthread_t *const pt );

	int mtxx_pthread_setaffinity_np(mtxx_pthread_t *pt,
			const uint32_t cpu_mask);

	mtxx_pthread_t pthread_self(void);
}

#endif	/* INC_MTXX_PTHREAD_H */
