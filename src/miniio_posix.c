/* miniio POSIX */
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "miniio.h"

struct pos_event_s {
    struct pos_event_s* prev;
    struct pos_event_s* next;
    uintptr_t* event;
    size_t eventlen;
};

struct pos_fd_s {
    struct pos_fd_s* prev;
    struct pos_fd_s* next;
    int fd;
};

enum pos_evobj_type_e {
    EVOBJ_CHIME = 1,
    EVOBJ_TIMER /* FIXME: Implement this */,
    EVOBJ_DNS /* FIXME: Implement this */
};

/* FIXME: Static assert to ensure sizeof(pos_evnote_s) < PIPE_BUF */
struct pos_evobj_s {
    struct pos_evobj_s* prev;
    struct pos_evobj_s* next;
    enum pos_evobj_type_e type;
    void* userdata;
    int destroyed;
};

enum pos_evnote_cmd_e {
    EVNOTE_CMD_TRIGGER = 1,
    EVNOTE_CMD_DESTROY
};

struct pos_evnote_s {
    struct pos_evobj_s* obj;
    enum pos_evnote_cmd_e cmd;
};

struct pos_ctx_s {
    struct pos_event_s* first;
    struct pos_event_s* last;
    struct pos_fd_s* fds;
    struct pos_evobj_s* evobjs;
    size_t total_queued_event_len;
    fd_set rfds;
    int rfd_count;
    int wakeup_read;
    int wakeup_write;
};

struct pos_netparam_s {
    struct pos_ctx_s* ctx;
    void* userdata;
    const char* hostname;
    int has_addrinfo;
    char portnamebuf[8]; /* enough to store "65535" */
};

/* Event handling */

static void
addevent(struct pos_ctx_s* ctx, uintptr_t* event){
    /* Add to `last` */
    struct pos_event_s* ev;
    uintptr_t* msg;
    uintptr_t len;
    len = event[0];
    ev = malloc(sizeof(struct pos_event_s));
    if(! ev){
        abort();
    }
    msg = malloc(sizeof(uintptr_t) * len);
    if(! msg){
        abort();
    }
    memcpy(msg, event, sizeof(uintptr_t) * len);
    ev->event = msg;
    ev->eventlen = len;
    ev->prev = ctx->last;
    if(ev->prev){
        ev->prev->next = ev;
    }
    ev->next = 0;
    ctx->total_queued_event_len += len;
    ctx->last = ev;
    if(! ctx->first){
        ctx->first = ev;
    }
}

static void
delevent(struct pos_ctx_s* ctx){
    struct pos_event_s* ev;
    /* Remove from `first` */
    if(ctx->first){
        ev = ctx->first;
        ctx->first = ev->next;
        if(ctx->last == ev){
            ctx->last = 0;
        }
        if(ev->prev){
            abort();
        }
        if(ev->next){
            ev->next->prev = 0;
        }
        ctx->total_queued_event_len -= ev->eventlen;
        free(ev->event);
        free(ev);
    }else{
        abort();
    }
}


/* Context */
/* I/O Context (No NCCC export) */
void*
miniio_ioctx_create(void){
    int p[2];
    int i,j,r;
    struct pos_ctx_s* ctx;
    errno = 0;

    /* Create blocking, CLOEXEC pipe */
    r = pipe(&p);
    if(r != 0){
        goto err0;
    }
    for(j=0;j!=2;j++){
        r = fcntl(p[j], F_GETFD);
        if(r == -1){
            goto err1;
        }
        i = r | FD_CLOEXEC;
        r = fcntl(p[j], F_SETFD, i);
        if(r == -1){
            goto err1;
        }
    }

    /* Mark read end as non-block */
    r = fcntl(p[0], F_GETFL);
    if(r == -1){
        goto err1;
    }
    i = r | O_NONBLOCK;
    r = fcntl(p[0], F_SETFL, i);
    if(r == -1){
        goto err1;
    }

    ctx = malloc(sizeof(struct pos_ctx_s));
    if(! ctx){
        goto err1;
    }
    ctx->wakeup_read = p[0];
    ctx->wakeup_write = p[1];
    ctx->first = ctx->last = 0;
    ctx->total_queued_event_len = 0;
    ctx->evobjs = 0;
    FD_ZERO(&ctx->rfds);
    ctx->rfd_count = 0;
    return ctx;

err1:
    close(p[0]);
    close(p[1]);

err0:
    return 0;
}

