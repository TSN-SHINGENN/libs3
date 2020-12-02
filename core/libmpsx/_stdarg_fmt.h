#ifndef INC_MPSX_STDARG_FMT_H
#define INC_MPSX_STDARG_FMT_H

#pragma once

#if defined(__MINGW32__) /* old Windows format (64-bit abariable type) */
#define FMT_LLU  "%I64u"
#define FMT_LLD  "%I64d"
#define FMT_LLX  "%I64x"
#define FMT_ZLLU "%0*I64u"
#define FMT_ZLLD "%0*I64d"
#define FMT_ZLLX "%0*I64x"
#else
#define FMT_LLU "%llu"
#define FMT_LLD "%lld"
#define FMT_LLX  "%llx"
#define FMT_ZLLU "%0*llu"
#define FMT_ZLLD "%0*lld"
#define FMT_ZLLX "%0*llx"
#endif

#if defined(_MPSX_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif 

#endif /* end of INC_MPSX_STDARG_FMT_H */

