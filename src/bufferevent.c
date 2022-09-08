#include <smolrtsp-libevent/bufferevent.h>

#include <event2/buffer.h>

#include <assert.h>
#include <interface99.h>

typedef struct bufferevent BuffereventWriter;

static ssize_t BuffereventWriter_write(VSelf, CharSlice99 data) {
    VSELF(BuffereventWriter);
    assert(self);
    bufferevent_write(self, data.ptr, data.len);
    return data.len;
}

static void BuffereventWriter_lock(VSelf) {
    VSELF(BuffereventWriter);
    bufferevent_lock(self);
}

static void BuffereventWriter_unlock(VSelf) {
    VSELF(BuffereventWriter);
    bufferevent_unlock(self);
}

static size_t BuffereventWriter_filled(VSelf) {
    VSELF(BuffereventWriter);

    struct evbuffer *output = bufferevent_get_output(self);
    return evbuffer_get_length(output);
}

static int
BuffereventWriter_vwritef(VSelf, const char *restrict fmt, va_list ap) {
    VSELF(BuffereventWriter);

    assert(self);
    assert(fmt);

    struct evbuffer *evb = bufferevent_get_output(self);
    assert(evb);
    return evbuffer_add_vprintf(evb, fmt, ap);
}

static int BuffereventWriter_writef(VSelf, const char *restrict fmt, ...) {
    VSELF(BuffereventWriter);

    assert(self);
    assert(fmt);

    va_list ap;
    va_start(ap, fmt);
    const int result = BuffereventWriter_vwritef(self, fmt, ap);
    va_end(ap);

    return result;
}

impl(SmolRTSP_Writer, BuffereventWriter);

SmolRTSP_Writer smolrtsp_bufferevent_writer(struct bufferevent *bev) {
    assert(bev);

    return DYN(BuffereventWriter, SmolRTSP_Writer, bev);
}
