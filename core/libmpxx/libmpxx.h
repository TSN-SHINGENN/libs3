#ifndef INC_LIBMPXX_H
#define INC_LIBMPXX_H

#pragma once

extern const char *const _libmultios_idkeyword;

#ifdef __cplusplus
extern "C" {
#endif

const char *mpxx_get_lib_revision(void);
const char *mpxx_get_lib_name(void);
const char *mpxx_get_idkeyword(void);

#ifdef __cplusplus
}
#endif


#endif /* end of INC_LIBMPXX_H */
