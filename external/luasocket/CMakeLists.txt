# LuaSocket CMakeLists.txt
# Builds socket.core and mime.core modules for LuaSocket 3.1.0
# MIT License - https://github.com/lunarmodules/luasocket

cmake_minimum_required(VERSION 3.16)

# Handle both LUA_INCLUDE_DIR (macOS/Windows) and LUA_INCLUDE_DIRS (Linux pkg-config)
if(NOT LUA_INCLUDE_DIR AND LUA_INCLUDE_DIRS)
    set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIRS})
endif()

# Platform-specific socket implementation
if(WIN32)
    set(SOCKET_PLATFORM_SOURCE src/wsocket.c)
    set(SOCKET_PLATFORM_LIBS ws2_32)
else()
    set(SOCKET_PLATFORM_SOURCE src/usocket.c)
    set(SOCKET_PLATFORM_LIBS "")
endif()

# socket.core - main socket library
add_library(socket_core MODULE
    src/luasocket.c
    src/timeout.c
    src/buffer.c
    src/io.c
    src/auxiliar.c
    src/options.c
    src/inet.c
    src/except.c
    src/select.c
    src/tcp.c
    src/udp.c
    src/compat.c
    ${SOCKET_PLATFORM_SOURCE}
)

target_include_directories(socket_core PRIVATE
    src
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(socket_core PRIVATE
    ${LUA_LIBRARIES}
    ${SOCKET_PLATFORM_LIBS}
)

# Set output name to "core" in a "socket" subdirectory
set_target_properties(socket_core PROPERTIES
    OUTPUT_NAME "core"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/socket"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/socket"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/socket"
)

# macOS: Use .so extension for Lua compatibility (not .dylib)
if(APPLE)
    set_target_properties(socket_core PROPERTIES SUFFIX ".so")
endif()

# mime.core - MIME encoding library
add_library(mime_core MODULE
    src/mime.c
    src/compat.c
)

target_include_directories(mime_core PRIVATE
    src
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(mime_core PRIVATE
    ${LUA_LIBRARIES}
)

set_target_properties(mime_core PROPERTIES
    OUTPUT_NAME "core"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/mime"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/mime"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/mime"
)

if(APPLE)
    set_target_properties(mime_core PROPERTIES SUFFIX ".so")
endif()

# Unix socket support (macOS/Linux only)
if(UNIX)
    add_library(socket_unix MODULE
        src/buffer.c
        src/compat.c
        src/auxiliar.c
        src/options.c
        src/timeout.c
        src/io.c
        src/usocket.c
        src/unix.c
        src/unixdgram.c
        src/unixstream.c
    )

    target_include_directories(socket_unix PRIVATE
        src
        ${LUA_INCLUDE_DIR}
    )

    target_link_libraries(socket_unix PRIVATE
        ${LUA_LIBRARIES}
    )

    set_target_properties(socket_unix PROPERTIES
        OUTPUT_NAME "unix"
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/socket"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/socket"
    )

    if(APPLE)
        set_target_properties(socket_unix PROPERTIES SUFFIX ".so")
    endif()
endif()

# Copy Lua files to build directory for runtime loading
set(LUA_SOCKET_FILES
    src/socket.lua
    src/ltn12.lua
    src/mime.lua
)

set(LUA_SOCKET_SUBFILES
    src/http.lua
    src/url.lua
    src/tp.lua
    src/ftp.lua
    src/smtp.lua
    src/headers.lua
)

# Copy top-level Lua files
foreach(LUA_FILE ${LUA_SOCKET_FILES})
    get_filename_component(FILENAME ${LUA_FILE} NAME)
    configure_file(${LUA_FILE} "${CMAKE_BINARY_DIR}/lua/${FILENAME}" COPYONLY)
endforeach()

# Copy socket subdirectory Lua files
foreach(LUA_FILE ${LUA_SOCKET_SUBFILES})
    get_filename_component(FILENAME ${LUA_FILE} NAME)
    configure_file(${LUA_FILE} "${CMAKE_BINARY_DIR}/lua/socket/${FILENAME}" COPYONLY)
endforeach()
