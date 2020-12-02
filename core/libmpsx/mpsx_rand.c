/**
 *	Copyright TSN.SINGENN
 *	Basic Author: Seiichi Takeda  '2019-August-21 Active
 *		Last Alteration $Author: takeda $
 **/

/**
 * @file mpsx_rand.c
 * @brief 標準的な乱数発生関数集
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

#if defined(_S3_DMSG_IS_UART)
#define EOL_CRLF "\n\r"
#else
#define EOL_CRLF "\n"
#endif

/* this */
#include "mpsx_rand.h"

#if defined(__GNUC__) && defined(_LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE )

extern mpsx_rand_xorshift_t mpsx_rand_xorshift_initializer(void)
    __attribute__ ((optimize("Os")));

extern uint32_t mpsx_rand_xorshift_exec(mpsx_rand_xorshift_t *const)
    __attribute__ ((optimize("Os")));
extern uint32_t mpsx_rand_xorshift(void)
    __attribute__ ((optimize("Os")));

extern mpsx_rand_ansi_t mpsx_rand_ansi_initializer(void)
    __attribute__ ((optimize("Os")));
extern int mpsx_rand_ansi_exec(mpsx_rand_ansi_t *const)
    __attribute__ ((optimize("Os")));
extern void mpsx_rand_ansi_set_seed(const unsigned long int)
    __attribute__ ((optimize("Os")));
extern void mpsx_rand_ansi_set_seed_exec(mpsx_rand_ansi_t *const, const unsigned long int)
    __attribute__ ((optimize("Os")));
extern int mpsx_rand_ansi(void);
    __attribute__ ((optimize("Os")));

extern double mpsx_drand_extend_ansi(void)
    __attribute__ ((optimize("Os")));

extern mpsx_rand_wichmann_hill_t mpsx_rand_wichmann_hill_initializer(void)
    __attribute__ ((optimize("Os")));

extern double mpsx_rand_wichmann_hill_exec(mpsx_rand_wichmann_hill_t *const)
    __attribute__ ((optimize("Os")));
extern double mpsx_rand_wichmann_hill(void);
    __attribute__ ((optimize("Os")));

#endif /* end of _LIBS3_ENABLE_GCC_OPTIMIZE_FOR_SIZE */



/**
 * @fn mpsx_rand_xorshift_t mpsx_rand_xorshift_initializer(void)
 * @brief Xorshift疑似乱数発生アルゴリズム 内部初期化ルーチン
 * @return mpsx_rand_xorshift_t構造体初期値
 */
mpsx_rand_xorshift_t mpsx_rand_xorshift_initializer(void)
{
    const mpsx_rand_xorshift_t retval = MPSX_RAND_XORSHIFT_INIT_VALUE;

    return retval;
}


/**
 * @fn uint32_t mpsx_rand_xorshift_exec(mpsx_rand_xorshift_t *const self_p)
 * @brief Xorshift疑似乱数発生アルゴリズム 処理本体
 * @return 発生した乱数
 */
uint32_t mpsx_rand_xorshift_exec(mpsx_rand_xorshift_t *const self_p)
{
    uint32_t t;

    t = self_p->x ^ (self_p->x << 11);

    self_p->x = self_p->y;
    self_p->y = self_p->z;
    self_p->z = self_p->w;

    self_p->w ^= t ^ (t>>8) ^ (self_p->w>>19);

    return self_p->w;
}

static mpsx_rand_xorshift_t _rand_xorshift_self = MPSX_RAND_XORSHIFT_INIT_VALUE;

/**
 * @fn void mpsx_rand_xorshift_set_seed(uint32_t seedv)
 * @brief Xorshiftの乱数発生処理のSEED変更
 */
void mpsx_rand_xorshift_set_seed_exec(mpsx_rand_xorshift_t *const self_p, uint32_t seedv)
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
 * @fn uint32_t mpsx_rand_xorshift(void)
 * @brief 一つにまとめられた Xorshift疑似乱数発生アルゴリズム処理関数。seedは渡っていないので初期値からの値変化は同じなので注意
 * @return 発生した乱数
 */
