#ifndef WAVO_OUTPUT_H
#define WAVO_OUTPUT_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include "wavo/server.h"

struct wavo_output {
    struct wavo_server *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    struct wl_list link;  // wavo_server::outputs

    struct wl_listener frame;
    struct wl_listener destroy;
};

struct wavo_output *wavo_output_create(struct wavo_server *server,
    struct wlr_output *wlr_output);
void wavo_output_destroy(struct wavo_output *output);

#endif // WAVO_OUTPUT_H
