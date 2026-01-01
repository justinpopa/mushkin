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
#include <climits>

// ============================================================================
// HELPER MACROS FOR TABLE DEFINITIONS
// ============================================================================

// O(arg) expands to: offsetof(WorldDocument, arg), sizeof(WorldDocument::arg)
// This calculates the byte offset AND size of a field for generic access
#define O(arg) offsetof(WorldDocument, arg), sizeof(((WorldDocument*)nullptr)->arg)

// A(arg) expands to: offsetof(WorldDocument, arg)
// Simpler version for alpha (string) options which don't need size
#define A(arg) offsetof(WorldDocument, arg)

// ============================================================================
// CONSTANTS & ENUMS (from original MUSHclient)
// ============================================================================
// NOTE: These will need proper definitions once we port the relevant subsystems.
// For now, using placeholder values to allow compilation.

// RGB color macro (Windows style: 0x00BBGGRR)
#define RGB(r, g, b) (static_cast<unsigned int>((b) << 16 | (g) << 8 | (r)))

// Font weights
#define FW_DONTCARE 0

// Character sets
#define DEFAULT_CHARSET 1

// Color limits (MAX_CUSTOM is already defined in world_document.h)

// Port limits
#ifndef USHRT_MAX
#    define USHRT_MAX 65535
#endif

// Line width
#define MAX_LINE_WIDTH 32000

// Connect methods (already defined in world_document.h)
#define eConnectTypeMax 4

// SendTo enum now in automation/sendto.h

// Default sequences
#define DEFAULT_TRIGGER_SEQUENCE 100
#define DEFAULT_ALIAS_SEQUENCE 100

// Debug levels
enum { DBG_NONE = 0 };

// Reload options - see ScriptReloadOption enum in world_document.h
// eReloadConfirm = 0, eReloadAlways = 1, eReloadNever = 2

// MXP modes
enum { eNoMXP = 0, eUseMXP = 1, eOnCommandMXP = 2 };

// Sound literal
#define NOSOUNDLIT ""

// Plugin ID length
#define PLUGIN_UNIQUE_ID_LENGTH 24

// ============================================================================
// NUMERIC OPTIONS TABLE (173 entries)
// ============================================================================
// Format: { name, default_value, offsetof(field), sizeof(field), min, max, flags }
// Note: If min==0 and max==0, the option is treated as boolean

