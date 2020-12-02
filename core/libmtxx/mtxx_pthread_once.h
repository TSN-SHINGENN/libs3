#ifndef INC_MTXX_PTHREAD_ONCE_H
#define INC_MTXX_PTHREAD_ONCE_H

#pragma once

#define MTXX_PTHREAD_ONCE_INIT ((short)(-1))

namespace s3 {

typedef short mtxx_pthread_once_t;


typedef struct _mtxx_pthread_once_exec {
    void *ext;
} mtxx_pthread_once_exec_t;

int mtxx_pthread_once( mtxx_pthread_once_t *once_control, void (*init_routine)(void));

int mtxx_pthread_once_exec_init(void);
int mtxx_pthread_once_exec( mtxx_pthread_once_exec_t o, void (*init_routine)(void));
int mtxx_pthread_once_exec_reset_instance( mtxx_pthread_once_exec_t o);
int mtxx_pthread_once_exec_get_instance(mtxx_pthread_once_exec_t *o_p);
int mtxx_pthread_once_exec_get_number_of_remaining_instance(int *max_p);

}

#endif	/* INC_MTXX_PTHREAD_ONCE_H */
