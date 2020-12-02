#ifndef INC_MPXX_STL_DEQUE_H
#define INC_MPXX_STL_DEQUE_H

#pragma once

#include <stddef.h>

typedef struct _mpxx_stl_deque {
   size_t sizeof_element;
   void *ext;
} mpxx_stl_deque_t;

int mpxx_stl_deque_init( mpxx_stl_deque_t *const self_p, const size_t sizeof_element);
int mpxx_stl_deque_destroy( mpxx_stl_deque_t *const self_p);
int mpxx_stl_deque_push_back( mpxx_stl_deque_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mpxx_stl_deque_push_front( mpxx_stl_deque_t *const self_p, const void *const el_p, const size_t sizof_element );
int mpxx_stl_deque_pop_front( mpxx_stl_deque_t *const self_p);
int mpxx_stl_deque_pop_back( mpxx_stl_deque_t *const self_p);
int mpxx_stl_deque_insert( mpxx_stl_deque_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mpxx_stl_deque_get_element_at( mpxx_stl_deque_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpxx_stl_deque_clear( mpxx_stl_deque_t *const self_p);
size_t mpxx_stl_deque_get_pool_cnt( mpxx_stl_deque_t *const self_p);
int mpxx_stl_deque_is_empty( mpxx_stl_deque_t *const self_p);
int mpxx_stl_deque_remove_at( mpxx_stl_deque_t *const self_p,  const size_t num);
int mpxx_stl_deque_overwrite_element_at( mpxx_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mpxx_stl_deque_front( mpxx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);
int mpxx_stl_deque_back( mpxx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);

int mpxx_stl_deque_element_swap_at( mpxx_stl_deque_t *const self_p, const size_t at1, const size_t at2);

void mpxx_stl_deque_dump_element_region_list( mpxx_stl_deque_t *const self_p);

void *mpxx_stl_deque_ptr_at( mpxx_stl_deque_t *const self_p, const size_t num);

#endif /* end of INC_MPXX_STL_DEQUEUE_H */
