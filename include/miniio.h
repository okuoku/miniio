#ifndef YUNI__MINIIO_H
#define YUNI__MINIIO_H

#ifdef __cplusplus
extern "C" {
#endif
/* } */

#include <stdint.h>

/* Events */
/* [4  0(handle-close) handle userdata] */
#define MINIIO_EVT_HANDLE_CLOSE 0
/* [4  1(timer) handle userdata] */
#define MINIIO_EVT_TIMER 1
/* [5  2(netresolve) handle userdata result] */
#define MINIIO_EVT_NETRESOLVE 2
/* [5  3(incomming) handle userdata result] */
#define MINIIO_EVT_CONNECT_INCOMMING 3
/* [5  4(outgoing) handle userdata result] */
#define MINIIO_EVT_CONNECT_OUTGOING 4
/* [5  5(shutdown) handle userdata result] */
#define MINIIO_EVT_SHUTDOWN 5
/* [5  6(write-complete) buffer-handle buffer-userdata result] */
#define MINIIO_EVT_WRITE_COMPLETE 6
/* [7  7(read-complete) stm-handle stm-userdata buf-handle start n] */
#define MINIIO_EVT_READ_COMPLETE 7
/* [4  8(read-eof) stm-handle stm-userdata] */
#define MINIIO_EVT_READ_EOF 8
/* [5  9(read-stop) stm-handle stm-userdata bufend?] */
#define MINIIO_EVT_READ_STOP 9
/* [5 10(read-error) stm-handle stm-userdata err] */
#define MINIIO_EVT_READ_ERROR 10
/* [6 11(process-exit) handle userdata status signal] */
#define MINIIO_EVT_PROCESS_EXIT 11

/* I/O Context */
void* miniio_ioctx_create(void);
int miniio_ioctx_process(void* ctx);
void miniio_ioctx_destroy(void* ctx);

/* Context, Eventqueue */
int miniio_get_events(void* ctx, uintptr_t* buf, uint32_t bufcount,
                      uint32_t* out_written, uint32_t* out_current);

/* Timer */
void* miniio_timer_create(void* ctx, void* userdata);
void miniio_timer_destroy(void* ctx, void* handle);
int miniio_timer_start(void* ctx, void* handle, uint64_t timeout, 
                       uint64_t interval);

/* TCP(Network stream) */
void* miniio_net_param_create(void* ctx, void* userdata);
void miniio_net_param_destroy(void* ctx, void* param);
int miniio_net_param_hostname(void* ctx, void* param, const char* hostname);
int miniio_net_param_port(void* ctx, void* param, int port);
int miniio_net_param_name_resolve(void* ctx, void* param);
int miniio_net_param_name_fetch(void* ctx, void* param, uint32_t idx, 
                                uint32_t* ipversion,
                                uint8_t** addr, uint32_t* addrlen);
void* miniio_tcp_create(void* ctx, void* param, uint32_t idx, void* userdata);
int miniio_tcp_listen(void* ctx, void* handle);
int miniio_tcp_connect(void* ctx, void* handle, void* param, uint32_t idx);
void* miniio_tcp_accept(void* ctx, void* handle, void* userdata);
int miniio_tcp_shutdown(void* ctx, void* handle);

/* Process */
void* miniio_process_param_create(void* ctx, const char* execpath, 
                                  void* userdata);
void miniio_process_param_destroy(void* ctx, void* param);
int miniio_process_param_workdir(void* ctx, void* param, const char* dir);
int miniio_process_param_args(void* ctx, void* param, void* argv, int argc);
int miniio_process_param_stdin(void* ctx, void* param, void* pipe);
int miniio_process_param_stdout(void* ctx, void* param, void* pipe);
int miniio_process_param_stderr(void* ctx, void* param, void* pipe);
void* miniio_process_spawn(void* ctx, void* param);
int miniio_process_abort(void* ctx, void* handle);
void miniio_process_destroy(void* ctx, void* handle);
void* miniio_pipe_new(void* ctx, void* userdata);

/* Stream I/O */
void miniio_close(void* ctx, void* stream); /* For tcp and pipe */
void* miniio_buffer_create(void* ctx, uint32_t size, void* userdata);
void miniio_buffer_destroy(void* ctx, void* handle);
void* miniio_buffer_lock(void* ctx, void* handle, uint32_t offset,
                         uint32_t len);
void miniio_buffer_unlock(void* ctx, void* handle);
int miniio_write(void* ctx, void* stream, void* buffer, uint32_t offset, 
                 uint32_t len);
int miniio_start_read(void* ctx, void* stream, void* buffer);

/* { */
#ifdef __cplusplus
};
#endif

#endif /* YUNI__MINIIO_H */
