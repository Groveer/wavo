#ifndef WAVO_VIEW_H
#define WAVO_VIEW_H

#include <stdbool.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include "wavo/server.h"

struct wavo_server;

struct wavo_view {
    struct wavo_server *server;
    struct wlr_xdg_surface *xdg_surface;
    struct wlr_scene_tree *scene_tree;
    struct wl_list link;  // wavo_server::views

    bool mapped;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener set_title;
    struct wl_listener set_app_id;
};

struct wavo_view *wavo_view_create(struct wavo_server *server,
    struct wlr_xdg_surface *xdg_surface);
void wavo_view_destroy(struct wavo_view *view);
void wavo_view_activate(struct wavo_view *view, bool activate);
void wavo_view_maximize(struct wavo_view *view, bool maximize);
void wavo_view_set_fullscreen(struct wavo_view *view, bool fullscreen);
struct wavo_view *wavo_view_from_node(struct wlr_scene_node *node);

#endif // WAVO_VIEW_H
