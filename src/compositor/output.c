#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <time.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/util/log.h>
#include "wavo/output.h"
#include "wavo/server.h"

static void output_frame(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene_output *scene_output = output->scene_output;
    struct timespec now;

    // Get the current time for presentation feedback
    clock_gettime(CLOCK_MONOTONIC, &now);

    // Render the scene
    if (!wlr_scene_output_commit(scene_output, NULL)) {
        wlr_log(WLR_ERROR, "%s", "Failed to commit scene output");
        return;
    }

    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_destroy(struct wl_listener *listener, void *data) {
    (void)data;
    struct wavo_output *output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}

struct wavo_output *wavo_output_create(struct wavo_server *server,
    struct wlr_output *wlr_output) {
    struct wavo_output *output = calloc(1, sizeof(struct wavo_output));
    if (!output) {
        wlr_log(WLR_ERROR, "%s", "Failed to allocate wavo_output");
        return NULL;
    }

    output->server = server;
    output->wlr_output = wlr_output;

    // Setup output mode
    if (!wlr_output_init_render(wlr_output, server->allocator, server->renderer)) {
        wlr_log(WLR_ERROR, "%s", "Failed to initialize output render");
        free(output);
        return NULL;
    }

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        struct wlr_output_state state;
        wlr_output_state_init(&state);
        wlr_output_state_set_enabled(&state, true);
        wlr_output_state_set_mode(&state, mode);
        if (!wlr_output_commit_state(wlr_output, &state)) {
            wlr_log(WLR_ERROR, "%s", "Failed to commit output mode");
            wlr_output_state_finish(&state);
            free(output);
            return NULL;
        }
        wlr_output_state_finish(&state);
    }

    // Create scene output
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
    if (!output->scene_output) {
        wlr_log(WLR_ERROR, "%s", "Failed to create scene output");
        free(output);
        return NULL;
    }

    // Add it to the output layout
    wlr_output_layout_add_auto(server->output_layout, wlr_output);

    // Setup listeners
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    // Add to outputs list
    wl_list_insert(&server->outputs, &output->link);

    // Enable the output
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    if (!wlr_output_commit_state(wlr_output, &state)) {
        wlr_log(WLR_ERROR, "%s", "Failed to commit output");
        wlr_output_state_finish(&state);
        wavo_output_destroy(output);
        return NULL;
    }
    wlr_output_state_finish(&state);

    return output;
}

void wavo_output_destroy(struct wavo_output *output) {
    if (!output) return;
    wlr_scene_output_destroy(output->scene_output);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}
