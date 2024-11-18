#ifndef WAVO_INPUT_H
#define WAVO_INPUT_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>

struct wavo_input {
    struct wavo_server *server;
    struct wlr_seat *seat;
    
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    
    struct wl_list keyboards;  // wavo_keyboard::link
    struct wl_list pointers;   // wavo_pointer::link
    
    void *grab_data;  // For interactive move/resize
    
    struct wl_listener new_input;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener request_cursor;
};

struct wavo_keyboard {
    struct wavo_input *input;
    struct wlr_input_device *device;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_list link;

    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
};

struct wavo_pointer {
    struct wavo_input *input;
    struct wlr_input_device *device;
    struct wlr_pointer *wlr_pointer;
    struct wl_list link;
    
    struct wl_listener destroy;
};

struct wavo_input *wavo_input_create(struct wavo_server *server);
void wavo_input_destroy(struct wavo_input *input);

#endif // WAVO_INPUT_H
