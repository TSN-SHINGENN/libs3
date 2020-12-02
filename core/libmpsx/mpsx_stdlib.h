#ifndef INC_MPSX_STDLIB_H
#define INC_MPSX_STDLIB_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

int mpsx_ctoi(const char c);
int mpsx_atoi(const char *str);

int32_t mpsx_atoi32( const char * const str);
int64_t mpsx_atoi64( const char * const str);
uint32_t mpsx_atou32( const char * const str);
uint64_t mpsx_atou64( const char * const str);
uint32_t mpsx_atox32( const char * const str );
uint64_t mpsx_atox64( const char * const str );
uint32_t mpsx_atoo32( const char * const str );
uint64_t mpsx_atoo64( const char * const str );

#define MPSX_XTOA_RADIX_BIN (2)
#define MPSX_XTOA_RADIX_OCT (8)
#define MPSX_XTOA_RADIX_DEC (10)
#define MPSX_XTOA_RADIX_HEX (16)

#define MPSX_XTOA_WORD_IS_UPPER (1)
#define MPSX_XTOA_WORD_IS_LOWER (0)

/* 推奨バッファサイズ(符号+数列+終端) */
#define MPSX_XTOA_BIN64_BUFSZ (64+2)
#define MPSX_XTOA_BIN32_BUFSZ (32+2)
#define MPSX_XTOA_HEX64_BUFSZ (16+2)
#define MPSX_XTOA_HEX32_BUFSZ ( 8+2)
#define MPSX_XTOA_DEC64_BUFSZ (20+2)
#define MPSX_XTOA_DEC32_BUFSZ (10+2)

typedef union mpsx_xtoa_attrbute {
    uint8_t flags;
    struct {
	uint8_t zero_padding:1;
	uint8_t alternative:1;
	uint8_t thousand_group:1;
	uint8_t str_is_lower:1;
	uint8_t with_sign_char:1;
	uint8_t left_justified:1;
	uint8_t no_assign_terminate:1;
    } f;
} mpsx_xtoa_attr_t;

#define mpsx_xtoa_attribute_initalizer() {0} 

char *mpsx_i32toa( const int32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p);
char *mpsx_i64toa( const int64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p);
char *mpsx_u32toa( const uint32_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p);
char *mpsx_u64toa( const uint64_t value, void * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p);

char *mpsx_u32toahex( const uint32_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t * const attr_p);
char *mpsx_u64toahex( const uint64_t value, void * const buf, const size_t bufsz, const mpsx_xtoa_attr_t * const attr_p);

char *mpsx_u64toadec( const uint64_t value, void * const buf, const size_t bufsz);
char *mpsx_u32toadec( const uint32_t value, void * const buf, const size_t bufsz);
char *mpsx_i64toadec( const int64_t value, void * const buf, const size_t bufsz);
char *mpsx_i32toadec( const int32_t value, void * const buf, const size_t bufsz);

char *mpsx_u64toabin( const uint64_t value, void * const buf, const size_t bufsz);
char *mpsx_u32toabin( const uint32_t value, void * const buf, const size_t bufsz);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _MPSX_LITE_PRINTF_PUTCHAR_CALLBACK_T
#define _MPSX_LITE_PRINTF_PUTCHAR_CALLBACK_T
typedef int (*putchar_callback_ptr_t)(const int);
#endif /* end of _MPSX_LITE_PRINTF_PUTCHAR_CALLBACK_T */

typedef struct _mpsx_xtoa_output_method {
    putchar_callback_ptr_t _putchar_cb;
    size_t maxlenofstring;
    char *buf;
} mpsx_xtoa_output_method_t;

#define mpsx_xtoa_output_method_initializer_set_callback_func(f, m) { (f), (m), NULL }
#define mpsx_xtoa_output_method_initializer_set_buffer( b, m) { NULL, (m), (b) }

char *mpsx_signed_to_string( const int64_t value, char * const buf, size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t * const attr_p);
char *mpsx_unsigned_to_string( const uint64_t value, char * const buf, const size_t bufsz, const uint8_t radix, const mpsx_xtoa_attr_t *const attr_p);

int mpsx_integer_to_string_with_format(const mpsx_xtoa_output_method_t *const method_p, char sign, unsigned long long value, const int radix, int field_length, const mpsx_xtoa_attr_t *const attr_p, int *const retlen_p);

int mpsx_rand(void);
void mpsx_srand(const unsigned int seed);


#if defined(__cplusplus)
}
#endif


#endif /* end of INC_MPSX_STDLIB_H */