uint32_t mpsx_rand_xorshift(void)
{

    return mpsx_rand_xorshift_exec(&_rand_xorshift_self);
}

/**
 * @fn mpsx_rand_ansi_t mpsx_rand_ansi_initializer(void)
 * @brief ANSI/ISO/IEC C規格の乱数発生初期化
 * @return mpsx_rand_ansi_t構造体値
 */
mpsx_rand_ansi_t mpsx_rand_ansi_initializer(void)
{
    const mpsx_rand_ansi_t retval = MPSX_RAND_ANSI_INIT_VALUE;

    return retval;
}


/**
 * @fn int mpsx_rand_ansi_exec(mpsx_rand_ansi_t *const self_p)
 * @brief ANSI/ISO/IEC C規格の乱数発生処理本体。大きさはMPSX_RAND_ANSI_MAX以下
 * @param mpsx_rand_ansi_t構造体値
 * @return 発生した乱数
 */
int mpsx_rand_ansi_exec(mpsx_rand_ansi_t *const self_p)
{
    self_p->next *= 1103515245;
    self_p->next += 12345;

    return ((int)self_p->next / 65536) % 32768;
}

static mpsx_rand_ansi_t _rand_ansi_self = MPSX_RAND_ANSI_INIT_VALUE;

/**
 * @fn void mpsx_rand_ansi_set_seed(const unsigned long int seedv)
 * @brief ANSI/ISO/IEC C規格の乱数発生処理のSEED変更
 */
void mpsx_rand_ansi_set_seed(const unsigned long int seedv)
{
    mpsx_rand_ansi_set_seed_exec( &_rand_ansi_self, seedv);

    return;
}

void mpsx_rand_ansi_set_seed_exec(mpsx_rand_ansi_t *const self_p, const unsigned long int seedv)
{
    self_p->next = seedv;
    return;
}



/**
 * @fn int mpsx_rand_ansi(void)
 * @brief 一つにまとめられた AMSI/ISO/IEC C規格の乱数発生処理。
 * @return 発生した乱数(大きさはMPSX_RAND_ANSI_MAX以下)
 */
int mpsx_rand_ansi(void)
{
    return mpsx_rand_ansi_exec(&_rand_ansi_self);
}

/**
 * @fn double mpsx_drand_extend_ansi(void)
 * @brief mpsx_rand_ansi()を拡張した、区間[0.0,1.0]の乱数発生処理本体
 * @return 発生した乱数値(大きさはMPSX_RAND_ANSI_MAX以下)
 */
double mpsx_drand_extend_ansi(void)
{
    double m, a;

    m = (double)MPSX_RAND_ANSI_MAX;
    m += 1.0;

    a = ((double)mpsx_rand_ansi() + 0.5) / m;
    a = ((double)mpsx_rand_ansi() + a) / m;

    return ((double)mpsx_rand_ansi() + a) / m;
}


/**
 * @fn mpsx_rand_wichmann_hill_t mpsx_rand_wichmann_hill_initializer(void)
 * @brief WichmannHillの16ビット系でも使える区間[0.0,1.0]乱数発生処理初期化
 * @return mpsx_rand_ansi_t構造体値
 */
mpsx_rand_wichmann_hill_t mpsx_rand_wichmann_hill_initializer(void)
{
    const mpsx_rand_wichmann_hill_t retval = MPSX_RAND_WICHMANN_HILL_INIT_VALUE;

    return retval;
}


/**
 * @fn double mpsx_rand_wichmann_hill_exec(mpsx_rand_wichmann_hill_t *const self_p)
 * @brief WichmannHillの16ビット系でも使える区間[0.0,1.0]乱数発生処理本体
 * @return 発生した乱数値
 */
double mpsx_rand_wichmann_hill_exec(mpsx_rand_wichmann_hill_t *const self_p)
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




