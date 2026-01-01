# Find Qt6::Test for QSignalSpy (used by miniwindow tests)
find_package(Qt6 COMPONENTS Test QUIET)

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
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

# Register with CTest for automatic test discovery
# Register tests with CTest (add_test avoids running binaries at configure time)
include(GoogleTest)
add_test(NAME test-pipeline-gtest COMMAND test-pipeline-gtest)

# Trigger execution test (GoogleTest version)
add_executable(test-trigger-execution-gtest
    test_trigger_execution_gtest.cpp
)

target_link_libraries(test-trigger-execution-gtest PRIVATE
    GTest::gtest              # GoogleTest library (without main, we provide our own)
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-trigger-execution-gtest COMMAND test-trigger-execution-gtest)

# Timer structure test (GoogleTest version)
add_executable(test-timer-structure-gtest
    test_timer_structure_gtest.cpp
)

target_link_libraries(test-timer-structure-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-timer-structure-gtest COMMAND test-timer-structure-gtest)

# Lua basics test (GoogleTest version)
add_executable(test-lua-basics-gtest
    test_lua_basics_gtest.cpp
)

target_link_libraries(test-lua-basics-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-lua-basics-gtest COMMAND test-lua-basics-gtest)

# Note methods test (GoogleTest version)
add_executable(test-note-methods-gtest
    test_note_methods_gtest.cpp
)

target_link_libraries(test-note-methods-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-note-methods-gtest COMMAND test-note-methods-gtest)

# Script loading test (GoogleTest version)
add_executable(test-script-loading-gtest
    test_script_loading_gtest.cpp
)

target_link_libraries(test-script-loading-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-script-loading-gtest COMMAND test-script-loading-gtest)

# Lua API test (GoogleTest version)
add_executable(test-lua-api-gtest
    test_lua_api_gtest.cpp
)

target_link_libraries(test-lua-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-lua-api-gtest COMMAND test-lua-api-gtest)

# Triggers and aliases data structures test (GoogleTest version)
add_executable(test-triggers-aliases-gtest
    test_triggers_aliases_gtest.cpp
)

target_link_libraries(test-triggers-aliases-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-triggers-aliases-gtest COMMAND test-triggers-aliases-gtest)

# Timer fire calculation test (GoogleTest version)
add_executable(test-timer-fire-calculation-gtest
    test_timer_fire_calculation_gtest.cpp
)

target_link_libraries(test-timer-fire-calculation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-timer-fire-calculation-gtest COMMAND test-timer-fire-calculation-gtest)

# Timer evaluation test (GoogleTest version)
add_executable(test-timer-evaluation-gtest
    test_timer_evaluation_gtest.cpp
)

target_link_libraries(test-timer-evaluation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-timer-evaluation-gtest COMMAND test-timer-evaluation-gtest)

# Timer execution test (GoogleTest version)
add_executable(test-timer-execution-gtest
    test_timer_execution_gtest.cpp
)

target_link_libraries(test-timer-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-timer-execution-gtest COMMAND test-timer-execution-gtest)

# Timer Lua API test (GoogleTest version)
add_executable(test-timer-api-gtest
    test_timer_api_gtest.cpp
)

target_link_libraries(test-timer-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-timer-api-gtest COMMAND test-timer-api-gtest)

# Alias execution test (GoogleTest version)
add_executable(test-alias-execution-gtest
    test_alias_execution_gtest.cpp
)

target_link_libraries(test-alias-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-alias-execution-gtest COMMAND test-alias-execution-gtest)

# Alias integration test (GoogleTest version)
add_executable(test-alias-integration-gtest
    test_alias_integration_gtest.cpp
)

target_link_libraries(test-alias-integration-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-alias-integration-gtest COMMAND test-alias-integration-gtest)

# Speedwalk test (GoogleTest version)
add_executable(test-speedwalk-gtest
    test_speedwalk_gtest.cpp
)

target_link_libraries(test-speedwalk-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-speedwalk-gtest COMMAND test-speedwalk-gtest)

# Command execution test (GoogleTest version)
add_executable(test-command-execution-gtest
    test_command_execution_gtest.cpp
)

target_link_libraries(test-command-execution-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-command-execution-gtest COMMAND test-command-execution-gtest)

# Command stacking test (GoogleTest version)
add_executable(test-command-stacking-gtest
    test_command_stacking_gtest.cpp
)

