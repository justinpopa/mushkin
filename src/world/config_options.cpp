// config_options.cpp
// Options tables for world serialization
//
// These tables provide metadata for all ~240 configurable world options.
// The offsetof() macro calculates the byte offset of each field in WorldDocument,
// allowing generic serialization code to read/write any option without switch statements.
//
// Ported from: scriptingoptions.cpp (original MUSHclient)

#include "config_options.h"
#include "../automation/sendto.h"
#include "world_document.h"
#include <QColor>
#include <algorithm>
#include <climits>
#include <string>
#include <type_traits>
#include <unordered_map>

// ============================================================================
// HELPER MACROS FOR TABLE DEFINITIONS
// ============================================================================

// O(field) expands to a getter lambda and a setter lambda for numeric fields.
// The getter casts the field to double; the setter casts double back to the exact field type.
#define O(field)                                                                                   \
    +[](const WorldDocument& d) -> double { return static_cast<double>(d.field); },                \
        +[](WorldDocument& d, double v) {                                                          \
            d.field = static_cast<std::remove_cvref_t<decltype(d.field)>>(v);                      \
        }

// A(field) expands to a getter lambda and a setter lambda for QString fields.
// The getter returns a const reference; the setter assigns by value.
#define A(field)                                                                                   \
    +[](const WorldDocument& d) -> const QString& { return d.field; },                             \
        +[](WorldDocument& d, const QString& v) { d.field = v; }

// ============================================================================
// CONSTANTS & ENUMS (from original MUSHclient)
// ============================================================================
// NOTE: These will need proper definitions once we port the relevant subsystems.
// For now, using placeholder values to allow compilation.

// RGB helper (Windows style: 0x00BBGGRR) — replaces the original Windows RGB() macro
constexpr unsigned int makeRgb(int r, int g, int b)
{
    return static_cast<unsigned int>((b) << 16 | (g) << 8 | (r));
}
// Convenience macro so existing table entries (RGB(r,g,b)) continue to compile unchanged
#define RGB(r, g, b) makeRgb((r), (g), (b))

// Font weights
inline constexpr int FW_DONTCARE = 0;

// Character sets
inline constexpr int DEFAULT_CHARSET = 1;

// Color limits (MAX_CUSTOM is already defined in world_document.h)

// Port limits
#ifndef USHRT_MAX
#    define USHRT_MAX 65535
#endif

// Line width
inline constexpr int MAX_LINE_WIDTH = 32000;

// Connect methods (already defined in world_document.h)
inline constexpr int eConnectTypeMax = 4;

// SendTo enum now in automation/sendto.h

// Default sequences
inline constexpr int DEFAULT_TRIGGER_SEQUENCE = 100;
inline constexpr int DEFAULT_ALIAS_SEQUENCE = 100;

// Debug levels
enum { DBG_NONE = 0 };

// Reload options - see ScriptReloadOption enum in world_document.h
// eReloadConfirm = 0, eReloadAlways = 1, eReloadNever = 2

// MXP modes
enum { eNoMXP = 0, eUseMXP = 1, eOnCommandMXP = 2 };

// Sound literal
inline constexpr const char* NOSOUNDLIT = "";

// Plugin ID length
inline constexpr int PLUGIN_UNIQUE_ID_LENGTH = 24;

// ============================================================================
// NUMERIC OPTIONS TABLE (173 entries)
// ============================================================================
// Format: { name, default_value, getter, setter, min, max, flags }
// Note: If min==0 and max==0, the option is treated as boolean

