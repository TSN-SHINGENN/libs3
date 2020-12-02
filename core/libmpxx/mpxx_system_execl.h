#ifndef INC_MPXX_SYSTEM_EXECL_H
#define INC_MPXX_SYSTEM_EXECL_H

#include <stddef.h>
#include <stdint.h>

typedef union _mpxx_system_execl_attr {
    unsigned int flags;
    struct {
//	unsigned int ignore_response_when_wakeup_messeage:1; /* コマンド実行時のWELCOMEメッセージを無視させる(読み捨て）*/
	unsigned int process_terminated_after_only_once:1; /* 一回の通信のみでプロセスを終了する処理をします */
	unsigned int stdout_block_by_length_of_responce:1;
	unsigned int stderr_block_by_length_of_responce:1;
	unsigned int prefix_absolute_path:1; /* 絶対パスを内部処理で付ける */
    } f;
} mpxx_system_execl_attr_t;

typedef struct _mpxx_system_execl {
    void *ext;
} mpxx_system_execl_t;

#if defined (__cplusplus )
extern "C" {
#endif

int mpxx_system_execl_init( mpxx_system_execl_t *const self_p, char *const commandstr, const mpxx_system_execl_attr_t *const attr_p);
void mpxx_system_execl_destroy( mpxx_system_execl_t *const self_p);
void mpxx_system_execl_set_command_progress_waiting_time( mpxx_system_execl_t *const self_p, const size_t time_msec);
int mpxx_system_execl_command_exec( mpxx_system_execl_t *const self_p, const char *const cmd, const int timeout_msec, void *const resbuf, size_t *reslen_p, char *const errbuf, size_t *errlen_p);

int mpxx_system_execl_writeto_stdin(mpxx_system_execl_t *const self_p, const void *const dat, const size_t writelen);
int mpxx_system_execl_readfrom_stdout(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen);
int mpxx_system_execl_readfrom_stdout_withtimeout(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen, const int timeout_msec);
int mpxx_system_execl_readfrom_stderr(mpxx_system_execl_t *const self_p, void *const buf, const size_t readlen);

#if defined (__cplusplus )
} 
#endif

class mpxx_csystem_execl {
private:
    mpxx_system_execl_t obj;

public:
	mpxx_csystem_execl();
	mpxx_csystem_execl(mpxx_system_execl_attr_t &attr);
    virtual ~mpxx_csystem_execl();

    int set_command_progress_waiting_time(const unsigned int delay_time);
    int command(mpxx_system_execl_t *const self_p, const char *const cmd, char *resbuf, const size_t len);
};

#endif /* end of INC_MPXX_SYSTEM_EXEC_H */
