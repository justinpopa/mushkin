# Third-party libraries - isolated from application code
add_library(third-party STATIC
    lsqlite3.c
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

# Set C files to be compiled as C (not C++)
set_source_files_properties(
    lsqlite3.c
    lpcap.c
    lpcode.c
    lpcset.c
    lpprint.c
    lptree.c
    lpvm.c
    PROPERTIES LANGUAGE C
)
