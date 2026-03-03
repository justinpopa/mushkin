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

# ========== Shared Test Infrastructure ==========
# Object libraries providing main() so individual tests don't need their own.

add_library(test_main_core OBJECT test_main_core.cpp)
target_link_libraries(test_main_core PUBLIC GTest::gtest Qt6::Core)

add_library(test_main_gui OBJECT test_main_gui.cpp)
target_link_libraries(test_main_gui PUBLIC GTest::gtest Qt6::Core Qt6::Gui Qt6::Widgets)

# ========== GoogleTest Tests ==========
include(GoogleTest)

# Pipeline test
add_executable(test-pipeline-gtest
    test_pipeline_gtest.cpp
)

target_link_libraries(test-pipeline-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-pipeline-gtest
    DISCOVERY_TIMEOUT 30
)

# Trigger execution test
add_executable(test-trigger-execution-gtest
    test_trigger_execution_gtest.cpp
)

target_link_libraries(test-trigger-execution-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-trigger-execution-gtest
    DISCOVERY_TIMEOUT 30
)

# Timer structure test
add_executable(test-timer-structure-gtest
    test_timer_structure_gtest.cpp
)

target_link_libraries(test-timer-structure-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-timer-structure-gtest
    DISCOVERY_TIMEOUT 30
)

# Lua basics test
add_executable(test-lua-basics-gtest
    test_lua_basics_gtest.cpp
)

target_link_libraries(test-lua-basics-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-lua-basics-gtest
    DISCOVERY_TIMEOUT 30
)

# Note methods test
add_executable(test-note-methods-gtest
    test_note_methods_gtest.cpp
)

target_link_libraries(test-note-methods-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-note-methods-gtest
    DISCOVERY_TIMEOUT 30
)

# Script loading test
add_executable(test-script-loading-gtest
    test_script_loading_gtest.cpp
)

target_link_libraries(test-script-loading-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-script-loading-gtest
    DISCOVERY_TIMEOUT 30
)

# Lua API test
add_executable(test-lua-api-gtest
    test_lua_api_gtest.cpp
)

