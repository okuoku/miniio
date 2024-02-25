/* Local-connection only stub for miniio */
// NB: accepted connection must start_read immediately...
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "miniio.h"

struct lns_ctx_s;

struct lns_chime_s {
    void* userdata;
};

struct lns_buffer_s {
    void* userdata;
    char* data;
    size_t len;
};

struct lns_netparam_s {
    void* userdata;
    int port;
};

struct lns_connection_s {
    struct lns_connection_s* prev; /* On root */
    struct lns_connection_s* next; /* On root */
    struct lns_ctx_s* ctx;
    struct lns_connection_s* peer;
    void* userdata;
    int listener_port; /* -1 for outgoing connection, 0 for accepted conn */
    int reading;
};

#define LNS_MAX_EVT_LEN 8

struct lns_event_s {
    struct lns_event_s* prev;
    struct lns_event_s* next;
    uintptr_t evt[LNS_MAX_EVT_LEN];
};

struct lns_ctx_s {
    struct lns_event_s* top;
    struct lns_event_s* bottom;
    pthread_mutex_t mtx;
    pthread_cond_t cnd;
};

static struct lns_connection_s* listeners = 0;

/* Context */
/* I/O Context (No NCCC export) */

void*
miniio_ioctx_create(void){
    struct lns_ctx_s* ctx;
    ctx = (struct lns_ctx_s*)malloc(sizeof(struct lns_ctx_s));
    memset(ctx, 0, sizeof(struct lns_ctx_s));
    pthread_mutex_init(&ctx->mtx, 0);
    pthread_cond_init(&ctx->cnd, 0);
    return ctx;
}

int 
miniio_ioctx_process(void* pctx){
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    // FIXME: Handle nothing-to-do case
    pthread_mutex_lock(&ctx->mtx);
    for(;;){
        if(ctx->top){
            break;
        }
        pthread_cond_wait(&ctx->cnd, &ctx->mtx);
    }
    pthread_mutex_unlock(&ctx->mtx);
    return 0;
}

void 
miniio_ioctx_destroy(void* pctx){
    abort();
    free(pctx);
}


/* Context, Eventqueue */
int 
miniio_get_events(void* pctx, uintptr_t* buf, uint32_t bufcount, 
                  uint32_t* out_written,
                  uint32_t* out_current){
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_event_s* e;
    uint32_t cur, wrt;
    intptr_t rem;
    uint32_t size;
    cur = 0;
    wrt = 0;
    rem = bufcount;
    pthread_mutex_lock(&ctx->mtx);
    e = ctx->top;
    while(e){
        size = e->evt[0];
        if(size <= rem){
            /* We can dequeue event */
            memcpy(&buf[wrt], e->evt, sizeof(uintptr_t) * size);
            wrt += size;
            cur += size;
            if(ctx->top == ctx->bottom){
                ctx->bottom = 0;
                ctx->top = 0;
                e = 0;
            }else{
                e->next->prev = 0;
                e = e->next;
                ctx->top = e;
            }
        }else{
            /* Just go to next */
            cur += size;
            e = e->next;
        }
    }
    pthread_mutex_unlock(&ctx->mtx);
    *out_written = wrt;
    *out_current = cur;
    return 0;
}

static void
queue_event(struct lns_ctx_s* ctx, uintptr_t a[]){
    struct lns_event_s* e;
    if(a[0] > LNS_MAX_EVT_LEN){ /* sanity */
        abort();
    }
    e = (struct lns_event_s*)malloc(sizeof(struct lns_event_s));
    e->next = 0;
    memcpy(e->evt, a, sizeof(uintptr_t) * a[0]);

    pthread_mutex_lock(&ctx->mtx);
    if(ctx->bottom){
        ctx->bottom->next = e;
    }
    e->prev = ctx->bottom;
    ctx->bottom = e;
    if(! ctx->top){
        ctx->top = e;
    }
    pthread_cond_signal(&ctx->cnd);
    pthread_mutex_unlock(&ctx->mtx);
}

/* Timer */
void* 
miniio_timer_create(void* pctx, void* userdata){
    abort();
    return malloc(1);
}

void 
miniio_timer_destroy(void* pctx, void* phandle){
    abort();
    free(phandle);
}

int 
miniio_timer_start(void* pctx, void* phandle, uint64_t timeout,
                       uint64_t interval){
    abort();
    return 0;
}

/* TCP(Network stream) */
void* 
miniio_net_param_create(void* ctx, void* userdata){
    struct lns_netparam_s* param;
    (void)ctx;
    param = (struct lns_netparam_s*)malloc(sizeof(struct lns_netparam_s));
    param->userdata = userdata;
    param->port = 0;
    return param;
}

