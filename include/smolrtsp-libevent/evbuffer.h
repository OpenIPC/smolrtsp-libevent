#pragma once

#include <event2/buffer.h>
#include <smolrtsp.h>

SmolRTSP_Writer smolrtsp_evbuffer_writer(struct evbuffer *evb);