target_link_libraries(test-lua-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-lua-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Triggers and aliases data structures test
add_executable(test-triggers-aliases-gtest
    test_triggers_aliases_gtest.cpp
)

target_link_libraries(test-triggers-aliases-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-triggers-aliases-gtest
    DISCOVERY_TIMEOUT 30
)

# Timer fire calculation test
add_executable(test-timer-fire-calculation-gtest
    test_timer_fire_calculation_gtest.cpp
)

target_link_libraries(test-timer-fire-calculation-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-timer-fire-calculation-gtest
    DISCOVERY_TIMEOUT 30
)

# Timer evaluation test
add_executable(test-timer-evaluation-gtest
    test_timer_evaluation_gtest.cpp
)

target_link_libraries(test-timer-evaluation-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-timer-evaluation-gtest
    DISCOVERY_TIMEOUT 30
)

# Timer execution test
add_executable(test-timer-execution-gtest
    test_timer_execution_gtest.cpp
)

target_link_libraries(test-timer-execution-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-timer-execution-gtest
    DISCOVERY_TIMEOUT 30
)

# Timer Lua API test
add_executable(test-timer-api-gtest
    test_timer_api_gtest.cpp
)

target_link_libraries(test-timer-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-timer-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Alias execution test
add_executable(test-alias-execution-gtest
    test_alias_execution_gtest.cpp
)

target_link_libraries(test-alias-execution-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-alias-execution-gtest
    DISCOVERY_TIMEOUT 30
)

# Alias integration test
add_executable(test-alias-integration-gtest
    test_alias_integration_gtest.cpp
)

target_link_libraries(test-alias-integration-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-alias-integration-gtest
    DISCOVERY_TIMEOUT 30
)

# Speedwalk test
add_executable(test-speedwalk-gtest
    test_speedwalk_gtest.cpp
)

target_link_libraries(test-speedwalk-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-speedwalk-gtest
    DISCOVERY_TIMEOUT 30
)

# Command execution test
add_executable(test-command-execution-gtest
    test_command_execution_gtest.cpp
)

target_link_libraries(test-command-execution-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-command-execution-gtest
    DISCOVERY_TIMEOUT 30
)

# Command stacking test
add_executable(test-command-stacking-gtest
    test_command_stacking_gtest.cpp
)

target_link_libraries(test-command-stacking-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-command-stacking-gtest
    DISCOVERY_TIMEOUT 30
)

# Trigger matching test
add_executable(test-trigger-matching-gtest
    test_trigger_matching_gtest.cpp
)

target_link_libraries(test-trigger-matching-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-trigger-matching-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin API test
add_executable(test-plugin-api-gtest
    test_plugin_api_gtest.cpp
)

target_link_libraries(test-plugin-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Variable expansion test
add_executable(test-variable-expansion-gtest
    test_variable_expansion_gtest.cpp
)

target_link_libraries(test-variable-expansion-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-variable-expansion-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin callbacks test
add_executable(test-plugin-callbacks-gtest
    test_plugin_callbacks_gtest.cpp
)

target_link_libraries(test-plugin-callbacks-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-callbacks-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin error path test
add_executable(test-plugin-errors-gtest
    test_plugin_errors_gtest.cpp
)

target_link_libraries(test-plugin-errors-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-errors-gtest
    DISCOVERY_TIMEOUT 30
)

# Trigger/Alias Lua API test
add_executable(test-trigger-alias-api-gtest
    test_trigger_alias_api_gtest.cpp
)

target_link_libraries(test-trigger-alias-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-trigger-alias-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Command Window and Color Lua API test
add_executable(test-command-color-api-gtest
    test_command_color_api_gtest.cpp
)

target_link_libraries(test-command-color-api-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-command-color-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Notepad API test
add_executable(test-notepad-api-gtest
    test_notepad_api_gtest.cpp
)

target_link_libraries(test-notepad-api-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-notepad-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin evaluation order test
add_executable(test-plugin-evaluation-gtest
    test_plugin_evaluation_gtest.cpp
)

target_link_libraries(test-plugin-evaluation-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-evaluation-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin state saving test
add_executable(test-plugin-state-gtest
    test_plugin_state_gtest.cpp
)

target_link_libraries(test-plugin-state-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-state-gtest
    DISCOVERY_TIMEOUT 30
)

# Command history navigation test
add_executable(test-command-history-navigation-gtest
    test_command_history_navigation_gtest.cpp
)

target_link_libraries(test-command-history-navigation-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-command-history-navigation-gtest
    DISCOVERY_TIMEOUT 30
)

# Plugin serialization test
add_executable(test-plugin-serialization-gtest
    test_plugin_serialization_gtest.cpp
)

target_link_libraries(test-plugin-serialization-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-plugin-serialization-gtest
    DISCOVERY_TIMEOUT 30
)

# Text data classes test
add_executable(test-text-data-classes-gtest
    test_text_data_classes_gtest.cpp
)

target_link_libraries(test-text-data-classes-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-text-data-classes-gtest
    DISCOVERY_TIMEOUT 30
)

# Retrospective logging test
add_executable(test-retrospective-logging-gtest
    test_retrospective_logging_gtest.cpp
)

target_link_libraries(test-retrospective-logging-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-retrospective-logging-gtest
    DISCOVERY_TIMEOUT 30
)

# Tab completion test
add_executable(test-tab-completion-gtest
    test_tab_completion_gtest.cpp
)

target_link_libraries(test-tab-completion-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-tab-completion-gtest
    DISCOVERY_TIMEOUT 30
)

# Word wrap test
add_executable(test-word-wrap-gtest
    test_word_wrap_gtest.cpp
)

target_link_libraries(test-word-wrap-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-word-wrap-gtest
    DISCOVERY_TIMEOUT 30
)

# Line logging test
add_executable(test-line-logging-gtest
    test_line_logging_gtest.cpp
)

target_link_libraries(test-line-logging-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-line-logging-gtest
    DISCOVERY_TIMEOUT 30
)

# Database test
add_executable(test-database-gtest
    test_database_gtest.cpp
)

target_link_libraries(test-database-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    ui
)

gtest_discover_tests(test-database-gtest
    DISCOVERY_TIMEOUT 30
    PROPERTIES RESOURCE_LOCK database_singleton
)

# Lua Database API test
add_executable(test-lua-database-gtest
    test_lua_database_gtest.cpp
)

target_link_libraries(test-lua-database-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-lua-database-gtest
    DISCOVERY_TIMEOUT 30
)

# XML serialization test
add_executable(test-xml-serialization-gtest
    test_xml_serialization_gtest.cpp
)

target_link_libraries(test-xml-serialization-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-xml-serialization-gtest
    DISCOVERY_TIMEOUT 30
)

# Options tables test
add_executable(test-options-tables-gtest
    test_options_tables_gtest.cpp
)

target_link_libraries(test-options-tables-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-options-tables-gtest
    DISCOVERY_TIMEOUT 30
)

# XML roundtrip test
add_executable(test-xml-roundtrip-gtest
    test_xml_roundtrip_gtest.cpp
)

target_link_libraries(test-xml-roundtrip-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-xml-roundtrip-gtest
    DISCOVERY_TIMEOUT 30
)

# Miniwindow creation test (non-GoogleTest, uses Qt6::Test)
add_executable(test-miniwindow-creation
    test_miniwindow_creation.cpp
)

target_link_libraries(test-miniwindow-creation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    ui
)

# Miniwindow drawing test (non-GoogleTest, uses Qt6::Test)
add_executable(test-miniwindow-drawing
    test_miniwindow_drawing.cpp
)

target_link_libraries(test-miniwindow-drawing PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    ui
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

# SendTo integration test
add_executable(test-sendto-integration-gtest
    test_sendto_integration_gtest.cpp
)

target_link_libraries(test-sendto-integration-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-sendto-integration-gtest
    DISCOVERY_TIMEOUT 30
)

# Sound API test
add_executable(test-sound-api-gtest
    test_sound_api_gtest.cpp
)

target_link_libraries(test-sound-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    ui
)

gtest_discover_tests(test-sound-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Unique ID API test
add_executable(test-unique-id-api-gtest
    test_unique_id_api_gtest.cpp
)

target_link_libraries(test-unique-id-api-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-unique-id-api-gtest
    DISCOVERY_TIMEOUT 30
)

# Consolidated Lua libraries test (bit + lfs + progress)
add_executable(test-lua-libraries-gtest
    test_lua_libraries_gtest.cpp
)

target_link_libraries(test-lua-libraries-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-lua-libraries-gtest
    DISCOVERY_TIMEOUT 30
)

# Utility string functions test
add_executable(test-utility-string-functions-gtest
    test_utility_string_functions_gtest.cpp
)

target_link_libraries(test-utility-string-functions-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-utility-string-functions-gtest
    DISCOVERY_TIMEOUT 30
)

# Utils hashing and encoding test
add_executable(test-utils-hashing-gtest
    test_utils_hashing_gtest.cpp
)

target_link_libraries(test-utils-hashing-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-utils-hashing-gtest
    DISCOVERY_TIMEOUT 30
)

# MXP (MUD eXtension Protocol) test
add_executable(test-mxp-gtest
    test_mxp_gtest.cpp
)

target_link_libraries(test-mxp-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-mxp-gtest
    DISCOVERY_TIMEOUT 30
)

# Array API test
add_executable(test-array-api-gtest
    test_array_api_gtest.cpp
)

target_link_libraries(test-array-api-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-array-api-gtest
    DISCOVERY_TIMEOUT 30
)

# URL Detection test
add_executable(test-url-detection-gtest
    test_url_detection_gtest.cpp
)

target_link_libraries(test-url-detection-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-url-detection-gtest
    DISCOVERY_TIMEOUT 30
)

# Config Introspection test
add_executable(test-config-introspection-gtest
    test_config_introspection_gtest.cpp
)

target_link_libraries(test-config-introspection-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-config-introspection-gtest
    DISCOVERY_TIMEOUT 30
)

# DoCommand and GetInternalCommandsList Lua API test
add_executable(test-docommand-gtest
    test_docommand_gtest.cpp
)

target_link_libraries(test-docommand-gtest PRIVATE
    test_main_gui
    ui
)

gtest_discover_tests(test-docommand-gtest
    DISCOVERY_TIMEOUT 30
)

# SSH Host Key test
add_executable(test-ssh-host-key-gtest
    test_ssh_host_key_gtest.cpp
)

target_link_libraries(test-ssh-host-key-gtest PRIVATE
    test_main_core
    network
    Qt6::Core
)

gtest_discover_tests(test-ssh-host-key-gtest
    DISCOVERY_TIMEOUT 30
)

add_executable(test-telnet-parser-gtest
    test_telnet_parser_gtest.cpp
)

target_link_libraries(test-telnet-parser-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-telnet-parser-gtest
    DISCOVERY_TIMEOUT 30
)

add_executable(test-ansi-formatter-gtest
    test_ansi_formatter_gtest.cpp
)

target_link_libraries(test-ansi-formatter-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-ansi-formatter-gtest
    DISCOVERY_TIMEOUT 30
)

add_executable(test-lua-registration-gtest
    test_lua_registration_gtest.cpp
)

target_link_libraries(test-lua-registration-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-lua-registration-gtest
    DISCOVERY_TIMEOUT 30
)

# World output Lua API test
add_executable(test-world-output-gtest
    test_world_output_gtest.cpp
)

target_link_libraries(test-world-output-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-world-output-gtest
    DISCOVERY_TIMEOUT 30
)

# World protocol test
add_executable(test-world-protocol-gtest
    test_world_protocol_gtest.cpp
)

target_link_libraries(test-world-protocol-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-world-protocol-gtest
    DISCOVERY_TIMEOUT 30
)

# World serialization test
add_executable(test-world-serialization-gtest
    test_world_serialization_gtest.cpp
)

target_link_libraries(test-world-serialization-gtest PRIVATE
    test_main_core
    Qt6::Gui
    Qt6::Widgets
    ui
)

gtest_discover_tests(test-world-serialization-gtest
    DISCOVERY_TIMEOUT 30
)
