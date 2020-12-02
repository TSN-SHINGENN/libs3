/* this */
#include "libmcxx_revision.h"
#include "libmcxx.h"

/**
 * @fn const char *const s3::mcxx_get_lib_revision(void)
 * @brief libeia232�̔ŊǗ��ԍ��̎擾
 * @return �ŊǗ��ԍ�������|�C���^
 */
const char *const s3::mcxx_get_lib_revision(void)
{
    return LIB_MCXX_REVISION;
}

/**
 * @fn const char *const s3::mcxx_get_lib_name(void)
 * @brief libmultios���C�u�����̕W������Ԃ��܂�
 * @return �W����������|�C���^
 */
const char *const s3::mcxx_get_lib_name(void)
{
    return LIB_MCXX_NAME;
}

const char *const s3::_mcxx_idkeyword =
    "@(#) mcxx_idkeyword : " LIB_MCXX_NAME " revision"
    LIB_MCXX_REVISION
    " CopyRight (C) TSN-SHINGENN All Rights Reserved.";

/**
 * @fn const char *const s3::mcxx_get_idkeyword(void)
 * @brief libmultios_idkeyword��Ԃ��܂��B
 * @return idkeyword������|�C���^
 */
const char *const s3::mcxx_get_idkeyword(void)
{
    return s3::_mcxx_idkeyword;
}
