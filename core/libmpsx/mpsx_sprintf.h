#ifndef INC_MPSX_SPRINTF_H
#define INC_MPSX_SPRINTF_H

#include <stdio.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif
int mpsx_snprintf( char * __restrict buf, size_t size, const char * __restrict fmt, ...) /* no colon */
#if !defined(__GNUC__) 
	; /* end of mpsx_snprintf() */
#else
	__attribute__ ((format (printf, 3, 4)));
#endif

int mpsx_vsnprintf( char * __restrict buf, size_t size, const char * __restrict fmt, va_list ap);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void _mpsx_vprintf(void (*putc_cb)(int), const char *fmt, va_list ap);
void _mpsx_vsnprintf(void (*putc_cb)(int), const size_t maxlen, const char *fmt, va_list ap);

void _mpsx_printf(const char *const fmt, ...);

int _mpsx_sprintf(char *str, const char *format, ...); 
int _mpsx_snprintf(char *str, const size_t maxlen, const char *format, ...); 


#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_SPRINTF_H */
