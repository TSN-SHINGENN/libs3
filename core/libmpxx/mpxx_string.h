#ifndef INC_MPXX_STRING_H
#define INC_MPXX_STRING_H

#pragma once

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

void *mpxx_memcpy(void* const dest, const void *const src, const size_t n);
void *mpxx_memcpy_path_through_level_cache(void *const dest, const void *const src, const size_t n);
void *mpxx_memset_path_through_level_cache(void *const s, const int c, const size_t n);

int mpxx_strcasecmp(const char *const s1, const char *const s2);
int mpxx_strncasecmp(const char *const s1, const char *const s2, const size_t n);

int mpxx_strtoupper(const char *const before_str, char *const after_buf, const size_t after_bufsize);
int mpxx_strtolower(const char *const before_str, char *const after_buf, const size_t after_bufsize);

char *mpxx_strcasestr(const char *const haystack, const char *const needle);

char *mpxx_strtrim(char *const buf, const char *const accepts, const int c);

char *mpxx_strchr(const char *const str, const int code);
char *mpxx_strrchr(const char *const str, const int code);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_STRING_H */