target_link_libraries(test-command-stacking-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-command-stacking-gtest COMMAND test-command-stacking-gtest)

# Trigger matching test (GoogleTest version)
add_executable(test-trigger-matching-gtest
    test_trigger_matching_gtest.cpp
)

target_link_libraries(test-trigger-matching-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-trigger-matching-gtest COMMAND test-trigger-matching-gtest)

# Plugin API test (GoogleTest version)
add_executable(test-plugin-api-gtest
    test_plugin_api_gtest.cpp
)

target_link_libraries(test-plugin-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-api-gtest COMMAND test-plugin-api-gtest)

# Variable expansion test (GoogleTest version)
add_executable(test-variable-expansion-gtest
    test_variable_expansion_gtest.cpp
)

target_link_libraries(test-variable-expansion-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-variable-expansion-gtest COMMAND test-variable-expansion-gtest)

# Plugin callbacks test (GoogleTest version)
add_executable(test-plugin-callbacks-gtest
    test_plugin_callbacks_gtest.cpp
)

target_link_libraries(test-plugin-callbacks-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-callbacks-gtest COMMAND test-plugin-callbacks-gtest)

# Plugin error path test (GoogleTest version)
add_executable(test-plugin-errors-gtest
    test_plugin_errors_gtest.cpp
)

target_link_libraries(test-plugin-errors-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-errors-gtest COMMAND test-plugin-errors-gtest)

# Trigger/Alias Lua API test (GoogleTest version)
add_executable(test-trigger-alias-api-gtest
    test_trigger_alias_api_gtest.cpp
)

target_link_libraries(test-trigger-alias-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-trigger-alias-api-gtest COMMAND test-trigger-alias-api-gtest)

# Command Window and Color Lua API test (GoogleTest version)
add_executable(test-command-color-api-gtest
    test_command_color_api_gtest.cpp
)

target_link_libraries(test-command-color-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-command-color-api-gtest COMMAND test-command-color-api-gtest)

# Notepad API test (GoogleTest version)
add_executable(test-notepad-api-gtest
    test_notepad_api_gtest.cpp
)

target_link_libraries(test-notepad-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-notepad-api-gtest COMMAND test-notepad-api-gtest)

# Plugin evaluation order test (GoogleTest version)
add_executable(test-plugin-evaluation-gtest
    test_plugin_evaluation_gtest.cpp
)

target_link_libraries(test-plugin-evaluation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-evaluation-gtest COMMAND test-plugin-evaluation-gtest)

# Plugin state saving test (GoogleTest version)
add_executable(test-plugin-state-gtest
    test_plugin_state_gtest.cpp
)

target_link_libraries(test-plugin-state-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-state-gtest COMMAND test-plugin-state-gtest)

# Command history navigation test (GoogleTest version)
add_executable(test-command-history-navigation-gtest
    test_command_history_navigation_gtest.cpp
)

target_link_libraries(test-command-history-navigation-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-command-history-navigation-gtest COMMAND test-command-history-navigation-gtest)

# Plugin serialization test (GoogleTest version)
add_executable(test-plugin-serialization-gtest
    test_plugin_serialization_gtest.cpp
)

target_link_libraries(test-plugin-serialization-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-plugin-serialization-gtest COMMAND test-plugin-serialization-gtest)

# Text data classes test (GoogleTest version)
add_executable(test-text-data-classes-gtest
    test_text_data_classes_gtest.cpp
)

target_link_libraries(test-text-data-classes-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-text-data-classes-gtest COMMAND test-text-data-classes-gtest)

# Retrospective logging test (GoogleTest version)
add_executable(test-retrospective-logging-gtest
    test_retrospective_logging_gtest.cpp
)

target_link_libraries(test-retrospective-logging-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-retrospective-logging-gtest COMMAND test-retrospective-logging-gtest)

# Tab completion test (GoogleTest version)
add_executable(test-tab-completion-gtest
    test_tab_completion_gtest.cpp
)

target_link_libraries(test-tab-completion-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-tab-completion-gtest COMMAND test-tab-completion-gtest)

# Word wrap test (GoogleTest version)
add_executable(test-word-wrap-gtest
    test_word_wrap_gtest.cpp
)

target_link_libraries(test-word-wrap-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-word-wrap-gtest COMMAND test-word-wrap-gtest)

# Line logging test (GoogleTest version)
add_executable(test-line-logging-gtest
    test_line_logging_gtest.cpp
)

