#ifndef INC_MPSX_STRING_H
#define INC_MPSX_STRING_H

#pragma once

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

void *mpsx_memcpy(void* const dest, const void *const src, const size_t n);
void *mpsx_memcpy_path_through_level_cache(void *const dest, const void *const src, const size_t n);
void *mpsx_memset_path_through_level_cache(void *const s, const int c, const size_t n);

int mpsx_strcasecmp(const char *const s1, const char *const s2);
int mpsx_strncasecmp(const char *const s1, const char *const s2, const size_t n);

int mpsx_strtoupper(const char *const before_str, char *const after_buf, const size_t after_bufsize);
int mpsx_strtolower(const char *const before_str, char *const after_buf, const size_t after_bufsize);

char *mpsx_strcasestr(const char *const haystack, const char *const needle);

char *mpsx_strtrim(char *const buf, const char *const accepts, const int c);

char *mpsx_strchr(const char *const str, const int code);
char *mpsx_strrchr(const char *const str, const int code);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_STRING_H */
