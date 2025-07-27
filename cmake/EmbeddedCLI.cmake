# this creates a static lib containing the embedded CLI library
add_library(EmbeddedCLI STATIC
    thirdparty/EmbeddedCLI/embedded_cli.c)

target_include_directories(EmbeddedCLI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/EmbeddedCLI>
    $<INSTALL_INTERFACE:include>)
