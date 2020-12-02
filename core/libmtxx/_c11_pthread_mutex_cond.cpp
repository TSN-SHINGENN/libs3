/**
 *	Copyright 2020 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2020-April-12 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file _c11_pthread_mutex_cond.c
 * @brief pthread_mutex_*()/pthread_cond_* APIのC++11用ライブラリコード
 */

#ifndef WIN32
#error This source code is program for WIN32
#else
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif

#include <mutex>

/* libs3 */
#include "libmpxx/mpxx_time.h"

/* this */
#include "_c11_pthread_mutex_cond.h"

/**
 * @fn int s3::c11_pthread_mutex_init( s3::c11_pthread_mutex_t *m_p)
 * @brief POSIXのpthread_mutex_init()関数のC++11ラッパー
 * @param m_p s3::c11_pthread_mutex_t 構造体ポインタ
 * @param attr NULLを指定
 * @retval 0 成功
 * @retval -1 致命的な失敗
 */
int s3::c11_pthread_mutex_init( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m = new(std::mutex);

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_destroy( c11_pthread_mutex_t*m_p )
 * @brief POSIXのpthread_mutex_destroy()関数のC++11ラッパー
 * @param m_p s3::c11_pthread_mutex_t 構造体ポインタ
 * @retval 0 成功
 * @retval -1 致命的な失敗
 */
int s3::c11_pthread_mutex_destroy( c11_pthread_mutex_t*m_p )
{
    delete m_p->m;

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_lock( s3::c11_pthread_mutex_t *m_p)
 * @brief POSIXのpthread_mutex_lock()関数のC++11ラッパー
 * @param m_p s3::c11_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::c11_pthread_mutex_lock( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m_->lock();

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_trylock( s3::c11_pthread_mutex_t *m_p)
 * @brief POSIXのpthread_mutex_trylock()関数のC++11ラッパー
 * @param m_p s3::c11_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::c11_pthread_mutex_trylock( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m_->trylock();

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_timedlock( s3::c11_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts)
 * @brief POSIXのpthread_mutex_timedlock()関数のC++11ラッパー
 * @param m_p s3::c11_pthread_mutex_t構造体ポインタ
 * @return 0…成功、-1…失敗
 */
int s3::c11_pthread_mutex_timedlock( s3::c11_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts)
{


}

