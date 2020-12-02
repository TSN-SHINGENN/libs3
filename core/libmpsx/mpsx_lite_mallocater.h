#ifndef INC_MPSX_LITE_MALLOCATER_H
#define INC_MPSX_LITE_MALLOCATER_H

#pragma once

#include <stddef.h>

typedef struct _mpsx_lite_malllocate_area_header {
    size_t size;
    union {
	uint32_t magic_no;
	uint32_t occupied;
    } stamp;
    struct _mpsx_lite_malllocate_area_header *next_p;
    struct _mpsx_lite_malllocate_area_header *prev_p;
} mpsx_lite_malllocate_header_t;

typedef struct _mpsx_lite_mallocater {
    void *buf;
    size_t bufsiz;
    mpsx_lite_malllocate_header_t base;
    uint8_t bufofs;
    union {
	uint8_t flags;
	struct {
	    uint8_t initialized:1;
	} f;
    } init;
} mpsx_lite_mallocater_t;

extern mpsx_lite_mallocater_t _mpsx_lite_mallocater_heap_emulate_obj;

#if defined (__cplusplus )
extern "C" {
#endif

void *mpsx_lite_mallocater_alloc(const size_t size);
void mpsx_lite_mallocater_free(void * const ptr); 
void *mpsx_lite_mallocater_realloc(void * const ptr, const size_t size);

int mpsx_lite_mallocater_init_obj(mpsx_lite_mallocater_t *const self_p, void *const buf, const size_t bufsize);
void *mpsx_lite_mallocater_alloc_with_obj(mpsx_lite_mallocater_t *const self_p,const size_t size);
void mpsx_lite_mallocater_free_with_obj(mpsx_lite_mallocater_t *const self_p, void * const ptr); 
void *mpsx_lite_mallocater_realloc_with_obj(mpsx_lite_mallocater_t *const self_p, void *const ptr, const size_t size);

int64_t mpsx_lite_mallocater_avphys_with_obj(mpsx_lite_mallocater_t *const self_p);

void _mpsx_lite_mallocater_dump_region_list(mpsx_lite_mallocater_t const *const self_p);

#if defined (__cplusplus )
}
#endif

#endif /* end of INC_MPSX_LITE_MALLOCATER_H */
