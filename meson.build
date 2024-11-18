project('wavo', 'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>=0.59.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ],
)

# Add unstable wlroots feature flag
add_project_arguments(
  '-DWLR_USE_UNSTABLE',
  language: 'c',
)

# Include directories
inc = include_directories('include')
proto_inc = include_directories('.')

# Dependencies
wlroots = dependency('wlroots-0.18')
wayland_server = dependency('wayland-server')
wayland_protos = dependency('wayland-protocols')
lua = dependency('lua-5.4')
xkbcommon = dependency('xkbcommon')
pixman = dependency('pixman-1')
criterion = dependency('criterion', required: false)

# Protocol generation
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')

# Generate the header from the protocol xml file
xdg_shell_xml = join_paths(wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml')
wl_protos_src = []
wl_protos_headers = []

wl_protos_headers += custom_target(
  'xdg-shell-protocol.h',
  input: xdg_shell_xml,
  output: '@BASENAME@-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

wl_protos_src += custom_target(
  'xdg-shell-protocol.c',
  input: xdg_shell_xml,
  output: '@BASENAME@-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

# Subprojects
subdir('src')

# Tests
if criterion.found()
  subdir('tests')
endif
