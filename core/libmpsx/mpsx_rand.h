#ifndef INC_MPSX_RAND_H
#define INC_MPSX_RAND_H

#include <stdint.h>

typedef struct _mpsx_rand_xorshift {
    uint32_t x,y,z,w;
} mpsx_rand_xorshift_t;

typedef struct _mpsx_rand_ansi {
    unsigned long int next;
} mpsx_rand_ansi_t;

typedef struct _mpsx_rand_wichmann_hill {
    int x,y,z;
} mpsx_rand_wichmann_hill_t;

#define MPSX_RAND_XORSHIFT_INIT_VALUE { 123456789, 362436069, 521288629, 88675123 }

#define MPSX_RAND_ANSI_INIT_VALUE { 1 }
#define MPSX_RAND_ANSI_MAX (32767)

#define MPSX_RAND_WICHMANN_HILL_INIT_VALUE { 1, 1, 1 }

#if defined(__cplusplus)
extern "C" {
#endif

mpsx_rand_xorshift_t mpsx_rand_xorshift_initializer(void);

uint32_t mpsx_rand_xorshift_exec(mpsx_rand_xorshift_t *const self_p);
uint32_t mpsx_rand_xorshift(void);

mpsx_rand_ansi_t mpsx_rand_ansi_initializer(void);
int mpsx_rand_ansi_exec(mpsx_rand_ansi_t *const self_p);
void mpsx_rand_ansi_set_seed(const unsigned long int seedv);
void mpsx_rand_ansi_set_seed_exec(mpsx_rand_ansi_t *const self_p, const unsigned long int seedv);
int mpsx_rand_ansi(void);

double mpsx_drand_extend_ansi(void);

mpsx_rand_wichmann_hill_t mpsx_rand_wichmann_hill_initializer(void);

double mpsx_rand_wichmann_hill_exec(mpsx_rand_wichmann_hill_t *const self_p);
double mpsx_rand_wichmann_hill(void);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_RAND_H */




