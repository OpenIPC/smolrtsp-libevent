#pragma once

#include <event2/buffer.h>
#include <smolrtsp/writer.h>

SmolRTSP_Writer smolrtsp_evbuffer_writer(struct evbuffer *evb);
