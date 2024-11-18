#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "wavo/config.h"

bool wavo_config_load_default(struct wavo_config *config) {
    if (!config) return false;
    
    // Set default values
    config->terminal = strdup("alacritty");
    if (!config->terminal) goto error;
    
    config->mod_key = strdup("Mod4");
    if (!config->mod_key) goto error;
    
    config->menu = strdup("rofi -show drun");
    if (!config->menu) goto error;
    
    config->enable_animations = true;
    config->workspace_count = 9;
    
    config->repeat_rate = 25;
    config->repeat_delay = 600;
    
    config->background_color = strdup("#000000");
    if (!config->background_color) goto error;
    
    config->border_width = 2;
    config->border_color = strdup("#333333");
    if (!config->border_color) goto error;
    
    return true;

error:
    wavo_config_free(config);
    return false;
}

bool wavo_config_load_file(struct wavo_config *config, const char *path) {
    (void)path; // Suppress unused parameter warning
    // TODO: Implement Lua configuration loading
    return wavo_config_load_default(config);
}

bool wavo_config_validate(struct wavo_config *config) {
    if (!config) return false;
    if (!config->terminal) return false;
    if (!config->mod_key) return false;
    if (!config->menu) return false;
    
    return true;
}

void wavo_config_free(struct wavo_config *config) {
    if (!config) return;
    
    free(config->terminal);
    free(config->mod_key);
    free(config->menu);
    free(config->background_color);
    free(config->border_color);
    
    // Reset all pointers to NULL
    memset(config, 0, sizeof(*config));
}
