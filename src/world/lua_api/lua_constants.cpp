/**
 * lua_constants.cpp - Lua Constants Registration
 *
 * This file registers constant tables in Lua:
 * - error_code: Error code constants (eOK, eTriggerNotFound, etc.)
 * - trigger_flag: Trigger flag constants (Enabled, KeepEvaluating, etc.)
 * - alias_flag: Alias flag constants (same as trigger_flag)
 * - custom_colour: Custom colour constants for AddTrigger (NoChange, Custom1-16, etc.)
 * - sendto: SendTo destination constants (World, Command, Output, etc.)
 * - timer_flag: Timer flag constants (Enabled, AtTime, OneShot, etc.)
 * - miniwin: Miniwindow constants (position modes, flags, pen/brush styles, etc.)
 * - extended_colours: XTerm 256-color palette
 *
 * Extracted from lua_methods.cpp for better organization.
 */

#include "../../automation/sendto.h"
#include "../script_engine.h"
#include "lua_common.h"
#include "miniwindow.h"

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <QColor>

// NOTE: Error codes and flag constants (eOK, eEnabled, eIgnoreAliasCase, etc.)
// are imported from lua_common.h to ensure consistency between Lua API and C++ implementation

// ========== XTerm 256 Color Palette ==========

// Global array of 256 xterm colors (in QRgb format: 0xAARRGGBB)
// This is used to provide the `extended_colours` Lua global table
// Defined in world_protocol.cpp and declared in world_document.h

// ========== Constants Registration ==========

/**
 * register_lua_constants - Register all constant tables in Lua
 *
 * Creates and registers global constant tables:
 * - error_code
 * - trigger_flag
 * - alias_flag
 * - custom_colour
 * - sendto
 * - timer_flag
 * - miniwin
 * - extended_colours
 *
 * @param L Lua state
 */
