#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/log.h>
#include "wavo/input.h"
#include "wavo/server.h"

// Forward declarations
static void process_cursor_move(struct wavo_server *server, uint32_t time_msec);
static void process_cursor_resize(struct wavo_server *server, uint32_t time_msec);

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    (void)data;  // Unused parameter
    struct wavo_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    struct wlr_keyboard *wlr_keyboard = keyboard->wlr_keyboard;

    wlr_seat_keyboard_notify_modifiers(keyboard->input->seat,
        &wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    struct wavo_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct wlr_keyboard_key_event *event = data;

    wlr_seat_keyboard_notify_key(keyboard->input->seat, event->time_msec,
        event->keycode, event->state);
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
    (void)data;  // Unused parameter
    struct wavo_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);
    free(keyboard);
}

static void pointer_handle_destroy(struct wl_listener *listener, void *data) {
    (void)data;  // Unused parameter
    struct wavo_pointer *pointer = wl_container_of(listener, pointer, destroy);

    wl_list_remove(&pointer->destroy.link);
    wl_list_remove(&pointer->link);
    free(pointer);
}

static void handle_cursor_motion(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, cursor_motion);
    struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(input->cursor, &event->pointer->base, event->delta_x, event->delta_y);

    if (input->grab_data) {
        process_cursor_move(input->server, event->time_msec);
        return;
    }

    wlr_seat_pointer_notify_motion(input->seat, event->time_msec, 
        input->cursor->x, input->cursor->y);
}

static void handle_cursor_motion_absolute(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;

    wlr_cursor_warp_absolute(input->cursor, &event->pointer->base, event->x, event->y);

    if (input->grab_data) {
        process_cursor_move(input->server, event->time_msec);
        return;
    }

    wlr_seat_pointer_notify_motion(input->seat, event->time_msec,
        input->cursor->x, input->cursor->y);
}

static void handle_cursor_button(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, cursor_button);
    struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(input->seat, event->time_msec,
        event->button, event->state);
}

static void handle_cursor_axis(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, cursor_axis);
    struct wlr_pointer_axis_event *event = data;

    wlr_seat_pointer_notify_axis(input->seat, event->time_msec,
        event->orientation, event->delta, event->delta_discrete, event->source,
        event->relative_direction);
}

static void handle_cursor_frame(struct wl_listener *listener, void *data) {
    (void)data;  // Unused parameter
    struct wavo_input *input = wl_container_of(listener, input, cursor_frame);
    wlr_seat_pointer_notify_frame(input->seat);
}

static void handle_new_input(struct wl_listener *listener, void *data) {
    struct wavo_input *input = wl_container_of(listener, input, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
        struct wavo_keyboard *keyboard = calloc(1, sizeof(struct wavo_keyboard));
        if (!keyboard) {
            wlr_log(WLR_ERROR, "Failed to allocate keyboard: %s", "Out of memory");
            return;
        }

        keyboard->input = input;
        keyboard->device = device;
        keyboard->wlr_keyboard = wlr_keyboard_from_input_device(device);

        keyboard->modifiers.notify = keyboard_handle_modifiers;
        wl_signal_add(&keyboard->wlr_keyboard->events.modifiers,
            &keyboard->modifiers);

        keyboard->key.notify = keyboard_handle_key;
        wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

        keyboard->destroy.notify = keyboard_handle_destroy;
        wl_signal_add(&device->events.destroy, &keyboard->destroy);

        wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, 25, 600);

        wl_list_insert(&input->keyboards, &keyboard->link);

        wlr_seat_set_keyboard(input->seat, keyboard->wlr_keyboard);
        break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
        struct wavo_pointer *pointer = calloc(1, sizeof(struct wavo_pointer));
        if (!pointer) {
            wlr_log(WLR_ERROR, "Failed to allocate pointer: %s", "Out of memory");
            return;
        }

        pointer->input = input;
        pointer->device = device;
        pointer->wlr_pointer = wlr_pointer_from_input_device(device);

        pointer->destroy.notify = pointer_handle_destroy;
        wl_signal_add(&device->events.destroy, &pointer->destroy);

        wl_list_insert(&input->pointers, &pointer->link);

        wlr_cursor_attach_input_device(input->cursor, device);
        break;
    }
    default:
        wlr_log(WLR_INFO, "Unsupported input device type: %d", device->type);
        break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&input->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(input->seat, caps);
}

struct wavo_input *wavo_input_create(struct wavo_server *server) {
    struct wavo_input *input = calloc(1, sizeof(struct wavo_input));
    if (!input) {
        wlr_log(WLR_ERROR, "Failed to allocate input: %s", "Out of memory");
        return NULL;
    }

    input->server = server;
    input->seat = wlr_seat_create(server->wl_display, "seat0");
    if (!input->seat) {
        wlr_log(WLR_ERROR, "Failed to create seat: %s", "wlr_seat_create failed");
        free(input);
        return NULL;
    }

    input->cursor = wlr_cursor_create();
    if (!input->cursor) {
        wlr_log(WLR_ERROR, "Failed to create cursor: %s", "wlr_cursor_create failed");
        wlr_seat_destroy(input->seat);
        free(input);
        return NULL;
    }

    wlr_cursor_attach_output_layout(input->cursor, server->output_layout);

    input->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    if (!input->cursor_mgr) {
        wlr_log(WLR_ERROR, "Failed to create xcursor manager: %s", "wlr_xcursor_manager_create failed");
        wlr_cursor_destroy(input->cursor);
        wlr_seat_destroy(input->seat);
        free(input);
        return NULL;
    }

    wl_list_init(&input->keyboards);
    wl_list_init(&input->pointers);

    input->new_input.notify = handle_new_input;
    wl_signal_add(&server->backend->events.new_input, &input->new_input);

    input->cursor_motion.notify = handle_cursor_motion;
    wl_signal_add(&input->cursor->events.motion, &input->cursor_motion);

    input->cursor_motion_absolute.notify = handle_cursor_motion_absolute;
    wl_signal_add(&input->cursor->events.motion_absolute,
        &input->cursor_motion_absolute);

    input->cursor_button.notify = handle_cursor_button;
    wl_signal_add(&input->cursor->events.button, &input->cursor_button);

    input->cursor_axis.notify = handle_cursor_axis;
    wl_signal_add(&input->cursor->events.axis, &input->cursor_axis);

    input->cursor_frame.notify = handle_cursor_frame;
    wl_signal_add(&input->cursor->events.frame, &input->cursor_frame);

    return input;
}

void wavo_input_destroy(struct wavo_input *input) {
    if (!input) {
        return;
    }

    wl_list_remove(&input->new_input.link);
    wl_list_remove(&input->cursor_motion.link);
    wl_list_remove(&input->cursor_motion_absolute.link);
    wl_list_remove(&input->cursor_button.link);
    wl_list_remove(&input->cursor_axis.link);
    wl_list_remove(&input->cursor_frame.link);

    wlr_xcursor_manager_destroy(input->cursor_mgr);
    wlr_cursor_destroy(input->cursor);
    wlr_seat_destroy(input->seat);
    free(input);
}
