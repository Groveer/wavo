#include <criterion/criterion.h>
#include "wavo/config.h"

static struct wavo_config config;

void setup(void) {
    memset(&config, 0, sizeof(config));
}

void teardown(void) {
    wavo_config_free(&config);
}

TestSuite(config, .init = setup, .fini = teardown);

Test(config, load_default) {
    cr_assert(wavo_config_load_default(&config));
    
    // Verify default values
    cr_assert_str_eq(config.terminal, "alacritty");
    cr_assert_str_eq(config.mod_key, "Mod4");
    cr_assert_str_eq(config.menu, "rofi -show drun");
    
    cr_assert(config.enable_animations);
    cr_assert_eq(config.workspace_count, 9);
    
    cr_assert_eq(config.repeat_rate, 25);
    cr_assert_eq(config.repeat_delay, 600);
    
    cr_assert_str_eq(config.background_color, "#000000");
    cr_assert_eq(config.border_width, 2);
    cr_assert_str_eq(config.border_color, "#333333");
}

Test(config, load_default_null) {
    cr_assert_not(wavo_config_load_default(NULL));
}

Test(config, validate) {
    // Empty config should fail validation
    cr_assert_not(wavo_config_validate(&config));
    
    // Load default config
    cr_assert(wavo_config_load_default(&config));
    
    // Default config should pass validation
    cr_assert(wavo_config_validate(&config));
    
    // Test validation with missing required fields
    char *old_terminal = config.terminal;
    config.terminal = NULL;
    cr_assert_not(wavo_config_validate(&config));
    config.terminal = old_terminal;
    
    char *old_mod_key = config.mod_key;
    config.mod_key = NULL;
    cr_assert_not(wavo_config_validate(&config));
    config.mod_key = old_mod_key;
    
    char *old_menu = config.menu;
    config.menu = NULL;
    cr_assert_not(wavo_config_validate(&config));
    config.menu = old_menu;
}

Test(config, validate_null) {
    cr_assert_not(wavo_config_validate(NULL));
}

Test(config, free_null) {
    // Should not crash when freeing NULL config
    wavo_config_free(NULL);
}

Test(config, free) {
    cr_assert(wavo_config_load_default(&config));
    wavo_config_free(&config);
    
    // All pointers should be NULL after free
    cr_assert_null(config.terminal);
    cr_assert_null(config.mod_key);
    cr_assert_null(config.menu);
    cr_assert_null(config.background_color);
    cr_assert_null(config.border_color);
    
    // All numeric values should be 0
    cr_assert_eq(config.workspace_count, 0);
    cr_assert_eq(config.repeat_rate, 0);
    cr_assert_eq(config.repeat_delay, 0);
    cr_assert_eq(config.border_width, 0);
    
    // All boolean values should be false
    cr_assert_not(config.enable_animations);
}
