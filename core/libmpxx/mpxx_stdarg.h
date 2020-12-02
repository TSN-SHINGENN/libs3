#ifndef INC_MPXX_STDARG_H
#define INC_MPXX_STDARG_H

#pragma once

#include <stdarg.h>
#include <stdint.h>

typedef enum _enum_mpxx_stdarg_integer_type {
    MPXX_STDARG_INTEGER_IS_Ptr =  3,    // for pointer size of cpu type
    MPXX_STDARG_INTEGER_IS_LL  =  2,    // Long Long
    MPXX_STDARG_INTEGER_IS_Long  =  1,    // Long int
    MPXX_STDARG_INTEGER_IS_Normal=  0,    // Integer
    MPXX_STDARG_INTEGER_IS_Short = -1,    // Short
    MPXX_STDARG_INTEGER_IS_Char  = -2,    // Char
    MPXX_STDARG_INTEGER_IS_eot = INT8_MAX,  // end of enum integer type table
} enum_mpxx_stdarg_integer_type_t;

#ifdef __cplusplus
extern "C" {
#endif

long long int mpxx_get_va_list_signed(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type);
unsigned long long int mpxx_get_va_list_unsigned(va_list *const ap_p, const enum_mpxx_stdarg_integer_type_t type);

void *mpxx_get_va_list_pointer(va_list *const ap_p);

#ifdef __cplusplus
}
#endif


#endif /* end of INC_MPXX_STDARG_H */