static void
evobj_destroy(struct pos_ctx_s* ctx, struct pos_evobj_s* obj){
    if(obj->next){
        obj->next->prev = obj->prev;
    }
    if(obj->prev){
        obj->prev->next = obj->next;
    }else{
        ctx->evobjs = obj->next;
    }

    free(obj);
}

static int /* 1 = continue */
proc_wakeup(struct pos_ctx_s* ctx){
    ssize_t r;
    struct pos_evnote_s n;
    int e;
    uintptr_t ev[4];
    errno = 0;
    r = read(ctx->wakeup_read, &n, sizeof(n));
    if(r < 0){
        e = errno;
        if(e == EAGAIN || e == EWOULDBLOCK){
            return 0;
        }else{
            abort();
        }
    }
    if(r != sizeof(n)){
        /* Fragmented */
        abort();
    }

    /* Generate event */
    switch(n->cmd){
        case EVNOTE_CMD_TRIGGER:
            switch(n->obj->type){
                case EVOBJ_CHIME:
                    /* [4 12(chime) handle userdata] */
                    ev[0] = 4;
                    ev[1] = MINIIO_EVT_CHIME;
                    ev[2] = (uintptr_t)n->obj;
                    ev[3] = (uintptr_t)n->obj->userdata;
                    addevent(ctx, ev);
                    break;
                default:
                    abort();
                    break;
            }
            break;
        case EVNOTE_CMD_DESTROY:
            evobj_destroy(ctx, n->obj);
            break;
        default:
            abort();
            break;
    }

    return 1;
}

static void
proc_fd_read(struct pos_ctx_s* ctx, struct pos_fd_s* pfd){

}

int 
miniio_ioctx_process(void* pctx){
    struct pos_ctx_s* ctx = (struct pos_ctx_s*)pctx;
    struct pos_fd_s* pfd;
    int maxfd = -1;
    int r;

    FD_ZERO(&ctx->rfds);
    if(ctx->evobjs){
        /* Add wakeup_read */
        FD_SET(ctx->wakeup_read, &ctx->rfds);
        if(maxfd < ctx->wakeup_read){
            maxfd = ctx->wakeup_read;
        }
    }
    if(ctx->fds){
        /* Add fds */
        pfd = ctx->fds;
        while(pfd){
            FD_SET(pfd->fd, &ctx->rfds);
            if(maxfd < pfd->fd){
                maxfd = pfd->fd;
            }
            pfd = pfd->next;
        }
    }

    if(maxfd < 0 /* No fd */){
        return 0;
    }

    /* Wait */
    errno = 0;
    r = select(maxfd + 1, &ctx->rfds, 0, 0, 0);
    if(r == -1){
        switch(errno){
            case EINTR:
                return 0;
            default:
                abort();
                return 0;
        }
    }else{
        ctx->rfd_count = r;
    }

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
    struct pos_ctx_s* ctx = (struct pos_ctx_s*)pctx;
    struct pos_fd_s* pfd;
    *out_written = 0;
    *out_current = 0;

    /* Consume waited read fds */
    if(FD_ISSET(ctx->wakeup_read, &rfds)){
        rr = 1;
        while(rr){
            rr = proc_wakeup(ctx);
        }
        ctx->rfd_count--;
    }

    pfd = ctx->fds;

    while(ctx->rfd_count){
        if(FD_ISSET(pfd->fd, &rfds)){
            proc_fd_read(ctx, pfd);
            ctx->rfd_count--;
        }
        pfd = pfd->next;
    }

    return 0;
}

/* Timer */
void* 
miniio_timer_create(void* pctx, void* userdata){
    /* FIXME: Implement this */
    abort();
    return malloc(1);
}

void 
miniio_timer_destroy(void* pctx, void* phandle){
    /* FIXME: Implement this */
    abort();
    free(phandle);
}

int 
miniio_timer_start(void* pctx, void* phandle, uint64_t timeout,
                       uint64_t interval){
    /* FIXME: Implement this */
    abort();
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
    /* FIXME: Implement write queue */
    /*        Write immediately now */
    return 0;
}

int 
miniio_start_read(void* ctx, void* stream){
    return 0;
}
