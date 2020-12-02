#ifndef INC_MPXX_STL_QUEUE_H
#define INC_MPXX_STL_QUEUE_H

#pragma once

#include <stddef.h>

typedef enum _mpxx_stl_queue_implement_type {
    MPXX_STL_QUEUE_TYPE_IS_DEFAULT = 11,
    MPXX_STL_QUEUE_TYPE_IS_SLIST,
    MPXX_STL_QUEUE_TYPE_IS_DEQUE,
    MPXX_STL_QUEUE_TYPE_IS_LIST,
    MPXX_STL_QUEUE_TYPE_IS_OTHERS
} enum_mpxx_stl_queue_implement_type_t;

typedef struct _mpxx_stl_queue {
   size_t sizof_element;
   void *ext;
} mpxx_stl_queue_t;

int mpxx_stl_queue_init( mpxx_stl_queue_t *const self_p, const size_t sizof_element);
int mpxx_stl_queue_init_ex( mpxx_stl_queue_t *const self_p, const size_t sizof_element, const enum_mpxx_stl_queue_implement_type_t implement_type, void *const attr_p);

int mpxx_stl_queue_destroy( mpxx_stl_queue_t *const self_p);
int mpxx_stl_queue_push( mpxx_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpxx_stl_queue_pop( mpxx_stl_queue_t *const self_p);
int mpxx_stl_queue_front( mpxx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element );
int mpxx_stl_queue_get_element_at( mpxx_stl_queue_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpxx_stl_queue_clear( mpxx_stl_queue_t *const self_p);
size_t mpxx_stl_queue_get_pool_cnt( mpxx_stl_queue_t *const self_p);
int mpxx_stl_queue_is_empty( mpxx_stl_queue_t *const self_p);

int mpxx_stl_queue_back( mpxx_stl_queue_t *const self_p, void *const el_p, const size_t  sizof_element );

#endif /* end of INC_MPXX_STL_QUEUE_H */
