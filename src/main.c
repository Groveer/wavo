#include <stdio.h>
#include <wayland-server-core.h>
#include "wavo/server.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    struct wavo_server *server = wavo_server_create();
    if (!server) {
        fprintf(stderr, "Failed to create wavo server\n");
        return 1;
    }
    
    printf("Running wavo compositor...\n");
    wl_display_run(server->wl_display);
    
    wavo_server_destroy(server);
    return 0;
}
