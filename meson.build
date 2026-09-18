project(
  'minihttpd',
  'c',
  default_options: [
    'warning_level=2',
    'c_std=c11',
  ],
)

add_project_arguments('-pedantic', language: 'c')

threads_dep = dependency('threads')
lib_inc = include_directories('lib')

minihttpd_lib = both_libraries(
  'minihttpd',
  sources: ['lib/minihttpd.c', 'lib/mh_platform.c'],
  include_directories: lib_inc,
  dependencies: threads_dep,
  soversion: '2',
  install: true,
)

install_headers('lib/minihttpd.h')

minihttpd_dep = declare_dependency(
  link_with: minihttpd_lib,
  include_directories: lib_inc,
  dependencies: threads_dep,
)

executable(
  'minihttpd',
  sources: ['main.c'],
  include_directories: [include_directories('.'), lib_inc],
  link_with: minihttpd_lib,
  dependencies: threads_dep,
  install: true,
)

