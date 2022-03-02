#pragma once

#include <event2/bufferevent.h>
#include <smolrtsp.h>

SmolRTSP_Writer smolrtsp_bufferevent_writer(struct bufferevent *bev);
