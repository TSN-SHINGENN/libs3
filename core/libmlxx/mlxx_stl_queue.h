#ifndef INC_MULTIOS_STL_QUEUE_H
#define INC_MULTIOS_STL_QUEUE_H

#include <stddef.h>

typedef enum _multios_stl_queue_implement_type {
    MULTIOS_STL_QUEUE_TYPE_IS_DEFAULT = 11,
    MULTIOS_STL_QUEUE_TYPE_IS_SLIST,
    MULTIOS_STL_QUEUE_TYPE_IS_DEQUE,
    MULTIOS_STL_QUEUE_TYPE_IS_LIST,
    MULTIOS_STL_QUEUE_TYPE_IS_OTHERS
} enum_multios_stl_queue_implement_type_t;

typedef struct _multios_stl_queue {
   size_t sizof_element;
   void *ext;
} multios_stl_queue_t;

#if defined (__cplusplus )
extern "C" {
#endif

int multios_stl_queue_init( multios_stl_queue_t *const self_p, const size_t sizof_element);
int multios_stl_queue_init_ex( multios_stl_queue_t *const self_p, const size_t sizof_element, const enum_multios_stl_queue_implement_type_t implement_type, void *const attr_p);

int multios_stl_queue_destroy( multios_stl_queue_t *const self_p);
int multios_stl_queue_push( multios_stl_queue_t *const self_p, const void *const el_p, const size_t sizof_element);
int multios_stl_queue_pop( multios_stl_queue_t *const self_p);
int multios_stl_queue_front( multios_stl_queue_t *const self_p, void *const el_p, const size_t sizof_element );
int multios_stl_queue_get_element_at( multios_stl_queue_t *const self_p, const size_t num, void *const el_p, const size_t sizof_element);
int multios_stl_queue_clear( multios_stl_queue_t *const self_p);
size_t multios_stl_queue_get_pool_cnt( multios_stl_queue_t *const self_p);
int multios_stl_queue_is_empty( multios_stl_queue_t *const self_p);

int multios_stl_queue_back( multios_stl_queue_t *const self_p, void *const el_p, const size_t  sizof_element );

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MULTIOS_STL_QUEUE_H */
