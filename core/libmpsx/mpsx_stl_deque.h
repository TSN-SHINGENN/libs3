#ifndef INC_MPSX_STL_DEQUE_H
#define INC_MPSX_STL_DEQUE_H

#pragma once

#include <stddef.h>

typedef struct _mpsx_stl_deque {
   size_t sizeof_element;
   void *ext;
} mpsx_stl_deque_t;

int mpsx_stl_deque_init( mpsx_stl_deque_t *const self_p, const size_t sizeof_element);
int mpsx_stl_deque_destroy( mpsx_stl_deque_t *const self_p);
int mpsx_stl_deque_push_back( mpsx_stl_deque_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mpsx_stl_deque_push_front( mpsx_stl_deque_t *const self_p, const void *const el_p, const size_t sizof_element );
int mpsx_stl_deque_pop_front( mpsx_stl_deque_t *const self_p);
int mpsx_stl_deque_pop_back( mpsx_stl_deque_t *const self_p);
int mpsx_stl_deque_insert( mpsx_stl_deque_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mpsx_stl_deque_get_element_at( mpsx_stl_deque_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpsx_stl_deque_clear( mpsx_stl_deque_t *const self_p);
size_t mpsx_stl_deque_get_pool_cnt( mpsx_stl_deque_t *const self_p);
int mpsx_stl_deque_is_empty( mpsx_stl_deque_t *const self_p);
int mpsx_stl_deque_remove_at( mpsx_stl_deque_t *const self_p,  const size_t num);
int mpsx_stl_deque_overwrite_element_at( mpsx_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mpsx_stl_deque_front( mpsx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);
int mpsx_stl_deque_back( mpsx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);

int mpsx_stl_deque_element_swap_at( mpsx_stl_deque_t *const self_p, const size_t at1, const size_t at2);

void mpsx_stl_deque_dump_element_region_list( mpsx_stl_deque_t *const self_p);

void *mpsx_stl_deque_ptr_at( mpsx_stl_deque_t *const self_p, const size_t num);

#endif /* end of INC_MPSX_STL_DEQUEUE_H */
