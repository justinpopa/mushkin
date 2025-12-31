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

# Pass version from CMake to code
target_compile_definitions(mushkin PRIVATE
    MUSHKIN_VERSION="${PROJECT_VERSION}"
)

# ========== Static Qt Plugin Handling ==========
# When Qt is built statically, plugins must be explicitly imported
# See: https://doc.qt.io/qt-6/plugins-howto.html#static-plugins
if(STATIC_BUILD AND NOT BUILD_SHARED_LIBS)
    if(APPLE)
        # macOS platform plugin
        qt_import_plugins(mushkin
            INCLUDE
                Qt6::QCocoaIntegrationPlugin
                Qt6::QSQLiteDriverPlugin
                Qt6::QGifPlugin
                Qt6::QJpegPlugin
                Qt6::QICOPlugin
                Qt6::QSvgPlugin
        )
    elseif(WIN32)
        # Windows platform plugin
        qt_import_plugins(mushkin
            INCLUDE
                Qt6::QWindowsIntegrationPlugin
                Qt6::QSQLiteDriverPlugin
                Qt6::QGifPlugin
                Qt6::QJpegPlugin
                Qt6::QICOPlugin
                Qt6::QSvgPlugin
        )
    elseif(UNIX)
        # Linux platform plugins - use XCB for X11 display
        qt_import_plugins(mushkin
            INCLUDE
                Qt6::QXcbIntegrationPlugin
                Qt6::QSQLiteDriverPlugin
                Qt6::QGifPlugin
                Qt6::QJpegPlugin
                Qt6::QICOPlugin
                Qt6::QSvgPlugin
        )
    endif()
    message(STATUS "Static build: Qt plugins will be embedded in binary")
endif()

# Set output directory
set_target_properties(mushkin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# macOS bundle settings
if(APPLE)
    # Set the icon file as a source (gets copied into bundle Resources)
    set(MACOS_ICON "${CMAKE_SOURCE_DIR}/resources/mushkin.icns")
    set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(mushkin PRIVATE ${MACOS_ICON})

    set_target_properties(mushkin PROPERTIES
        MACOSX_BUNDLE ${BUILD_APP_BUNDLE}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mushkin.app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_ICON_FILE "mushkin.icns"
        # Set rpath so bundled Qt frameworks can be found at runtime
        # This is critical for self-contained app bundles
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # Ad-hoc codesign the app bundle after build (required for macOS to run unsigned apps)
    # This gets invalidated by macdeployqt, so deployment scripts must re-sign
    if(BUILD_APP_BUNDLE)
        add_custom_command(TARGET mushkin POST_BUILD
            COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:mushkin>"
            COMMENT "Ad-hoc signing mushkin.app"
        )
    endif()
endif()

# Install
install(TARGETS mushkin
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# ========== macOS Qt Deployment (self-contained app bundle) ==========
# This bundles Qt frameworks into the .app so it doesn't depend on external Qt installation
# See: https://doc.qt.io/qt-6/qt-generate-deploy-app-script.html
if(APPLE AND BUILD_APP_BUNDLE)
    # Generate deployment script that will run macdeployqt
    qt_generate_deploy_app_script(
        TARGET mushkin
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    # Run the deployment script at install time
    install(SCRIPT ${deploy_script})
endif()

# Add subdirectories for modules
add_subdirectory(third-party)        # Third-party libraries (lsqlite3, etc.)
add_subdirectory(world)              # Core document, world state, MiniWindow, and Lua API (scripting)
add_subdirectory(text)
add_subdirectory(storage)
add_subdirectory(ui)                 # All UI components (views, dialogs, main_window)
add_subdirectory(network)
add_subdirectory(automation)
add_subdirectory(utils)
