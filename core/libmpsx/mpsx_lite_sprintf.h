#ifndef INC_MPSX_LITE_SPRINTF_H
#define INC_MPSX_LITE_SPRINTF_H

#pragma once

#include <stddef.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

int _mpsx_vsprintf_lite(void (*putc_cb)(int), const char *const fmt, va_list ap);
int _mpsx_vsnprintf_lite(void (*_putc_cb)(int), const size_t max_len, const char *const fmt, va_list ap);

int mpsx_lite_vprintf(const char *const fmt, va_list ap);
int mpsx_lite_vsprintf(char *const buf, const char *const fmt, va_list ap);
int mpsx_lite_vsnprintf(char *const buf, const size_t max_length, const char *const fmt, va_list ap);


int mpsx_lite_putchar_cb_sprintf(int (*putchar_cb)(int) , const char *const fmt, ...);
int mpsx_lite_putchar_cb_snprintf(int (*putchar_cb)(int), const size_t max_length, const char *const *fmt, ...);


int mpsx_lite_printf_init(int (*const putchar_cb_func)(const int asicccode));
int mpsx_lite_printf_change_putchar_callback(int (*const putchar_cb_func)(const int asicccode));

int mpsx_lite_printf(const char *const fmt, ...);
int mpsx_lite_sprintf(char *const buf, const char *const fmt, ...);
int mpsx_lite_snprintf(char *const buf, const size_t max_length, const char *const format, ...);


#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_LITE_SPRINTF_H */