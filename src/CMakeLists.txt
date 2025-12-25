# Main executable
add_executable(mushkin
    main.cpp
    ${CMAKE_SOURCE_DIR}/resources.qrc
)

target_link_libraries(mushkin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    ui        # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

# Set output directory
set_target_properties(mushkin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# macOS bundle settings
if(APPLE)
    set_target_properties(mushkin PROPERTIES
        MACOSX_BUNDLE ${BUILD_APP_BUNDLE}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mushkin.app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# Install
install(TARGETS mushkin
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Add subdirectories for modules
add_subdirectory(third-party)        # Third-party libraries (lsqlite3, etc.)
add_subdirectory(world)              # Core document, world state, MiniWindow, and Lua API (scripting)
add_subdirectory(text)
add_subdirectory(storage)
add_subdirectory(ui)
add_subdirectory(network)
add_subdirectory(automation)
add_subdirectory(utils)
