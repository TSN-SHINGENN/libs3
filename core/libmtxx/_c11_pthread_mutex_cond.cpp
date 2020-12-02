/**
 *	Copyright 2020 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2020-April-12 Active
 *		Last Alteration $Author: takeda $
 */

/**
 * @file _c11_pthread_mutex_cond.c
 * @brief pthread_mutex_*()/pthread_cond_* API��C++11�p���C�u�����R�[�h
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
 * @brief POSIX��pthread_mutex_init()�֐���C++11���b�p�[
 * @param m_p s3::c11_pthread_mutex_t �\���̃|�C���^
 * @param attr NULL���w��
 * @retval 0 ����
 * @retval -1 �v���I�Ȏ��s
 */
int s3::c11_pthread_mutex_init( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m = new(std::mutex);

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_destroy( c11_pthread_mutex_t*m_p )
 * @brief POSIX��pthread_mutex_destroy()�֐���C++11���b�p�[
 * @param m_p s3::c11_pthread_mutex_t �\���̃|�C���^
 * @retval 0 ����
 * @retval -1 �v���I�Ȏ��s
 */
int s3::c11_pthread_mutex_destroy( c11_pthread_mutex_t*m_p )
{
    delete m_p->m;

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_lock( s3::c11_pthread_mutex_t *m_p)
 * @brief POSIX��pthread_mutex_lock()�֐���C++11���b�p�[
 * @param m_p s3::c11_pthread_mutex_t�\���̃|�C���^
 * @return 0�c�����A-1�c���s
 */
int s3::c11_pthread_mutex_lock( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m_->lock();

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_trylock( s3::c11_pthread_mutex_t *m_p)
 * @brief POSIX��pthread_mutex_trylock()�֐���C++11���b�p�[
 * @param m_p s3::c11_pthread_mutex_t�\���̃|�C���^
 * @return 0�c�����A-1�c���s
 */
int s3::c11_pthread_mutex_trylock( s3::c11_pthread_mutex_t *m_p)
{
    m_p->m_->trylock();

    return 0;
}

/**
 * @fn int s3::c11_pthread_mutex_timedlock( s3::c11_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts)
 * @brief POSIX��pthread_mutex_timedlock()�֐���C++11���b�p�[
 * @param m_p s3::c11_pthread_mutex_t�\���̃|�C���^
 * @return 0�c�����A-1�c���s
 */
int s3::c11_pthread_mutex_timedlock( s3::c11_pthread_mutex_t *m_p, mpxx_mtimespec_t *ts)
{


}

