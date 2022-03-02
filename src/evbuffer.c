#include <smolrtsp-libevent/evbuffer.h>

#include <assert.h>
#include <interface99.h>

typedef struct evbuffer EvbufferWriter;

static ssize_t EvbufferWriter_write(VSelf, CharSlice99 data) {
    VSELF(EvbufferWriter);
    assert(self);
    evbuffer_add(self, data.ptr, data.len);
    return data.len;
}

static int EvbufferWriter_vwritef(VSelf, const char *restrict fmt, va_list ap) {
    VSELF(EvbufferWriter);

    assert(self);
    assert(fmt);

    return evbuffer_add_vprintf(self, fmt, ap);
}

static int EvbufferWriter_writef(VSelf, const char *restrict fmt, ...) {
    VSELF(EvbufferWriter);

    assert(self);
    assert(fmt);

    va_list ap;
    va_start(ap, fmt);

    const int result = evbuffer_add_vprintf(self, fmt, ap);
    va_end(ap);

    return result;
}

impl(SmolRTSP_Writer, EvbufferWriter);

SmolRTSP_Writer smolrtsp_evbuffer_writer(struct evbuffer *evb) {
    assert(evb);

    return DYN(EvbufferWriter, SmolRTSP_Writer, evb);
}
