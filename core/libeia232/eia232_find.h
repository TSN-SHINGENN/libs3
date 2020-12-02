#ifndef _INC_LIBEIA232_FIND_H
#define _INC_LIBEIA232_FIND_H

#include <stdint.h>

typedef struct _eia232dev_find_device {
   size_t index_point;		/*!< ���݂�Index�|�C���g(�f�o�C�X�p�X�ԍ��ɂ͑Ή����Ȃ�) */
   size_t num_found_device;	/*!< �������� */
   void *ext;			/*!< OS���ɉB�������g���̈�|�C���^ */
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
