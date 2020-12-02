#ifndef INC_MLXX_STL_VECTOR_H
#define INC_MLXX_STL_VECTOR_H

#include <stddef.h>
#include <stdint.h>

namespace libs3 {

typedef union _mlxx_stl_vector_attr {
    unsigned int flags;
    struct {
	unsigned int is_aligned16:1; /* 128 SSE向け */
	unsigned int is_aligned32:1; /* 256 AVX向け */
    } f;
} mlxx_stl_vector_attr_t;

typedef struct _mlxx_stl_vector {
    size_t sizof_element;
    void *ext;
} mlxx_stl_vector_t;

int mlxx_stl_vector_init( mlxx_stl_vector_t *const self_p, const size_t sizof_element);
int mlxx_stl_vector_init_ex( mlxx_stl_vector_t *const self_p, const size_t sizof_element, mlxx_stl_vector_attr_t *const attr_p);
int mlxx_stl_vector_attach_memory_fixed( mlxx_stl_vector_t *const self_p, void *const mem_ptr, const size_t len);

int mlxx_stl_vector_destroy( mlxx_stl_vector_t *const self_p);

int mlxx_stl_vector_push_back( mlxx_stl_vector_t *const self_p, const void *const el_p, const size_t sizof_element);
int mlxx_stl_vector_pop_back( mlxx_stl_vector_t *const self_p);

int mlxx_stl_vector_resize( mlxx_stl_vector_t *const self_p, const size_t num_elements, const void *const el_p, const size_t sizof_element);
int mlxx_stl_vector_reserve( mlxx_stl_vector_t *const self_p, const size_t num_elements);
int mlxx_stl_vector_clear( mlxx_stl_vector_t *const self_p);

size_t mlxx_stl_vector_capacity( mlxx_stl_vector_t *const self_p);
void *mlxx_stl_vector_ptr_at( mlxx_stl_vector_t *const self_p, const size_t num);
int mlxx_stl_vector_is_empty( mlxx_stl_vector_t *const self_p);

size_t mlxx_stl_vector_size( mlxx_stl_vector_t *const self_p);
size_t mlxx_stl_vector_get_pool_cnt( mlxx_stl_vector_t *const self_p);

int mlxx_stl_vector_front( mlxx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);
int mlxx_stl_vector_back( mlxx_stl_vector_t *const self_p, void *const el_p, const size_t sizof_element);

int mlxx_stl_vector_insert( mlxx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mlxx_stl_vector_remove_at( mlxx_stl_vector_t *const self_p, const size_t num);

int mlxx_stl_vector_get_element_at( mlxx_stl_vector_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mlxx_stl_vector_overwrite_element_at( mlxx_stl_vector_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element);

int mlxx_stl_vector_element_swap_at( mlxx_stl_vector_t *const self_p, const size_t at1, const size_t at2);

int mlxx_stl_vector_shrink( mlxx_stl_vector_t *const self_p, const size_t num_elements);

}

#endif /* end of INC_MLXX_STL_VECTOR_H */
