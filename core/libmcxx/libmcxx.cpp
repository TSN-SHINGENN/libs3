/* this */
#include "libmcxx_revision.h"
#include "libmcxx.h"

/**
 * @fn const char *const s3::mcxx_get_lib_revision(void)
 * @brief libeia232の版管理番号の取得
 * @return 版管理番号文字列ポインタ
 */
const char *const s3::mcxx_get_lib_revision(void)
{
    return LIB_MCXX_REVISION;
}

/**
 * @fn const char *const s3::mcxx_get_lib_name(void)
 * @brief libmultiosライブラリの標準名を返します
 * @return 標準名文字列ポインタ
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
 * @brief libmultios_idkeywordを返します。
 * @return idkeyword文字列ポインタ
 */
const char *const s3::mcxx_get_idkeyword(void)
{
    return s3::_mcxx_idkeyword;
}
