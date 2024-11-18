#ifndef WAVO_COMPOSITOR_VIEW_H
#define WAVO_COMPOSITOR_VIEW_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_shell.h>

struct wavo_server;

struct wavo_view {
    struct wavo_server *server;
    struct wlr_xdg_surface *xdg_surface;
    struct wl_list link;  // wavo_server::views

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;

    double x, y;
    bool mapped;
};

struct wavo_view *wavo_view_create(struct wavo_server *server,
    struct wlr_xdg_surface *xdg_surface);
void wavo_view_destroy(struct wavo_view *view);
void wavo_view_focus(struct wavo_view *view);

#endif // WAVO_COMPOSITOR_VIEW_H