void 
miniio_net_param_destroy(void* ctx, void* param){
    free(param);
}

int 
miniio_net_param_hostname(void* ctx, void* param, const char* hostname){
    /* Ignore hostname parameter */
    return 0;
}

int 
miniio_net_param_port(void* ctx, void* pparam, int port){
    struct lns_netparam_s* param = (struct lns_netparam_s*)pparam;
    param->port = port;
    return 0;
}

int
miniio_net_param_name_resolve(void* pctx, void* pparam){
    uintptr_t ev[LNS_MAX_EVT_LEN];
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_netparam_s* param = (struct lns_netparam_s*)pparam;

    /* Immediately success */
    ev[0] = 5;
    ev[1] = MINIIO_EVT_NETRESOLVE;
    ev[2] = (uintptr_t)param;
    ev[3] = (uintptr_t)param->userdata;
    ev[4] = 0;
    queue_event(ctx, ev);

    return 0;
}

int 
miniio_net_param_name_fetch(void* pctx, void* param, uint32_t idx,
                            uint32_t* ipversion,
                            uint8_t** addr, uint32_t* addrlen){
    abort();
    return 1;
}

void* 
miniio_tcp_create(void* pctx, void* pparam, uint32_t idx, void* userdata){
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_netparam_s* param = (struct lns_netparam_s*)pparam;
    struct lns_connection_s* handle;
    handle = (struct lns_connection_s*)malloc(sizeof(struct lns_connection_s));
    handle->next = handle->prev = 0;
    handle->ctx = ctx;
    handle->peer = 0;
    handle->userdata = userdata;
    handle->listener_port = param ? param->port : -1;
    handle->reading = 0;
}

int
miniio_tcp_listen(void* pctx, void* phandle){
    struct lns_connection_s* handle = (struct lns_connection_s*)phandle;
    (void)pctx;
    if(listeners){
        listeners->prev = handle;
    }
    listeners = handle;
    return 0;
}

int
miniio_tcp_connect(void* pctx, void* phandle, void* pparam, uint32_t idx){
    uintptr_t ev[LNS_MAX_EVT_LEN];
    struct lns_connection_s* handle = (struct lns_connection_s*)phandle;
    struct lns_netparam_s* param = (struct lns_netparam_s*)pparam;
    struct lns_connection_s* c;
    struct lns_ctx_s* peer_ctx;

    /* Search a appropriate listener */
    c = listeners;
    while(c){
        if(c->listener_port == param->port){
            break;
        }
        c = c->next;
    }
    if(!c){
        abort();
    }

    peer_ctx = c->ctx;

    /* Queue peer */
    if(c->peer){
        abort();
    }
    c->peer = handle;

    /* Send connection event to peer */
    ev[0] = 5;
    ev[1] = MINIIO_EVT_CONNECT_INCOMMING;
    ev[2] = (uintptr_t)c;
    ev[3] = (uintptr_t)c->userdata;
    ev[4] = 0;
    queue_event(peer_ctx, ev);

    return 0;
}

void*
miniio_tcp_accept(void* pctx, void* phandle, void* new_userdata){
    uintptr_t ev[LNS_MAX_EVT_LEN];
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_connection_s* handle = (struct lns_connection_s*)phandle;
    struct lns_connection_s* newconn;
    if(! handle->peer){
        abort();
    }
    newconn = (struct lns_connection_s*)malloc(sizeof(struct lns_connection_s));
    newconn->prev = newconn->next = 0;
    newconn->ctx = ctx;
    newconn->peer = handle->peer;
    handle->peer = 0;
    newconn->userdata = new_userdata;
    newconn->listener_port = 0;
    newconn->reading = 1;
    newconn->peer->peer = newconn;

    /* Emit connected event for peer */
    ev[0] = 5;
    ev[1] = MINIIO_EVT_CONNECT_OUTGOING;
    ev[2] = (uintptr_t)newconn->peer;
    ev[3] = (uintptr_t)newconn->peer->userdata;
    ev[4] = 0;
    queue_event(newconn->peer->ctx, ev);

    return newconn;
}

int
miniio_tcp_shutdown(void* pctx, void* handle){
    abort();
    return 0;
}

/* Process */
void* 
miniio_process_param_create(void* ctx, const char* execpath, void* userdata){
    abort();
    return malloc(1);
}

void 
miniio_process_param_destroy(void* ctx, void* param){
    abort();
    free(param);
}

int 
miniio_process_param_workdir(void* ctx, void* param, const char* dir){
    abort();
    return 0;
}

int 
miniio_process_param_args(void* ctx, void* param, void* pargv, int argc){
    abort();
    return 0;
}

int 
miniio_process_param_stdin(void* ctx, void* param, void* pipe){
    abort();
    return 0;
}

