#ifndef INC_MLXX_STL_DEQUE_H
#define INC_MLXX_STL_DEQUE_H

#include <stddef.h>

namespace libs3 {

typedef struct _mlxx_stl_deque {
   size_t sizeof_element;
   void *ext;
} mlxx_stl_deque_t;

int mlxx_stl_deque_init( mlxx_stl_deque_t *const self_p, const size_t sizeof_element);
int mlxx_stl_deque_destroy( mlxx_stl_deque_t *const self_p);
int mlxx_stl_deque_push_back( mlxx_stl_deque_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mlxx_stl_deque_push_front( mlxx_stl_deque_t *const self_p, const void *const el_p, const size_t sizof_element );
int mlxx_stl_deque_pop_front( mlxx_stl_deque_t *const self_p);
int mlxx_stl_deque_pop_back( mlxx_stl_deque_t *const self_p);
int mlxx_stl_deque_insert( mlxx_stl_deque_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mlxx_stl_deque_get_element_at( mlxx_stl_deque_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mlxx_stl_deque_clear( mlxx_stl_deque_t *const self_p);
size_t mlxx_stl_deque_get_pool_cnt( mlxx_stl_deque_t *const self_p);
int mlxx_stl_deque_is_empty( mlxx_stl_deque_t *const self_p);
int mlxx_stl_deque_remove_at( mlxx_stl_deque_t *const self_p,  const size_t num);
int mlxx_stl_deque_overwrite_element_at( mlxx_stl_deque_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mlxx_stl_deque_front( mlxx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);
int mlxx_stl_deque_back( mlxx_stl_deque_t *const self_p, void *const el_p, const size_t sizof_element);

int mlxx_stl_deque_element_swap_at( mlxx_stl_deque_t *const self_p, const size_t at1, const size_t at2);

void mlxx_stl_deque_dump_element_region_list( mlxx_stl_deque_t *const self_p);

void *mlxx_stl_deque_ptr_at( mlxx_stl_deque_t *const self_p, const size_t num);

}

#endif /* end of INC_MLXX_STL_DEQUEUE_H */
