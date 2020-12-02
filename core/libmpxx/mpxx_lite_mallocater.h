#ifndef INC_MPXX_LITE_MALLOCATER_H
#define INC_MPXX_LITE_MALLOCATER_H

#include <stddef.h>

typedef struct _mpxx_lite_malllocate_area_header {
    size_t size;
    union {
	uint32_t magic_no;
	uint32_t occupied;
    } stamp;
    struct _mpxx_lite_malllocate_area_header *next_p;
    struct _mpxx_lite_malllocate_area_header *prev_p;
} mpxx_lite_malllocate_header_t;

typedef struct _mpxx_lite_mallocater {
    void *buf;
    size_t bufsiz;
    mpxx_lite_malllocate_header_t base;
    uint8_t bufofs;
    union {
	uint8_t flags;
	struct {
	    uint8_t initialized:1;
	} f;
    } init;
} mpxx_lite_mallocater_t;

extern mpxx_lite_mallocater_t _mpxx_lite_mallocater_heap_emulate_obj;

#if defined (__cplusplus )
extern "C" {
#endif

void *mpxx_lite_mallocater_alloc(const size_t size);
void mpxx_lite_mallocater_free(void * const ptr); 
void *mpxx_lite_mallocater_realloc(void * const ptr, const size_t size);

int mpxx_lite_mallocater_init_obj(mpxx_lite_mallocater_t *const self_p, void *const buf, const size_t bufsize);
void *mpxx_lite_mallocater_alloc_with_obj(mpxx_lite_mallocater_t *const self_p,const size_t size);
void mpxx_lite_mallocater_free_with_obj(mpxx_lite_mallocater_t *const self_p, void * const ptr); 
void *mpxx_lite_mallocater_realloc_with_obj(mpxx_lite_mallocater_t *const self_p, void *const ptr, const size_t size);

int64_t mpxx_lite_mallocater_avphys_with_obj(mpxx_lite_mallocater_t *const self_p);

void _mpxx_lite_mallocater_dump_region_list(mpxx_lite_mallocater_t const *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MPXX_LITE_MALLOCATER_H */
