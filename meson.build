project('cefbrowser', 'cpp', 'c',
  version : '0.1',
  default_options : ['warning_level=1', 'cpp_std=c++20', 'default_library=static', 'optimization=3', 'buildtype=debug' ],
  meson_version: '>=0.63.0')

add_global_arguments('-O3', language : 'cpp')

arch=get_option('arch')

if get_option('vdrsternelec')
    subarch = '-march=' + get_option('subarch')
    install_prefix = get_option('prefix')
endif

cmake = import('cmake')
CXX = meson.get_compiler('cpp')

#
# Build cef
#
# An empty cef_libdir keeps the bundled CEF: setup.sh downloads the binary
# distribution into subprojects/cef. Packagers point it at an installed
# libcef.so instead, because a distribution build cannot reach the network.
cef_libdir = get_option('cef_libdir')

if cef_libdir == ''
    run_command('./setup.sh', arch, meson.current_build_dir(), check: false)
    cef_libdir = meson.project_source_root() / 'subprojects' / 'cef' / 'Release'
endif

cef_opt_var = cmake.subproject_options()

if arch == 'arm'
    cef_opt_var.add_cmake_defines({'PROJECT_ARCH': 'armhf'})
elif arch == 'arm64'
    cef_opt_var.add_cmake_defines({'PROJECT_ARCH': 'arm64'})
elif arch == 'x86_64'
    cef_opt_var.add_cmake_defines({'PROJECT_ARCH': 'x86_64'})
endif

if get_option('vdrsternelec')
    cef_opt_var.add_cmake_defines({'CMAKE_CXX_FLAGS': subarch})
    cef_opt_var.add_cmake_defines({'CMAKE_C_FLAGS': subarch})
endif

cef_opt_var.append_compile_args('cpp', '-Wno-unused-variable')
cef_opt_var.add_cmake_defines({'CMAKE_BUILD_TYPE': get_option('buildtype') == 'debug' ? 'Debug' : 'Release'})
cef_proj = cmake.subproject('cef', options: cef_opt_var)
cef_lib = cef_proj.dependency('libcef_dll_wrapper')

#
# spdlog
#
spdlog_dep = dependency('spdlog',
                        default_options: ['tests=disabled', 'compile_library=true'])

#
# mINI
#
mini_proj = subproject('mini')
mini_dep = mini_proj.get_variable('mini_dep')

#
# sqlite3
#
sqlite3_dep = dependency('sqlite3')

#
# ssl
#
ssl_dep = dependency('libssl')
crypto_dep = dependency('libcrypto')

#
# thrift
#
thrift_dep = dependency('thrift', required: false)

if not thrift_dep.found()
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
                                   'CMAKE_INSTALL_PREFIX': '.',
                                   'CMAKE_POSITION_INDEPENDENT_CODE': 'ON',
                                   'CMAKE_CXX_STANDARD': '11'})

    thrift_proj = cmake.subproject('thrift', options: thrift_opts)
    thrift_dep = thrift_proj.get_variable('thrift_dep')
endif

#
# curl
#
curl_dep = dependency('libcurl', fallback: ['curl', 'curl_dep'])

#
# cpr
#
cpr_dep = dependency('cpr')

libidn2 = dependency('libidn2', required: true)

#
# Build browser
#
shared_libs = ['cef', 'z']
deps = []
foreach lib_name : shared_libs
        deps += CXX.find_library(lib_name, dirs : [cef_libdir])
endforeach

incdir = include_directories(['thrift-services/src-gen', 'thrift-services/src-client'])
browser_compile_args = ['-DPHTTPLIB_ZLIB_SUPPORT', '-DCPPHTTPLIB_OPENSSL_SUPPORT']

if get_option('debug_thrift')
    browser_compile_args += '-DDEBUG_THRIFTCLIENT'
endif

sources = ['cefbrowser', 'main.cpp', 'mainapp.cpp', 'logger.cpp',
           'browserclient.cpp', 'osdsender.cpp', 'browserserver.cpp', 'moviestream.cpp', 'mserequest.cpp',
           'httpinterception.cpp', 'keycodes.cpp', 'trackinginterception.cpp', 'statichandler.cpp', 'jsinterception.cpp',
           'v8handler.cpp', 'tools.cpp', 'database.cpp', 'xhrinterception.cpp', 'pagemodifier.cpp', 'baseinterception.cpp',
           'thrift-services/src-client/VdrClient.cpp', 'thrift-services/src-client/TranscoderClient.cpp',
           'thrift-services/src-gen/CommonService.cpp', 'thrift-services/src-gen/common_types.cpp',
           'thrift-services/src-gen/CefBrowser.cpp', 'thrift-services/src-gen/cefbrowser_types.cpp',
           'thrift-services/src-gen/VdrPluginWeb.cpp', 'thrift-services/src-gen/pluginweb_types.cpp',
           'thrift-services/src-gen/RemoteTranscoder.cpp', 'thrift-services/src-gen/remotetranscoder_types.cpp']

deps = [deps, mini_dep, cef_lib, spdlog_dep, sqlite3_dep, ssl_dep, crypto_dep, thrift_dep, curl_dep, cpr_dep, libidn2]

# cef_libdir is kept off the loader's default path on purpose: it also holds
# libEGL, libGLESv2 and libvulkan, which must not shadow the system copies for
# anything else. An rpath on this one binary is how libcef.so is found.
exe = executable(sources,
                 include_directories : incdir,
                 cpp_args : browser_compile_args,
                 install : true,
                 install_rpath : cef_libdir,
                 link_args : get_option('vdrsternelec') ? ['-lrt'] : [],
                 dependencies : deps)

#
# install static content
#
# The browser rewrites js/_zoom_level.js under staticdir on every start, so
# the tree cannot sit under a read-only prefix.
foreach part : ['js', 'css', 'database', 'application', 'fonts']
    install_subdir('static-content' / part, install_dir : get_option('staticdir'))
endforeach

