#ifndef WAVO_SERVER_H
#define WAVO_SERVER_H

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

struct wavo_input;  // Forward declaration

struct wavo_server {
    struct wl_display *wl_display;
    struct wl_event_loop *event_loop;
    
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_compositor *compositor;
    
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_output_layout *output_layout;
    
    struct wlr_scene *scene;           // Root scene tree
    struct wlr_scene_tree *view_tree;  // Tree for views
    
    struct wl_list outputs;  // wavo_output::link
    struct wl_list views;    // wavo_view::link
    
    struct wavo_input *input;  // Input device manager
    
    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
};

struct wavo_server *wavo_server_create(void);
void wavo_server_destroy(struct wavo_server *server);
bool wavo_server_start(struct wavo_server *server);

#endif // WAVO_SERVER_H
