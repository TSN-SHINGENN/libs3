#ifndef INC_MPSX_STL_VECTOR_H
#define INC_MPSX_STL_VECTOR_H

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef union _mpsx_stl_vector_attr {
    unsigned int flags;
    struct {
	unsigned int is_aligned16:1; /* 128 SSE向け */
	unsigned int is_aligned32:1; /* 256 AVX向け */
    } f;
} mpsx_stl_vector_attr_t;

typedef struct _mpsx_stl_vector {
    size_t sizof_element;
    void *ext;
} mpsx_stl_vector_t;

int mpsx_stl_vector_init( mpsx_stl_vector_t *const self_p, const size_t sizof_element);
int mpsx_stl_vector_init_ex( mpsx_stl_vector_t *const self_p, const size_t sizof_element, mpsx_stl_vector_attr_t *const attr_p);
int mpsx_stl_vector_attach_memory_fixed( mpsx_stl_vector_t *const self_p, void *const mem_ptr, const size_t len);

int mpsx_stl_vector_destroy( mpsx_stl_vector_t *const self_p);

int mpsx_stl_vector_push_back( mpsx_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpsx_stl_vector_pop_back( mpsx_stl_vector_t *const self_p);

int mpsx_stl_vector_resize( mpsx_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element);
int mpsx_stl_vector_reserve( mpsx_stl_vector_t *const self_p, const size_t num_elements);
int mpsx_stl_vector_clear( mpsx_stl_vector_t *const self_p);

size_t mpsx_stl_vector_capacity( mpsx_stl_vector_t *const self_p);
void *mpsx_stl_vector_ptr_at( mpsx_stl_vector_t *const self_p, const size_t num);
int mpsx_stl_vector_is_empty( mpsx_stl_vector_t *const self_p);

size_t mpsx_stl_vector_size( mpsx_stl_vector_t *const self_p);
size_t mpsx_stl_vector_get_pool_cnt( mpsx_stl_vector_t *const self_p);

int mpsx_stl_vector_front( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);
int mpsx_stl_vector_back( mpsx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);

int mpsx_stl_vector_insert( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mpsx_stl_vector_remove_at( mpsx_stl_vector_t *const self_p, const size_t num);

int mpsx_stl_vector_get_element_at( mpsx_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpsx_stl_vector_overwrite_element_at( mpsx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mpsx_stl_vector_element_swap_at( mpsx_stl_vector_t *const self_p, const size_t at1, const size_t at2);

int mpsx_stl_vector_shrink( mpsx_stl_vector_t *const self_p, const size_t num_elements);

#endif /* end of INC_MPSX_STL_VECTOR_H */
