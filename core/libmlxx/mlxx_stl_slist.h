#ifndef INC_MULTIOS_STL_SLIST_H
#define INC_MULTIOS_STL_SLIST_H

#include <stddef.h>

typedef struct _multios_stl_slist {
   size_t sizof_element;
   void *ext;
} multios_stl_slist_t;

#if defined (__cplusplus )
extern "C" {
#endif

int multios_stl_slist_init( multios_stl_slist_t *const self_p, const size_t sizof_element);
int multios_stl_slist_destroy( multios_stl_slist_t *const self_p);
int multios_stl_slist_push( multios_stl_slist_t *const self_p, const void *const el_p, const size_t sizof_element);
int multios_stl_slist_pop( multios_stl_slist_t *const self_p);
int multios_stl_slist_front( multios_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int multios_stl_slist_get_element_at( multios_stl_slist_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int multios_stl_slist_clear( multios_stl_slist_t *const self_p);
size_t multios_stl_slist_get_pool_cnt( multios_stl_slist_t *const self_p);
int multios_stl_slist_is_empty( multios_stl_slist_t *const self_p);

int multios_stl_slist_back( multios_stl_slist_t *const self_p, void *const el_p, const size_t  sizof_element );
int multios_stl_slist_insert_at( multios_stl_slist_t *const self_p, const size_t no, void *const el_p, const size_t  sizof_element );
int multios_stl_slist_erase_at( multios_stl_slist_t *const self_p, const size_t no);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MULTIOS_STL_SLIST_H */