const tConfigurationNumericOption OptionsTable[] = {
    {"alternative_inverse", false, O(m_bAlternativeInverse), 0, 0, 0},
    {"alt_arrow_recalls_partial", false, O(m_bAltArrowRecallsPartial), 0, 0, 0},
    {"always_record_command_history", false, O(m_bAlwaysRecordCommandHistory), 0, 0, 0},
    {"arrows_change_history", true, O(m_bArrowsChangeHistory), 0, 0, 0},
    {"arrow_keys_wrap", false, O(m_bArrowKeysWrap), 0, 0, 0},
    {"arrow_recalls_partial", false, O(m_bArrowRecallsPartial), 0, 0, 0},
    {"autosay_exclude_macros", false, O(m_bExcludeMacros), 0, 0, 0},
    {"autosay_exclude_non_alpha", false, O(m_bExcludeNonAlpha), 0, 0, 0},
    {"auto_copy_to_clipboard_in_html", false, O(m_bAutoCopyInHTML), 0, 0, 0},
    {"auto_pause", true, O(m_bAutoFreeze), 0, 0, 0},
    {"keep_pause_at_bottom", false, O(m_bKeepFreezeAtBottom), 0, 0, 0},
    {"auto_repeat", false, O(m_bAutoRepeat), 0, 0, 0},
    {"auto_resize_command_window", false, O(m_bAutoResizeCommandWindow), 0, 0, 0},
    {"auto_resize_minimum_lines", 1, O(m_iAutoResizeMinimumLines), 1, 100, 0},
    {"auto_resize_maximum_lines", 20, O(m_iAutoResizeMaximumLines), 1, 100, 0},
    {"auto_wrap_window_width", false, O(m_bAutoWrapWindowWidth), 0, 0, 0},
    {"carriage_return_clears_line", false, O(m_bCarriageReturnClearsLine), 0, 0, 0},
    {"confirm_before_replacing_typing", true, O(m_bConfirmBeforeReplacingTyping), 0, 0, 0},
    {"confirm_on_paste", true, O(m_bConfirmOnPaste), 0, 0, 0},
    {"confirm_on_send", true, O(m_bConfirmOnSend), 0, 0, 0},
    {"connect_method", eNoAutoConnect, O(m_connect_now), eNoAutoConnect, eConnectTypeMax - 1, 0},
    {"copy_selection_to_clipboard", false, O(m_bCopySelectionToClipboard), 0, 0, 0},
    {"convert_ga_to_newline", false, O(m_bConvertGAtoNewline), 0, 0, 0},
    {"ctrl_n_goes_to_next_command", false, O(m_bCtrlNGoesToNextCommand), 0, 0, 0},
    {"ctrl_p_goes_to_previous_command", false, O(m_bCtrlPGoesToPreviousCommand), 0, 0, 0},
    {"ctrl_z_goes_to_end_of_buffer", false, O(m_bCtrlZGoesToEndOfBuffer), 0, 0, 0},
    {"ctrl_backspace_deletes_last_word", false, O(m_bCtrlBackspaceDeletesLastWord), 0, 0, 0},
    {"custom_16_is_default_colour", false, O(m_bCustom16isDefaultColour), 0, 0, OPT_UPDATE_VIEWS},

    {"default_trigger_send_to", eSendToWorld, O(m_iDefaultTriggerSendTo), eSendToWorld,
     eSendToLast - 1, 0},
    {"default_trigger_sequence", DEFAULT_TRIGGER_SEQUENCE, O(m_iDefaultTriggerSequence), 0, 10000,
     0},
    {"default_trigger_regexp", false, O(m_bDefaultTriggerRegexp), 0, 0, 0},
    {"default_trigger_expand_variables", false, O(m_bDefaultTriggerExpandVariables), 0, 0, 0},
    {"default_trigger_keep_evaluating", false, O(m_bDefaultTriggerKeepEvaluating), 0, 0, 0},
    {"default_trigger_ignore_case", false, O(m_bDefaultTriggerIgnoreCase), 0, 0, 0},

    {"default_alias_send_to", eSendToWorld, O(m_iDefaultAliasSendTo), eSendToWorld, eSendToLast - 1,
     0},
    {"default_alias_sequence", DEFAULT_ALIAS_SEQUENCE, O(m_iDefaultAliasSequence), 0, 10000, 0},
    {"default_alias_regexp", false, O(m_bDefaultAliasRegexp), 0, 0, 0},
    {"default_alias_expand_variables", false, O(m_bDefaultAliasExpandVariables), 0, 0, 0},
    {"default_alias_keep_evaluating", false, O(m_bDefaultAliasKeepEvaluating), 0, 0, 0},
    {"default_alias_ignore_case", false, O(m_bDefaultAliasIgnoreCase), 0, 0, 0},

    {"default_timer_send_to", eSendToWorld, O(m_iDefaultTimerSendTo), eSendToWorld, eSendToLast - 1,
     0},

    {"detect_pueblo", true, O(m_bPueblo), 0, 0, 0},
    {"do_not_add_macros_to_command_history", false, O(m_bDoNotAddMacrosToCommandHistory), 0, 0, 0},
    {"do_not_show_outstanding_lines", false, O(m_bDoNotShowOutstandingLines), 0, 0, 0},
    {"do_not_translate_iac_to_iac_iac", false, O(m_bDoNotTranslateIACtoIACIAC), 0, 0, 0},
    {"disable_compression", false, O(m_bDisableCompression), 0, 0, 0},
    {"display_my_input", true, O(m_display_my_input), 0, 0, 0},
    {"double_click_inserts", false, O(m_bDoubleClickInserts), 0, 0, 0},
    {"double_click_sends", false, O(m_bDoubleClickSends), 0, 0, 0},
    {"echo_colour", 0, O(m_echo_colour), 0, MAX_CUSTOM, OPT_CUSTOM_COLOUR | OPT_UPDATE_VIEWS},
    {"echo_hyperlink_in_output_window", true, O(m_bEchoHyperlinkInOutputWindow), 0, 0, 0},
    {"edit_script_with_notepad", true, O(m_bEditScriptWithNotepad), 0, 0, 0},
    {"enable_aliases", true, O(m_enable_aliases), 0, 0, 0},
    {"enable_auto_say", false, O(m_bEnableAutoSay), 0, 0, 0},
    {"enable_beeps", true, O(m_enable_beeps), 0, 0, 0},
    {"enable_command_stack", false, O(m_enable_command_stack), 0, 0, 0},
    {"enable_scripts", true, O(m_bEnableScripts), 0, 0, 0},
    {"enable_spam_prevention", false, O(m_bEnableSpamPrevention), 0, 0, 0},
    {"enable_speed_walk", false, O(m_enable_speed_walk), 0, 0, 0},
    {"enable_timers", true, O(m_bEnableTimers), 0, 0, 0},
    {"enable_triggers", true, O(m_enable_triggers), 0, 0, 0},
    {"enable_trigger_sounds", true, O(m_enable_trigger_sounds), 0, 0, 0},
    {"escape_deletes_input", false, O(m_bEscapeDeletesInput), 0, 0, 0},
    {"fade_output_buffer_after_seconds", 0, O(m_iFadeOutputBufferAfterSeconds), 0, 3600,
     OPT_UPDATE_VIEWS},
    {"fade_output_opacity_percent", 20, O(m_FadeOutputOpacityPercent), 0, 100, OPT_UPDATE_VIEWS},
    {"fade_output_seconds", 8, O(m_FadeOutputSeconds), 1, 60, OPT_UPDATE_VIEWS},
    {"flash_taskbar_icon", false, O(m_bFlashIcon), 0, 0, 0},
    {"history_lines", 1000, O(m_nHistoryLines), 20, 5000, 0},
    {"hyperlink_adds_to_command_history", true, O(m_bHyperlinkAddsToCommandHistory), 0, 0, 0},
    {"hyperlink_colour", RGB(0, 128, 255), O(m_iHyperlinkColour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"ignore_mxp_colour_changes", false, O(m_bIgnoreMXPcolourChanges), 0, 0, 0},
    {"indent_paras", true, O(m_indent_paras), 0, 0, 0},
    {"input_background_colour", RGB(255, 255, 255), O(m_input_background_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"input_font_height", 12, O(m_input_font_height), 1, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_italic", false, O(m_input_font_italic), 0, 0,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_weight", FW_DONTCARE, O(m_input_font_weight), 0, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_font_charset", DEFAULT_CHARSET, O(m_input_font_charset), 0, 65536,
     OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"input_text_colour", RGB(0, 0, 0), O(m_input_text_colour), 0, 0xFFFFFF,
     OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"keep_commands_on_same_line", false, O(m_bKeepCommandsOnSameLine), 0, 0, 0},
    {"keypad_enable", true, O(m_keypad_enable), 0, 0, 0},
    {"line_information", true, O(m_bLineInformation), 0, 0, 0},
    {"line_spacing", 0, O(m_iLineSpacing), 0, 100, OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"log_html", false, O(m_bLogHTML), 0, 0, 0},
    {"log_input", false, O(m_log_input), 0, 0, 0},
    {"log_in_colour", false, O(m_bLogInColour), 0, 0, 0},
    {"log_notes", false, O(m_bLogNotes), 0, 0, 0},
    {"log_output", true, O(m_bLogOutput), 0, 0, 0},
    {"log_raw", false, O(m_bLogRaw), 0, 0, 0},
    {"log_script_errors", false, O(m_bLogScriptErrors), 0, 0, 0},
    {"lower_case_tab_completion", false, O(m_bLowerCaseTabCompletion), 0, 0, 0},
    {"map_failure_regexp", false, O(m_bMapFailureRegexp), 0, 0, 0},
    {"max_output_lines", 5000, O(m_maxlines), 200, 500000, OPT_FIX_OUTPUT_BUFFER},
    {"mud_can_change_link_colour", true, O(m_bMudCanChangeLinkColour), 0, 0, OPT_SERVER_CAN_WRITE},
    {"mud_can_remove_underline", false, O(m_bMudCanRemoveUnderline), 0, 0, OPT_SERVER_CAN_WRITE},
    {"mud_can_change_options", true, O(m_bMudCanChangeOptions), 0, 0, 0},
    {"mxp_debug_level", DBG_NONE, O(m_iMXPdebugLevel), 0, 4, 0},
    {"naws", false, O(m_bNAWS), 0, 0, 0},
    {"use_msp", false, O(m_bUseMSP), 0, 0, 0},
    {"note_text_colour", 4, O(m_iNoteTextColour), 0, 0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"no_echo_off", false, O(m_bNoEchoOff), 0, 0, 0},
    {"omit_date_from_save_files", false, O(m_bOmitSavedDateFromSaveFiles), 0, 0, 0},
    {"output_font_height", 12, O(m_font_height), 1, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"output_font_weight", FW_DONTCARE, O(m_font_weight), 0, 1000,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"output_font_charset", DEFAULT_CHARSET, O(m_font_charset), 0, 65536,
     OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"paste_commented_softcode", false, O(m_bPasteCommentedSoftcode), 0, 0, 0},
    {"paste_delay", 0, O(m_nPasteDelay), 0, 100000, 0},
    {"paste_delay_per_lines", 1, O(m_nPasteDelayPerLines), 1, 100000, 0},
    {"paste_echo", false, O(m_bPasteEcho), 0, 0, 0},
    {"play_sounds_in_background", false, O(m_bPlaySoundsInBackground), 0, 0, 0},
    {"pixel_offset", 1, O(m_iPixelOffset), 0, 20, OPT_UPDATE_VIEWS | OPT_FIX_INPUT_WRAP},
    {"port", 4000, O(m_port), 1, USHRT_MAX, OPT_PLUGIN_CANNOT_WRITE},
    {"re_evaluate_auto_say", false, O(m_bReEvaluateAutoSay), 0, 0, 0},
    {"save_deleted_command", false, O(m_bSaveDeletedCommand), 0, 0, 0},
    {"save_world_automatically", false, O(m_bSaveWorldAutomatically), 0, 0, 0},
    {"script_reload_option", 0, O(m_nReloadOption), 0, 2, 0}, // 0 = eReloadConfirm (default)
    {"script_errors_to_output_window", false, O(m_bScriptErrorsToOutputWindow), 0, 0, 0},
    {"send_echo", false, O(m_bSendEcho), 0, 0, 0},
    {"send_file_commented_softcode", false, O(m_bFileCommentedSoftcode), 0, 0, 0},
    {"send_file_delay", 0, O(m_nFileDelay), 0, 100000, 0},
    {"send_file_delay_per_lines", 1, O(m_nFileDelayPerLines), 1, 100000, 0},
    {"send_keep_alives", 0, O(m_bSendKeepAlives), 0, 0, 0},
    {"send_mxp_afk_response", true, O(m_bSendMXP_AFK_Response), 0, 0, 0},
    {"show_bold", false, O(m_bShowBold), 0, 0, OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"show_connect_disconnect", true, O(m_bShowConnectDisconnect), 0, 0, 0},
    {"show_italic", true, O(m_bShowItalic), 0, 0, OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"show_underline", true, O(m_bShowUnderline), 0, 0, OPT_UPDATE_VIEWS | OPT_UPDATE_OUTPUT_FONT},
    {"spam_line_count", 20, O(m_iSpamLineCount), 5, 500, 0},
    {"speed_walk_delay", 0, O(m_iSpeedWalkDelay), 0, 30000, OPT_FIX_SPEEDWALK_DELAY},
    {"spell_check_on_send", false, O(m_bSpellCheckOnSend), 0, 0, 0},
    {"start_paused", false, O(m_bStartPaused), 0, 0, 0},
    {"tab_completion_lines", 200, O(m_iTabCompletionLines), 1, 500000, 0},
    {"tab_completion_space", false, O(m_bTabCompletionSpace), 0, 0, 0},
    {"timestamp_input_text_colour", RGB(128, 0, 0), O(m_OutputLinePreambleInputTextColour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_notes_text_colour", RGB(0, 0, 255), O(m_OutputLinePreambleNotesTextColour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_output_text_colour", RGB(255, 255, 255), O(m_OutputLinePreambleOutputTextColour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_input_back_colour", RGB(0, 0, 0), O(m_OutputLinePreambleInputBackColour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_notes_back_colour", RGB(0, 0, 0), O(m_OutputLinePreambleNotesBackColour), 0,
     0xFFFFFF, OPT_RGB_COLOUR | OPT_UPDATE_VIEWS},
    {"timestamp_output_back_colour", RGB(0, 0, 0), O(m_OutputLinePreambleOutputBackColour), 0,
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
    {"use_default_colours", false, O(m_bUseDefaultColours), 0, 0, 0},
    {"use_default_input_font", false, O(m_bUseDefaultInputFont), 0, 0, 0},
    {"use_default_macros", false, O(m_bUseDefaultMacros), 0, 0, 0},
    {"use_default_output_font", false, O(m_bUseDefaultOutputFont), 0, 0, 0},
    {"use_default_timers", false, O(m_bUseDefaultTimers), 0, 0, 0},
    {"use_default_triggers", false, O(m_bUseDefaultTriggers), 0, 0, 0},
    {"use_mxp", eOnCommandMXP, O(m_iUseMXP), 0, 3, OPT_USE_MXP},
    {"utf_8", false, O(m_bUTF_8), 0, 0, 0},
    {"warn_if_scripting_inactive", true, O(m_bWarnIfScriptingInactive), 0, 0, 0},
    {"wrap", true, O(m_wrap), 0, 0, 0},
    {"wrap_input", false, O(m_bAutoWrapInput), 0, 0, OPT_FIX_INPUT_WRAP},
    {"wrap_column", 80, O(m_nWrapColumn), 20, MAX_LINE_WIDTH,
     OPT_FIX_WRAP_COLUMN | OPT_FIX_INPUT_WRAP},

    {"write_world_name_to_log", true, O(m_bWriteWorldNameToLog), 0, 0, 0},

    // Remote Access Server settings
    {"remote_access_enabled", false, O(m_bEnableRemoteAccess), 0, 0, 0},
    {"remote_port", 0, O(m_iRemotePort), 0, 65535, 0},
    {"remote_scrollback_lines", 100, O(m_iRemoteScrollbackLines), 0, 10000, 0},
    {"remote_max_clients", 5, O(m_iRemoteMaxClients), 1, 100, 0},
    {"remote_lockout_attempts", 3, O(m_iRemoteLockoutAttempts), 0, 100, 0},
    {"remote_lockout_seconds", 300, O(m_iRemoteLockoutSeconds), 0, 86400, 0},

    {nullptr} // NULL sentinel - end of table marker
};

// ============================================================================
// ALPHA (STRING) OPTIONS TABLE (67 entries)
// ============================================================================
// Format: { name, default_value, offsetof(field), flags }

const tConfigurationAlphaOption AlphaOptionsTable[] = {
    {"auto_log_file_name", "", A(m_strAutoLogFileName)},
    {"auto_say_override_prefix", "-", A(m_strOverridePrefix)},
    {"auto_say_string", "say ", A(m_strAutoSayString), OPT_KEEP_SPACES},
    {"beep_sound", "", A(m_strBeepSound)},
    {"command_stack_character", ";", A(m_strCommandStackCharacter), OPT_COMMAND_STACK},
    {"connect_text", "", A(m_connect_text), OPT_MULTLINE},
    {"editor_window_name", "", A(m_strEditorWindowName)},
    {"filter_aliases", "", A(m_strAliasesFilter), OPT_MULTLINE},
    {"filter_timers", "", A(m_strTimersFilter), OPT_MULTLINE},
    {"filter_triggers", "", A(m_strTriggersFilter), OPT_MULTLINE},
    {"filter_variables", "", A(m_strVariablesFilter), OPT_MULTLINE},
    {"id", "", A(m_strWorldID), OPT_WORLD_ID},
    {"input_font_name", "FixedSys", A(m_input_font_name), OPT_UPDATE_VIEWS | OPT_UPDATE_INPUT_FONT},
    {"log_file_postamble", "", A(m_strLogFilePostamble), OPT_MULTLINE},
    {"log_file_preamble", "", A(m_strLogFilePreamble), OPT_MULTLINE},
    {"log_line_postamble_input", "", A(m_strLogLinePostambleInput), OPT_KEEP_SPACES},
    {"log_line_postamble_notes", "", A(m_strLogLinePostambleNotes), OPT_KEEP_SPACES},
    {"log_line_postamble_output", "", A(m_strLogLinePostambleOutput), OPT_KEEP_SPACES},
    {"log_line_preamble_input", "", A(m_strLogLinePreambleInput), OPT_KEEP_SPACES},
    {"log_line_preamble_notes", "", A(m_strLogLinePreambleNotes), OPT_KEEP_SPACES},
    {"log_line_preamble_output", "", A(m_strLogLinePreambleOutput), OPT_KEEP_SPACES},
    {"mapping_failure", "Alas, you cannot go that way.", A(m_strMappingFailure), OPT_KEEP_SPACES},
    {"name", "", A(m_mush_name), OPT_PLUGIN_CANNOT_WRITE},
    {"new_activity_sound", NOSOUNDLIT, A(m_new_activity_sound)},
    {"notes", "", A(m_notes), OPT_MULTLINE},
    {"on_mxp_close_tag", "", A(m_strOnMXP_CloseTag)},
    {"on_mxp_error", "", A(m_strOnMXP_Error)},
    {"on_mxp_open_tag", "", A(m_strOnMXP_OpenTag)},
    {"on_mxp_set_variable", "", A(m_strOnMXP_SetVariable)},
    {"on_mxp_start", "", A(m_strOnMXP_Start)},
    {"on_mxp_stop", "", A(m_strOnMXP_Stop)},
    {"on_world_close", "", A(m_strWorldClose)},
    {"on_world_save", "", A(m_strWorldSave)},
    {"on_world_connect", "", A(m_strWorldConnect)},
    {"on_world_disconnect", "", A(m_strWorldDisconnect)},
    {"on_world_get_focus", "", A(m_strWorldGetFocus)},
    {"on_world_lose_focus", "", A(m_strWorldLoseFocus)},
    {"on_world_open", "", A(m_strWorldOpen)},
    {"output_font_name", "FixedSys", A(m_font_name), OPT_UPDATE_OUTPUT_FONT},
    {"password", "", A(m_password), OPT_PASSWORD | OPT_PLUGIN_CANNOT_RW},
    {"paste_line_postamble", "", A(m_pasteline_postamble), OPT_KEEP_SPACES},
    {"paste_line_preamble", "", A(m_pasteline_preamble), OPT_KEEP_SPACES},
    {"paste_postamble", "", A(m_paste_postamble), OPT_MULTLINE},
    {"paste_preamble", "", A(m_paste_preamble), OPT_MULTLINE},
    {"player", "", A(m_name), OPT_PLUGIN_CANNOT_WRITE},
    {"recall_line_preamble", "", A(m_strRecallLinePreamble), OPT_KEEP_SPACES},
    {"script_editor", "notepad", A(m_strScriptEditor)},
    {"script_editor_argument", "%file", A(m_strScriptEditorArgument)},
    {"script_filename", "", A(m_strScriptFilename)},
    {"script_language", "lua", A(m_strLanguage)},
    {"script_prefix", "", A(m_strScriptPrefix)},
    {"send_to_world_file_postamble", "", A(m_file_postamble), OPT_MULTLINE},
    {"send_to_world_file_preamble", "", A(m_file_preamble), OPT_MULTLINE},
    {"send_to_world_line_postamble", "", A(m_line_postamble)},
    {"send_to_world_line_preamble", "", A(m_line_preamble)},
    {"site", "", A(m_server), OPT_PLUGIN_CANNOT_WRITE},
    {"spam_message", "look", A(m_strSpamMessage)},
    {"speed_walk_filler", "", A(m_strSpeedWalkFiller), OPT_KEEP_SPACES},
    {"speed_walk_prefix", "#", A(m_speed_walk_prefix), OPT_KEEP_SPACES},
    {"tab_completion_defaults", "", A(m_strTabCompletionDefaults), OPT_MULTLINE},
    {"terminal_identification", "mushkin", A(m_strTerminalIdentification)},
    {"timestamp_input", "", A(m_strOutputLinePreambleInput), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},
    {"timestamp_notes", "", A(m_strOutputLinePreambleNotes), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},
    {"timestamp_output", "", A(m_strOutputLinePreambleOutput), OPT_KEEP_SPACES | OPT_UPDATE_VIEWS},

    // Remote Access Server settings
    {"remote_password", "", A(m_strRemotePassword), OPT_PASSWORD | OPT_PLUGIN_CANNOT_RW},

    {nullptr} // NULL sentinel - end of table marker
};
