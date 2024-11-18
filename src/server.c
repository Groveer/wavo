#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>
#include <wlr/types/wlr_output_management_v1.h>
#include "wavo/input.h"
#include "wavo/output.h"
#include "wavo/server.h"
#include "wavo/view.h"

static void server_new_output(struct wl_listener *listener, void *data) {
    struct wavo_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wavo_output *output = wavo_output_create(server, wlr_output);
    if (!output) {
        wlr_log(WLR_ERROR, "%s", "Failed to create output");
        return;
    }

    // The output has been created successfully
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    if (!wlr_output_commit_state(wlr_output, &state)) {
        wlr_log(WLR_ERROR, "%s", "Failed to commit output");
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);
}

static void xdg_surface_map(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, map);
    view->mapped = true;
}

static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    free(view);
}

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct wavo_server *server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    wlr_log(WLR_DEBUG, "%s", "New toplevel xdg surface");

    struct wavo_view *view = wavo_view_create(server, xdg_surface);
    if (!view) {
        wlr_log(WLR_ERROR, "%s", "Failed to create view");
        return;
    }

    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->surface->events.map, &view->map);

    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);

    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);
}

struct wavo_server *wavo_server_create(void) {
    struct wavo_server *server = calloc(1, sizeof(struct wavo_server));
    if (!server) {
        wlr_log(WLR_ERROR, "%s", "Failed to allocate server");
        return NULL;
    }

    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "%s", "Failed to create wayland display");
        free(server);
        return NULL;
    }

    struct wl_event_loop *event_loop = wl_display_get_event_loop(server->wl_display);
    server->backend = wlr_backend_autocreate(event_loop, NULL);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "%s", "Failed to create wlr_backend");
        goto error_display;
    }

    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "%s", "Failed to create renderer");
        goto error_backend;
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    server->allocator = wlr_allocator_autocreate(server->backend,
        server->renderer);
    if (!server->allocator) {
        wlr_log(WLR_ERROR, "%s", "Failed to create allocator");
        goto error_renderer;
    }

    server->compositor = wlr_compositor_create(server->wl_display, 6,
        server->renderer);
    if (!server->compositor) {
        wlr_log(WLR_ERROR, "%s", "Failed to create compositor");
        goto error_allocator;
    }

    server->scene = wlr_scene_create();
    if (!server->scene) {
        wlr_log(WLR_ERROR, "%s", "Failed to create scene");
        goto error_compositor;
    }

    server->output_layout = wlr_output_layout_create(server->wl_display);
    if (!server->output_layout) {
        wlr_log(WLR_ERROR, "%s", "Failed to create output layout");
        goto error_scene;
    }

    wl_list_init(&server->outputs);

    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
    if (!server->xdg_shell) {
        wlr_log(WLR_ERROR, "%s", "Failed to create XDG shell");
        goto error_output_layout;
    }

    server->input = wavo_input_create(server);
    if (!server->input) {
        wlr_log(WLR_ERROR, "%s", "Failed to create input manager");
        goto error_xdg_shell;
    }

    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface,
        &server->new_xdg_surface);

    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_log(WLR_ERROR, "%s", "Failed to create wayland socket");
        goto error_input;
    }

    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "%s", "Failed to start backend");
        wl_display_destroy_clients(server->wl_display);
        goto error_input;
    }

    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Running compositor on wayland display '%s'", socket);

    return server;

error_input:
    wavo_input_destroy(server->input);
error_xdg_shell:
    wl_list_remove(&server->new_xdg_surface.link);
    wl_global_destroy(server->xdg_shell->global);
error_output_layout:
    wl_list_remove(&server->new_output.link);
    wlr_output_layout_destroy(server->output_layout);
error_scene:
    wlr_scene_node_destroy(&server->scene->tree.node);
error_compositor:
    wl_list_remove(&server->new_xdg_surface.link);
error_allocator:
    wlr_allocator_destroy(server->allocator);
error_renderer:
    wlr_renderer_destroy(server->renderer);
error_backend:
    wlr_backend_destroy(server->backend);
error_display:
    wl_display_destroy(server->wl_display);
    free(server);
    return NULL;
}

void wavo_server_destroy(struct wavo_server *server) {
    if (!server) {
        return;
    }

    wl_display_destroy_clients(server->wl_display);

    wavo_input_destroy(server->input);
    wl_list_remove(&server->new_xdg_surface.link);
    wl_global_destroy(server->xdg_shell->global);
    wl_list_remove(&server->new_output.link);
    wlr_output_layout_destroy(server->output_layout);
    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    free(server);
}
