# Storage module - preferences database

add_library(storage STATIC
    database.h
    database.cpp
    global_options.h
    global_options.cpp
)

target_link_libraries(storage PUBLIC
    Qt6::Core
    Qt6::Sql
    utils
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
