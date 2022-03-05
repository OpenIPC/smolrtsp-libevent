#include <smolrtsp-libevent/dispatcher.h>

#include <smolrtsp-libevent/bufferevent.h>
#include <smolrtsp-libevent/evbuffer.h>

#include <smolrtsp/context.h>
#include <smolrtsp/types/request.h>
#include <smolrtsp/types/response.h>
#include <smolrtsp/types/status_code.h>
#include <smolrtsp/writer.h>

#include <assert.h>
#include <stdlib.h>

#include <datatype99.h>
#include <slice99.h>

typedef struct {
    SmolRTSP_Controller controller;
} DispatchCtx;

static void dispatch(
    SmolRTSP_Writer conn, SmolRTSP_Request req, SmolRTSP_Controller controller);

void smolrtsp_libevent_dispatch_cb(struct bufferevent *bev, void *arg) {
    assert(bev);
    assert(arg);

    DispatchCtx *ctx = arg;

    struct evbuffer *input = bufferevent_get_input(bev);
    const SmolRTSP_Writer conn = smolrtsp_bufferevent_writer(bev);

    const CharSlice99 buf = smolrtsp_evbuffer_slice(input);

    SmolRTSP_Request req = SmolRTSP_Request_uninit();
    const SmolRTSP_ParseResult res = SmolRTSP_Request_parse(&req, buf);

    match(res) {
        of(SmolRTSP_ParseResult_Success, status) match(*status) {
            of(SmolRTSP_ParseStatus_Complete, offset) {
                dispatch(conn, req, ctx->controller);
                evbuffer_drain(input, *offset);
            }
            otherwise return; // Partial input, skip it.
        }
        of(SmolRTSP_ParseResult_Failure, _) {
            // TODO: handler properly.
            fputs("Failed to parse the request", stderr);

            evbuffer_drain(input, buf.len);
            return;
        }
    }
}

static void dispatch(
    SmolRTSP_Writer conn, SmolRTSP_Request req,
    SmolRTSP_Controller controller) {
    SmolRTSP_Context *ctx = SmolRTSP_Context_new(conn, req.cseq);

    VCALL(controller, before, ctx, &req);

    const SmolRTSP_Method method = req.start_line.method,
                          options = SMOLRTSP_METHOD_OPTIONS,
                          describe = SMOLRTSP_METHOD_DESCRIBE,
                          setup = SMOLRTSP_METHOD_SETUP,
                          play = SMOLRTSP_METHOD_PLAY,
                          teardown = SMOLRTSP_METHOD_TEARDOWN;

    ssize_t ret;
    if (SmolRTSP_Method_eq(&method, &options)) {
        ret = VCALL(controller, options, ctx, &req);
    } else if (SmolRTSP_Method_eq(&method, &describe)) {
        ret = VCALL(controller, describe, ctx, &req);
    } else if (SmolRTSP_Method_eq(&method, &setup)) {
        ret = VCALL(controller, setup, ctx, &req);
    } else if (SmolRTSP_Method_eq(&method, &play)) {
        ret = VCALL(controller, play, ctx, &req);
    } else if (SmolRTSP_Method_eq(&method, &teardown)) {
        ret = VCALL(controller, teardown, ctx, &req);
    } else {
        ret = VCALL(controller, unknown, ctx, &req);
    }

    VCALL(controller, after, ret, ctx, &req);

    VTABLE(SmolRTSP_Context, SmolRTSP_Droppable).drop(ctx);
}

void *smolrtsp_libevent_ctx(SmolRTSP_Controller controller) {
    assert(controller.self && controller.vptr);

    DispatchCtx *self = malloc(sizeof *self);
    assert(self);
    self->controller = controller;
    return self;
}

SmolRTSP_Controller smolrtsp_libevent_ctx_controller(void *ctx) {
    assert(ctx);

    DispatchCtx *self = ctx;
    return self->controller;
}

void smolrtsp_libevent_ctx_free(void *ctx) {
    assert(ctx);

    DispatchCtx *self = ctx;
    VCALL_SUPER(self->controller, SmolRTSP_Droppable, drop);
    free(ctx);
}
