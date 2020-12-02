#ifndef INC_MPXX_STL_VECTOR_H
#define INC_MPXX_STL_VECTOR_H

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef union _mpxx_stl_vector_attr {
    unsigned int flags;
    struct {
	unsigned int is_aligned16:1; /* 128 SSE向け */
	unsigned int is_aligned32:1; /* 256 AVX向け */
    } f;
} mpxx_stl_vector_attr_t;

typedef struct _mpxx_stl_vector {
    size_t sizof_element;
    void *ext;
} mpxx_stl_vector_t;

int mpxx_stl_vector_init( mpxx_stl_vector_t *const self_p, const size_t sizof_element);
int mpxx_stl_vector_init_ex( mpxx_stl_vector_t *const self_p, const size_t sizof_element, mpxx_stl_vector_attr_t *const attr_p);
int mpxx_stl_vector_attach_memory_fixed( mpxx_stl_vector_t *const self_p, void *const mem_ptr, const size_t len);

int mpxx_stl_vector_destroy( mpxx_stl_vector_t *const self_p);

int mpxx_stl_vector_push_back( mpxx_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpxx_stl_vector_pop_back( mpxx_stl_vector_t *const self_p);

int mpxx_stl_vector_resize( mpxx_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element);
int mpxx_stl_vector_reserve( mpxx_stl_vector_t *const self_p, const size_t num_elements);
int mpxx_stl_vector_clear( mpxx_stl_vector_t *const self_p);

size_t mpxx_stl_vector_capacity( mpxx_stl_vector_t *const self_p);
void *mpxx_stl_vector_ptr_at( mpxx_stl_vector_t *const self_p, const size_t num);
int mpxx_stl_vector_is_empty( mpxx_stl_vector_t *const self_p);

size_t mpxx_stl_vector_size( mpxx_stl_vector_t *const self_p);
size_t mpxx_stl_vector_get_pool_cnt( mpxx_stl_vector_t *const self_p);

int mpxx_stl_vector_front( mpxx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);
int mpxx_stl_vector_back( mpxx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);

int mpxx_stl_vector_insert( mpxx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mpxx_stl_vector_remove_at( mpxx_stl_vector_t *const self_p, const size_t num);

int mpxx_stl_vector_get_element_at( mpxx_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpxx_stl_vector_overwrite_element_at( mpxx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mpxx_stl_vector_element_swap_at( mpxx_stl_vector_t *const self_p, const size_t at1, const size_t at2);

int mpxx_stl_vector_shrink( mpxx_stl_vector_t *const self_p, const size_t num_elements);

#endif /* end of INC_MPXX_STL_VECTOR_H */
