#ifndef INC_MPXX_STL_LIST_H
#define INC_MPXX_STL_LIST_H

#pragma once

typedef struct _mpxx_stl_list {
   size_t sizeof_element;
   void *ext;
} mpxx_stl_list_t;

int mpxx_stl_list_init( mpxx_stl_list_t *const self_p, const size_t sizeof_element);
int mpxx_stl_list_destroy( mpxx_stl_list_t *const self_p);
int mpxx_stl_list_push_back( mpxx_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mpxx_stl_list_push_front( mpxx_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element );
int mpxx_stl_list_pop_front( mpxx_stl_list_t *const self_p);
int mpxx_stl_list_pop_back( mpxx_stl_list_t *const self_p);
int mpxx_stl_list_insert( mpxx_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mpxx_stl_list_get_element_at( mpxx_stl_list_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpxx_stl_list_clear( mpxx_stl_list_t *const self_p);
size_t mpxx_stl_list_get_pool_cnt( mpxx_stl_list_t *const self_p);
int mpxx_stl_list_is_empty( mpxx_stl_list_t *const self_p);
int mpxx_stl_list_remove_at( mpxx_stl_list_t *const self_p,  const size_t num);
int mpxx_stl_list_overwrite_element_at( mpxx_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mpxx_stl_list_front( mpxx_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);
int mpxx_stl_list_back( mpxx_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);

void mpxx_stl_list_dump_element_region_list( mpxx_stl_list_t *const self_p);

#endif /* end of INC_MPXX_STL_LIST_H */
