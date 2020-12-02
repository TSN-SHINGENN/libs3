#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <core/libmpxx/mpxx_sys_types.h>
#include <core/libmpxx/mpxx_stdlib.h>
#include <core/libmpxx/mpxx_unistd.h>
#include <core/libmpxx/mpxx_sprintf.h>
#include <core/libmpxx/mpxx_lite_sprintf.h>
// #include "myvprintf.h"


int myputc(const int c)
{
    putc(c, stdout);

    return (0xff & c);
}


void myprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	mpxx_lite_vprintf(fmt, ap);
//	_multios_lite_vsprintf(myputc,fmt, ap);
//	myvprintf(myputc, fmt, ap);
	va_end(ap);
}


void xil_print_log_timer(void)
{
    char after_decmal_str[MPXX_XTOA_DEC32_BUFSZ];
    char integer_str[MPXX_XTOA_DEC64_BUFSZ];
    const uint64_t jiffies = time(NULL);
    int64_t integer_num = jiffies / 1000;
    int16_t after_decimal_num = (jiffies % 1000);

    mpxx_u64toadec((after_decimal_num / 10), after_decmal_str, MPXX_XTOA_DEC32_BUFSZ);
    mpxx_u64toadec(integer_num, integer_str, MPXX_XTOA_DEC64_BUFSZ);
    
//    xil_printf("[%08s.%02s] ", integer_str, after_decmal_str);
    mpxx_lite_printf("[%08s.%02s] ", integer_str, after_decmal_str);
    mpxx_lite_printf("[%08u.%02u] ", (unsigned int)integer_num, (unsigned int)after_decimal_num);
    mpxx_lite_printf("[%08llu.%02llu] ",
		    (unsigned long long int)integer_num, (unsigned  long long int)after_decimal_num);

    return;
}

void main()
{
    char *const msg = "hello world!";
    const uint64_t time64 = time(NULL);
    int monotonic =0;
    size_t n;
    int ret;

    mpxx_lite_printf_init(myputc);

    xil_print_log_timer();
    mpxx_lite_printf("%s\n", msg);

    Sleep(1000);
    xil_print_log_timer();
    mpxx_lite_printf("%40s\n", msg);
    Sleep(1000);
    xil_print_log_timer();
    mpxx_lite_printf("%-40s\n", msg);
    Sleep(1000);

    mpxx_lite_printf("a=%c\n", 'a');
    mpxx_lite_printf("b=%c\n", 'b');
    mpxx_lite_printf("c=%c\n", 'c');


    for(n=0;;mpxx_msleep(750), ++monotonic, ++n) {
	char operation_str[10];

	xil_print_log_timer();
	mpxx_lite_printf("%s\n", msg);

	//mpxx_lite_snprintf(operation_str, 10, "%u %04d", n, (int)monotonic);
	ret = mpxx_lite_snprintf(operation_str, 5, "%u-%02d", n, (int)monotonic);
	mpxx_lite_printf("ret=%d errno=%d:strerror=%s Monotonic '%s'\n", ret, errno, strerror(errno), operation_str);
    }

    system("pause");
}