int 
miniio_process_param_stdout(void* ctx, void* param, void* pipe){
    abort();
    return 0;
}

int 
miniio_process_param_stderr(void* ctx, void* param, void* pipe){
    abort();
    return 0;
}

void* 
miniio_process_spawn(void* pctx, void* param){
    abort();
    return 0;
}

int 
miniio_process_abort(void* ctx, void* handle){
    abort();
    return 1;
}

void
miniio_process_destroy(void* ctx, void* handle){
    abort();
    free(handle);
}

void* 
miniio_pipe_new(void* pctx, void* userdata){
    abort();
    return malloc(1);
}

/* Chime */
void* 
miniio_chime_new(void* pctx, void* userdata){
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_chime_s* c;
    c = (struct lns_chime_s*)malloc(sizeof(struct lns_chime_s));
    c->userdata = userdata;
    return c;
}

int 
miniio_chime_trigger(void* pctx, void* phandle){
    uintptr_t ev[LNS_MAX_EVT_LEN];
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_chime_s* handle = (struct lns_chime_s*)phandle;
    ev[0] = 4;
    ev[1] = MINIIO_EVT_CHIME;
    ev[2] = (uintptr_t)handle;
    ev[3] = (uintptr_t)handle->userdata;
    queue_event(ctx, ev);
    return 0;
}

void 
miniio_chime_destroy(void* ctx, void* handle){
    free(handle);
}


/* Stream I/O */
void 
miniio_close(void* ctx, void* stream){
    abort();
}

void* 
miniio_buffer_create(void* ctx, uint32_t size, void* userdata){
    struct lns_buffer_s* buf;
    buf = (struct lns_buffer_s*)malloc(sizeof(struct lns_buffer_s));
    buf->userdata = userdata;
    buf->len = size;
    buf->data = (char*)malloc(size);
    return buf;
}

void 
miniio_buffer_destroy(void* ctx, void* phandle){
    struct lns_buffer_s* handle = (struct lns_buffer_s*)phandle;
    (void)ctx;

    free(handle->data);
    free(handle);
}

void* 
miniio_buffer_lock(void* ctx, void* phandle, uint32_t offset, uint32_t len){
    struct lns_buffer_s* handle = (struct lns_buffer_s*)phandle;
    void* r;
    (void)ctx;
    (void)len;
    r = &handle->data[offset];
    return r;
}

void 
miniio_buffer_unlock(void* ctx, void* handle){
    /* Do nothing */
    (void)ctx;
    (void)handle;
}

int 
miniio_write(void* pctx, void* pstream, void* pbuffer, uint32_t offset,
                 uint32_t len){
    uintptr_t ev[LNS_MAX_EVT_LEN];

    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_connection_s* stream = (struct lns_connection_s*)pstream;
    struct lns_buffer_s* buffer = (struct lns_buffer_s*)pbuffer;
    struct lns_buffer_s* newbuffer;
    void* data;
    void* tgt;
    if(! stream->peer){
        abort();
    }
    if(! stream->peer->reading){
        abort();
    }
    data = miniio_buffer_lock(ctx, buffer, offset, len);
    newbuffer = miniio_buffer_create(ctx, len, 0);
    tgt = miniio_buffer_lock(ctx, newbuffer, 0, len);
    memcpy(tgt, data, len);
    miniio_buffer_unlock(ctx, newbuffer);
    miniio_buffer_unlock(ctx, buffer);

    /* Emit write done for myself */
    ev[0] = 5;
    ev[1] = MINIIO_EVT_WRITE_COMPLETE;
    ev[2] = (uintptr_t)buffer;
    ev[3] = (uintptr_t)buffer->userdata;
    ev[4] = 0;
    queue_event(ctx, ev);

    /* Emit read done for peer */
    ev[0] = 7;
    ev[1] = MINIIO_EVT_READ_COMPLETE;
    ev[2] = (uintptr_t)stream->peer;
    ev[3] = (uintptr_t)stream->peer->userdata;
    ev[4] = (uintptr_t)newbuffer;
    ev[5] = 0;
    ev[6] = len;
    queue_event(stream->peer->ctx, ev);

    return 0;
}

int 
miniio_start_read(void* pctx, void* pstream){
    uintptr_t ev[LNS_MAX_EVT_LEN];
    struct lns_ctx_s* ctx = (struct lns_ctx_s*)pctx;
    struct lns_connection_s* stream = (struct lns_connection_s*)pstream;
    (void)pctx;
    if(stream->reading && (stream->listener_port != 0)){
        abort();
    }
    if(!stream->peer){
        abort();
    }
    stream->reading = 1;

    if(stream->peer->reading){ /* both ends now reading */
    }else{
        // limitation
        abort();
    }


    return 0;
}
