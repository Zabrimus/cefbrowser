project(
   'liblifthttp',
   'c', 'cpp',
   version: '05a1d42727d066bdf9880faafa61470276712b0b',
   license: 'Apache-2.0 license',
   default_options: ['cpp_std=c++17'],
   meson_version: '>=0.63.0',
)

#dep_curl = dependency('libcurl_dep')
dep_z = dependency('zlib')

liblifthttp_dependencies = [dep_z]
liblifthttp_compile_args = ['-O3']

inc = include_directories('inc')

src = files(
    'src/client_pool.cpp',
    'src/client.cpp',
    'src/escape.cpp',
    'src/executor.cpp',
    'src/header.cpp',
    'src/http.cpp',
    'src/init.cpp',
    'src/lift_status.cpp',
    'src/mime_field.cpp',
    'src/query_builder.cpp',
    'src/request.cpp',
    'src/resolve_host.cpp',
    'src/response.cpp'
)

liblifthttp_lib = static_library(
    'liblifthttp',
    src,
    include_directories : inc,
    dependencies: liblifthttp_dependencies,
    cpp_args: liblifthttp_compile_args,
    c_args: liblifthttp_compile_args,
)

liblifthttp_dep = declare_dependency(
    dependencies: liblifthttp_dependencies,
    include_directories: inc,
    link_with: liblifthttp_lib,
    compile_args: liblifthttp_compile_args,
)