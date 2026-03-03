add_library(utils STATIC
    app_paths.cpp
    app_paths.h
    name_generator.cpp
    name_generator.h
    logging.cpp
    logging.h
    url_linkifier.cpp
    url_linkifier.h
)

target_link_libraries(utils PUBLIC
    Qt6::Core
    Qt6::Gui
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
