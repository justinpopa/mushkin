# YueScript CMakeLists.txt for Mushkin
# Builds YueScript as a static library linked into the main application
# This avoids Windows DLL loading issues with Lua C modules
# MIT License - https://github.com/pigpigyyy/Yuescript

cmake_minimum_required(VERSION 3.16)

# Handle both LUA_INCLUDE_DIR (macOS/Windows) and LUA_INCLUDE_DIRS (Linux pkg-config)
if(NOT LUA_INCLUDE_DIR AND LUA_INCLUDE_DIRS)
    set(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIRS})
endif()

set(YUE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../yuescript/src")

# Core YueScript compiler sources
set(YUE_SOURCES
    ${YUE_SRC_DIR}/yuescript/ast.cpp
    ${YUE_SRC_DIR}/yuescript/parser.cpp
    ${YUE_SRC_DIR}/yuescript/yue_ast.cpp
    ${YUE_SRC_DIR}/yuescript/yue_parser.cpp
    ${YUE_SRC_DIR}/yuescript/yue_compiler.cpp
    ${YUE_SRC_DIR}/yuescript/yuescript.cpp
)

# Build YueScript as a static library (linked into main app)
# This avoids Windows DLL hell with Lua C modules
add_library(yue_static STATIC ${YUE_SOURCES})

target_include_directories(yue_static PUBLIC
    ${YUE_SRC_DIR}
    ${YUE_SRC_DIR}/3rdParty
    ${LUA_INCLUDE_DIR}
)

target_compile_definitions(yue_static PRIVATE
    YUE_UTF8_IMPL
)

# C++17 required
target_compile_features(yue_static PRIVATE cxx_std_17)

# Compiler-specific options
if(MSVC)
    target_compile_options(yue_static PRIVATE
        # Make MSVC report __cplusplus correctly for C++17
        # Required for utf8cpp.h to enable the C++11+ string overloads
        /Zc:__cplusplus
    )
else()
    target_compile_options(yue_static PRIVATE
        $<$<CONFIG:Release>:-O3>
        -fPIC
    )
endif()

# Don't link against Lua here - the main app provides the Lua symbols
# target_link_libraries(yue_static PRIVATE ${LUA_LIBRARIES})

# Also build the module for standalone use (Linux/macOS only)
# This is optional and primarily for testing or standalone yue usage
if(NOT WIN32)
    add_library(yue_module MODULE ${YUE_SOURCES})

    target_include_directories(yue_module PRIVATE
        ${YUE_SRC_DIR}
        ${YUE_SRC_DIR}/3rdParty
        ${LUA_INCLUDE_DIR}
    )

    target_compile_definitions(yue_module PRIVATE
        YUE_UTF8_IMPL
    )

    target_compile_features(yue_module PRIVATE cxx_std_17)

    target_compile_options(yue_module PRIVATE
        $<$<CONFIG:Release>:-O3>
        -fPIC
    )

    target_link_libraries(yue_module PRIVATE ${LUA_LIBRARIES})

    # Output as yue.so (not libyue.so)
    set_target_properties(yue_module PROPERTIES
        OUTPUT_NAME "yue"
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )

    # macOS: Use .so extension for Lua compatibility (not .dylib)
    if(APPLE)
        set_target_properties(yue_module PROPERTIES SUFFIX ".so")
    endif()
endif()
