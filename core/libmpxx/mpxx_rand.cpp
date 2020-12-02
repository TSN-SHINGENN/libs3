/**
 *	Copyright TSN.SINGENN
 *	Basic Author: Seiichi Takeda  '2019-August-21 Active
 *		Last Alteration $Author: takeda $
 **/

/**
 * @file mpxx_rand.c
 * @brief �W���I�ȗ��������֐��W
 */

#ifdef WIN32
/* Microsoft Windows Series */
#if _MSC_VER >= 1400		/* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* CRL */
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static int debuglevel = 1;
#else
#ifdef __GNUC__
__attribute__ ((unused))
#endif
static const int debuglevel = 0;
#endif
#include "dbms.h"

#if defined(_LIBS3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#include "mpxx_rand.h"

#if defined(__GNUC__) && defined(_LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern mpxx_rand_xorshift_t mpxx_rand_xorshift_initializer(void)
    __attribute__ ((optimize("Os")));

extern uint32_t mpxx_rand_xorshift_exec(mpxx_rand_xorshift_t *const)
    __attribute__ ((optimize("Os")));
extern uint32_t mpxx_rand_xorshift(void)
    __attribute__ ((optimize("Os")));

extern mpxx_rand_ansi_t mpxx_rand_ansi_initializer(void)
    __attribute__ ((optimize("Os")));
extern int mpxx_rand_ansi_exec(mpxx_rand_ansi_t *const)
    __attribute__ ((optimize("Os")));
extern void mpxx_rand_ansi_set_seed(const unsigned long int)
    __attribute__ ((optimize("Os")));
extern void mpxx_rand_ansi_set_seed_exec(mpxx_rand_ansi_t *const, const unsigned long int)
    __attribute__ ((optimize("Os")));
extern int mpxx_rand_ansi(void);
    __attribute__ ((optimize("Os")));

extern double mpxx_drand_extend_ansi(void)
    __attribute__ ((optimize("Os")));

extern mpxx_rand_wichmann_hill_t mpxx_rand_wichmann_hill_initializer(void)
    __attribute__ ((optimize("Os")));

extern double mpxx_rand_wichmann_hill_exec(mpxx_rand_wichmann_hill_t *const)
    __attribute__ ((optimize("Os")));
extern double mpxx_rand_wichmann_hill(void);
    __attribute__ ((optimize("Os")));

#endif /* end of _LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE */



/**
 * @fn mpxx_rand_xorshift_t mpxx_rand_xorshift_initializer(void)
 * @brief Xorshift�^�����������A���S���Y�� �������������[�`��
 * @return mpxx_rand_xorshift_t�\���̏����l
 */
mpxx_rand_xorshift_t mpxx_rand_xorshift_initializer(void)
{
    const mpxx_rand_xorshift_t retval = MPXX_RAND_XORSHIFT_INIT_VALUE;

    return retval;
}


/**
 * @fn uint32_t mpxx_rand_xorshift_exec(mpxx_rand_xorshift_t *const self_p)
 * @brief Xorshift�^�����������A���S���Y�� �����{��
 * @return ������������
 */
uint32_t mpxx_rand_xorshift_exec(mpxx_rand_xorshift_t *const self_p)
{
    uint32_t t;

    t = self_p->x ^ (self_p->x << 11);

    self_p->x = self_p->y;
    self_p->y = self_p->z;
    self_p->z = self_p->w;

    self_p->w ^= t ^ (t>>8) ^ (self_p->w>>19);

    return self_p->w;
}

static mpxx_rand_xorshift_t _rand_xorshift_self = MPXX_RAND_XORSHIFT_INIT_VALUE;

/**
 * @fn void mpxx_rand_xorshift_set_seed(uint32_t seedv)
 * @brief Xorshift�̗�������������SEED�ύX
 */
void mpxx_rand_xorshift_set_seed_exec(mpxx_rand_xorshift_t *const self_p, uint32_t seedv)
{

    do {
        seedv = (seedv * 1812433253) + 1; seedv ^= seedv<<13; seedv ^= seedv>>17;
        self_p->x =  123464980 ^ seedv;
        seedv = (seedv * 1812433253) + 1; seedv ^= seedv<<13; seedv ^= seedv>>17;
        self_p->y = 3447902351 ^ seedv;
        seedv = (seedv * 1812433253) + 1; seedv ^= seedv<<13; seedv ^= seedv>>17;
        self_p->z = 2859490775 ^ seedv;
        seedv = (seedv * 1812433253) + 1; seedv ^= seedv<<13; seedv ^= seedv>>17;
        self_p->w =   47621719 ^ seedv;
    } while((self_p->x==0) && (self_p->y==0) && (self_p->x==0) && (self_p->w==0));

    return;
}


/**
 * @fn uint32_t mpxx_rand_xorshift(void)
 * @brief ��ɂ܂Ƃ߂�ꂽ Xorshift�^�����������A���S���Y�������֐��Bseed�͓n���Ă��Ȃ��̂ŏ����l����̒l�ω��͓����Ȃ̂Œ���
 * @return ������������
 */
