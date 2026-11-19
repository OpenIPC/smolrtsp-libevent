#include <smolrtsp-libevent/dispatcher.h>

#include <smolrtsp-libevent/bufferevent.h>
#include <smolrtsp-libevent/evbuffer.h>

#include <event2/event.h>

#include <smolrtsp/context.h>
#include <smolrtsp/types/request.h>
#include <smolrtsp/types/response.h>
#include <smolrtsp/types/status_code.h>
#include <smolrtsp/util.h>
#include <smolrtsp/writer.h>

#include <assert.h>
#include <stdlib.h>

#include <datatype99.h>
#include <slice99.h>

typedef struct {
    SmolRTSP_Controller controller;
    SmolRTSP_InterleavedHandler on_frame;
    void *on_frame_ctx;
} DispatchCtx;

/* More than SMOLRTSP_LIBEVENT_MAX_REQUEST bytes and still no complete request.
 * Drop the bytes, stop reading, and hand the connection to the integrator's
 * event callback as a read error -- the one path every integrator already has
 * for tearing a connection down.
 *
 * No 413 is sent. A socket bufferevent's output cannot be flushed from inside
 * a callback, and the event callback typically frees the bufferevent, which
 * discards anything still queued; the peer that gets here is broken or
 * hostile, and a closed connection is answer enough. */
static void
refuse_oversized_request(struct bufferevent *bev, struct evbuffer *input) {
    evbuffer_drain(input, evbuffer_get_length(input));
    bufferevent_disable(bev, EV_READ);

    bufferevent_event_cb eventcb = NULL;
    void *cbarg = NULL;
    bufferevent_getcb(bev, NULL, NULL, &eventcb, &cbarg);
    if (eventcb) {
        eventcb(bev, BEV_EVENT_READING | BEV_EVENT_ERROR, cbarg);
    } else {
        /* Nobody to hand it to: a read-disabled connection would otherwise
         * stay open for good. */
        bufferevent_free(bev);
    }
}

void smolrtsp_libevent_cb(struct bufferevent *bev, void *arg) {
    assert(bev);
    assert(arg);

    DispatchCtx *ctx = arg;

    struct evbuffer *input = bufferevent_get_input(bev);
    const SmolRTSP_Writer conn = smolrtsp_bufferevent_writer(bev);

    /* Drain any TCP-interleaved binary frames at the head of the buffer
     * before attempting to parse an RTSP request. Frames are dispatched
     * to the registered handler (if any); silently dropped otherwise —
     * matching the pre-existing behaviour where SmolRTSP_Request_parse
     * skipped over them. */
    for (;;) {
        const CharSlice99 head = smolrtsp_evbuffer_slice(input);
        uint8_t channel_id = 0;
        U8Slice99 payload = U8Slice99_empty();
        size_t consumed = 0;

        const SmolRTSP_InterleavedFrameStatus status =
            smolrtsp_parse_interleaved_frame(
                head, &channel_id, &payload, &consumed);

        if (SmolRTSP_InterleavedFrameStatus_Complete == status) {
            if (ctx->on_frame) {
                ctx->on_frame(channel_id, payload, ctx->on_frame_ctx);
            }
            evbuffer_drain(input, consumed);
            continue;
        }
        if (SmolRTSP_InterleavedFrameStatus_Partial == status) {
            return; // wait for more bytes
        }
        break; // NotInterleaved — fall through to RTSP parse
    }

    /* At most SMOLRTSP_LIBEVENT_MAX_REQUEST bytes are parsed, so the work per
     * read stays bounded however much has arrived. A request that completes
     * within them is dispatched even when more -- a pipelined request --
     * follows; one that is still Partial there can only be larger, and is
     * refused. */
    const size_t buffered = evbuffer_get_length(input);
    const size_t window = buffered < SMOLRTSP_LIBEVENT_MAX_REQUEST
                              ? buffered
                              : SMOLRTSP_LIBEVENT_MAX_REQUEST;
    const CharSlice99 buf =
        CharSlice99_new((char *)evbuffer_pullup(input, window), window);

    SmolRTSP_Request req = SmolRTSP_Request_uninit();
    const SmolRTSP_ParseResult res = SmolRTSP_Request_parse(&req, buf);

    match(res) {
        of(SmolRTSP_ParseResult_Success, status) match(*status) {
            of(SmolRTSP_ParseStatus_Complete, offset) {
                smolrtsp_dispatch(conn, ctx->controller, &req);
                evbuffer_drain(input, *offset);
            }
            otherwise {
                if (buffered >= SMOLRTSP_LIBEVENT_MAX_REQUEST) {
                    refuse_oversized_request(bev, input);
                }
                return; // Partial input, wait for more.
            }
        }
        of(SmolRTSP_ParseResult_Failure, e) {
            fputs("Failed to parse the request: ", stderr);
            const int err_bytes =
                SmolRTSP_ParseError_print(*e, smolrtsp_file_writer(stderr));
            assert(err_bytes >= 0);
            fputs(".\n", stderr);

            /* Reply with `400 Bad Request` so a peer that sent a malformed
             * request fails fast instead of hanging on a silently dropped
             * connection. `cseq` is 0 here — the request did not parse. */
            const SmolRTSP_Response resp = {
                .start_line =
                    {
                        .version = {1, 0},
                        .code = SMOLRTSP_STATUS_BAD_REQUEST,
                        .reason = CharSlice99_from_str("Bad Request"),
                    },
                .header_map = SmolRTSP_HeaderMap_empty(),
                .body = CharSlice99_empty(),
                .cseq = req.cseq,
            };
            const ssize_t written = SmolRTSP_Response_serialize(&resp, conn);
            (void)written;

            evbuffer_drain(input, buffered);
            return;
        }
    }
}

void *smolrtsp_libevent_ctx(SmolRTSP_Controller controller) {
    return smolrtsp_libevent_ctx_with_interleaved(controller, NULL, NULL);
}

void *smolrtsp_libevent_ctx_with_interleaved(
    SmolRTSP_Controller controller, SmolRTSP_InterleavedHandler on_frame,
    void *user_ctx) {
    assert(controller.self && controller.vptr);

    DispatchCtx *self = malloc(sizeof *self);
    assert(self);
    self->controller = controller;
    self->on_frame = on_frame;
    self->on_frame_ctx = user_ctx;
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
