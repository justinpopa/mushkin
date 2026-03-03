# llthreads2 CMakeLists.txt
# Builds llthreads2 module for Lua threading support
# MIT License - https://github.com/moteus/lua-llthreads2

cmake_minimum_required(VERSION 3.16)

# Handle both LUA_INCLUDE_DIR (macOS/Windows) and LUA_INCLUDE_DIRS (Linux pkg-config)
if(NOT LUA_INCLUDE_DIR AND LUA_INCLUDE_DIRS)
    set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIRS})
endif()

add_library(llthreads2 MODULE
    llthread.c
    l52util.c
)

target_include_directories(llthreads2 PRIVATE
    ${LUA_INCLUDE_DIR}
)

# Define module name so luaopen_llthreads2 is the entry point
target_compile_definitions(llthreads2 PRIVATE
    LLTHREAD_MODULE_NAME=llthreads2
)

target_link_libraries(llthreads2 PRIVATE
    ${LUA_LIBRARIES}
)

# Platform-specific threading library
if(NOT WIN32)
    target_link_libraries(llthreads2 PRIVATE pthread)
endif()

# Output as llthreads2.so / llthreads2.dll at top of lib/
set_target_properties(llthreads2 PROPERTIES
    PREFIX ""
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(WIN32)
    set_target_properties(llthreads2 PROPERTIES SUFFIX ".dll")
endif()

# Suppress warnings in third-party code
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(llthreads2 PRIVATE -w)
endif()

message(STATUS "llthreads2: Building Lua threading module")
