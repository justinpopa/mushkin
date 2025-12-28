cmake_minimum_required(VERSION 3.16)
project(mushkin VERSION 0.1.0 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== macOS App Bundle Option ==========
if(APPLE)
    option(BUILD_APP_BUNDLE "Build macOS app bundle (.app) instead of plain binary" ON)
endif()

# ========== Static Build Option ==========
# When enabled, statically links Qt while keeping other deps (pcre, sqlite, luajit)
# dynamically linked from system packages (brew/apt). This produces a portable binary
# that only requires common system libraries.
option(STATIC_BUILD "Build with static Qt (dynamic deps from package manager)" OFF)

if(STATIC_BUILD)
    message(STATUS "Static build enabled - Qt will be statically linked")
    # Disable app bundle for static builds (we want a single binary)
    if(APPLE)
        set(BUILD_APP_BUNDLE OFF)
    endif()
endif()

# Qt setup - enable automatic MOC/UIC/RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ========== Qt Discovery ==========
# Check ~/.cache/mushkin/Qt for aqt-installed Qt (from build scripts)
# This allows "cmake -B build" to just work after running the build script
if(NOT DEFINED CMAKE_PREFIX_PATH OR CMAKE_PREFIX_PATH STREQUAL "")
    if(APPLE)
        set(_QT_CACHE_DIR "$ENV{HOME}/.cache/mushkin/Qt/6.9.3/macos")
    else()
        set(_QT_CACHE_DIR "$ENV{HOME}/.cache/mushkin/Qt/6.9.3/gcc_64")
    endif()
    if(EXISTS "${_QT_CACHE_DIR}/lib/cmake/Qt6")
        list(APPEND CMAKE_PREFIX_PATH "${_QT_CACHE_DIR}")
        message(STATUS "Found Qt in cache: ${_QT_CACHE_DIR}")
    endif()
endif()

# ========== Static Build: Pre-create wrapper targets ==========
# Static Qt builds require these wrapper targets to exist before find_package(Qt6)
# because the Qt CMake config files check for them
if(STATIC_BUILD AND APPLE)
    # WrapResolv - DNS resolution library (libresolv on macOS)
    # The Qt FindWrapResolv.cmake fails to find .tbd stubs, so we create the target manually
    if(NOT TARGET WrapResolv::WrapResolv)
        add_library(WrapResolv::WrapResolv INTERFACE IMPORTED)
        target_link_libraries(WrapResolv::WrapResolv INTERFACE "-lresolv")
        set(WrapResolv_FOUND ON CACHE BOOL "WrapResolv found (manually created)")
    endif()
endif()

# Find packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
)

# Qt6::Multimedia for sound playback, Qt6::SpatialAudio for stereo panning
find_package(Qt6 COMPONENTS Multimedia SpatialAudio REQUIRED)

# zlib for MCCP compression
find_package(ZLIB REQUIRED)

