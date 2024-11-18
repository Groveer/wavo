#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/edges.h>
#include <wlr/util/log.h>
#include <linux/input-event-codes.h>
#include "wavo/view.h"
#include "wavo/server.h"
#include "wavo/input.h"

struct wavo_drag_grab {
    struct wavo_view *view;
    double x, y;
    struct wlr_box geometry;
    uint32_t resize_edges;
};

static void process_cursor_move(struct wavo_server *server, uint32_t time_msec) {
    (void)time_msec;
    struct wavo_drag_grab *grab = server->input->grab_data;
    struct wavo_view *view = grab->view;

    double dx = server->input->cursor->x - grab->x;
    double dy = server->input->cursor->y - grab->y;

    struct wlr_scene_node *node = &view->scene_tree->node;
    node->x = node->x + dx;
    node->y = node->y + dy;

    grab->x = server->input->cursor->x;
    grab->y = server->input->cursor->y;
}

static void process_cursor_resize(struct wavo_server *server, uint32_t time_msec) {
    (void)time_msec;
    struct wavo_drag_grab *grab = server->input->grab_data;
    struct wavo_view *view = grab->view;
    struct wlr_xdg_surface *surface = view->xdg_surface;

    double dx = server->input->cursor->x - grab->x;
    double dy = server->input->cursor->y - grab->y;

    struct wlr_box new_geo = grab->geometry;

    if (grab->resize_edges & WLR_EDGE_TOP) {
        new_geo.y = grab->geometry.y + dy;
        new_geo.height = grab->geometry.height - dy;
    } else if (grab->resize_edges & WLR_EDGE_BOTTOM) {
        new_geo.height = grab->geometry.height + dy;
    }
    if (grab->resize_edges & WLR_EDGE_LEFT) {
        new_geo.x = grab->geometry.x + dx;
        new_geo.width = grab->geometry.width - dx;
    } else if (grab->resize_edges & WLR_EDGE_RIGHT) {
        new_geo.width = grab->geometry.width + dx;
    }

    // Ensure minimum size
    new_geo.width = fmax(new_geo.width, 50);
    new_geo.height = fmax(new_geo.height, 50);

    // Apply the new geometry
    wlr_xdg_toplevel_set_size(surface->toplevel, new_geo.width, new_geo.height);
}

static void handle_pointer_button(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, cursor_button);
    struct wlr_pointer_button_event *event = data;

    if (event->state == WLR_BUTTON_RELEASED && input->grab_data) {
        free(input->grab_data);
        input->grab_data = NULL;
        return;
    }

    // Only handle button press
    if (event->state != WLR_BUTTON_PRESSED) {
        return;
    }

    // Focus the view under the cursor
    double sx, sy;
    struct wlr_scene_node *node = wlr_scene_node_at(
        &input->server->scene->tree.node,
        input->cursor->x, input->cursor->y, &sx, &sy);
    
    if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
        return;
    }

    struct wavo_view *view = wavo_view_from_node(node);
    if (!view) {
        return;
    }

    wavo_view_activate(view, true);

    if (event->button == BTN_LEFT) {
        struct wavo_drag_grab *grab = calloc(1, sizeof(struct wavo_drag_grab));
        grab->view = view;
        grab->x = input->cursor->x;
        grab->y = input->cursor->y;
        input->grab_data = grab;
    }
}

static void view_map(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, map);
    struct wlr_xdg_surface *xdg_surface = view->xdg_surface;

    if (!xdg_surface->toplevel) {
        wlr_log(WLR_ERROR, "XDG surface has no toplevel");
        return;
    }

    view->scene_tree = wlr_scene_xdg_surface_create(
        view->server->view_tree, view->xdg_surface);
    if (!view->scene_tree) {
        wlr_log(WLR_ERROR, "Failed to create scene tree");
        return;
    }

    view->scene_tree->node.data = view;
    wl_list_insert(&view->server->views, &view->link);
}

static void view_unmap(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, unmap);
    wl_list_remove(&view->link);
}

static void view_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->link);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);
    wl_list_remove(&view->request_maximize.link);
    wl_list_remove(&view->request_fullscreen.link);
    free(view);
}

static void view_request_move(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, request_move);
    struct wavo_server *server = view->server;
    struct wavo_input *input = server->input;

    if (input->grab_data) {
        return;
    }

    struct wavo_drag_grab *grab = calloc(1, sizeof(struct wavo_drag_grab));
    grab->view = view;
    grab->x = input->cursor->x;
    grab->y = input->cursor->y;
    input->grab_data = grab;
}

static void view_request_resize(struct wl_listener *listener, void *data) {
    struct wavo_view *view = wl_container_of(listener, view, request_resize);
    struct wlr_xdg_toplevel_resize_event *event = data;
    struct wavo_server *server = view->server;
    struct wavo_input *input = server->input;

    if (input->grab_data) {
        return;
    }

    struct wavo_drag_grab *grab = calloc(1, sizeof(struct wavo_drag_grab));
    grab->view = view;
    grab->x = input->cursor->x;
    grab->y = input->cursor->y;
    grab->resize_edges = event->edges;

    // Store the current geometry
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo);
    grab->geometry = geo;

    input->grab_data = grab;
}

static void view_request_maximize(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, request_maximize);
    wlr_xdg_surface_schedule_configure(view->xdg_surface);
}

static void view_request_fullscreen(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_view *view = wl_container_of(listener, view, request_fullscreen);
    wlr_xdg_surface_schedule_configure(view->xdg_surface);
}

struct wavo_view *wavo_view_create(struct wavo_server *server,
    struct wlr_xdg_surface *xdg_surface) {
    struct wavo_view *view = calloc(1, sizeof(struct wavo_view));
    if (!view) {
        wlr_log(WLR_ERROR, "Failed to allocate view");
        return NULL;
    }

    view->server = server;
    view->xdg_surface = xdg_surface;
    xdg_surface->data = view;

    view->map.notify = view_map;
    view->unmap.notify = view_unmap;
    view->destroy.notify = view_destroy;
    view->request_move.notify = view_request_move;
    view->request_resize.notify = view_request_resize;
    view->request_maximize.notify = view_request_maximize;
    view->request_fullscreen.notify = view_request_fullscreen;

    wl_signal_add(&xdg_surface->events.map, &view->map);
    wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);
    wl_signal_add(&xdg_surface->toplevel->events.request_move,
        &view->request_move);
    wl_signal_add(&xdg_surface->toplevel->events.request_resize,
        &view->request_resize);
    wl_signal_add(&xdg_surface->toplevel->events.request_maximize,
        &view->request_maximize);
    wl_signal_add(&xdg_surface->toplevel->events.request_fullscreen,
        &view->request_fullscreen);

    return view;
}

void wavo_view_destroy(struct wavo_view *view) {
    if (!view) {
        return;
    }

    if (view->scene_tree) {
        wlr_scene_node_destroy(&view->scene_tree->node);
    }

    free(view);
}

void wavo_view_activate(struct wavo_view *view, bool activate) {
    if (!view->xdg_surface->toplevel) {
        return;
    }

    wlr_xdg_toplevel_set_activated(view->xdg_surface->toplevel, activate);
}

struct wavo_view *wavo_view_from_node(struct wlr_scene_node *node) {
    if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }

    struct wlr_scene_buffer *buffer = wlr_scene_buffer_from_node(node);
    if (!buffer) {
        return NULL;
    }

    struct wlr_scene_tree *tree = buffer->node.parent;
    if (!tree) {
        return NULL;
    }

    return tree->node.data;
}
