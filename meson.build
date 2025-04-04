project('cefbrowser', 'cpp', 'c',
  version : '0.1',
  default_options : ['warning_level=1', 'cpp_std=c++17', 'default_library=static', 'optimization=3', 'buildtype=release' ],
  meson_version: '>=0.63.0')

# add_global_arguments('-O3', language : 'cpp')

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

# thrift
# thrift
thrift_opts = cmake.subproject_options()
thrift_opts.add_cmake_defines({'WITH_AS3': 'OFF',
                               'WITH_QT5': 'OFF',
                               'BUILD_JAVA': 'OFF',
                               'BUILD_JAVASCRIPT': 'OFF',
                               'BUILD_PYTHON': 'OFF',
                               'BUILD_TESTING': 'OFF',
                               'WITH_C_GLIB': 'OFF',
                               'WITH_OPENSSL': 'OFF',
                               'BUILD_SHARED_LIBS': 'OFF',
                               'BUILD_COMPILER': 'OFF',
                               'CMAKE_POSITION_INDEPENDENT_CODE': 'ON' })

thrift_proj = cmake.subproject('thrift', options: thrift_opts)
thrift_dep = thrift_proj.get_variable('thrift_dep')

#
# Build browser
#
shared_libs = ['cef', 'z']
deps = []
foreach lib_name : shared_libs
        deps += CXX.find_library(lib_name, dirs : [meson.project_source_root() + '/subprojects/cef/Release'])
endforeach

incdir = include_directories(['thrift-services/src-gen', 'thrift-services/src-client'])

exe = executable('cefbrowser', 'main.cpp', 'mainapp.cpp', 'logger.cpp',
                 'browserclient.cpp', 'browserserver.cpp', 'moviestream.cpp',
                 'httpinterception.cpp', 'keycodes.cpp',
                 'v8handler.cpp', 'tools.cpp', 'database.cpp', 'xhrinterception.cpp',
                 'thrift-services/src-client/VdrClient.cpp', 'thrift-services/src-client/TranscoderClient.cpp',
                 'thrift-services/src-gen/CommonService.cpp', 'thrift-services/src-gen/common_types.cpp',
                 'thrift-services/src-gen/CefBrowser.cpp', 'thrift-services/src-gen/cefbrowser_types.cpp',
                 'thrift-services/src-gen/VdrPluginWeb.cpp', 'thrift-services/src-gen/pluginweb_types.cpp',
                 'thrift-services/src-gen/RemoteTranscoder.cpp', 'thrift-services/src-gen/remotetranscoder_types.cpp',
                 include_directories : incdir,
                 cpp_args : ['-DPHTTPLIB_ZLIB_SUPPORT', '-DCPPHTTPLIB_OPENSSL_SUPPORT'],
                 install : true,
                 install_dir : meson.current_build_dir() + '/Release',
                 dependencies: [mini_dep, cef_lib, spdlog_dep, deps, sqlite3_dep, dep_ssl, dep_crypto, thrift_dep])


#
# install static content
#
install_subdir('static-content/js', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/css', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/database', install_dir : meson.current_build_dir() + '/Release')
install_subdir('static-content/application', install_dir : meson.current_build_dir() + '/Release')
