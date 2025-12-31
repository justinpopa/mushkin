# UI Module - All UI components (views, dialogs, main window)
#
# Uses view interfaces (IOutputView, IInputView) defined in world module
# to break circular dependencies while keeping all UI in one library.

add_library(ui
    main_window.h
    main_window.cpp
    # Views
    views/world_widget.h
    views/world_widget.cpp
    views/output_view.h
    views/output_view.cpp
    views/output_view_miniwindows.cpp
    views/input_view.h
    views/input_view.cpp
    # Widgets
    widgets/activity_window.h
    widgets/activity_window.cpp
    # Dialogs - World-specific
    dialogs/item_list_dialog_base.h
    dialogs/item_list_dialog_base.cpp
    dialogs/world_properties_dialog.h
    dialogs/world_properties_dialog.cpp
    dialogs/trigger_list_dialog.h
    dialogs/trigger_list_dialog.cpp
    dialogs/trigger_edit_dialog.h
    dialogs/trigger_edit_dialog.cpp
    dialogs/alias_list_dialog.h
    dialogs/alias_list_dialog.cpp
    dialogs/alias_edit_dialog.h
    dialogs/alias_edit_dialog.cpp
    dialogs/timer_list_dialog.h
    dialogs/timer_list_dialog.cpp
    dialogs/timer_edit_dialog.h
    dialogs/timer_edit_dialog.cpp
    dialogs/shortcut_list_dialog.h
    dialogs/shortcut_list_dialog.cpp
    dialogs/shortcut_edit_dialog.h
    dialogs/shortcut_edit_dialog.cpp
    dialogs/command_history_dialog.h
    dialogs/command_history_dialog.cpp
    dialogs/command_options_dialog.h
    dialogs/command_options_dialog.cpp
    dialogs/find_dialog.h
    dialogs/find_dialog.cpp
    dialogs/recall_search_dialog.h
    dialogs/recall_search_dialog.cpp
    dialogs/plugin_dialog.h
    dialogs/plugin_dialog.cpp
    dialogs/plugin_wizard.h
    dialogs/plugin_wizard.cpp
    dialogs/immediate_dialog.h
    dialogs/immediate_dialog.cpp
    dialogs/variable_edit_dialog.h
    dialogs/variable_edit_dialog.cpp
    dialogs/macro_edit_dialog.h
    dialogs/macro_edit_dialog.cpp
    dialogs/colour_picker_dialog.h
    dialogs/colour_picker_dialog.cpp
    dialogs/log_dialog.h
    dialogs/log_dialog.cpp
    dialogs/highlight_phrase_dialog.h
    dialogs/highlight_phrase_dialog.cpp
    dialogs/tab_defaults_dialog.h
    dialogs/tab_defaults_dialog.cpp
    dialogs/import_xml_dialog.h
    dialogs/import_xml_dialog.cpp
    dialogs/map_dialog.h
    dialogs/map_dialog.cpp
    dialogs/mxp_script_routines_dialog.h
    dialogs/mxp_script_routines_dialog.cpp
    # Dialogs - Generic utilities
    dialogs/global_preferences_dialog.h
    dialogs/global_preferences_dialog.cpp
    dialogs/complete_word_dialog.h
    dialogs/complete_word_dialog.cpp
    dialogs/debug_lua_dialog.h
    dialogs/debug_lua_dialog.cpp
    dialogs/debug_world_input_dialog.h
    dialogs/debug_world_input_dialog.cpp
    dialogs/function_list_dialog.h
    dialogs/function_list_dialog.cpp
    dialogs/recall_dialog.h
    dialogs/recall_dialog.cpp
    dialogs/generate_name_dialog.h
    dialogs/generate_name_dialog.cpp
    dialogs/generate_id_dialog.h
    dialogs/generate_id_dialog.cpp
    dialogs/insert_unicode_dialog.h
    dialogs/insert_unicode_dialog.cpp
    dialogs/edit_dialog.h
    dialogs/edit_dialog.cpp
    dialogs/multiline_edit_dialog.h
    dialogs/multiline_edit_dialog.cpp
    dialogs/send_to_all_dialog.h
    dialogs/send_to_all_dialog.cpp
    dialogs/script_error_dialog.h
    dialogs/script_error_dialog.cpp
    dialogs/missing_entry_points_dialog.h
    dialogs/missing_entry_points_dialog.cpp
    dialogs/lua_choose_box_dialog.h
    dialogs/lua_choose_box_dialog.cpp
    dialogs/quick_connect_dialog.h
    dialogs/quick_connect_dialog.cpp
    dialogs/password_dialog.h
    dialogs/password_dialog.cpp
    dialogs/go_to_line_dialog.h
    dialogs/go_to_line_dialog.cpp
    dialogs/lua_input_box_dialog.h
    dialogs/lua_input_box_dialog.cpp
    dialogs/lua_input_edit_dialog.h
    dialogs/lua_input_edit_dialog.cpp
    dialogs/regexp_problem_dialog.h
    dialogs/regexp_problem_dialog.cpp
    dialogs/lua_choose_list_dialog.h
    dialogs/lua_choose_list_dialog.cpp
    dialogs/global_change_dialog.h
    dialogs/global_change_dialog.cpp
    dialogs/lua_gsub_dialog.h
    dialogs/lua_gsub_dialog.cpp
    dialogs/ascii_art_dialog.h
    dialogs/ascii_art_dialog.cpp
    dialogs/choose_notepad_dialog.h
    dialogs/choose_notepad_dialog.cpp
    dialogs/lua_choose_list_multi_dialog.h
    dialogs/lua_choose_list_multi_dialog.cpp
    dialogs/progress_dialog.h
    dialogs/progress_dialog.cpp
    dialogs/key_name_dialog.h
    dialogs/key_name_dialog.cpp
    dialogs/confirm_preamble_dialog.h
    dialogs/confirm_preamble_dialog.cpp
    dialogs/text_attributes_dialog.h
    dialogs/text_attributes_dialog.cpp
    dialogs/multiline_trigger_dialog.h
    dialogs/multiline_trigger_dialog.cpp
    dialogs/map_comment_dialog.h
    dialogs/map_comment_dialog.cpp
    dialogs/map_move_dialog.h
    dialogs/map_move_dialog.cpp
    # Unified preferences dialog
    preferences/preferences_page_base.h
    preferences/preferences_page_base.cpp
    preferences/item_list_page_base.h
    preferences/item_list_page_base.cpp
    preferences/unified_preferences_dialog.h
    preferences/unified_preferences_dialog.cpp
    preferences/pages/stub_page.h
    preferences/pages/stub_page.cpp
    preferences/pages/triggers_page.h
    preferences/pages/triggers_page.cpp
    preferences/pages/aliases_page.h
    preferences/pages/aliases_page.cpp
    preferences/pages/timers_page.h
    preferences/pages/timers_page.cpp
    preferences/pages/connection_page.h
    preferences/pages/connection_page.cpp
    preferences/pages/output_page.h
    preferences/pages/output_page.cpp
    preferences/pages/logging_page.h
    preferences/pages/logging_page.cpp
    preferences/pages/scripting_page.h
    preferences/pages/scripting_page.cpp
    preferences/pages/input_page.h
    preferences/pages/input_page.cpp
    preferences/pages/paste_send_page.h
    preferences/pages/paste_send_page.cpp
    preferences/pages/macros_page.h
    preferences/pages/macros_page.cpp
    # Lua dialog callback registration
    lua_dialog_registration.h
    lua_dialog_registration.cpp
)

target_link_libraries(ui PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    world          # For WorldDocument, MiniWindow, view interfaces
    storage        # For Database class
    text           # For Line and Style classes
    utils          # For name generator utilities
    automation     # For Plugin class
)

# Allow other modules to find headers
target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
