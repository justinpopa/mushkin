# LuaSec CMakeLists.txt
# Builds ssl.so module for LuaSec
# MIT License - https://github.com/brunoos/luasec

cmake_minimum_required(VERSION 3.16)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Handle both LUA_INCLUDE_DIR (macOS/Windows) and LUA_INCLUDE_DIRS (Linux pkg-config)
if(NOT LUA_INCLUDE_DIR AND LUA_INCLUDE_DIRS)
    set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIRS})
endif()

# ssl.so - main SSL library
# Includes LuaSocket support files from src/luasocket/
add_library(ssl_core MODULE
    src/ssl.c
    src/context.c
    src/x509.c
    src/config.c
    src/ec.c
    src/options.c
    # LuaSocket dependencies
    src/luasocket/buffer.c
    src/luasocket/io.c
    src/luasocket/timeout.c
    src/luasocket/usocket.c
)

target_include_directories(ssl_core PRIVATE
    src
    src/luasocket
    ${LUA_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ssl_core PRIVATE
    ${LUA_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Set output name to "ssl" in lib directory
set_target_properties(ssl_core PROPERTIES
    OUTPUT_NAME "ssl"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# macOS: Use .so extension for Lua compatibility (not .dylib)
if(APPLE)
    set_target_properties(ssl_core PROPERTIES SUFFIX ".so")
endif()

# Copy Lua files to build directory
set(LUA_SSL_FILES
    src/ssl.lua
    src/https.lua
)

# Create ssl subdirectory for https.lua
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lua/ssl")

# Copy ssl.lua to lua/
configure_file(src/ssl.lua "${CMAKE_BINARY_DIR}/lua/ssl.lua" COPYONLY)

# Copy https.lua to lua/ssl/
configure_file(src/https.lua "${CMAKE_BINARY_DIR}/lua/ssl/https.lua" COPYONLY)

# Copy options.lua to lua/ssl/
configure_file(src/options.lua "${CMAKE_BINARY_DIR}/lua/ssl/options.lua" COPYONLY)
