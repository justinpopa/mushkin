# Text rendering classes library
# Port of CLine, CStyle, CAction from original MUSHclient

add_library(text STATIC
    action.h
    action.cpp
    style.h
    style.cpp
    line.h
    line.cpp
)

target_link_libraries(text PUBLIC
    Qt6::Core
    Qt6::Gui
)

# Allow other modules to include text headers
target_include_directories(text PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
