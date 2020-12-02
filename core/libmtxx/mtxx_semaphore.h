#ifndef INC_MTXX_SEMAPHORE_H
#define INC_MTXX_SEMAPHORE_H

namespace s3 {

typedef struct _mtxx_sem {
   void *ext;
} mtxx_sem_t;

int mtxx_sem_init( mtxx_sem_t *sem, int pshared, unsigned int value);
int mtxx_sem_wait( mtxx_sem_t *sem );
int mtxx_sem_trywait( mtxx_sem_t * sem);
int mtxx_sem_post( mtxx_sem_t * sem);
int mtxx_sem_getvalue( mtxx_sem_t * sem, int * sval);
int mtxx_sem_destroy( mtxx_sem_t * sem);

}

#endif /* end of INC_MTXX_SEMAPHORE_H */
