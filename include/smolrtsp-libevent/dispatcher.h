#pragma once

#include <smolrtsp/controller.h>

#include <event2/bufferevent.h>
#include <slice99.h>
#include <stdint.h>

/**
 * The most bytes of one RTSP request #smolrtsp_libevent_cb buffers before it
 * gives up on the connection.
 *
 * A request that never completes -- a header with no CRLF, a URI with no
 * space -- would otherwise be kept whole for as long as the peer keeps
 * sending, and parsed again from its first byte on every read, so one
 * connection could starve the event loop and then exhaust memory. No real
 * RTSP request comes near this: headers are capped at
 * #SMOLRTSP_MAX_HEADER_VALUE and bodies at #SMOLRTSP_MAX_CONTENT_LENGTH.
 * Interleaved frames are drained before this is checked and do not count.
 *
 * Define this macro when compiling the library to override it.
 */
#ifndef SMOLRTSP_LIBEVENT_MAX_REQUEST
#define SMOLRTSP_LIBEVENT_MAX_REQUEST (128 * 1024)
#endif

/**
 * The read callback that parses requests and dispatches them to the
 * controller.
 *
 * A single request that cannot complete within #SMOLRTSP_LIBEVENT_MAX_REQUEST
 * bytes is dropped, reading is disabled, and the bufferevent's event callback
 * is invoked with `BEV_EVENT_READING | BEV_EVENT_ERROR`, so the integrator
 * tears the connection down the same way as for any other read error. With no
 * event callback registered, the bufferevent is freed instead. Either way it
 * must not be used after this callback returns in that case. A complete
 * request followed by more data -- a pipelined one -- is dispatched as usual.
 */
void smolrtsp_libevent_cb(struct bufferevent *bev, void *ctx);

void *smolrtsp_libevent_ctx(SmolRTSP_Controller controller);
SmolRTSP_Controller smolrtsp_libevent_ctx_controller(void *ctx);
void smolrtsp_libevent_ctx_free(void *ctx);

/**
 * Handler for inbound TCP-interleaved binary frames (RTSP back channel /
 * ONVIF Profile T).
 *
 * Called from #smolrtsp_libevent_cb when a `$<channel><len><payload>`
 * frame is parsed off the receive buffer. The @p payload slice is valid
 * only for the duration of the callback — copy out anything you need
 * to keep.
 *
 * @param[in] channel_id The interleaved channel identifier the frame
 *            arrived on. Match against the per-stream SETUP-time channel
 *            assignment to dispatch to the right stream.
 * @param[in] payload The raw RTP/RTCP payload (no `$`/channel/length
 *            prefix).
 * @param[in] user_ctx The user pointer registered via
 *            #smolrtsp_libevent_ctx_with_interleaved.
 */
typedef void (*SmolRTSP_InterleavedHandler)(
    uint8_t channel_id, U8Slice99 payload, void *user_ctx);

/**
 * Variant of #smolrtsp_libevent_ctx that also registers a callback for
 * inbound TCP-interleaved binary frames.
 *
 * When @p on_frame is non-NULL, frames arriving at the head of the
 * receive buffer are dispatched to it before any RTSP request parsing
 * is attempted. When NULL (or when the default constructor is used),
 * incoming frames are silently dropped — matching the pre-existing
 * behaviour for servers that have no back channel.
 *
 * @param[in] controller The RTSP request controller (same as the default
 *            constructor).
 * @param[in] on_frame Interleaved-frame callback, or NULL.
 * @param[in] user_ctx Opaque pointer forwarded to @p on_frame.
 */
void *smolrtsp_libevent_ctx_with_interleaved(
    SmolRTSP_Controller controller, SmolRTSP_InterleavedHandler on_frame,
    void *user_ctx);