# PCRE for rex Lua library (Perl-compatible regular expressions)
# Original MUSHclient uses PCRE 8.x (legacy PCRE, not PCRE2)
if(APPLE)
    # macOS: Use Homebrew's PCRE
    find_path(PCRE_INCLUDE_DIR
        NAMES pcre.h
        PATHS /opt/homebrew/opt/pcre /usr/local/opt/pcre /opt/homebrew /usr/local
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )

    find_library(PCRE_LIBRARY
        NAMES pcre
        PATHS /opt/homebrew/opt/pcre /usr/local/opt/pcre /opt/homebrew /usr/local
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(PCRE_INCLUDE_DIR AND PCRE_LIBRARY)
        set(PCRE_FOUND TRUE)
        message(STATUS "Found PCRE: ${PCRE_LIBRARY}")
        message(STATUS "  Include: ${PCRE_INCLUDE_DIR}")

        add_library(PCRE::PCRE INTERFACE IMPORTED)
        set_target_properties(PCRE::PCRE PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PCRE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "PCRE not found. Install via Homebrew: brew install pcre")
    endif()
elseif(WIN32)
    # Windows: Use vcpkg's PCRE with proper Debug/Release handling
    find_path(PCRE_INCLUDE_DIR NAMES pcre.h)
    find_library(PCRE_LIBRARY_RELEASE NAMES pcre PATHS "${VCPKG_INSTALLED_DIR}/x64-windows/lib" NO_DEFAULT_PATH)
    find_library(PCRE_LIBRARY_DEBUG NAMES pcred PATHS "${VCPKG_INSTALLED_DIR}/x64-windows/debug/lib" NO_DEFAULT_PATH)

    # Fallback to default search if vcpkg paths not found
    if(NOT PCRE_LIBRARY_RELEASE)
        find_library(PCRE_LIBRARY_RELEASE NAMES pcre)
    endif()
    if(NOT PCRE_LIBRARY_DEBUG)
        find_library(PCRE_LIBRARY_DEBUG NAMES pcred pcre)
    endif()

    if(PCRE_INCLUDE_DIR AND (PCRE_LIBRARY_RELEASE OR PCRE_LIBRARY_DEBUG))
        set(PCRE_FOUND TRUE)
        message(STATUS "Found PCRE via vcpkg:")
        message(STATUS "  Release: ${PCRE_LIBRARY_RELEASE}")
        message(STATUS "  Debug: ${PCRE_LIBRARY_DEBUG}")

        add_library(PCRE::PCRE INTERFACE IMPORTED)
        set_target_properties(PCRE::PCRE PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")

        # Use generator expressions for proper Debug/Release library selection
        if(PCRE_LIBRARY_DEBUG AND PCRE_LIBRARY_RELEASE)
            set_target_properties(PCRE::PCRE PROPERTIES
                INTERFACE_LINK_LIBRARIES "$<IF:$<CONFIG:Debug>,${PCRE_LIBRARY_DEBUG},${PCRE_LIBRARY_RELEASE}>")
        elseif(PCRE_LIBRARY_DEBUG)
            set_target_properties(PCRE::PCRE PROPERTIES
                INTERFACE_LINK_LIBRARIES "${PCRE_LIBRARY_DEBUG}")
        else()
            set_target_properties(PCRE::PCRE PROPERTIES
                INTERFACE_LINK_LIBRARIES "${PCRE_LIBRARY_RELEASE}")
        endif()
    else()
        message(FATAL_ERROR "PCRE not found via vcpkg. Install with: vcpkg install pcre:x64-windows")
    endif()
else()
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCRE REQUIRED libpcre)
    message(STATUS "Found PCRE via pkg-config: ${PCRE_LIBRARIES}")

    add_library(PCRE::PCRE INTERFACE IMPORTED)
    set_target_properties(PCRE::PCRE PROPERTIES
        INTERFACE_LINK_LIBRARIES "${PCRE_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIRS}")
endif()

# SQLite3 for database support (used by lsqlite3 Lua module)
if(APPLE)
    # macOS: Use Homebrew's SQLite3 (keg-only formula)
    find_path(SQLITE3_INCLUDE_DIR
        NAMES sqlite3.h
        PATHS /opt/homebrew/opt/sqlite /usr/local/opt/sqlite /opt/homebrew /usr/local
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )

    find_library(SQLITE3_LIBRARY
        NAMES sqlite3
        PATHS /opt/homebrew/opt/sqlite /usr/local/opt/sqlite /opt/homebrew /usr/local
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
        set(SQLITE3_FOUND TRUE)
        message(STATUS "Found SQLite3: ${SQLITE3_LIBRARY}")
        message(STATUS "  Include: ${SQLITE3_INCLUDE_DIR}")

        add_library(SQLite::SQLite3 INTERFACE IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            INTERFACE_LINK_LIBRARIES "${SQLITE3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "SQLite3 not found. Install via Homebrew: brew install sqlite")
    endif()
else()
    # Windows/Linux: Use find_package
    find_package(SQLite3 REQUIRED)
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 INTERFACE IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            INTERFACE_LINK_LIBRARIES "${SQLite3_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}")
    endif()
endif()

# LuaJIT for scripting (Lua 5.1 compatible with FFI support)
# Original MUSHclient uses LuaJIT 2.1.0-beta3 for FFI module support
if(APPLE)
    # macOS: Use Homebrew's LuaJIT
    set(LUAJIT_SEARCH_PATHS
        "/opt/homebrew"      # Apple Silicon
        "/usr/local"         # Intel Mac
    )

    find_path(LUA_INCLUDE_DIR
        NAMES luajit.h
        PATHS ${LUAJIT_SEARCH_PATHS}
        PATH_SUFFIXES include/luajit-2.1 include/luajit-2.0
        NO_DEFAULT_PATH
    )

    find_library(LUA_LIBRARIES
        NAMES luajit-5.1 luajit
        PATHS ${LUAJIT_SEARCH_PATHS}
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )

    if(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
        set(LUA_FOUND TRUE)

        message(STATUS "Found LuaJIT: ${LUA_LIBRARIES}")
        message(STATUS "  Include: ${LUA_INCLUDE_DIR}")

        # Create alias target for compatibility
        if(NOT TARGET Lua::Lua)
            add_library(Lua::Lua INTERFACE IMPORTED)
            set_target_properties(Lua::Lua PROPERTIES
                INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
        endif()
    else()
        message(FATAL_ERROR "LuaJIT not found. Install via Homebrew:
  brew install luajit")
    endif()
elseif(WIN32)
    # Windows: Use vcpkg's LuaJIT
    # Install via: vcpkg install luajit:x64-windows
    # Note: vcpkg's luajit port doesn't provide CMake config, use manual find

    find_path(LUA_INCLUDE_DIR
        NAMES luajit.h lua.h
        PATH_SUFFIXES include include/luajit-2.1 include/luajit
    )

    find_library(LUA_LIBRARY
        NAMES lua51 luajit
        PATH_SUFFIXES lib
    )

    if(LUA_INCLUDE_DIR AND LUA_LIBRARY)
        set(LUA_LIBRARIES "${LUA_LIBRARY}")
        set(LUA_FOUND TRUE)

        message(STATUS "Found LuaJIT via vcpkg: ${LUA_LIBRARIES}")
        message(STATUS "  Include: ${LUA_INCLUDE_DIR}")

        # Create alias target for compatibility
        if(NOT TARGET Lua::Lua)
            add_library(Lua::Lua INTERFACE IMPORTED)
            set_target_properties(Lua::Lua PROPERTIES
                INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
        endif()
    else()
        message(FATAL_ERROR "LuaJIT not found via vcpkg. Install with:
  vcpkg install luajit:x64-windows")
    endif()
else()
    # Linux/Unix: Try pkg-config first for LuaJIT, fall back to Lua 5.1
    find_package(PkgConfig REQUIRED)

    # Try LuaJIT first (preferred)
    pkg_check_modules(LUA luajit)

    if(LUA_FOUND)
        message(STATUS "Found LuaJIT via pkg-config: ${LUA_LIBRARIES}")
    else()
        # Fall back to standard Lua 5.1 (will lack FFI support)
        pkg_check_modules(LUA REQUIRED lua5.1)
        message(STATUS "Found Lua 5.1 via pkg-config: ${LUA_LIBRARIES}")
        message(WARNING "Using standard Lua 5.1 - FFI module will not be available. Install LuaJIT for full plugin compatibility.")
    endif()

    message(STATUS "  Include: ${LUA_INCLUDE_DIRS}")

    # Create alias target for compatibility
    if(NOT TARGET Lua::Lua)
        add_library(Lua::Lua INTERFACE IMPORTED)
        set_target_properties(Lua::Lua PROPERTIES
            INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIRS}")
    endif()
endif()

# Add subdirectories
add_subdirectory(src)

# ========== Copy Source Lua Files ==========
# Copy Lua files from source lua/ directory to build lua/ directory
# These include stub modules (like luacom) for cross-platform compatibility
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lua")
file(GLOB LUA_SOURCE_FILES "${CMAKE_SOURCE_DIR}/lua/*.lua")
foreach(LUA_FILE ${LUA_SOURCE_FILES})
    get_filename_component(FILENAME ${LUA_FILE} NAME)
    configure_file(${LUA_FILE} "${CMAKE_BINARY_DIR}/lua/${FILENAME}" COPYONLY)
endforeach()
message(STATUS "Copied ${CMAKE_SOURCE_DIR}/lua/*.lua to build directory")

# ========== LuaSocket Library ==========
# Build socket.core and mime.core for HTTP client support in plugins
# Required for plugins that use socket.http for web requests
if(EXISTS "${CMAKE_SOURCE_DIR}/external/luasocket/CMakeLists.txt")
    add_subdirectory(external/luasocket)
    message(STATUS "LuaSocket: Building socket.core and mime.core modules")
endif()

# ========== LuaSec Library ==========
# Build ssl.so for HTTPS support in plugins
# Required for plugins that use ssl.https for secure web requests
if(EXISTS "${CMAKE_SOURCE_DIR}/external/luasec/CMakeLists.txt")
    add_subdirectory(external/luasec)
    message(STATUS "LuaSec: Building ssl module for HTTPS support")
endif()

# ========== Google Test Framework ==========
# Fetch GoogleTest for unit testing
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2  # Latest stable release
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make GoogleTest available
FetchContent_MakeAvailable(googletest)

# Testing
enable_testing()
add_subdirectory(tests)

# ========== Dependency Visualization ==========
# Generate CMake dependency graph for analysis
# Optional: requires 'graphviz' to be installed (brew install graphviz)
# Search in common locations since find_program may not find homebrew tools
find_program(GRAPHVIZ_EXECUTABLE dot
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
    NO_DEFAULT_PATH
)

# If not found in common paths, try system search
if(NOT GRAPHVIZ_EXECUTABLE)
    find_program(GRAPHVIZ_EXECUTABLE dot)
endif()

# Always add target that generates DOT file (graphviz optional for PNG conversion)
add_custom_target(dependency-graph-dot
    COMMAND ${CMAKE_COMMAND} --graphviz=${CMAKE_BINARY_DIR}/dependencies.dot .
    COMMAND ${CMAKE_COMMAND} -E echo "CMake dependency graph: ${CMAKE_BINARY_DIR}/dependencies.dot"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating CMake dependency graph (DOT format)"
    VERBATIM
)

# Add PNG conversion target if graphviz is available
if(GRAPHVIZ_EXECUTABLE)
    add_custom_target(dependency-graph
        DEPENDS dependency-graph-dot
        COMMAND ${GRAPHVIZ_EXECUTABLE} -Tpng ${CMAKE_BINARY_DIR}/dependencies.dot -o ${CMAKE_BINARY_DIR}/dependencies.png
        COMMAND ${CMAKE_COMMAND} -E echo "Dependency graph PNG: ${CMAKE_BINARY_DIR}/dependencies.png"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Converting dependency graph to PNG (requires graphviz)"
        VERBATIM
    )
else()
    # Fallback target that just reminds user to install graphviz
    add_custom_target(dependency-graph
        COMMAND ${CMAKE_COMMAND} -E echo_append ""
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: Graphviz 'dot' not found. Install with: brew install graphviz"
        COMMAND ${CMAKE_COMMAND} -E echo "Or manually convert: dot -Tpng build/dependencies.dot -o build/dependencies.png"
        COMMAND false
    )
endif()

# ========== API Documentation ==========
# Generate API documentation with Doxygen
# Optional: requires doxygen (brew install doxygen)
find_program(DOXYGEN_EXECUTABLE doxygen
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
    NO_DEFAULT_PATH
)

if(NOT DOXYGEN_EXECUTABLE)
    find_program(DOXYGEN_EXECUTABLE doxygen)
endif()

if(DOXYGEN_EXECUTABLE)
    # C++ API documentation target
    add_custom_target(api-docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E echo "API documentation generated in docs/api/html/index.html"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating C++ API documentation with Doxygen"
        VERBATIM
    )
else()
    add_custom_target(api-docs
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: Doxygen not found. Install with: brew install doxygen"
        COMMAND false
    )
endif()

# Check for LDoc (Lua documentation tool)
find_program(LDOC_EXECUTABLE ldoc
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
    NO_DEFAULT_PATH
)

if(NOT LDOC_EXECUTABLE)
    find_program(LDOC_EXECUTABLE ldoc)
endif()

if(LDOC_EXECUTABLE)
    # Lua API documentation target
    add_custom_target(lua-docs
        COMMAND ${LDOC_EXECUTABLE} -d ${CMAKE_SOURCE_DIR}/docs/api/lua ${CMAKE_SOURCE_DIR}/lua
        COMMAND ${CMAKE_COMMAND} -E echo "Lua documentation generated in docs/api/lua/index.html"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating Lua API documentation with LDoc"
        VERBATIM
    )
    
    # Combined documentation target
    add_custom_target(docs
        DEPENDS api-docs lua-docs
        COMMENT "Generating all documentation (C++ and Lua)"
    )
else()
    # No LDoc, just generate C++ docs
    add_custom_target(docs
        DEPENDS api-docs
        COMMENT "Generating C++ API documentation (LDoc not found for Lua docs)"
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Mushkin Configuration Summary")
message(STATUS "====================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(GRAPHVIZ_EXECUTABLE)
    message(STATUS "Graphviz: enabled - run 'cmake --build build --target dependency-graph' for PNG")
else()
    message(STATUS "Graphviz: not found - run 'cmake --build build --target dependency-graph-dot' for DOT file")
endif()
if(DOXYGEN_EXECUTABLE)
    message(STATUS "Doxygen: enabled - run 'cmake --build build --target api-docs' for C++ docs")
else()
    message(STATUS "Doxygen: not found - install with 'brew install doxygen'")
endif()
if(LDOC_EXECUTABLE)
    message(STATUS "LDoc: enabled - run 'cmake --build build --target lua-docs' for Lua docs")
else()
    message(STATUS "LDoc: not found - install with 'luarocks install ldoc'")
endif()
message(STATUS "")
