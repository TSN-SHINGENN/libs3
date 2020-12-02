#ifndef INC_MPSX_STL_LIST_H
#define INC_MPSX_STL_LIST_H

#pragma once

typedef struct _mpsx_stl_list {
   size_t sizeof_element;
   void *ext;
} mpsx_stl_list_t;

int mpsx_stl_list_init( mpsx_stl_list_t *const self_p, const size_t sizeof_element);
int mpsx_stl_list_destroy( mpsx_stl_list_t *const self_p);
int mpsx_stl_list_push_back( mpsx_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element);
int mpsx_stl_list_push_front( mpsx_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element );
int mpsx_stl_list_pop_front( mpsx_stl_list_t *const self_p);
int mpsx_stl_list_pop_back( mpsx_stl_list_t *const self_p);
int mpsx_stl_list_insert( mpsx_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int mpsx_stl_list_get_element_at( mpsx_stl_list_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpsx_stl_list_clear( mpsx_stl_list_t *const self_p);
size_t mpsx_stl_list_get_pool_cnt( mpsx_stl_list_t *const self_p);
int mpsx_stl_list_is_empty( mpsx_stl_list_t *const self_p);
int mpsx_stl_list_remove_at( mpsx_stl_list_t *const self_p,  const size_t num);
int mpsx_stl_list_overwrite_element_at( mpsx_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int mpsx_stl_list_front( mpsx_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);
int mpsx_stl_list_back( mpsx_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);

void mpsx_stl_list_dump_element_region_list( mpsx_stl_list_t *const self_p);

#endif /* end of INC_MPSX_STL_LIST_H */
