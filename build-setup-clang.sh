#
# to be able to use this cross file a special build of cef is necessary and needs to be installed manually:
# https://bitbucket.org/chromiumembedded/cef/wiki/UsingAddressSanitizer.md
#

meson setup build-clang --cross-file clang-sanitize
