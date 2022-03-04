#pragma once

#include <smolrtsp/controller.h>

#include <event2/bufferevent.h>

void smolrtsp_libevent_dispatch_cb(struct bufferevent *bev, void *ctx);

void *smolrtsp_libevent_ctx(SmolRTSP_Controller controller);
void smolrtsp_libevent_ctx_free(void *ctx);
