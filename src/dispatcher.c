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

void smolrtsp_libevent_cb(struct bufferevent *bev, void *arg) {
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
                smolrtsp_dispatch(conn, ctx->controller, &req);
                evbuffer_drain(input, *offset);
            }
            otherwise return; // Partial input, skip it.
        }
        of(SmolRTSP_ParseResult_Failure, e) {
            // TODO: handler properly.
            fputs("Failed to parse the request: ", stderr);
            const int err_bytes =
                SmolRTSP_ParseError_print(*e, smolrtsp_file_writer(stderr));
            assert(err_bytes >= 0);
            fputs(".\n", stderr);

            evbuffer_drain(input, buf.len);
            return;
        }
    }
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
