#ifndef INC_MPXX_MEMAREACPY_H
#define INC_MPXX_MEMAREACPY_H

#include <stdint.h>


/**
 * @typedef mpxx_memareacpy_attr_t
 * @brief mpxx_memareacpyの属性フラグ集合を設定するときに使用するunion共同体です
 */
typedef union _mpxx_memareacpy_attr {
    unsigned int flags;
    struct {
	unsigned int dest_is_cleared_to_zero:1;			/*!< コピー先バッファを0クリアします */
	unsigned int use_func_is_stdmemcpy:1;			/*!< OS標準のmemcpy関数を内部で使用します */
	unsigned int use_func_is_path_through_level_cache:1;	/*!< CPUレベルキャッシュをスルーします。 */
    } f;
} mpxx_memareacpy_attr_t;

typedef struct _mpxx_memareacpy_ext {
    size_t src_start_valid_line;
    size_t src_start_ofs_in_line;
    size_t dest_start_valid_line;
    size_t dest_start_ofs_in_line;
    union {
	unsigned int flags;
	struct {
	    unsigned int src_parameter_is_valid:1;
	    unsigned int dest_parameter_is_valid:1;
	} f;
    } stat;
} mpxx_memareacpy_ext_t;

#if defined(__cplusplus)
extern "C" {
#endif

int mpxx_memareacpy( void *dest, const void *src, size_t line_length, size_t num_total_lines, size_t valid_line_length, size_t num_valid_lines, mpxx_memareacpy_ext_t *ext_p, mpxx_memareacpy_attr_t *attr_p);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_MEMAREACPY_H */
