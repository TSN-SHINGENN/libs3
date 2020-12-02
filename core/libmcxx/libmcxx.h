#ifndef INC_LIBMCXX_H
#define INC_LIBMCXX_H

#pragma once

namespace s3 {

extern const char *const _mcxx_idkeyword;

const char *const mcxx_get_lib_revision(void);
const char *const mcxx_get_lib_name(void);
const char *const mcxx_get_idkeyword(void);

}

#endif /* INC_LIBMCXX_H */
