#ifndef _INC_LIBEIA232_FIND_H
#define _INC_LIBEIA232_FIND_H

#include <stdint.h>

typedef struct _eia232dev_find_device {
   size_t index_point;		/*!< 現在のIndexポイント(デバイスパス番号には対応しない) */
   size_t num_found_device;	/*!< 見つけた数 */
   void *ext;			/*!< OS毎に隠蔽される拡張領域ポインタ */
} eia232dev_find_device_t;

#if defined (__cplusplus )
extern "C" {
#endif

int eia232dev_find_device_init( eia232dev_find_device_t *self_p);
int eia232dev_find_device_reset_instance(eia232dev_find_device_t *self_p);
int eia232dev_find_device_exec( eia232dev_find_device_t *self_p, uint32_t vid,  uint32_t pid, char *name_buf, int bufsize);
void eia232dev_find_device_destroy( eia232dev_find_device_t *self_p);

#if defined (__cplusplus )
} 
#endif

#endif /* end of _INC_LIBEIA232_FIND_H */