const tConfigurationNumericOption OptionsTable[] = {
    {"alternative_inverse", false, O(m_bAlternativeInverse), 0, 0, 0},
    {"alt_arrow_recalls_partial", false, O(m_input.alt_arrow_recalls_partial), 0, 0, 0},
    {"always_record_command_history", false, O(m_bAlwaysRecordCommandHistory), 0, 0, 0},
    {"arrows_change_history", true, O(m_input.arrows_change_history), 0, 0, 0},
    {"arrow_keys_wrap", false, O(m_command_window.arrow_keys_wrap), 0, 0, 0},
    {"arrow_recalls_partial", false, O(m_input.arrow_recalls_partial), 0, 0, 0},
    {"autosay_exclude_macros", false, O(m_auto_say.exclude_macros), 0, 0, 0},
    {"autosay_exclude_non_alpha", false, O(m_auto_say.exclude_non_alpha), 0, 0, 0},
    {"auto_copy_to_clipboard_in_html", false, O(m_display.auto_copy_in_html), 0, 0, 0},
    {"auto_pause", true, O(m_display.auto_freeze), 0, 0, 0},
    {"keep_pause_at_bottom", false, O(m_display.keep_freeze_at_bottom), 0, 0, 0},
    {"auto_repeat", false, O(m_bAutoRepeat), 0, 0, 0},
    {"auto_resize_command_window", false, O(m_command_window.auto_resize), 0, 0, 0},
    {"auto_resize_minimum_lines", 1, O(m_command_window.minimum_lines), 1, 100, 0},
    {"auto_resize_maximum_lines", 20, O(m_command_window.maximum_lines), 1, 100, 0},
    {"append_to_log_file", true, O(m_logging.append_to_log_file), 0, 0, 0},
    {"auto_wrap_window_width", false, O(m_bAutoWrapWindowWidth), 0, 0, 0},
    {"carriage_return_clears_line", false, O(m_spam.carriage_return_clears_line), 0, 0, 0},
    {"confirm_before_replacing_typing", true, O(m_auto_say.confirm_before_replacing), 0, 0, 0},
    {"confirm_on_paste", true, O(m_paste.confirm_on_paste), 0, 0, 0},
    {"confirm_on_send", true, O(m_bConfirmOnSend), 0, 0, 0},
    {"connect_method", 0 /*AutoConnect::eNoAutoConnect*/, O(m_connect_now), 0, eConnectTypeMax - 1,
     0},
    {"copy_selection_to_clipboard", false, O(m_display.copy_selection_to_clipboard), 0, 0, 0},
    {"convert_ga_to_newline", false, O(m_spam.convert_ga_to_newline), 0, 0, 0},
    {"ctrl_n_goes_to_next_command", false, O(m_bCtrlNGoesToNextCommand), 0, 0, 0},
    {"ctrl_p_goes_to_previous_command", false, O(m_bCtrlPGoesToPreviousCommand), 0, 0, 0},
    {"ctrl_z_goes_to_end_of_buffer", false, O(m_bCtrlZGoesToEndOfBuffer), 0, 0, 0},
    {"ctrl_backspace_deletes_last_word", false, O(m_bCtrlBackspaceDeletesLastWord), 0, 0, 0},
    {"custom_16_is_default_colour", false, O(m_bCustom16isDefaultColour), 0, 0, OPT_UPDATE_VIEWS},

    {"default_trigger_send_to", eSendToWorld, O(m_automation_defaults.trigger_send_to),
     eSendToWorld, eSendToLast - 1, 0},
    {"default_trigger_sequence", DEFAULT_TRIGGER_SEQUENCE,
     O(m_automation_defaults.trigger_sequence), 0, 10000, 0},
    {"default_trigger_regexp", false, O(m_automation_defaults.trigger_regexp), 0, 0, 0},
    {"default_trigger_expand_variables", false, O(m_automation_defaults.trigger_expand_variables),
     0, 0, 0},
    {"default_trigger_keep_evaluating", false, O(m_automation_defaults.trigger_keep_evaluating), 0,
     0, 0},
    {"default_trigger_ignore_case", false, O(m_automation_defaults.trigger_ignore_case), 0, 0, 0},

    {"default_alias_send_to", eSendToWorld, O(m_automation_defaults.alias_send_to), eSendToWorld,
     eSendToLast - 1, 0},
    {"default_alias_sequence", DEFAULT_ALIAS_SEQUENCE, O(m_automation_defaults.alias_sequence), 0,
     10000, 0},
    {"default_alias_regexp", false, O(m_automation_defaults.alias_regexp), 0, 0, 0},
    {"default_alias_expand_variables", false, O(m_automation_defaults.alias_expand_variables), 0, 0,
     0},
    {"default_alias_keep_evaluating", false, O(m_automation_defaults.alias_keep_evaluating), 0, 0,
     0},
    {"default_alias_ignore_case", false, O(m_automation_defaults.alias_ignore_case), 0, 0, 0},

    {"default_timer_send_to", eSendToWorld, O(m_automation_defaults.timer_send_to), eSendToWorld,
     eSendToLast - 1, 0},

    {"detect_pueblo", true, O(m_bPueblo), 0, 0, 0},
    {"do_not_add_macros_to_command_history", false, O(m_command_window.no_macros_in_history), 0, 0,
     0},
    {"do_not_show_outstanding_lines", false, O(m_display.do_not_show_outstanding_lines), 0, 0, 0},
    {"do_not_translate_iac_to_iac_iac", false, O(m_spam.do_not_translate_iac), 0, 0, 0},
    {"disable_compression", false, O(m_bDisableCompression), 0, 0, 0},
    {"display_my_input", true, O(m_display_my_input), 0, 0, 0},
    {"double_click_inserts", false, O(m_display.double_click_inserts), 0, 0, 0},
    {"double_click_sends", false, O(m_input.double_click_sends), 0, 0, 0},
    {"echo_colour", 0, O(m_output.echo_colour), 0, MAX_CUSTOM,
     OPT_CUSTOM_COLOUR | OPT_UPDATE_VIEWS},
    {"echo_hyperlink_in_output_window", true, O(m_output.echo_hyperlink_in_output_window), 0, 0, 0},
    {"edit_script_with_notepad", true, O(m_scripting.edit_with_notepad), 0, 0, 0},
    {"enable_aliases", true, O(m_enable_aliases), 0, 0, 0},
    {"enable_auto_say", false, O(m_auto_say.enabled), 0, 0, 0},
    {"enable_beeps", true, O(m_sound.enable_beeps), 0, 0, 0},
    {"enable_command_stack", false, O(m_input.enable_command_stack), 0, 0, 0},
    {"enable_scripts", true, O(m_scripting.enabled), 0, 0, 0},
    {"enable_spam_prevention", false, O(m_spam.enabled), 0, 0, 0},
    {"enable_speed_walk", false, O(m_speedwalk.enabled), 0, 0, 0},
    {"enable_timers", true, O(m_bEnableTimers), 0, 0, 0},
    {"enable_triggers", true, O(m_enable_triggers), 0, 0, 0},
    {"enable_trigger_sounds", true, O(m_sound.enable_trigger_sounds), 0, 0, 0},
    {"escape_deletes_input", false, O(m_input.escape_deletes_input), 0, 0, 0},
    {"fade_output_buffer_after_seconds", 0, O(m_output.fade_after_seconds), 0, 3600,
     OPT_UPDATE_VIEWS},
    {"fade_output_opacity_percent", 20, O(m_output.fade_opacity_percent), 0, 100, OPT_UPDATE_VIEWS},
    {"fade_output_seconds", 8, O(m_output.fade_seconds), 1, 60, OPT_UPDATE_VIEWS},
    {"flash_taskbar_icon", false, O(m_display.flash_icon), 0, 0, 0},
    {"history_lines", 1000, O(m_input.history_lines), 20, 5000, 0},
    {"hyperlink_adds_to_command_history", true, O(m_bHyperlinkAddsToCommandHistory), 0, 0, 0},
    {"hyperlink_colour", RGB(0, 128, 255), O(m_colors.hyperlink_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"ignore_mxp_colour_changes", false, O(m_bIgnoreMXPcolourChanges), 0, 0, 0},
    {"indent_paras", true, O(m_display.indent_paras), 0, 0, 0},
    {"input_background_colour", RGB(255, 255, 255), O(m_input.background_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"input_font_height", 12, O(m_input.font_height), 1, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_italic", false, O(m_input.font_italic), 0, 0,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_weight", FW_DONTCARE, O(m_input.font_weight), 0, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_charset", DEFAULT_CHARSET, O(m_input.font_charset), 0, 65536,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_text_colour", RGB(0, 0, 0), O(m_input.text_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"keep_commands_on_same_line", false, O(m_display.keep_commands_on_same_line), 0, 0, 0},
    {"keypad_enable", true, O(m_keypad_enable), 0, 0, 0},
    {"line_information", true, O(m_display.line_information), 0, 0, 0},
    {"line_spacing", 0, O(m_display.line_spacing), 0, 100,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"log_html", false, O(m_logging.log_html), 0, 0, 0},
    {"log_input", false, O(m_logging.log_input), 0, 0, 0},
    {"log_in_colour", false, O(m_logging.log_in_colour), 0, 0, 0},
    {"log_lines", 0, O(m_logging.log_lines), 0, 500000, 0},
    {"log_notes", false, O(m_logging.log_notes), 0, 0, 0},
    {"log_output", false, O(m_logging.log_output), 0, 0, 0},
    {"log_raw", false, O(m_logging.log_raw), 0, 0, 0},
    {"log_script_errors", false, O(m_logging.log_script_errors), 0, 0, 0},
    {"lower_case_tab_completion", false, O(m_command_window.lower_case_tab_completion), 0, 0, 0},
    {"map_failure_regexp", false, O(m_mapping.failure_regexp), 0, 0, 0},
    {"max_output_lines", 5000, O(m_display.max_lines), 200, 500000, OPT_FIX_OUTPUT_BUFFER},
    {"mud_can_change_link_colour", true, O(m_bMudCanChangeLinkColour), 0, 0, OPT_SERVER_CAN_WRITE},
    {"mud_can_remove_underline", false, O(m_bMudCanRemoveUnderline), 0, 0, OPT_SERVER_CAN_WRITE},
    {"mud_can_change_options", true, O(m_bMudCanChangeOptions), 0, 0, 0},
    {"mxp_debug_level", DBG_NONE, O(m_iMXPdebugLevel), 0, 4, 0},
    {"naws", false, O(m_bNAWS), 0, 0, 0},
    {"use_msp", false, O(m_sound.use_msp), 0, 0, 0},
    {"note_text_colour", 4, O(m_colors.note_text_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"no_echo_off", false, O(m_input.no_echo_off), 0, 0, 0},
    {"omit_date_from_save_files", false, O(m_bOmitSavedDateFromSaveFiles), 0, 0, 0},
    {"output_font_height", 12, O(m_output.font_height), 1, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"output_font_weight", FW_DONTCARE, O(m_output.font_weight), 0, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"output_font_charset", DEFAULT_CHARSET, O(m_output.font_charset), 0, 65536,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"paste_commented_softcode", false, O(m_paste.paste_commented_softcode), 0, 0, 0},
    {"paste_delay", 0, O(m_paste.paste_delay), 0, 100000, 0},
    {"paste_delay_per_lines", 1, O(m_paste.paste_delay_per_lines), 1, 100000, 0},
    {"paste_echo", false, O(m_paste.paste_echo), 0, 0, 0},
    {"play_sounds_in_background", false, O(m_sound.play_in_background), 0, 0, 0},
    {"pixel_offset", 1, O(m_display.pixel_offset), 0, 20, OPT_UPDATE_VIEWS | OPT_FIX_INPUT_WRAP},
    {"port", 4000, O(m_port), 1, USHRT_MAX, OPT_PLUGIN_CANNOT_WRITE},
    {"proxy_port", 0, O(m_proxy.port), 0, 65535, 0},
    {"proxy_type", 0, O(m_proxy.type), 0, 2, 0},
    {"re_evaluate_auto_say", false, O(m_auto_say.re_evaluate), 0, 0, 0},
    {"save_deleted_command", false, O(m_input.save_deleted_command), 0, 0, 0},
    {"save_world_automatically", false, O(m_bSaveWorldAutomatically), 0, 0, 0},
    {"script_reload_option", 0, O(m_scripting.reload_option), 0, 2,
     0}, // 0 = eReloadConfirm (default)
    {"script_errors_to_output_window", false, O(m_scripting.errors_to_output_window), 0, 0, 0},
    {"send_echo", false, O(m_input.send_echo), 0, 0, 0},
    {"send_file_commented_softcode", false, O(m_paste.file_commented_softcode), 0, 0, 0},
    {"send_file_delay", 0, O(m_paste.file_delay), 0, 100000, 0},
    {"send_file_delay_per_lines", 1, O(m_paste.file_delay_per_lines), 1, 100000, 0},
    {"send_keep_alives", 0, O(m_bSendKeepAlives), 0, 0, 0},
    {"send_mxp_afk_response", true, O(m_bSendMXP_AFK_Response), 0, 0, 0},
    {"show_bold", true, O(m_display.show_bold), 0, 0, OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"show_connect_disconnect", true, O(m_bShowConnectDisconnect), 0, 0, 0},
    {"show_italic", true, O(m_display.show_italic), 0, 0,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"show_underline", true, O(m_display.show_underline), 0, 0,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"spam_line_count", 20, O(m_spam.line_count), 5, 500, 0},
    {"speed_walk_delay", 0, O(m_speedwalk.delay), 0, 30000, OPT_FIX_SPEEDWALK_DELAY},
    {"spell_check_on_send", false, O(m_command_window.spell_check_on_send), 0, 0, 0},
    {"start_paused", false, O(m_bStartPaused), 0, 0, 0},
    {"tab_completion_lines", 200, O(m_command_window.tab_completion_lines), 1, 500000, 0},
    {"tab_completion_space", false, O(m_command_window.tab_completion_space), 0, 0, 0},
    {"timestamp_input_text_colour", RGB(128, 0, 0), O(m_output.preamble_input_text_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_notes_text_colour", RGB(0, 0, 255), O(m_output.preamble_notes_text_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_output_text_colour", RGB(255, 255, 255), O(m_output.preamble_output_text_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_input_back_colour", RGB(0, 0, 0), O(m_output.preamble_input_back_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_notes_back_colour", RGB(0, 0, 0), O(m_output.preamble_notes_back_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_output_back_colour", RGB(0, 0, 0), O(m_output.preamble_output_back_colour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"tool_tip_visible_time", 5000, O(m_iToolTipVisibleTime), 0, 120000, OPT_FIX_TOOLTIP_VISIBLE},
    {"tool_tip_start_time", 400, O(m_iToolTipStartTime), 0, 120000, OPT_FIX_TOOLTIP_START},
    {"translate_backslash_sequences", false, O(m_bTranslateBackslashSequences), 0, 0, 0},
    {"translate_german", false, O(m_bTranslateGerman), 0, 0, 0},
    {"treeview_triggers", true, O(m_bTreeviewTriggers), 0, 0, 0},
    {"treeview_aliases", true, O(m_bTreeviewAliases), 0, 0, 0},
    {"treeview_timers", true, O(m_bTreeviewTimers), 0, 0, 0},

    {"underline_hyperlinks", true, O(m_bUnderlineHyperlinks), 0, 0, OPT_SERVER_CAN_WRITE},
    {"unpause_on_send", true, O(m_bUnpauseOnSend), 0, 0, 0},
    {"use_custom_link_colour", true, O(m_bUseCustomLinkColour), 0, 0, OPT_SERVER_CAN_WRITE},
    {"use_default_aliases", false, O(m_bUseDefaultAliases), 0, 0, 0},
    {"use_default_colours", false, O(m_colors.use_default_colours), 0, 0, 0},
    {"use_default_input_font", false, O(m_bUseDefaultInputFont), 0, 0, 0},
    {"use_default_macros", false, O(m_bUseDefaultMacros), 0, 0, 0},
    {"use_default_output_font", false, O(m_bUseDefaultOutputFont), 0, 0, 0},
    {"use_default_timers", false, O(m_bUseDefaultTimers), 0, 0, 0},
    {"use_default_triggers", false, O(m_bUseDefaultTriggers), 0, 0, 0},
    {"use_mxp", eOnCommandMXP, O(m_iUseMXP), 0, 3, OPT_USE_MXP},
    {"utf_8", false, O(m_display.utf8), 0, 0, 0},
    {"warn_if_scripting_inactive", true, O(m_scripting.warn_if_inactive), 0, 0, 0},
    {"wrap", true, O(m_display.wrap), 0, 0, 0},
    {"wrap_input", false, O(m_input.auto_wrap), 0, 0, OPT_FIX_INPUT_WRAP},
    {"wrap_column", 80, O(m_display.wrap_column), 20, MAX_LINE_WIDTH,
     OPT_FIX_WRAP_COLUMN | OPT_FIX_INPUT_WRAP},

    {"write_world_name_to_log", true, O(m_logging.write_world_name), 0, 0, 0},

    // Remote Access Server settings
    {"remote_access_enabled", false, O(m_remote.enabled), 0, 0, 0},
    {"remote_port", 0, O(m_remote.port), 0, 65535, 0},
    {"remote_scrollback_lines", 100, O(m_remote.scrollback_lines), 0, 10000, 0},
    {"remote_max_clients", 5, O(m_remote.max_clients), 1, 100, 0},

    {nullptr} // NULL sentinel - end of table marker
};

// ============================================================================
// ALPHA (STRING) OPTIONS TABLE (67 entries)
// ============================================================================
// Format: { name, default_value, getter, setter, flags }

const tConfigurationAlphaOption AlphaOptionsTable[] = {
    {"auto_log_file_name", "", A(m_logging.auto_log_file_name)},
    {"auto_say_override_prefix", "-", A(m_auto_say.override_prefix)},
    {"auto_say_string", "say ", A(m_auto_say.say_string), OPT_KEEP_SPACES},
    {"beep_sound", "", A(m_sound.beep_sound)},
    {"command_stack_character", ";", A(m_input.command_stack_character), OPT_COMMAND_STACK},
    {"connect_text", "", A(m_connect_text), OPT_MULTLINE},
    {"editor_window_name", "", A(m_strEditorWindowName)},
    {"filter_aliases", "", A(m_scripting.aliases_filter), OPT_MULTLINE},
    {"filter_timers", "", A(m_scripting.timers_filter), OPT_MULTLINE},
    {"filter_triggers", "", A(m_scripting.triggers_filter), OPT_MULTLINE},
    {"filter_variables", "", A(m_scripting.variables_filter), OPT_MULTLINE},
    {"id", "", A(m_strWorldID), OPT_WORLD_ID},
    {"input_font_name", "FixedSys", A(m_input.font_name), OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"log_file_postamble", "", A(m_logging.file_postamble), OPT_MULTLINE},
    {"log_file_preamble", "", A(m_logging.file_preamble), OPT_MULTLINE},
    {"log_line_postamble_input", "", A(m_logging.line_postamble_input), OPT_KEEP_SPACES},
    {"log_line_postamble_notes", "", A(m_logging.line_postamble_notes), OPT_KEEP_SPACES},
    {"log_line_postamble_output", "", A(m_logging.line_postamble_output), OPT_KEEP_SPACES},
    {"log_line_preamble_input", "", A(m_logging.line_preamble_input), OPT_KEEP_SPACES},
    {"log_line_preamble_notes", "", A(m_logging.line_preamble_notes), OPT_KEEP_SPACES},
    {"log_line_preamble_output", "", A(m_logging.line_preamble_output), OPT_KEEP_SPACES},
    {"mapping_failure", "Alas, you cannot go that way.", A(m_mapping.failure_message),
     OPT_KEEP_SPACES},
    {"name", "", A(m_mush_name), OPT_PLUGIN_CANNOT_WRITE},
    {"new_activity_sound", NOSOUNDLIT, A(m_sound.new_activity_sound)},
    {"notes", "", A(m_notes), OPT_MULTLINE},
    {"on_mxp_close_tag", "", A(m_strOnMXP_CloseTag)},
    {"on_mxp_error", "", A(m_strOnMXP_Error)},
    {"on_mxp_open_tag", "", A(m_strOnMXP_OpenTag)},
    {"on_mxp_set_variable", "", A(m_strOnMXP_SetVariable)},
    {"on_mxp_start", "", A(m_strOnMXP_Start)},
    {"on_mxp_stop", "", A(m_strOnMXP_Stop)},
    {"on_world_close", "", A(m_scripting.on_world_close)},
    {"on_world_save", "", A(m_scripting.on_world_save)},
    {"on_world_connect", "", A(m_scripting.on_world_connect)},
    {"on_world_disconnect", "", A(m_scripting.on_world_disconnect)},
    {"on_world_get_focus", "", A(m_scripting.on_world_get_focus)},
    {"on_world_lose_focus", "", A(m_scripting.on_world_lose_focus)},
    {"on_world_open", "", A(m_scripting.on_world_open)},
    {"output_font_name", "FixedSys", A(m_output.font_name), OPT_UPDATE_OUTPUT_FONT},
    {"password", "", A(m_password), OPT_PASSWORD | OPT_PLUGIN_CANNOT_RW},
    {"proxy_password", "", A(m_proxy.password), OPT_PASSWORD | OPT_PLUGIN_CANNOT_RW},
    {"proxy_server", "", A(m_proxy.server), OPT_PLUGIN_CANNOT_WRITE},
    {"proxy_username", "", A(m_proxy.username), OPT_PLUGIN_CANNOT_WRITE},
    {"paste_line_postamble", "", A(m_paste.pasteline_postamble), OPT_KEEP_SPACES},
    {"paste_line_preamble", "", A(m_paste.pasteline_preamble), OPT_KEEP_SPACES},
    {"paste_postamble", "", A(m_paste.paste_postamble), OPT_MULTLINE},
    {"paste_preamble", "", A(m_paste.paste_preamble), OPT_MULTLINE},
    {"player", "", A(m_name), OPT_PLUGIN_CANNOT_WRITE},
    {"recall_line_preamble", "", A(m_output.recall_line_preamble), OPT_KEEP_SPACES},
    {"script_editor", "notepad", A(m_scripting.editor)},
    {"script_editor_argument", "%file", A(m_scripting.editor_argument)},
    {"script_filename", "", A(m_scripting.filename)},
    {"script_language", "lua", A(m_scripting.language)},
    {"script_prefix", "", A(m_scripting.prefix)},
    {"send_to_world_file_postamble", "", A(m_paste.file_postamble), OPT_MULTLINE},
    {"send_to_world_file_preamble", "", A(m_paste.file_preamble), OPT_MULTLINE},
    {"send_to_world_line_postamble", "", A(m_paste.line_postamble)},
    {"send_to_world_line_preamble", "", A(m_paste.line_preamble)},
    {"site", "", A(m_server), OPT_PLUGIN_CANNOT_WRITE},
    {"spam_message", "look", A(m_spam.message)},
    {"speed_walk_filler", "", A(m_speedwalk.filler), OPT_KEEP_SPACES},
    {"speed_walk_prefix", "#", A(m_speedwalk.prefix), OPT_KEEP_SPACES},
    {"tab_completion_defaults", "", A(m_command_window.tab_completion_defaults), OPT_MULTLINE},
    {"terminal_identification", "mushkin", A(m_strTerminalIdentification)},
    {"timestamp_input", "", A(m_output.preamble_input), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},
    {"timestamp_notes", "", A(m_output.preamble_notes), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},
    {"timestamp_output", "", A(m_output.preamble_output), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},

    // Remote Access Server settings
    {"remote_password", "", A(m_remote.password), OPT_PASSWORD | OPT_PLUGIN_CANNOT_RW},
    {"remote_authorized_keys", "", A(m_remote.authorized_keys_file), OPT_PLUGIN_CANNOT_RW},

    {nullptr} // NULL sentinel - end of table marker
};

// ============================================================================
// O(1) LOOKUP MAPS
// ============================================================================

const std::unordered_map<std::string, int>& getNumericOptionMap()
{
    static const auto map = []() {
        std::unordered_map<std::string, int> m;
        for (int i = 0; OptionsTable[i].pName != nullptr; i++) {
            std::string key(OptionsTable[i].pName);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            m[key] = i;
        }
        return m;
    }();
    return map;
}

const std::unordered_map<std::string, int>& getAlphaOptionMap()
{
    static const auto map = []() {
        std::unordered_map<std::string, int> m;
        for (int i = 0; AlphaOptionsTable[i].pName != nullptr; i++) {
            std::string key(AlphaOptionsTable[i].pName);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            m[key] = i;
        }
        return m;
    }();
    return map;
}
