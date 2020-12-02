#ifndef INC_MPSX_STL_QUEUE_H
#define INC_MPSX_STL_QUEUE_H

#pragma once

#include <stddef.h>

typedef enum _mpsx_stl_queue_implement_type {
    MPSX_STL_QUEUE_TYPE_IS_DEFAULT = 11,
    MPSX_STL_QUEUE_TYPE_IS_SLIST,
    MPSX_STL_QUEUE_TYPE_IS_DEQUE,
    MPSX_STL_QUEUE_TYPE_IS_LIST,
    MPSX_STL_QUEUE_TYPE_IS_OTHERS
} enum_mpsx_stl_queue_implement_type_t;

typedef struct _mpsx_stl_queue {
   size_t sizof_element;
   void *ext;
} mpsx_stl_queue_t;

int mpsx_stl_queue_init( mpsx_stl_queue_t *const self_p, const size_t sizof_element);
int mpsx_stl_queue_init_ex( mpsx_stl_queue_t *const self_p, const size_t sizof_element, const enum_mpsx_stl_queue_implement_type_t implement_type, void *const attr_p);

int mpsx_stl_queue_destroy( mpsx_stl_queue_t *const self_p);
int mpsx_stl_queue_push( mpsx_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpsx_stl_queue_pop( mpsx_stl_queue_t *const self_p);
int mpsx_stl_queue_front( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element );
int mpsx_stl_queue_get_element_at( mpsx_stl_queue_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpsx_stl_queue_clear( mpsx_stl_queue_t *const self_p);
size_t mpsx_stl_queue_get_pool_cnt( mpsx_stl_queue_t *const self_p);
int mpsx_stl_queue_is_empty( mpsx_stl_queue_t *const self_p);

int mpsx_stl_queue_back( mpsx_stl_queue_t *const self_p, void *const el_p, const size_t  sizof_element );

#endif /* end of INC_MPSX_STL_QUEUE_H */
