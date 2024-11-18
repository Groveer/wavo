-- Default configuration for wavo compositor

-- Basic settings
config = {
    mod_key = "Mod4",  -- Windows key
    terminal = "alacritty",
    menu = "rofi -show drun",
}

-- Key bindings
keys = {
    -- Terminal
    {mod = {"Mod4"}, key = "Return", cmd = "spawn", value = config.terminal},
    -- Menu
    {mod = {"Mod4"}, key = "d", cmd = "spawn", value = config.menu},
    -- Close window
    {mod = {"Mod4"}, key = "q", cmd = "close_window"},
    -- Quit compositor
    {mod = {"Mod4", "Shift"}, key = "e", cmd = "quit"},
}

-- Window rules
window_rules = {
    {
        match = {
            class = "Firefox",
        },
        properties = {
            workspace = 2,
            floating = false,
        }
    },
}

-- Workspaces
workspaces = {
    count = 9,
    names = {"1", "2", "3", "4", "5", "6", "7", "8", "9"},
}
