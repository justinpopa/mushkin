# Automation module: triggers, aliases, timers, plugins

add_library(automation
    trigger.h
    trigger.cpp
    alias.h
    alias.cpp
    timer.h
    timer.cpp
    plugin.h
    plugin.cpp
    sendto.h
)

target_link_libraries(automation PUBLIC
    Qt6::Core
    Qt6::Gui
    ZLIB::ZLIB
    utils
    storage
)

target_include_directories(automation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
