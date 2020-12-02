#ifndef INC_MPXX_CONIO_H
#define INC_MPXX_CONIO_H

#include <stdint.h>
#include <stdio.h>

/**
 * @var mpxx_conio_gets_attr_t
 * @brief  conioライブラリの属性フラグ
 **/

typedef union _mpxx_conio_gets_attr {
    uint8_t flags;
    struct {	
	uint8_t BSkey_is_not_manipulated_bs:1;    /*!< BSキーでバックスペース制御をしない */
	uint8_t DELkey_is_not_manipulated_bs:1;   /*!< DELキーでバックスペース制御をしない */
	uint8_t TABkey_is_exchange_to_space:1;     /*!< TABキーで空白文字への変換制御をしない */
	uint8_t nocheck_ascii_isprint_condition:1; /* アスキー文字に対する表示文字チェックをしない */
    } f;
} mpxx_conio_gets_attr_t;


#if defined(__cplusplus)
extern "C" {
#endif

int mpxx_conio_getchar(void);
int mpxx_conio_putchar(const int char_no);
int mpxx_conio_puts(const char *s);
int mpxx_conio_puts_without_newline(const char *s);

int mpxx_conio_kbhit(void);
int mpxx_conio_set_newline2stdout(void);

int mpxx_conio_gets_withechoback( char *const strbuf, const int maxlen, mpxx_conio_gets_attr_t * const attr_p);

char mpxx_conio_get_enter_code(void);

typedef struct _mpxx_conio_readline {
    char *readstr;
    size_t length;
} mpxx_conio_readline_t;

int mpxx_conio_readline_with_new_buffer( mpxx_conio_readline_t *const rd_p, const char eol_char, size_t maxszofline, FILE *frp);
void mpxx_conio_readline_delete_buffer( mpxx_conio_readline_t *const s_p);


#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_CONIO_H */