uint32_t mpxx_rand_xorshift(void)
{

    return mpxx_rand_xorshift_exec(&_rand_xorshift_self);
}

/**
 * @fn mpxx_rand_ansi_t mpxx_rand_ansi_initializer(void)
 * @brief ANSI/ISO/IEC C�K�i�̗�������������
 * @return mpxx_rand_ansi_t�\���̒l
 */
mpxx_rand_ansi_t mpxx_rand_ansi_initializer(void)
{
    const mpxx_rand_ansi_t retval = MPXX_RAND_ANSI_INIT_VALUE;

    return retval;
}


/**
 * @fn int mpxx_rand_ansi_exec(mpxx_rand_ansi_t *const self_p)
 * @brief ANSI/ISO/IEC C�K�i�̗������������{�́B�傫����MPXX_RAND_ANSI_MAX�ȉ�
 * @param mpxx_rand_ansi_t�\���̒l
 * @return ������������
 */
int mpxx_rand_ansi_exec(mpxx_rand_ansi_t *const self_p)
{
    self_p->next *= 1103515245;
    self_p->next += 12345;

    return ((int)self_p->next / 65536) % 32768;
}

static mpxx_rand_ansi_t _rand_ansi_self = MPXX_RAND_ANSI_INIT_VALUE;

/**
 * @fn void mpxx_rand_ansi_set_seed(const unsigned long int seedv)
 * @brief ANSI/ISO/IEC C�K�i�̗�������������SEED�ύX
 */
void mpxx_rand_ansi_set_seed(const unsigned long int seedv)
{
    mpxx_rand_ansi_set_seed_exec( &_rand_ansi_self, seedv);

    return;
}

void mpxx_rand_ansi_set_seed_exec(mpxx_rand_ansi_t *const self_p, const unsigned long int seedv)
{
    self_p->next = seedv;
    return;
}



/**
 * @fn int mpxx_rand_ansi(void)
 * @brief ��ɂ܂Ƃ߂�ꂽ AMSI/ISO/IEC C�K�i�̗������������B
 * @return ������������(�傫����MPXX_RAND_ANSI_MAX�ȉ�)
 */
int mpxx_rand_ansi(void)
{
    return mpxx_rand_ansi_exec(&_rand_ansi_self);
}

/**
 * @fn double mpxx_drand_extend_ansi(void)
 * @brief mpxx_rand_ansi()���g�������A���[0.0,1.0]�̗������������{��
 * @return �������������l(�傫����MPXX_RAND_ANSI_MAX�ȉ�)
 */
double mpxx_drand_extend_ansi(void)
{
    double m, a;

    m = (double)MPXX_RAND_ANSI_MAX;
    m += 1.0;

    a = ((double)mpxx_rand_ansi() + 0.5) / m;
    a = ((double)mpxx_rand_ansi() + a) / m;

    return ((double)mpxx_rand_ansi() + a) / m;
}


/**
 * @fn mpxx_rand_wichmann_hill_t mpxx_rand_wichmann_hill_initializer(void)
 * @brief WichmannHill��16�r�b�g�n�ł��g������[0.0,1.0]������������������
 * @return mpxx_rand_ansi_t�\���̒l
 */
mpxx_rand_wichmann_hill_t mpxx_rand_wichmann_hill_initializer(void)
{
    const mpxx_rand_wichmann_hill_t retval = MPXX_RAND_WICHMANN_HILL_INIT_VALUE;

    return retval;
}


/**
 * @fn double mpxx_rand_wichmann_hill_exec(mpxx_rand_wichmann_hill_t *const self_p)
 * @brief WichmannHill��16�r�b�g�n�ł��g������[0.0,1.0]�������������{��
 * @return �������������l
 */
double mpxx_rand_wichmann_hill_exec(mpxx_rand_wichmann_hill_t *const self_p)
{
    double r;

    self_p->x = 171 * ( self_p->x % 177 ) -  2 * ( self_p->x / 177 );
    self_p->y = 172 * ( self_p->y % 176 ) - 35 * ( self_p->y / 176 );
    self_p->z = 170 * ( self_p->z % 178 ) - 63 * ( self_p->z / 178 );
   
#if 0
    if(self_p->x < 0 ) {
	self_p->x += 30269;
    }
    if(self_p->y < 0 ) {
	self_p->y += 30307;
    }
    if(self_p->z < 0 ) {
	self_p->z += 30323;
    }
#else
    self_p->x = (int)((171 * self_p->x) % 30269);
    self_p->y = (int)((172 * self_p->y) % 30307);
    self_p->z = (int)((170 * self_p->z) % 30323);
#endif

    r = self_p->x / 30269.0 + self_p->y / 30307.0 + self_p->z / 30323.0;
    while( r >= 1.0 ) {
	r -= 1.0;
    }

    return r;
}




