#ifndef INC_MULTIOS_STL_LIST_H
#define INC_MULTIOS_STL_LIST_H

typedef struct _multios_stl_list {
   size_t sizeof_element;
   void *ext;
} multios_stl_list_t;

#if defined (__cplusplus )
extern "C" {
#endif

int multios_stl_list_init( multios_stl_list_t *const self_p, const size_t sizeof_element);
int multios_stl_list_destroy( multios_stl_list_t *const self_p);
int multios_stl_list_push_back( multios_stl_list_t *const self_p, const void *const el_p, const size_t sizeof_element);
int multios_stl_list_push_front( multios_stl_list_t *const self_p, const void *const el_p, const size_t sizof_element );
int multios_stl_list_pop_front( multios_stl_list_t *const self_p);
int multios_stl_list_pop_back( multios_stl_list_t *const self_p);
int multios_stl_list_insert( multios_stl_list_t *const self_p, const size_t num, const void *const el_p, const size_t sizof_element );
int multios_stl_list_get_element_at( multios_stl_list_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int multios_stl_list_clear( multios_stl_list_t *const self_p);
size_t multios_stl_list_get_pool_cnt( multios_stl_list_t *const self_p);
int multios_stl_list_is_empty( multios_stl_list_t *const self_p);
int multios_stl_list_remove_at( multios_stl_list_t *const self_p,  const size_t num);
int multios_stl_list_overwrite_element_at( multios_stl_list_t *const self_p,  const size_t num, const void *const el_p, const size_t sizof_element);

int multios_stl_list_front( multios_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);
int multios_stl_list_back( multios_stl_list_t *const self_p, void *const el_p, const size_t sizof_element);

void multios_stl_list_dump_element_region_list( multios_stl_list_t *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MULTIOS_STL_LIST_H */
