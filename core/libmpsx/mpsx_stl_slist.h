#ifndef INC_MPSX_STL_SLIST_H
#define INC_MPSX_STL_SLIST_H

#pragma once

#include <stddef.h>

typedef struct _mpsx_stl_slist {
   size_t sizof_element;
   void *ext;
} mpsx_stl_slist_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mpsx_stl_slist_init( mpsx_stl_slist_t *const self_p, const size_t sizof_element);
int mpsx_stl_slist_destroy( mpsx_stl_slist_t *const self_p);
int mpsx_stl_slist_push( mpsx_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpsx_stl_slist_pop( mpsx_stl_slist_t *const self_p);
int mpsx_stl_slist_front( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mpsx_stl_slist_get_element_at( mpsx_stl_slist_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpsx_stl_slist_clear( mpsx_stl_slist_t *const self_p);
size_t mpsx_stl_slist_get_pool_cnt( mpsx_stl_slist_t *const self_p);
int mpsx_stl_slist_is_empty( mpsx_stl_slist_t *const self_p);

int mpsx_stl_slist_back( mpsx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mpsx_stl_slist_insert_at( mpsx_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element );
int mpsx_stl_slist_erase_at( mpsx_stl_slist_t *const self_p, const size_t no);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MPSX_STL_SLIST_H */
