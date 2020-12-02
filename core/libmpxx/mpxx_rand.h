#ifndef INC_MPXX_RAND_H
#define INC_MPXX_RAND_H

#include <stdint.h>

typedef struct _mpxx_rand_xorshift {
    uint32_t x,y,z,w;
} mpxx_rand_xorshift_t;

typedef struct _mpxx_rand_ansi {
    unsigned long int next;
} mpxx_rand_ansi_t;

typedef struct _mpxx_rand_wichmann_hill {
    int x,y,z;
} mpxx_rand_wichmann_hill_t;

#define MPXX_RAND_XORSHIFT_INIT_VALUE { 123456789, 362436069, 521288629, 88675123 }

#define MPXX_RAND_ANSI_INIT_VALUE { 1 }
#define MPXX_RAND_ANSI_MAX (32767)

#define MPXX_RAND_WICHMANN_HILL_INIT_VALUE { 1, 1, 1 }

#if defined(__cplusplus)
extern "C" {
#endif

mpxx_rand_xorshift_t mpxx_rand_xorshift_initializer(void);

uint32_t mpxx_rand_xorshift_exec(mpxx_rand_xorshift_t *const self_p);
uint32_t mpxx_rand_xorshift(void);

mpxx_rand_ansi_t mpxx_rand_ansi_initializer(void);
int mpxx_rand_ansi_exec(mpxx_rand_ansi_t *const self_p);
void mpxx_rand_ansi_set_seed(const unsigned long int seedv);
void mpxx_rand_ansi_set_seed_exec(mpxx_rand_ansi_t *const self_p, const unsigned long int seedv);
int mpxx_rand_ansi(void);

double mpxx_drand_extend_ansi(void);

mpxx_rand_wichmann_hill_t mpxx_rand_wichmann_hill_initializer(void);

double mpxx_rand_wichmann_hill_exec(mpxx_rand_wichmann_hill_t *const self_p);
double mpxx_rand_wichmann_hill(void);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_RAND_H */




