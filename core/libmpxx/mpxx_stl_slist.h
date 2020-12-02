#ifndef INC_MPXX_STL_SLIST_H
#define INC_MPXX_STL_SLIST_H

#pragma once

#include <stddef.h>

typedef struct _mpxx_stl_slist {
   size_t sizof_element;
   void *ext;
} mpxx_stl_slist_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mpxx_stl_slist_init( mpxx_stl_slist_t *const self_p, const size_t sizof_element);
int mpxx_stl_slist_destroy( mpxx_stl_slist_t *const self_p);
int mpxx_stl_slist_push( mpxx_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element);
int mpxx_stl_slist_pop( mpxx_stl_slist_t *const self_p);
int mpxx_stl_slist_front( mpxx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mpxx_stl_slist_get_element_at( mpxx_stl_slist_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int mpxx_stl_slist_clear( mpxx_stl_slist_t *const self_p);
size_t mpxx_stl_slist_get_pool_cnt( mpxx_stl_slist_t *const self_p);
int mpxx_stl_slist_is_empty( mpxx_stl_slist_t *const self_p);

int mpxx_stl_slist_back( mpxx_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int mpxx_stl_slist_insert_at( mpxx_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element );
int mpxx_stl_slist_erase_at( mpxx_stl_slist_t *const self_p, const size_t no);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MPXX_STL_SLIST_H */
