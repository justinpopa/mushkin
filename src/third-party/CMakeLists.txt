# Third-party libraries - isolated from application code
add_library(third-party STATIC
    lsqlite3.c
    # LuaFileSystem - portable filesystem operations for Lua
    lfs.c
    # LPeg - Parsing Expression Grammars for Lua
    lpcap.c
    lpcode.c
    lpcset.c
    lpprint.c
    lptree.c
    lpvm.c
)

target_link_libraries(third-party PUBLIC
    SQLite::SQLite3
    Lua::Lua
)

# Suppress warnings from third-party C code (not our code to fix)
target_compile_options(third-party PRIVATE -w)

# Set C files to be compiled as C (not C++)
set_source_files_properties(
    lsqlite3.c
    lfs.c
    lpcap.c
    lpcode.c
    lpcset.c
    lpprint.c
    lptree.c
    lpvm.c
    PROPERTIES LANGUAGE C
)
