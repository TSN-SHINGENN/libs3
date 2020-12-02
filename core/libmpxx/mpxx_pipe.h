#ifndef INC_MPXX_PIPE_H
#define INC_MPXX_PIPE_H

#include <stddef.h>
#include <stdint.h>

#if defined (WIN32)
#include <windows.h>
#endif

#if defined (WIN32)
typedef HANDLE mpxx_pipe_handle_t;
#elif  defined(Linux) || defined(QNX) || defined(Darwin) || defined(__AVM2__)
typedef int mpxx_pipe_handle_t;
#else
#error 'not defined mpxx_pipe_handle_t on target OS'
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int mpxx_pipe_create( mpxx_pipe_handle_t *const rd_pipe, mpxx_pipe_handle_t *const wr_pipe);
int mpxx_pipe_get_pipesize( mpxx_pipe_handle_t hPipe, size_t *const retval_p);
int mpxx_pipe_set_pipesize( mpxx_pipe_handle_t hPipe, size_t value);
int mpxx_pipe_set_mode_nonblock( mpxx_pipe_handle_t hPipe);
int mpxx_pipe_set_mode_block( mpxx_pipe_handle_t hPipe);
int mpxx_pipe_close( mpxx_pipe_handle_t hPipe);
int mpxx_pipe_duplicate(const mpxx_pipe_handle_t hPipe, mpxx_pipe_handle_t * const new_pipe_p);
int mpxx_pipe_write(const void *const dat, const size_t lenofdat, const mpxx_pipe_handle_t hPipe);
int mpxx_pipe_read(const mpxx_pipe_handle_t hPipe, void *const buf, const size_t length);
int mpxx_pipe_get_std_handle( mpxx_pipe_handle_t *const hPipe_p, const int fileno);
int mpxx_pipe_set_std_handle( const int file_no, const mpxx_pipe_handle_t hPipe);
mpxx_pipe_handle_t mpxx_pipe_invalid_handle(void);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_PIPE_H */
