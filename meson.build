project('cefbrowser', 'cpp', 'c',
  version : '0.1',
  default_options : ['warning_level=1', 'cpp_std=c++17', 'default_library=static', 'optimization=3' ],
  meson_version: '>=0.63.0')

add_global_arguments('-O3', language : 'cpp')

arch=get_option('arch')

cmake = import('cmake')
CXX = meson.get_compiler('cpp')

#
# Download and extract subprojects
#
run_command('./setup.sh', arch, meson.current_build_dir(), check: false)

#
# Build cef
#
cef_opt_var = cmake.subproject_options()

if arch == 'arm'
    cef_opt_var.add_cmake_defines({'PROJECT_ARCH': 'armhf'})
elif arch == 'arm64'
    cef_opt_var.add_cmake_defines({'PROJECT_ARCH': 'aarch64'})
endif

cef_opt_var.append_compile_args('cpp', '-Wno-unused-variable')
cef_opt_var.add_cmake_defines({'CMAKE_BUILD_TYPE': 'Debug'})
cef_proj = cmake.subproject('cef', options: cef_opt_var)
cef_lib = cef_proj.dependency('libcef_dll_wrapper')

#
# spdlog
#
spdlog_proj = subproject('spdlog')
spdlog_dep = spdlog_proj.get_variable('spdlog_dep', ['tests=disabled', 'compile_library=true'] )

#
# tiny-process-library
#
tiny_process_library_proj = subproject('tiny_process_library')
tiny_process_library_dep = tiny_process_library_proj.get_variable('tiny_process_library_dep')

#
# mINI
#
mini_proj = subproject('mini')
mini_dep = mini_proj.get_variable('mini_dep')

#
# sqlite3
#
sqlite3_proj = subproject('sqlite3')
sqlite3_dep = sqlite3_proj.get_variable('sqlite3_dep')

#
# ssl
#
dep_ssl = dependency('libssl')
dep_crypto = dependency('libcrypto')

#
# Build browser
#
shared_libs = ['cef', 'z']
deps = []
foreach lib_name : shared_libs
        deps += CXX.find_library(lib_name, dirs : [meson.project_source_root() + '/subprojects/cef/Release'])
endforeach

exe = executable('cefbrowser', 'main.cpp', 'mainapp.cpp', 'logger.cpp',
                 'browserclient.cpp',
                 'sharedmemory.cpp', 'requestresponse.cpp', 'keycodes.cpp',
                 'v8handler.cpp', 'tools.cpp', 'database.cpp',
                 'transcoderremoteclient.cpp', 'vdrremoteclient.cpp',
                 'httplib.cpp', 'xhrinterception.cpp',
                 cpp_args : ['-DPHTTPLIB_ZLIB_SUPPORT', '-DCPPHTTPLIB_OPENSSL_SUPPORT'],
                 install : true,
                 install_dir : meson.current_build_dir() + '/Release',
                 dependencies: [mini_dep, cef_lib, spdlog_dep, deps, sqlite3_dep, dep_ssl, dep_crypto])

exe = executable('vdrclient', 'test-tools/vdr_tool.cpp', 'logger.cpp', 'sharedmemory.cpp', 'test-tools/common.cpp',
                'vdrremoteclient.cpp', 'test-tools/ffmpeghandler.cpp',
                'httplib.cpp',
                 cpp_args : ['-DCLIENT_ONLY', '-DPHTTPLIB_ZLIB_SUPPORT', '-DCPPHTTPLIB_OPENSSL_SUPPORT'],
                 install : true,
                 install_dir : meson.current_build_dir() + '/Release',
                 dependencies: [mini_dep, spdlog_dep, deps, tiny_process_library_dep, dep_ssl, dep_crypto])

#
# install static content
#
install_subdir('static-content/js', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/css', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/database', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/application', install_dir : meson.current_build_dir() + '/Release')
