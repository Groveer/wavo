#ifndef WAVO_CONFIG_H
#define WAVO_CONFIG_H

#include <stdbool.h>

struct wavo_config {
    char *terminal;
    char *mod_key;
    char *menu;
    
    // Window management
    bool enable_animations;
    int workspace_count;
    
    // Input
    int repeat_rate;
    int repeat_delay;
    
    // Theme
    char *background_color;
    int border_width;
    char *border_color;
};

// Load the default configuration
bool wavo_config_load_default(struct wavo_config *config);

// Load configuration from a file
bool wavo_config_load_file(struct wavo_config *config, const char *path);

// Validate configuration
bool wavo_config_validate(struct wavo_config *config);

// Free configuration resources
void wavo_config_free(struct wavo_config *config);

#endif // WAVO_CONFIG_H
