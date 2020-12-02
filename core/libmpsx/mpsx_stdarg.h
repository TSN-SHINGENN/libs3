#ifndef INC_MPSX_STDARG_H
#define INC_MPSX_STDARG_H

#pragma once

#include <stdarg.h>
#include <stdint.h>

typedef enum _enum_mpsx_stdarg_integer_type {
    MPSX_STDARG_INTEGER_IS_Ptr =  3,    // for pointer size of cpu type
    MPSX_STDARG_INTEGER_IS_LL  =  2,    // Long Long
    MPSX_STDARG_INTEGER_IS_Long  =  1,    // Long int
    MPSX_STDARG_INTEGER_IS_Normal=  0,    // Integer
    MPSX_STDARG_INTEGER_IS_Short = -1,    // Short
    MPSX_STDARG_INTEGER_IS_Char  = -2,    // Char
    MPSX_STDARG_INTEGER_IS_eot = INT8_MAX,  // end of enum integer type table
} enum_mpsx_stdarg_integer_type_t;

#if defined(__cplusplus)
extern "C" {
#endif

long long int mpsx_get_va_list_signed(va_list *const ap_p, const enum_mpsx_stdarg_integer_type_t type);
unsigned long long int mpsx_get_va_list_unsigned(va_list *const ap_p, const enum_mpsx_stdarg_integer_type_t type);

void *mpsx_get_va_list_pointer(va_list *const ap_p);

#if defined(__cplusplus)
}
#endif


#endif /* end of INC_MPSX_STDARG_H */
