/* Do-nothing stub for miniio */
#include <stdlib.h>
#include <stdio.h>
#include "miniio.h"

/* Context */
/* I/O Context (No NCCC export) */

void*
miniio_ioctx_create(void){
    return malloc(1);
}

int 
miniio_ioctx_process(void* pctx){
    /* FIXME: Which one should we choose ..? */
    return 1;
}

void 
miniio_ioctx_destroy(void* pctx){
    free(pctx);
}


/* Context, Eventqueue */
int 
miniio_get_events(void* pctx, uintptr_t* buf, uint32_t bufcount, 
                  uint32_t* out_written,
                  uint32_t* out_current){
    *out_written = 0;
    *out_current = 0;
    return 0;
}

/* Timer */
void* 
miniio_timer_create(void* pctx, void* userdata){
    return malloc(1);
}

void 
miniio_timer_destroy(void* pctx, void* phandle){
    free(phandle);
}

int 
miniio_timer_start(void* pctx, void* phandle, uint64_t timeout,
                       uint64_t interval){
    return 0;
}

/* TCP(Network stream) */
void* 
miniio_net_param_create(void* ctx, void* userdata){
    return malloc(1);
}

void 
miniio_net_param_destroy(void* ctx, void* param){
    free(param);
}

int 
miniio_net_param_hostname(void* ctx, void* param, const char* hostname){
    return 0;
}

int 
miniio_net_param_port(void* ctx, void* param, int port){
    return 0;
}

int
miniio_net_param_name_resolve(void* pctx, void* param){
    /* FIXME: Which one should we choose ..? */
    return 0;
}

int 
miniio_net_param_name_fetch(void* pctx, void* param, uint32_t idx,
                            uint32_t* ipversion,
                            uint8_t** addr, uint32_t* addrlen){
    return 1;
}

void* 
miniio_tcp_create(void* pctx, void* param, uint32_t idx, void* userdata){
    return malloc(1);
}

int
miniio_tcp_listen(void* pctx, void* handle){
    return 0;
}

int
miniio_tcp_connect(void* pctx, void* handle, void* param, uint32_t idx){
    return 0;
}

void*
miniio_tcp_accept(void* pctx, void* handle, void* new_userdata){
    return 0;
}

int
miniio_tcp_shutdown(void* pctx, void* handle){
    return 0;
}

/* Process */
void* 
miniio_process_param_create(void* ctx, const char* execpath, void* userdata){
    return malloc(1);
}

void 
miniio_process_param_destroy(void* ctx, void* param){
    free(param);
}

int 
miniio_process_param_workdir(void* ctx, void* param, const char* dir){
    return 0;
}

int 
miniio_process_param_args(void* ctx, void* param, void* pargv, int argc){
    return 0;
}

int 
miniio_process_param_stdin(void* ctx, void* param, void* pipe){
    return 0;
}

int 
miniio_process_param_stdout(void* ctx, void* param, void* pipe){
    return 0;
}

int 
miniio_process_param_stderr(void* ctx, void* param, void* pipe){
    return 0;
}

void* 
miniio_process_spawn(void* pctx, void* param){
    return 0;
}

int 
miniio_process_abort(void* ctx, void* handle){
    return 1;
}

void
miniio_process_destroy(void* ctx, void* handle){
    free(handle);
}

void* 
miniio_pipe_new(void* pctx, void* userdata){
    return malloc(1);
}

/* Chime */
void* 
miniio_chime_new(void* pctx, void* userdata){
    return malloc(1);
}

int 
miniio_chime_trigger(void* pctx, void* handle){
    return 0;
}

void 
miniio_chime_destroy(void* ctx, void* handle){
    free(handle);
}


/* Stream I/O */
void 
miniio_close(void* ctx, void* stream){
}

void* 
miniio_buffer_create(void* ctx, uint32_t size, void* userdata){
    return malloc(size);
}

void 
miniio_buffer_destroy(void* ctx, void* handle){
    free(handle);
}

void* 
miniio_buffer_lock(void* ctx, void* handle, uint32_t offset, uint32_t len){
    return handle + offset;
}

void 
miniio_buffer_unlock(void* ctx, void* handle){
    /* Do nothing */
    (void)ctx;
    (void)handle;
}

int 
miniio_write(void* ctx, void* stream, void* buffer, uint32_t offset,
                 uint32_t len){
    return 0;
}

int 
miniio_start_read(void* ctx, void* stream){
    return 0;
}
