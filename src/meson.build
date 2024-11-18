wavo_src = files(
  'server.c',
  'input.c',
  'lua/config.c',
  'input/keyboard.c',
  'input/pointer.c',
  'compositor/window.c',
  'compositor/output.c',
  'compositor/view.c',
)

# Build as a static library for reuse in tests
wavo_lib = static_library('wavo',
  wavo_src,
  wl_protos_headers,
  include_directories: [inc, proto_inc],
  dependencies: [
    wlroots,
    wayland_server,
    lua,
    xkbcommon,
    pixman,
  ],
)

# Main executable
executable('wavo',
  'main.c',
  wavo_src,
  include_directories: [inc, proto_inc],
  link_with: wavo_lib,
  dependencies: [
    wlroots,
    wayland_server,
  ],
  install: true,
)
