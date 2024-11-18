# Wavo - A Wayland Compositor

Wavo is a lightweight Wayland compositor built with wlroots and configured with Lua.

## Dependencies

- wlroots
- wayland
- lua 5.4
- xkbcommon
- pixman

## Building

```bash
# Create build directory
mkdir build
cd build

# Configure with meson
meson setup ..

# Build
ninja

# Install
sudo ninja install
```

## Configuration

The default configuration file is located at `~/.config/wavo/config.lua`. You can copy the example configuration from `conf/wavo.lua` to get started:

```bash
mkdir -p ~/.config/wavo
cp conf/wavo.lua ~/.config/wavo/config.lua
```

## Running

To run Wavo:

```bash
wavo
```

## License

MIT License
