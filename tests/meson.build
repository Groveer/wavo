test_src = files(
  'main.c',
  'unit/lua/test_config.c',
)

test_exe = executable('unit_tests',
  test_src,
  include_directories: [inc, proto_inc],
  dependencies: [
    criterion,
    wlroots,
    wayland_server,
    lua,
    xkbcommon,
    pixman,
  ],
  link_with: wavo_lib,
)

test('unit tests', test_exe)