target_link_libraries(test-line-logging-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-line-logging-gtest COMMAND test-line-logging-gtest)

# Database test (GoogleTest version)
add_executable(test-database-gtest
    test_database_gtest.cpp
)

target_link_libraries(test-database-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    ui
)

add_test(NAME test-database-gtest COMMAND test-database-gtest)

# Lua Database API test (GoogleTest version)
add_executable(test-lua-database-gtest
    test_lua_database_gtest.cpp
)

target_link_libraries(test-lua-database-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-lua-database-gtest COMMAND test-lua-database-gtest)

# XML serialization test (GoogleTest version)
add_executable(test-xml-serialization-gtest
    test_xml_serialization_gtest.cpp
)

target_link_libraries(test-xml-serialization-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-xml-serialization-gtest COMMAND test-xml-serialization-gtest)

# Options tables test (GoogleTest version)
add_executable(test-options-tables-gtest
    test_options_tables_gtest.cpp
)

target_link_libraries(test-options-tables-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-options-tables-gtest COMMAND test-options-tables-gtest)


# XML roundtrip test (GoogleTest version)
add_executable(test-xml-roundtrip-gtest
    test_xml_roundtrip_gtest.cpp
)

target_link_libraries(test-xml-roundtrip-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui
)

add_test(NAME test-xml-roundtrip-gtest COMMAND test-xml-roundtrip-gtest)

# Miniwindow creation test (doesn't require Catch2)
add_executable(test-miniwindow-creation
    test_miniwindow_creation.cpp
)

target_link_libraries(test-miniwindow-creation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
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
add_test(NAME test-miniwindow-creation COMMAND test-miniwindow-creation)
add_test(NAME test-miniwindow-drawing COMMAND test-miniwindow-drawing)

# SendTo integration test (GoogleTest version)
add_executable(test-sendto-integration-gtest
    test_sendto_integration_gtest.cpp
)

target_link_libraries(test-sendto-integration-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-sendto-integration-gtest COMMAND test-sendto-integration-gtest)

# Sound API test (GoogleTest version)
add_executable(test-sound-api-gtest
    test_sound_api_gtest.cpp
)

target_link_libraries(test-sound-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-sound-api-gtest COMMAND test-sound-api-gtest)

# Unique ID API test (GoogleTest version)
add_executable(test-unique-id-api-gtest
    test_unique_id_api_gtest.cpp
)

target_link_libraries(test-unique-id-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-unique-id-api-gtest COMMAND test-unique-id-api-gtest)

# Bit library compatibility test (GoogleTest version)
add_executable(test-bit-library-gtest
    test_bit_library_gtest.cpp
)

target_link_libraries(test-bit-library-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-bit-library-gtest COMMAND test-bit-library-gtest)

# Progress library compatibility test (GoogleTest version)
add_executable(test-progress-library-gtest
    test_progress_library_gtest.cpp
)

target_link_libraries(test-progress-library-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-progress-library-gtest COMMAND test-progress-library-gtest)

# Utility string functions test (GoogleTest version)
add_executable(test-utility-string-functions-gtest
    test_utility_string_functions_gtest.cpp
)

target_link_libraries(test-utility-string-functions-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-utility-string-functions-gtest COMMAND test-utility-string-functions-gtest)

# Utils hashing and encoding test (GoogleTest version)
add_executable(test-utils-hashing-gtest
    test_utils_hashing_gtest.cpp
)

target_link_libraries(test-utils-hashing-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-utils-hashing-gtest COMMAND test-utils-hashing-gtest)

# MXP (MUD eXtension Protocol) test (GoogleTest version)
add_executable(test-mxp-gtest
    test_mxp_gtest.cpp
)

target_link_libraries(test-mxp-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-mxp-gtest COMMAND test-mxp-gtest)

# ========== Array API Tests ==========
add_executable(test-array-api-gtest
    test_array_api_gtest.cpp
)

target_link_libraries(test-array-api-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-array-api-gtest COMMAND test-array-api-gtest)

# ========== URL Detection Tests ==========
add_executable(test-url-detection-gtest
    test_url_detection_gtest.cpp
)

target_link_libraries(test-url-detection-gtest PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ui          # Transitively brings: world (with Lua API), text, storage, automation, utils, network
)

add_test(NAME test-url-detection-gtest COMMAND test-url-detection-gtest)