void register_lua_constants(lua_State* L)
{
    // xterm 256-color palette is initialized in world_protocol.cpp

    // Create error_code table
    lua_newtable(L);

    lua_pushnumber(L, eOK);
    lua_setfield(L, -2, "eOK");

    lua_pushnumber(L, eWorldOpen);
    lua_setfield(L, -2, "eWorldOpen");

    lua_pushnumber(L, eWorldClosed);
    lua_setfield(L, -2, "eWorldClosed");

    lua_pushnumber(L, eItemInUse);
    lua_setfield(L, -2, "eItemInUse");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "eNoNameSpecified");

    lua_pushnumber(L, 30);
    lua_setfield(L, -2, "eVariableNotFound");

    lua_pushnumber(L, eTriggerNotFound);
    lua_setfield(L, -2, "eTriggerNotFound");

    lua_pushnumber(L, eTriggerAlreadyExists);
    lua_setfield(L, -2, "eTriggerAlreadyExists");

    lua_pushnumber(L, eAliasNotFound);
    lua_setfield(L, -2, "eAliasNotFound");

    lua_pushnumber(L, eAliasAlreadyExists);
    lua_setfield(L, -2, "eAliasAlreadyExists");

    lua_pushnumber(L, eTimerNotFound);
    lua_setfield(L, -2, "eTimerNotFound");

    lua_pushnumber(L, eTimerAlreadyExists);
    lua_setfield(L, -2, "eTimerAlreadyExists");

    lua_pushnumber(L, eTimeInvalid);
    lua_setfield(L, -2, "eTimeInvalid");

    // Logging error codes
    lua_pushnumber(L, eLogFileAlreadyOpen);
    lua_setfield(L, -2, "eLogFileAlreadyOpen");

    lua_pushnumber(L, eCouldNotOpenFile);
    lua_setfield(L, -2, "eCouldNotOpenFile");

    lua_pushnumber(L, eLogFileNotOpen);
    lua_setfield(L, -2, "eLogFileNotOpen");

    lua_pushnumber(L, eLogFileBadWrite);
    lua_setfield(L, -2, "eLogFileBadWrite");

    // Plugin error codes
    lua_pushnumber(L, eBadParameter);
    lua_setfield(L, -2, "eBadParameter");

    lua_pushnumber(L, eNoSuchPlugin);
    lua_setfield(L, -2, "eNoSuchPlugin");

    lua_pushnumber(L, ePluginDisabled);
    lua_setfield(L, -2, "ePluginDisabled");

    lua_pushnumber(L, eNoSuchRoutine);
    lua_setfield(L, -2, "eNoSuchRoutine");

    lua_pushnumber(L, eErrorCallingPluginRoutine);
    lua_setfield(L, -2, "eErrorCallingPluginRoutine");

    lua_pushnumber(L, ePluginFileNotFound);
    lua_setfield(L, -2, "ePluginFileNotFound");

    lua_pushnumber(L, eProblemsLoadingPlugin);
    lua_setfield(L, -2, "eProblemsLoadingPlugin");

    lua_pushnumber(L, eNotAPlugin);
    lua_setfield(L, -2, "eNotAPlugin");

    lua_pushnumber(L, ePluginCouldNotSaveState);
    lua_setfield(L, -2, "ePluginCouldNotSaveState");

    // Array error codes
    lua_pushnumber(L, eArrayAlreadyExists);
    lua_setfield(L, -2, "eArrayAlreadyExists");

    lua_pushnumber(L, eArrayDoesNotExist);
    lua_setfield(L, -2, "eArrayDoesNotExist");

    lua_pushnumber(L, eArrayNotEvenNumberOfValues);
    lua_setfield(L, -2, "eArrayNotEvenNumberOfValues");

    lua_pushnumber(L, eImportedWithDuplicates);
    lua_setfield(L, -2, "eImportedWithDuplicates");

    lua_pushnumber(L, eBadDelimiter);
    lua_setfield(L, -2, "eBadDelimiter");

    lua_pushnumber(L, eSetReplacingExistingValue);
    lua_setfield(L, -2, "eSetReplacingExistingValue");

    lua_pushnumber(L, eKeyDoesNotExist);
    lua_setfield(L, -2, "eKeyDoesNotExist");

    lua_pushnumber(L, eCannotImport);
    lua_setfield(L, -2, "eCannotImport");

    // Miniwindow error codes
    lua_pushnumber(L, eNoSuchWindow);
    lua_setfield(L, -2, "eNoSuchWindow");

    lua_pushnumber(L, eFileNotFound);
    lua_setfield(L, -2, "eFileNotFound");

    lua_pushnumber(L, eUnableToLoadImage);
    lua_setfield(L, -2, "eUnableToLoadImage");

    // Set as global "error_code"
    lua_setglobal(L, "error_code");

    // Create error_desc table (maps error codes to descriptions)
    // This is used by the check() function and plugins that need error descriptions
    lua_newtable(L);

    lua_pushstring(L, "No error");
    lua_rawseti(L, -2, eOK);

    lua_pushstring(L, "The world is already open");
    lua_rawseti(L, -2, eWorldOpen);

    lua_pushstring(L, "The world is closed");
    lua_rawseti(L, -2, eWorldClosed);

    lua_pushstring(L, "No name specified");
    lua_rawseti(L, -2, 2); // eNoNameSpecified

    lua_pushstring(L, "Could not open file");
    lua_rawseti(L, -2, eCouldNotOpenFile);

    lua_pushstring(L, "Log file not open");
    lua_rawseti(L, -2, eLogFileNotOpen);

    lua_pushstring(L, "Log file already open");
    lua_rawseti(L, -2, eLogFileAlreadyOpen);

    lua_pushstring(L, "Log file bad write");
    lua_rawseti(L, -2, eLogFileBadWrite);

    lua_pushstring(L, "Trigger not found");
    lua_rawseti(L, -2, eTriggerNotFound);

    lua_pushstring(L, "Trigger already exists");
    lua_rawseti(L, -2, eTriggerAlreadyExists);

    lua_pushstring(L, "Alias not found");
    lua_rawseti(L, -2, eAliasNotFound);

    lua_pushstring(L, "Alias already exists");
    lua_rawseti(L, -2, eAliasAlreadyExists);

    lua_pushstring(L, "Timer not found");
    lua_rawseti(L, -2, eTimerNotFound);

    lua_pushstring(L, "Timer already exists");
    lua_rawseti(L, -2, eTimerAlreadyExists);

    lua_pushstring(L, "Time invalid");
    lua_rawseti(L, -2, eTimeInvalid);

    lua_pushstring(L, "Variable not found");
    lua_rawseti(L, -2, 30); // eVariableNotFound

    lua_pushstring(L, "Bad parameter");
    lua_rawseti(L, -2, eBadParameter);

    lua_pushstring(L, "No such plugin");
    lua_rawseti(L, -2, eNoSuchPlugin);

    lua_pushstring(L, "Plugin is disabled");
    lua_rawseti(L, -2, ePluginDisabled);

    lua_pushstring(L, "No such routine");
    lua_rawseti(L, -2, eNoSuchRoutine);

    lua_pushstring(L, "Error calling plugin routine");
    lua_rawseti(L, -2, eErrorCallingPluginRoutine);

    lua_pushstring(L, "Plugin file not found");
    lua_rawseti(L, -2, ePluginFileNotFound);

    lua_pushstring(L, "Problems loading plugin");
    lua_rawseti(L, -2, eProblemsLoadingPlugin);

    lua_pushstring(L, "Not a plugin");
    lua_rawseti(L, -2, eNotAPlugin);

    lua_pushstring(L, "Plugin could not save state");
    lua_rawseti(L, -2, ePluginCouldNotSaveState);

    lua_pushstring(L, "Array already exists");
    lua_rawseti(L, -2, eArrayAlreadyExists);

    lua_pushstring(L, "Array does not exist");
    lua_rawseti(L, -2, eArrayDoesNotExist);

    lua_pushstring(L, "Item in use");
    lua_rawseti(L, -2, eItemInUse);

    // Miniwindow error descriptions
    lua_pushstring(L, "No such window");
    lua_rawseti(L, -2, eNoSuchWindow);

    lua_pushstring(L, "File not found");
    lua_rawseti(L, -2, eFileNotFound);

    lua_pushstring(L, "Unable to load image");
    lua_rawseti(L, -2, eUnableToLoadImage);

    // Set as global "error_desc"
    lua_setglobal(L, "error_desc");

    // Create trigger_flag table
    lua_newtable(L);

    lua_pushnumber(L, eEnabled);
    lua_setfield(L, -2, "Enabled");

    lua_pushnumber(L, eOmitFromLog);
    lua_setfield(L, -2, "OmitFromLog");

    lua_pushnumber(L, eOmitFromOutput);
    lua_setfield(L, -2, "OmitFromOutput");

    lua_pushnumber(L, eKeepEvaluating);
    lua_setfield(L, -2, "KeepEvaluating");

    lua_pushnumber(L, eTriggerRegularExpression);
    lua_setfield(L, -2, "RegularExpression");

    lua_pushnumber(L, eIgnoreCase);
    lua_setfield(L, -2, "IgnoreCase");

    lua_pushnumber(L, eExpandVariables);
    lua_setfield(L, -2, "ExpandVariables");

    lua_pushnumber(L, eTemporary);
    lua_setfield(L, -2, "Temporary");

    lua_pushnumber(L, eTriggerOneShot);
    lua_setfield(L, -2, "OneShot");

    lua_pushnumber(L, eReplace);
    lua_setfield(L, -2, "Replace");

    // Set as global "trigger_flag"
    lua_setglobal(L, "trigger_flag");

    // Create alias_flag table (same flags as trigger_flag)
    lua_newtable(L);

    lua_pushnumber(L, eEnabled);
    lua_setfield(L, -2, "Enabled");

    lua_pushnumber(L, eIgnoreAliasCase);
    lua_setfield(L, -2, "IgnoreCase");

    lua_pushnumber(L, eIgnoreAliasCase);
    lua_setfield(L, -2, "IgnoreAliasCase"); // Alias for IgnoreCase (used by some plugins)

    lua_pushnumber(L, eOmitFromLogFile);
    lua_setfield(L, -2, "OmitFromLog");

    lua_pushnumber(L, eAliasRegularExpression);
    lua_setfield(L, -2, "RegularExpression");

    lua_pushnumber(L, eAliasOmitFromOutput);
    lua_setfield(L, -2, "OmitFromOutput");

    lua_pushnumber(L, eExpandVariables);
    lua_setfield(L, -2, "ExpandVariables");

    lua_pushnumber(L, eAliasSpeedWalk);
    lua_setfield(L, -2, "SpeedWalk");

    lua_pushnumber(L, eAliasQueue);
    lua_setfield(L, -2, "Queue");

    lua_pushnumber(L, eAliasMenu);
    lua_setfield(L, -2, "Menu");

    lua_pushnumber(L, eTemporary);
    lua_setfield(L, -2, "Temporary");

    lua_pushnumber(L, eAliasOneShot);
    lua_setfield(L, -2, "OneShot");

    lua_pushnumber(L, eKeepEvaluating);
    lua_setfield(L, -2, "KeepEvaluating");

    // Set as global "alias_flag"
    lua_setglobal(L, "alias_flag");

    // Create custom_colour table (for AddTrigger colour parameter)
    lua_newtable(L);

    lua_pushnumber(L, -1);
    lua_setfield(L, -2, "NoChange");

    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "Custom1");

    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "Custom2");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "Custom3");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "Custom4");

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "Custom5");

    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "Custom6");

    lua_pushnumber(L, 6);
    lua_setfield(L, -2, "Custom7");

    lua_pushnumber(L, 7);
    lua_setfield(L, -2, "Custom8");

    lua_pushnumber(L, 8);
    lua_setfield(L, -2, "Custom9");

    lua_pushnumber(L, 9);
    lua_setfield(L, -2, "Custom10");

    lua_pushnumber(L, 10);
    lua_setfield(L, -2, "Custom11");

    lua_pushnumber(L, 11);
    lua_setfield(L, -2, "Custom12");

    lua_pushnumber(L, 12);
    lua_setfield(L, -2, "Custom13");

    lua_pushnumber(L, 13);
    lua_setfield(L, -2, "Custom14");

    lua_pushnumber(L, 14);
    lua_setfield(L, -2, "Custom15");

    lua_pushnumber(L, 15);
    lua_setfield(L, -2, "Custom16");

    lua_pushnumber(L, 16);
    lua_setfield(L, -2, "CustomOther");

    // Set as global "custom_colour"
    lua_setglobal(L, "custom_colour");

    // Create sendto table
    lua_newtable(L);

    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "World");

    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "Command");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "Output");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "Status");

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "Notepad");

    lua_pushnumber(L, 9);
    lua_setfield(L, -2, "Variable");

    lua_pushnumber(L, 10);
    lua_setfield(L, -2, "Execute");

    lua_pushnumber(L, 11);
    lua_setfield(L, -2, "Speedwalk");

    lua_pushnumber(L, 12);
    lua_setfield(L, -2, "Script");

    lua_pushnumber(L, 13);
    lua_setfield(L, -2, "Immediate");

    lua_pushnumber(L, 14);
    lua_setfield(L, -2, "ScriptAfterOmit");

    // Add lowercase aliases for compatibility with legacy plugins
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "world");

    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "command");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "output");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "status");

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "notepad");

    lua_pushnumber(L, 9);
    lua_setfield(L, -2, "variable");

    lua_pushnumber(L, 10);
    lua_setfield(L, -2, "execute");

    lua_pushnumber(L, 11);
    lua_setfield(L, -2, "speedwalk");

    lua_pushnumber(L, 12);
    lua_setfield(L, -2, "script");

    lua_pushnumber(L, 13);
    lua_setfield(L, -2, "immediate");

    lua_pushnumber(L, 14);
    lua_setfield(L, -2, "scriptafteromit");

    // Set as global "sendto"
    lua_setglobal(L, "sendto");

    // Create timer_flag table
    lua_newtable(L);

    lua_pushnumber(L, eTimerEnabled);
    lua_setfield(L, -2, "Enabled");

    lua_pushnumber(L, eTimerAtTime);
    lua_setfield(L, -2, "AtTime");

    lua_pushnumber(L, eTimerOneShot);
    lua_setfield(L, -2, "OneShot");

    lua_pushnumber(L, eTimerTemporary);
    lua_setfield(L, -2, "Temporary");

    lua_pushnumber(L, eTimerActiveWhenClosed);
    lua_setfield(L, -2, "ActiveWhenClosed");

    lua_pushnumber(L, eTimerReplace);
    lua_setfield(L, -2, "Replace");

    lua_pushnumber(L, eTimerSpeedWalk);
    lua_setfield(L, -2, "SpeedWalk");

    lua_pushnumber(L, eTimerNote);
    lua_setfield(L, -2, "Note");

    // Set as global "timer_flag"
    lua_setglobal(L, "timer_flag");

    // Create miniwin table
    lua_newtable(L);

    // Position modes - values must match original MUSHclient!
    // From lua_methods.cpp miniwindow_flags[] in original
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "pos_stretch_to_view");

    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "pos_stretch_to_view_with_aspect");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "pos_stretch_to_owner");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "pos_stretch_to_owner_with_aspect");

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "pos_top_left");

    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "pos_top_center");

    lua_pushnumber(L, 6);
    lua_setfield(L, -2, "pos_top_right");

    lua_pushnumber(L, 7);
    lua_setfield(L, -2, "pos_center_right");

    lua_pushnumber(L, 8);
    lua_setfield(L, -2, "pos_bottom_right");

    lua_pushnumber(L, 9);
    lua_setfield(L, -2, "pos_bottom_center");

    lua_pushnumber(L, 10);
    lua_setfield(L, -2, "pos_bottom_left");

    lua_pushnumber(L, 11);
    lua_setfield(L, -2, "pos_center_left");

    lua_pushnumber(L, 12);
    lua_setfield(L, -2, "pos_center_all");

    lua_pushnumber(L, 13);
    lua_setfield(L, -2, "pos_tile");

    // Flags
    lua_pushnumber(L, MINIWINDOW_DRAW_UNDERNEATH);
    lua_setfield(L, -2, "draw_underneath");

    lua_pushnumber(L, MINIWINDOW_ABSOLUTE_LOCATION);
    lua_setfield(L, -2, "absolute_location");

    lua_pushnumber(L, MINIWINDOW_TRANSPARENT);
    lua_setfield(L, -2, "transparent");

    lua_pushnumber(L, MINIWINDOW_IGNORE_MOUSE);
    lua_setfield(L, -2, "ignore_mouse");

    lua_pushnumber(L, MINIWINDOW_KEEP_HOTSPOTS);
    lua_setfield(L, -2, "keep_hotspots");

    // RectOp actions
    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "rect_frame");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "rect_fill");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "rect_invert");

    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "rect_3d_rect");

    // CircleOp actions
    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "circle_ellipse");

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "circle_rectangle");

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "circle_round_rect");

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "circle_chord");

    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "circle_pie");

    lua_pushnumber(L, 6);
    lua_setfield(L, -2, "circle_arc");

    // Pen styles - Qt::PenStyle values
    lua_pushnumber(L, 0); // Qt::NoPen
    lua_setfield(L, -2, "pen_none");

    lua_pushnumber(L, 1); // Qt::SolidLine
    lua_setfield(L, -2, "pen_solid");

    lua_pushnumber(L, 2); // Qt::DashLine
    lua_setfield(L, -2, "pen_dash");

    lua_pushnumber(L, 3); // Qt::DotLine
    lua_setfield(L, -2, "pen_dot");

    lua_pushnumber(L, 4); // Qt::DashDotLine
    lua_setfield(L, -2, "pen_dashdot");

    lua_pushnumber(L, 5); // Qt::DashDotDotLine
    lua_setfield(L, -2, "pen_dashdotdot");

    // Pen endcap styles (Windows GDI constants)
    lua_pushnumber(L, 0x00000000); // PS_ENDCAP_ROUND
    lua_setfield(L, -2, "pen_endcap_round");

    lua_pushnumber(L, 0x00000100); // PS_ENDCAP_SQUARE
    lua_setfield(L, -2, "pen_endcap_square");

    lua_pushnumber(L, 0x00000200); // PS_ENDCAP_FLAT
    lua_setfield(L, -2, "pen_endcap_flat");

    // Pen join styles (Windows GDI constants)
    lua_pushnumber(L, 0x00000000); // PS_JOIN_ROUND
    lua_setfield(L, -2, "pen_join_round");

    lua_pushnumber(L, 0x00001000); // PS_JOIN_BEVEL
    lua_setfield(L, -2, "pen_join_bevel");

    lua_pushnumber(L, 0x00002000); // PS_JOIN_MITER
    lua_setfield(L, -2, "pen_join_miter");

    // Brush styles - MUSHclient values (CircleOp maps these to Qt)
    // MUSHclient: 0=solid, 1=null, 2+=patterns
    lua_pushnumber(L, 0); // MUSHclient BS_SOLID
    lua_setfield(L, -2, "brush_solid");

    lua_pushnumber(L, 1); // MUSHclient BS_NULL
    lua_setfield(L, -2, "brush_null");

    lua_pushnumber(L, 2); // MUSHclient HS_HORIZONTAL
    lua_setfield(L, -2, "brush_hatch_horizontal");

    lua_pushnumber(L, 3); // MUSHclient HS_VERTICAL
    lua_setfield(L, -2, "brush_hatch_vertical");

    lua_pushnumber(L, 4); // MUSHclient HS_FDIAGONAL
    lua_setfield(L, -2, "brush_hatch_forwards_diagonal");

    lua_pushnumber(L, 5); // MUSHclient HS_BDIAGONAL
    lua_setfield(L, -2, "brush_hatch_backwards_diagonal");

    lua_pushnumber(L, 6); // MUSHclient HS_CROSS
    lua_setfield(L, -2, "brush_hatch_cross");

    lua_pushnumber(L, 7); // MUSHclient HS_DIAGCROSS
    lua_setfield(L, -2, "brush_hatch_cross_diagonal");

    lua_pushnumber(L, 8); // MUSHclient fine dots
    lua_setfield(L, -2, "brush_fine_pattern");

    lua_pushnumber(L, 9); // MUSHclient medium dots
    lua_setfield(L, -2, "brush_medium_pattern");

    lua_pushnumber(L, 10); // MUSHclient coarse dots
    lua_setfield(L, -2, "brush_coarse_pattern");

    lua_pushnumber(L, 11); // MUSHclient waves horizontal
    lua_setfield(L, -2, "brush_waves_horizontal");

    lua_pushnumber(L, 12); // MUSHclient waves vertical
    lua_setfield(L, -2, "brush_waves_vertical");

    // Legacy aliases
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "brush_none"); // Alias for brush_solid (for backwards compat)

    lua_pushnumber(L, 2);
    lua_setfield(L, -2, "brush_horizontal"); // Alias for brush_hatch_horizontal

    lua_pushnumber(L, 3);
    lua_setfield(L, -2, "brush_vertical"); // Alias for brush_hatch_vertical

    lua_pushnumber(L, 6);
    lua_setfield(L, -2, "brush_cross"); // Alias for brush_hatch_cross

    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "brush_bdiag"); // Alias for brush_hatch_backwards_diagonal

    lua_pushnumber(L, 4);
    lua_setfield(L, -2, "brush_fdiag"); // Alias for brush_hatch_forwards_diagonal

    lua_pushnumber(L, 7);
    lua_setfield(L, -2, "brush_diagcross"); // Alias for brush_hatch_cross_diagonal

    // Additional aliases and missing constants
    lua_pushnumber(L, MINIWINDOW_ABSOLUTE_LOCATION);
    lua_setfield(L, -2,
                 "create_absolute_location"); // Alias for absolute_location (used by aardmapper)

    lua_pushnumber(L, MINIWINDOW_KEEP_HOTSPOTS);
    lua_setfield(L, -2, "create_keep_hotspots"); // Alias for keep_hotspots

    // pen_null (no pen)
    lua_pushnumber(L, 5); // Qt::NoPen
    lua_setfield(L, -2, "pen_null");

    // Cursor types (MUSHclient cursor IDs - NOT Qt values!)
    // Based on lua_methods.cpp
    lua_pushnumber(L, -1); // No cursor
    lua_setfield(L, -2, "cursor_none");
    lua_pushnumber(L, 0); // Arrow
    lua_setfield(L, -2, "cursor_arrow");
    lua_pushnumber(L, 1); // Hand (pointing hand)
    lua_setfield(L, -2, "cursor_hand");
    lua_pushnumber(L, 2); // I-beam (text cursor)
    lua_setfield(L, -2, "cursor_ibeam");
    lua_pushnumber(L, 3); // Plus (cross)
    lua_setfield(L, -2, "cursor_plus");
    lua_pushnumber(L, 4); // Wait (hourglass)
    lua_setfield(L, -2, "cursor_wait");
    lua_pushnumber(L, 5); // Up arrow
    lua_setfield(L, -2, "cursor_up");
    lua_pushnumber(L, 6); // Size NW-SE arrow
    lua_setfield(L, -2, "cursor_nw_se_arrow");
    lua_pushnumber(L, 7); // Size NE-SW arrow
    lua_setfield(L, -2, "cursor_ne_sw_arrow");
    lua_pushnumber(L, 8); // Size E-W arrow (horizontal)
    lua_setfield(L, -2, "cursor_ew_arrow");
    lua_pushnumber(L, 9); // Size N-S arrow (vertical)
    lua_setfield(L, -2, "cursor_ns_arrow");
    lua_pushnumber(L, 10); // Size all directions (four-way arrows)
    lua_setfield(L, -2, "cursor_both_arrow");
    lua_pushnumber(L, 11); // X (forbidden)
    lua_setfield(L, -2, "cursor_x");
    lua_pushnumber(L, 12); // Help (question mark)
    lua_setfield(L, -2, "cursor_help");

    // Hotspot flag constants (from original MUSHclient miniwin.h)
    lua_pushnumber(L, 0x0001); // Left mouse button
    lua_setfield(L, -2, "hotspot_got_lh_mouse");
    lua_pushnumber(L, 0x0002); // Right mouse button
    lua_setfield(L, -2, "hotspot_got_rh_mouse");
    lua_pushnumber(L, 0x0004); // Shift key
    lua_setfield(L, -2, "hotspot_got_shift");
    lua_pushnumber(L, 0x0008); // Control key
    lua_setfield(L, -2, "hotspot_got_control");
    lua_pushnumber(L, 0x0010); // Alt key
    lua_setfield(L, -2, "hotspot_got_alt");

    // Gradient modes (for WindowGradient)
    lua_pushnumber(L, 1); // Left to right
    lua_setfield(L, -2, "gradient_vertical");
    lua_pushnumber(L, 2); // Top to bottom
    lua_setfield(L, -2, "gradient_horizontal");
    lua_pushnumber(L, 3); // Diagonal
    lua_setfield(L, -2, "gradient_diagonal");

    // Circle operation modes (for WindowCircleOp)
    // Based on lua_methods.cpp
    lua_pushnumber(L, 1); // Ellipse
    lua_setfield(L, -2, "circle_ellipse");
    lua_pushnumber(L, 2); // Rectangle
    lua_setfield(L, -2, "circle_rectangle");
    lua_pushnumber(L, 3); // Round rectangle
    lua_setfield(L, -2, "circle_round_rectangle");
    lua_pushnumber(L, 4); // Chord
    lua_setfield(L, -2, "circle_chord");
    lua_pushnumber(L, 5); // Pie
    lua_setfield(L, -2, "circle_pie");

    // Font family constants (Windows GDI FF_* values)
    lua_pushnumber(L, 0); // FF_DONTCARE
    lua_setfield(L, -2, "font_family_any");
    lua_pushnumber(L, 16); // FF_ROMAN << 4
    lua_setfield(L, -2, "font_family_roman");
    lua_pushnumber(L, 32); // FF_SWISS << 4
    lua_setfield(L, -2, "font_family_swiss");
    lua_pushnumber(L, 48); // FF_MODERN << 4 (monospace fonts)
    lua_setfield(L, -2, "font_family_modern");
    lua_pushnumber(L, 64); // FF_SCRIPT << 4
    lua_setfield(L, -2, "font_family_script");
    lua_pushnumber(L, 80); // FF_DECORATIVE << 4
    lua_setfield(L, -2, "font_family_decorative");

    // Font pitch constants (Windows GDI pitch values)
    lua_pushnumber(L, 0); // DEFAULT_PITCH
    lua_setfield(L, -2, "font_pitch_default");
    lua_pushnumber(L, 1); // FIXED_PITCH (monospaced)
    lua_setfield(L, -2, "font_pitch_fixed");
    lua_pushnumber(L, 1); // Alias for fixed pitch
    lua_setfield(L, -2, "font_pitch_monospaced");
    lua_pushnumber(L, 2); // VARIABLE_PITCH
    lua_setfield(L, -2, "font_pitch_variable");

    // Set as global "miniwin"
    lua_setglobal(L, "miniwin");

    // Register extended_colours table (xterm 256-color palette)
    // This provides access to the xterm_256_colours array from Lua
    lua_newtable(L);
    for (int i = 0; i < 256; i++) {
        lua_pushnumber(L, xterm_256_colours[i]);
        lua_rawseti(L, -2, i); // Lua arrays are 0-indexed in this case
    }
    lua_setglobal(L, "extended_colours");

    // ========== Global Flag Constants ==========
    // These are exposed as globals for backwards compatibility with old scripts
    // that use eOmitFromOutput, eEnabled, etc. directly instead of via tables

    // Trigger flags (as globals)
    lua_pushnumber(L, eEnabled);
    lua_setglobal(L, "eEnabled");

    lua_pushnumber(L, eOmitFromLog);
    lua_setglobal(L, "eOmitFromLog");

    lua_pushnumber(L, eOmitFromOutput);
    lua_setglobal(L, "eOmitFromOutput");

    lua_pushnumber(L, eKeepEvaluating);
    lua_setglobal(L, "eKeepEvaluating");

    lua_pushnumber(L, eIgnoreCase);
    lua_setglobal(L, "eIgnoreCase");

    lua_pushnumber(L, eTriggerRegularExpression);
    lua_setglobal(L, "eTriggerRegularExpression");

    lua_pushnumber(L, eExpandVariables);
    lua_setglobal(L, "eExpandVariables");

    lua_pushnumber(L, eReplace);
    lua_setglobal(L, "eReplace");

    lua_pushnumber(L, eLowercaseWildcard);
    lua_setglobal(L, "eLowercaseWildcard");

    lua_pushnumber(L, eTemporary);
    lua_setglobal(L, "eTemporary");

    lua_pushnumber(L, eTriggerOneShot);
    lua_setglobal(L, "eTriggerOneShot");

    // Alias-specific flags (as globals)
    lua_pushnumber(L, eIgnoreAliasCase);
    lua_setglobal(L, "eIgnoreAliasCase");

    lua_pushnumber(L, eAliasRegularExpression);
    lua_setglobal(L, "eAliasRegularExpression");

    lua_pushnumber(L, eAliasSpeedWalk);
    lua_setglobal(L, "eAliasSpeedWalk");

    lua_pushnumber(L, eAliasQueue);
    lua_setglobal(L, "eAliasQueue");

    lua_pushnumber(L, eAliasMenu);
    lua_setglobal(L, "eAliasMenu");

    lua_pushnumber(L, eAliasOneShot);
    lua_setglobal(L, "eAliasOneShot");

    // Color constants (as globals)
    lua_pushnumber(L, -1);
    lua_setglobal(L, "NOCHANGE");

    // ========== Built-in check() function ==========
    // This function is used by many MUSHclient plugins to verify API call results.
    // It throws an error if the result code is not eOK.
    // Equivalent to:
    //   function check(result)
    //     if result ~= error_code.eOK then
    //       error(error_desc[result] or string.format("Unknown error code: %i", result), 2)
    //     end
    //   end
    const char* checkFunction = R"lua(
function check(result)
    if result ~= error_code.eOK then
        error(error_desc[result] or string.format("Unknown error code: %i", result), 2)
    end
end
)lua";

    if (luaL_dostring(L, checkFunction) != 0) {
        const char* err = lua_tostring(L, -1);
        qWarning() << "Failed to register check() function:" << (err ? err : "unknown error");
        lua_pop(L, 1);
    }
}
