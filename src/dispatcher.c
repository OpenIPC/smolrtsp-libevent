#include <smolrtsp-libevent/dispatcher.h>

#include <smolrtsp-libevent/bufferevent.h>
#include <smolrtsp-libevent/evbuffer.h>

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
            if (smolrtsp_respond(
                    conn, 0, SMOLRTSP_STATUS_BAD_REQUEST, "Malformed request",
                    SmolRTSP_HeaderMap_empty()) < 0) {
                // TODO: replace with a generic logging interface.
                perror("Failed to respond");
            }

            evbuffer_drain(input, buf.len);
            return;
        }
    }
}

static void dispatch(
    SmolRTSP_Writer conn, SmolRTSP_Request req,
    SmolRTSP_Controller controller) {
    VCALL(controller, before, conn, &req);

    const SmolRTSP_Method method = req.start_line.method;

    ssize_t ret;
    if (SmolRTSP_Method_eq(method, SMOLRTSP_METHOD_OPTIONS)) {
        ret = VCALL(controller, options, conn, &req);
    } else if (SmolRTSP_Method_eq(method, SMOLRTSP_METHOD_DESCRIBE)) {
        ret = VCALL(controller, describe, conn, &req);
    } else if (SmolRTSP_Method_eq(method, SMOLRTSP_METHOD_SETUP)) {
        ret = VCALL(controller, setup, conn, &req);
    } else if (SmolRTSP_Method_eq(method, SMOLRTSP_METHOD_PLAY)) {
        ret = VCALL(controller, play, conn, &req);
    } else if (SmolRTSP_Method_eq(method, SMOLRTSP_METHOD_TEARDOWN)) {
        ret = VCALL(controller, teardown, conn, &req);
    } else {
        ret = VCALL(controller, unknown, conn, &req);
    }

    VCALL(controller, after, ret, conn, &req);
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

    return ((DispatchCtx *)ctx)->controller;
}

void smolrtsp_libevent_ctx_free(void *ctx) {
    assert(ctx);

    DispatchCtx *self = ctx;
    VCALL_SUPER(self->controller, SmolRTSP_Droppable, drop);
    free(ctx);
}
