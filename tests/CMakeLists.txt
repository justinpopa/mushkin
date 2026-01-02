# Find Qt6::Test for QSignalSpy (used by miniwindow tests)
find_package(Qt6 COMPONENTS Test QUIET)

# For static Qt builds on Linux, we need to link the offscreen platform plugin
# so tests can run headlessly (QT_QPA_PLATFORM=offscreen)
# macOS static Qt doesn't include the offscreen plugin (only Cocoa)
if(STATIC_BUILD AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # The offscreen plugin target name
    set(QT_STATIC_PLUGINS Qt6::QOffscreenIntegrationPlugin)
else()
    set(QT_STATIC_PLUGINS "")
endif()

# Create symlinks from tests directory to lua and lib directories
# This allows tests to find Lua modules via relative paths
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/lua
    ${CMAKE_CURRENT_BINARY_DIR}/lua
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/lib
)

# ========== GoogleTest Tests ==========
# New test infrastructure using GoogleTest framework

# Pipeline test (GoogleTest version) - proof of concept for migration
add_executable(test-pipeline-gtest
    test_pipeline_gtest.cpp
)

target_link_libraries(test-pipeline-gtest PRIVATE
    GTest::gtest              # GoogleTest library (without main, we provide our own)
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

# Register with CTest for automatic test discovery
# Register tests with CTest using gtest_discover_tests for granular test discovery
include(GoogleTest)
gtest_discover_tests(test-pipeline-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Trigger execution test (GoogleTest version)
add_executable(test-trigger-execution-gtest
    test_trigger_execution_gtest.cpp
)

target_link_libraries(test-trigger-execution-gtest PRIVATE
    GTest::gtest              # GoogleTest library (without main, we provide our own)
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-trigger-execution-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Timer structure test (GoogleTest version)
add_executable(test-timer-structure-gtest
    test_timer_structure_gtest.cpp
)

target_link_libraries(test-timer-structure-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-timer-structure-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Lua basics test (GoogleTest version)
add_executable(test-lua-basics-gtest
    test_lua_basics_gtest.cpp
)

target_link_libraries(test-lua-basics-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-lua-basics-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Note methods test (GoogleTest version)
add_executable(test-note-methods-gtest
    test_note_methods_gtest.cpp
)

target_link_libraries(test-note-methods-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-note-methods-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Script loading test (GoogleTest version)
add_executable(test-script-loading-gtest
    test_script_loading_gtest.cpp
)

target_link_libraries(test-script-loading-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-script-loading-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Lua API test (GoogleTest version)
add_executable(test-lua-api-gtest
    test_lua_api_gtest.cpp
)

target_link_libraries(test-lua-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-lua-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Triggers and aliases data structures test (GoogleTest version)
add_executable(test-triggers-aliases-gtest
    test_triggers_aliases_gtest.cpp
)

target_link_libraries(test-triggers-aliases-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-triggers-aliases-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Timer fire calculation test (GoogleTest version)
add_executable(test-timer-fire-calculation-gtest
    test_timer_fire_calculation_gtest.cpp
)

target_link_libraries(test-timer-fire-calculation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-timer-fire-calculation-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Timer evaluation test (GoogleTest version)
add_executable(test-timer-evaluation-gtest
    test_timer_evaluation_gtest.cpp
)

target_link_libraries(test-timer-evaluation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-timer-evaluation-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Timer execution test (GoogleTest version)
add_executable(test-timer-execution-gtest
    test_timer_execution_gtest.cpp
)

target_link_libraries(test-timer-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-timer-execution-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Timer Lua API test (GoogleTest version)
add_executable(test-timer-api-gtest
    test_timer_api_gtest.cpp
)

target_link_libraries(test-timer-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-timer-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Alias execution test (GoogleTest version)
add_executable(test-alias-execution-gtest
    test_alias_execution_gtest.cpp
)

target_link_libraries(test-alias-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-alias-execution-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Alias integration test (GoogleTest version)
add_executable(test-alias-integration-gtest
    test_alias_integration_gtest.cpp
)

target_link_libraries(test-alias-integration-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-alias-integration-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Speedwalk test (GoogleTest version)
add_executable(test-speedwalk-gtest
    test_speedwalk_gtest.cpp
)

target_link_libraries(test-speedwalk-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-speedwalk-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Command execution test (GoogleTest version)
add_executable(test-command-execution-gtest
    test_command_execution_gtest.cpp
)

target_link_libraries(test-command-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-command-execution-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Command stacking test (GoogleTest version)
add_executable(test-command-stacking-gtest
    test_command_stacking_gtest.cpp
)

target_link_libraries(test-command-stacking-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-command-stacking-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Trigger matching test (GoogleTest version)
add_executable(test-trigger-matching-gtest
    test_trigger_matching_gtest.cpp
)

target_link_libraries(test-trigger-matching-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-trigger-matching-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin API test (GoogleTest version)
add_executable(test-plugin-api-gtest
    test_plugin_api_gtest.cpp
)

target_link_libraries(test-plugin-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Variable expansion test (GoogleTest version)
add_executable(test-variable-expansion-gtest
    test_variable_expansion_gtest.cpp
)

target_link_libraries(test-variable-expansion-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-variable-expansion-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin callbacks test (GoogleTest version)
add_executable(test-plugin-callbacks-gtest
    test_plugin_callbacks_gtest.cpp
)

target_link_libraries(test-plugin-callbacks-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-callbacks-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin error path test (GoogleTest version)
add_executable(test-plugin-errors-gtest
    test_plugin_errors_gtest.cpp
)

target_link_libraries(test-plugin-errors-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-errors-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Trigger/Alias Lua API test (GoogleTest version)
add_executable(test-trigger-alias-api-gtest
    test_trigger_alias_api_gtest.cpp
)

target_link_libraries(test-trigger-alias-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-trigger-alias-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Command Window and Color Lua API test (GoogleTest version)
add_executable(test-command-color-api-gtest
    test_command_color_api_gtest.cpp
)

target_link_libraries(test-command-color-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-command-color-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Notepad API test (GoogleTest version)
add_executable(test-notepad-api-gtest
    test_notepad_api_gtest.cpp
)

target_link_libraries(test-notepad-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-notepad-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin evaluation order test (GoogleTest version)
add_executable(test-plugin-evaluation-gtest
    test_plugin_evaluation_gtest.cpp
)

target_link_libraries(test-plugin-evaluation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-evaluation-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin state saving test (GoogleTest version)
add_executable(test-plugin-state-gtest
    test_plugin_state_gtest.cpp
)

target_link_libraries(test-plugin-state-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-state-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Command history navigation test (GoogleTest version)
add_executable(test-command-history-navigation-gtest
    test_command_history_navigation_gtest.cpp
)

target_link_libraries(test-command-history-navigation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-command-history-navigation-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Plugin serialization test (GoogleTest version)
add_executable(test-plugin-serialization-gtest
    test_plugin_serialization_gtest.cpp
)

target_link_libraries(test-plugin-serialization-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-plugin-serialization-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Text data classes test (GoogleTest version)
add_executable(test-text-data-classes-gtest
    test_text_data_classes_gtest.cpp
)

target_link_libraries(test-text-data-classes-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-text-data-classes-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Retrospective logging test (GoogleTest version)
add_executable(test-retrospective-logging-gtest
    test_retrospective_logging_gtest.cpp
)

target_link_libraries(test-retrospective-logging-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-retrospective-logging-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Tab completion test (GoogleTest version)
add_executable(test-tab-completion-gtest
    test_tab_completion_gtest.cpp
)

target_link_libraries(test-tab-completion-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-tab-completion-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Word wrap test (GoogleTest version)
add_executable(test-word-wrap-gtest
    test_word_wrap_gtest.cpp
)

target_link_libraries(test-word-wrap-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-word-wrap-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Line logging test (GoogleTest version)
add_executable(test-line-logging-gtest
    test_line_logging_gtest.cpp
)

target_link_libraries(test-line-logging-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-line-logging-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Database test (GoogleTest version)
add_executable(test-database-gtest
    test_database_gtest.cpp
)

target_link_libraries(test-database-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    Qt6::Sql
    ui
)

gtest_discover_tests(test-database-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Lua Database API test (GoogleTest version)
add_executable(test-lua-database-gtest
    test_lua_database_gtest.cpp
)

target_link_libraries(test-lua-database-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-lua-database-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# XML serialization test (GoogleTest version)
add_executable(test-xml-serialization-gtest
    test_xml_serialization_gtest.cpp
)

target_link_libraries(test-xml-serialization-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-xml-serialization-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Options tables test (GoogleTest version)
add_executable(test-options-tables-gtest
    test_options_tables_gtest.cpp
)

target_link_libraries(test-options-tables-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-options-tables-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)


# XML roundtrip test (GoogleTest version)
add_executable(test-xml-roundtrip-gtest
    test_xml_roundtrip_gtest.cpp
)

target_link_libraries(test-xml-roundtrip-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui
)

gtest_discover_tests(test-xml-roundtrip-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Miniwindow creation test (doesn't require Catch2)
add_executable(test-miniwindow-creation
    test_miniwindow_creation.cpp
)

target_link_libraries(test-miniwindow-creation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    Qt6::Test
        ui          # Transitively brings: world, text, storage, automation, utils
                # world includes Lua API (scripting merged into world)
)

# Miniwindow drawing test (doesn't require Catch2)
add_executable(test-miniwindow-drawing
    test_miniwindow_drawing.cpp
)

target_link_libraries(test-miniwindow-drawing PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    Qt6::Test
        ui          # Transitively brings: world, text, storage, automation, utils
                # world includes Lua API (scripting merged into world)
)

# Copy test Lua files to build directory
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/test_valid.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/test_syntax_error.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_error.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/test_api.lua
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests
)

# Register miniwindow tests with CTest
gtest_discover_tests(test-miniwindow-creation
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)
gtest_discover_tests(test-miniwindow-drawing
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# SendTo integration test (GoogleTest version)
add_executable(test-sendto-integration-gtest
    test_sendto_integration_gtest.cpp
)

target_link_libraries(test-sendto-integration-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-sendto-integration-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Sound API test (GoogleTest version)
add_executable(test-sound-api-gtest
    test_sound_api_gtest.cpp
)

target_link_libraries(test-sound-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    Qt6::Multimedia
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-sound-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Unique ID API test (GoogleTest version)
add_executable(test-unique-id-api-gtest
    test_unique_id_api_gtest.cpp
)

target_link_libraries(test-unique-id-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-unique-id-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Bit library compatibility test (GoogleTest version)
add_executable(test-bit-library-gtest
    test_bit_library_gtest.cpp
)

target_link_libraries(test-bit-library-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-bit-library-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Progress library compatibility test (GoogleTest version)
add_executable(test-progress-library-gtest
    test_progress_library_gtest.cpp
)

target_link_libraries(test-progress-library-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-progress-library-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Utility string functions test (GoogleTest version)
add_executable(test-utility-string-functions-gtest
    test_utility_string_functions_gtest.cpp
)

target_link_libraries(test-utility-string-functions-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-utility-string-functions-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Utils hashing and encoding test (GoogleTest version)
add_executable(test-utils-hashing-gtest
    test_utils_hashing_gtest.cpp
)

target_link_libraries(test-utils-hashing-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-utils-hashing-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# MXP (MUD eXtension Protocol) test (GoogleTest version)
add_executable(test-mxp-gtest
    test_mxp_gtest.cpp
)

target_link_libraries(test-mxp-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-mxp-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# ========== Array API Tests ==========
add_executable(test-array-api-gtest
    test_array_api_gtest.cpp
)

target_link_libraries(test-array-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-array-api-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# ========== URL Detection Tests ==========
add_executable(test-url-detection-gtest
    test_url_detection_gtest.cpp
)

target_link_libraries(test-url-detection-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${QT_STATIC_PLUGINS}
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

gtest_discover_tests(test-url-detection-gtest
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)
