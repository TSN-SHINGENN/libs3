#ifndef INC_MCXX_DEV232_FIND_H
#define INC_MCXX_DEV232_FIND_H

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace s3 {

typedef struct _mcxx_dev232_find_device {
   size_t index_point;		/*!< ���݂�Index�|�C���g(�f�o�C�X�p�X�ԍ��ɂ͑Ή����Ȃ�) */
   size_t num_found_device;	/*!< �������� */
   void *ext;			/*!< OS���ɉB�������g���̈�|�C���^ */
} mcxx_dev232_find_device_t;

int mcxx_dev232_find_device_init( mcxx_dev232_find_device_t *const self_p);
int mcxx_dev232_find_device_reset_instance(mcxx_dev232_find_device_t *const self_p);
int mcxx_dev232_find_device_exec( mcxx_dev232_find_device_t *const self_p, const uint32_t vid,  const uint32_t pid, char *const name_buf, const size_t bufsize);
void mcxx_dev232_find_device_destroy( mcxx_dev232_find_device_t *const self_p);

}

#endif /* end of INC_MCXX_EIA232_FIND_H */
