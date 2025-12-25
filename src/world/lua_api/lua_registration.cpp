/**
 * lua_registration.cpp - Main Lua API Registration
 *
 * This file contains RegisterLuaRoutines(), which:
 * - Creates the "world" table
 * - Registers all Lua API functions in the world table
 * - Registers constant tables (error_code, trigger_flag, sendto, etc.)
 * - Registers global function aliases for backward compatibility
 *
 * Extracted from lua_methods.cpp for better organization.
 */

#include "lua_registration.h"

#include "../../automation/sendto.h"
#include "../../world/world_document.h"
#include "../script_engine.h"

#include <QStringList>

// Lua 5.1 C headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

// Forward declarations of all Lua API functions (defined in lua_methods.cpp)
// These are all static functions in lua_methods.cpp that we need to reference

// Output functions
extern int L_Note(lua_State* L);
extern int L_Trim(lua_State* L);
extern int L_ColourNote(lua_State* L);
extern int L_ColourTell(lua_State* L);
extern int L_Tell(lua_State* L);
extern int L_ANSI(lua_State* L);
extern int L_AnsiNote(lua_State* L);
extern int L_Hyperlink(lua_State* L);
extern int L_Simulate(lua_State* L);

// Info bar functions
extern int L_Info(lua_State* L);
extern int L_InfoClear(lua_State* L);
extern int L_InfoColour(lua_State* L);
extern int L_InfoBackground(lua_State* L);
extern int L_InfoFont(lua_State* L);
extern int L_ShowInfoBar(lua_State* L);

// Network functions
extern int L_Send(lua_State* L);
extern int L_SendNoEcho(lua_State* L);
extern int L_Connect(lua_State* L);
extern int L_Disconnect(lua_State* L);
extern int L_IsConnected(lua_State* L);

// Variable functions
extern int L_GetVariable(lua_State* L);
extern int L_SetVariable(lua_State* L);
extern int L_DeleteVariable(lua_State* L);
extern int L_GetVariableList(lua_State* L);

// World info functions
extern int L_GetInfo(lua_State* L);
extern int L_GetWorldName(lua_State* L);
extern int L_GetOption(lua_State* L);
extern int L_SetOption(lua_State* L);
extern int L_GetAlphaOption(lua_State* L);
extern int L_SetAlphaOption(lua_State* L);
extern int L_SetStatus(lua_State* L);
extern int L_Repaint(lua_State* L);
extern int L_TextRectangle(lua_State* L);
extern int L_SetBackgroundImage(lua_State* L);
extern int L_GetCommand(lua_State* L);
extern int L_SetCommand(lua_State* L);
extern int L_SetCommandSelection(lua_State* L);
extern int L_SetCommandWindowHeight(lua_State* L);
extern int L_SetScroll(lua_State* L);
extern int L_GetLineCount(lua_State* L);
extern int L_GetSentBytes(lua_State* L);
extern int L_GetReceivedBytes(lua_State* L);
extern int L_GetConnectDuration(lua_State* L);
extern int L_WorldAddress(lua_State* L);
extern int L_WorldPort(lua_State* L);
extern int L_WorldName(lua_State* L);
extern int L_Version(lua_State* L);
extern int L_GetLinesInBufferCount(lua_State* L);
extern int L_GetSelectionStartLine(lua_State* L);
extern int L_GetSelectionEndLine(lua_State* L);
extern int L_GetSelectionStartColumn(lua_State* L);
extern int L_GetSelectionEndColumn(lua_State* L);
extern int L_GetSysColor(lua_State* L);
extern int L_GetSystemMetrics(lua_State* L);
extern int L_GetDeviceCaps(lua_State* L);
extern int L_GetFrame(lua_State* L);
extern int L_GetAlphaOptionList(lua_State* L);
extern int L_GetOptionList(lua_State* L);
extern int L_GetGlobalOptionList(lua_State* L);

// Command and queue functions
extern int L_Queue(lua_State* L);
extern int L_DiscardQueue(lua_State* L);

// Color functions
extern int L_GetNormalColour(lua_State* L);
extern int L_GetBoldColour(lua_State* L);
extern int L_SetNormalColour(lua_State* L);
extern int L_SetBoldColour(lua_State* L);
extern int L_GetCustomColourText(lua_State* L);
extern int L_GetCustomColourBackground(lua_State* L);
extern int L_SetCustomColourText(lua_State* L);
extern int L_SetCustomColourBackground(lua_State* L);
extern int L_SetCustomColourName(lua_State* L);
extern int L_PickColour(lua_State* L);
extern int L_AdjustColour(lua_State* L);
extern int L_ColourNameToRGB(lua_State* L);
extern int L_RGBColourToName(lua_State* L);

// Trace/Echo/Speedwalk functions
extern int L_GetTrace(lua_State* L);
extern int L_SetTrace(lua_State* L);
extern int L_GetEchoInput(lua_State* L);
extern int L_SetEchoInput(lua_State* L);
extern int L_GetSpeedWalkDelay(lua_State* L);
extern int L_SetSpeedWalkDelay(lua_State* L);
extern int L_EvaluateSpeedwalk(lua_State* L);
extern int L_ReverseSpeedwalk(lua_State* L);
extern int L_RemoveBacktracks(lua_State* L);

// Trigger functions
extern int L_AddTrigger(lua_State* L);
extern int L_DeleteTrigger(lua_State* L);
extern int L_EnableTrigger(lua_State* L);
extern int L_IsTrigger(lua_State* L);
extern int L_GetTrigger(lua_State* L);
extern int L_GetTriggerInfo(lua_State* L);
extern int L_GetTriggerList(lua_State* L);
extern int L_EnableTriggerGroup(lua_State* L);
extern int L_DeleteTriggerGroup(lua_State* L);
extern int L_DeleteTemporaryTriggers(lua_State* L);
extern int L_GetTriggerOption(lua_State* L);
extern int L_SetTriggerOption(lua_State* L);
extern int L_AddTriggerEx(lua_State* L);
extern int L_StopEvaluatingTriggers(lua_State* L);

// Alias functions
extern int L_AddAlias(lua_State* L);
extern int L_DeleteAlias(lua_State* L);
extern int L_EnableAlias(lua_State* L);
extern int L_IsAlias(lua_State* L);
extern int L_GetAlias(lua_State* L);
extern int L_GetAliasInfo(lua_State* L);
extern int L_GetAliasList(lua_State* L);
extern int L_EnableAliasGroup(lua_State* L);
extern int L_DeleteAliasGroup(lua_State* L);
extern int L_DeleteTemporaryAliases(lua_State* L);
extern int L_GetAliasOption(lua_State* L);
extern int L_SetAliasOption(lua_State* L);

// Timer functions
extern int L_AddTimer(lua_State* L);
extern int L_DeleteTimer(lua_State* L);
extern int L_EnableTimer(lua_State* L);
extern int L_IsTimer(lua_State* L);
extern int L_GetTimer(lua_State* L);
extern int L_GetTimerInfo(lua_State* L);
extern int L_GetTimerList(lua_State* L);
extern int L_ResetTimer(lua_State* L);
extern int L_ResetTimers(lua_State* L);
extern int L_DoAfter(lua_State* L);
extern int L_DoAfterNote(lua_State* L);
extern int L_DoAfterSpeedWalk(lua_State* L);
extern int L_DoAfterSpecial(lua_State* L);
extern int L_EnableTimerGroup(lua_State* L);
extern int L_DeleteTimerGroup(lua_State* L);
extern int L_DeleteTemporaryTimers(lua_State* L);
extern int L_GetTimerOption(lua_State* L);
extern int L_SetTimerOption(lua_State* L);

// Utility functions
extern int L_Hash(lua_State* L);
extern int L_Base64Encode(lua_State* L);
extern int L_Base64Decode(lua_State* L);
extern int L_StripANSI(lua_State* L);
extern int L_FixupEscapeSequences(lua_State* L);
extern int L_FixupHTML(lua_State* L);
extern int L_MakeRegularExpression(lua_State* L);
extern int L_GetUniqueNumber(lua_State* L);
extern int L_GetUniqueID(lua_State* L);
extern int L_CreateGUID(lua_State* L);
extern int L_Execute(lua_State* L);
extern int L_GetGlobalOption(lua_State* L);
extern int L_SetCursor(lua_State* L);
extern int L_Accelerator(lua_State* L);
extern int L_AcceleratorList(lua_State* L);
extern int L_AcceleratorTo(lua_State* L);
extern int L_Activate(lua_State* L);
extern int L_ActivateClient(lua_State* L);
extern int L_GetWorldID(lua_State* L);
extern int L_GetWorldList(lua_State* L);
extern int L_GetWorldIdList(lua_State* L);
extern int L_EditDistance(lua_State* L);
extern int L_OpenBrowser(lua_State* L);
extern int L_ChangeDir(lua_State* L);
extern int L_TranslateDebug(lua_State* L);
extern int L_GetUdpPort(lua_State* L);
extern int L_UdpSend(lua_State* L);
extern int L_UdpListen(lua_State* L);
extern int L_UdpPortList(lua_State* L);
extern int L_SpellCheck(lua_State* L);
extern int L_SpellCheckDlg(lua_State* L);
extern int L_SpellCheckCommand(lua_State* L);
extern int L_AddSpellCheckWord(lua_State* L);
extern int L_Metaphone(lua_State* L);
extern int L_ResetIP(lua_State* L);
extern int L_ImportXML(lua_State* L);
extern int L_ExportXML(lua_State* L);
extern int L_EnableGroup(lua_State* L);
extern int L_DeleteGroup(lua_State* L);
extern int L_GetClipboard(lua_State* L);
extern int L_SetClipboard(lua_State* L);
extern int L_ErrorDesc(lua_State* L);
extern int L_Replace(lua_State* L);
extern int L_Save(lua_State* L);
extern int L_GetLineInfo(lua_State* L);
extern int L_GetStyleInfo(lua_State* L);
extern int L_GetRecentLines(lua_State* L);
extern int L_Menu(lua_State* L);
extern int L_NoteColour(lua_State* L);
extern int L_NoteColourFore(lua_State* L);
extern int L_NoteColourBack(lua_State* L);
extern int L_NoteColourRGB(lua_State* L);
extern int L_NoteColourName(lua_State* L);
extern int L_GetNoteColourFore(lua_State* L);
extern int L_GetNoteColourBack(lua_State* L);
extern int L_SetNoteColour(lua_State* L);
extern int L_SetNoteColourFore(lua_State* L);
extern int L_SetNoteColourBack(lua_State* L);
extern int L_NoteStyle(lua_State* L);
extern int L_GetNoteStyle(lua_State* L);
extern int L_NoteHr(lua_State* L);
extern int L_PasteCommand(lua_State* L);
extern int L_GetCommandList(lua_State* L);
extern int L_SelectCommand(lua_State* L);
extern int L_GetQueue(lua_State* L);
extern int L_ShiftTabCompleteItem(lua_State* L);
extern int L_GetTriggerWildcard(lua_State* L);
extern int L_GetAliasWildcard(lua_State* L);
extern int L_Trace(lua_State* L);
extern int L_TraceOut(lua_State* L);
extern int L_Debug(lua_State* L);

// World notes functions
extern int L_GetNotes(lua_State* L);
extern int L_SetNotes(lua_State* L);

// Command history functions
extern int L_DeleteCommandHistory(lua_State* L);
extern int L_PushCommand(lua_State* L);

// Document state functions
extern int L_SetChanged(lua_State* L);

// Logging functions
extern int L_OpenLog(lua_State* L);
extern int L_CloseLog(lua_State* L);
extern int L_WriteLog(lua_State* L);
extern int L_FlushLog(lua_State* L);
extern int L_IsLogOpen(lua_State* L);
extern int L_GetLogInput(lua_State* L);
extern int L_SetLogInput(lua_State* L);
extern int L_GetLogNotes(lua_State* L);
extern int L_SetLogNotes(lua_State* L);
extern int L_GetLogOutput(lua_State* L);
extern int L_SetLogOutput(lua_State* L);
extern int L_LogSend(lua_State* L);

// Random number functions
extern int L_MtRand(lua_State* L);
extern int L_MtSrand(lua_State* L);

// Network info functions
extern int L_GetHostAddress(lua_State* L);
extern int L_GetHostName(lua_State* L);

// Script timing functions
extern int L_GetScriptTime(lua_State* L);

// UI Control functions
extern int L_FlashIcon(lua_State* L);
extern int L_Redraw(lua_State* L);
extern int L_Pause(lua_State* L);
extern int L_SetTitle(lua_State* L);
extern int L_SetMainTitle(lua_State* L);
extern int L_GetMainWindowPosition(lua_State* L);
extern int L_GetWorldWindowPosition(lua_State* L);
extern int L_GetWorldWindowPositionX(lua_State* L);
extern int L_MoveMainWindow(lua_State* L);
extern int L_MoveWorldWindow(lua_State* L);
extern int L_MoveWorldWindowX(lua_State* L);
extern int L_SetBackgroundColour(lua_State* L);
extern int L_SetOutputFont(lua_State* L);
extern int L_SetInputFont(lua_State* L);
extern int L_SetWorldWindowStatus(lua_State* L);
extern int L_SetForegroundImage(lua_State* L);
extern int L_SetFrameBackgroundColour(lua_State* L);
extern int L_SetToolBarPosition(lua_State* L);

// Database functions
extern int L_DatabaseOpen(lua_State* L);
extern int L_DatabaseClose(lua_State* L);
extern int L_DatabasePrepare(lua_State* L);
extern int L_DatabaseStep(lua_State* L);
extern int L_DatabaseFinalize(lua_State* L);
extern int L_DatabaseExec(lua_State* L);
extern int L_DatabaseColumns(lua_State* L);
extern int L_DatabaseColumnType(lua_State* L);
extern int L_DatabaseReset(lua_State* L);
extern int L_DatabaseChanges(lua_State* L);
extern int L_DatabaseTotalChanges(lua_State* L);
extern int L_DatabaseError(lua_State* L);
extern int L_DatabaseColumnName(lua_State* L);
extern int L_DatabaseColumnText(lua_State* L);
extern int L_DatabaseColumnValue(lua_State* L);
extern int L_DatabaseColumnNames(lua_State* L);
extern int L_DatabaseColumnValues(lua_State* L);
extern int L_DatabaseGetField(lua_State* L);
extern int L_DatabaseInfo(lua_State* L);
extern int L_DatabaseLastInsertRowid(lua_State* L);
extern int L_DatabaseList(lua_State* L);

// Plugin functions
extern int L_GetPluginID(lua_State* L);
extern int L_GetPluginName(lua_State* L);
extern int L_GetPluginList(lua_State* L);
extern int L_IsPluginInstalled(lua_State* L);

// Sound functions
extern int L_PlaySound(lua_State* L);
extern int L_StopSound(lua_State* L);
extern int L_Sound(lua_State* L);
extern int L_GetSoundStatus(lua_State* L);

// Notepad functions
extern int L_SendToNotepad(lua_State* L);
extern int L_AppendToNotepad(lua_State* L);
extern int L_ReplaceNotepad(lua_State* L);
extern int L_ActivateNotepad(lua_State* L);
extern int L_CloseNotepad(lua_State* L);
extern int L_GetNotepadText(lua_State* L);
extern int L_GetNotepadLength(lua_State* L);
extern int L_GetNotepadList(lua_State* L);
extern int L_SaveNotepad(lua_State* L);
extern int L_NotepadFont(lua_State* L);
extern int L_NotepadColour(lua_State* L);
extern int L_NotepadReadOnly(lua_State* L);
extern int L_NotepadSaveMethod(lua_State* L);
extern int L_MoveNotepadWindow(lua_State* L);
extern int L_GetNotepadWindowPosition(lua_State* L);

// Plugin functions
extern int L_GetPluginInfo(lua_State* L);
extern int L_LoadPlugin(lua_State* L);
extern int L_ReloadPlugin(lua_State* L);
extern int L_UnloadPlugin(lua_State* L);
extern int L_EnablePlugin(lua_State* L);
extern int L_CallPlugin(lua_State* L);
extern int L_PluginSupports(lua_State* L);
extern int L_BroadcastPlugin(lua_State* L);
extern int L_SendPkt(lua_State* L);
extern int L_SaveState(lua_State* L);
extern int L_GetPluginVariable(lua_State* L);
extern int L_GetPluginVariableList(lua_State* L);
extern int L_GetPluginTriggerList(lua_State* L);
extern int L_GetPluginAliasList(lua_State* L);
extern int L_GetPluginTimerList(lua_State* L);
extern int L_GetPluginTriggerInfo(lua_State* L);
extern int L_GetPluginAliasInfo(lua_State* L);
extern int L_GetPluginTimerInfo(lua_State* L);
extern int L_GetPluginTriggerOption(lua_State* L);
extern int L_GetPluginAliasOption(lua_State* L);
extern int L_GetPluginTimerOption(lua_State* L);
extern int L_AddFont(lua_State* L);

// Miniwindow functions
extern int L_WindowCreate(lua_State* L);
extern int L_WindowShow(lua_State* L);
extern int L_WindowPosition(lua_State* L);
extern int L_WindowSetZOrder(lua_State* L);
extern int L_WindowDelete(lua_State* L);
extern int L_WindowInfo(lua_State* L);
extern int L_WindowResize(lua_State* L);

// Miniwindow drawing primitives
extern int L_WindowRectOp(lua_State* L);
extern int L_WindowCircleOp(lua_State* L);
extern int L_WindowLine(lua_State* L);
extern int L_WindowPolygon(lua_State* L);
extern int L_WindowGradient(lua_State* L);
extern int L_WindowSetPixel(lua_State* L);
extern int L_WindowGetPixel(lua_State* L);
extern int L_WindowArc(lua_State* L);
extern int L_WindowBezier(lua_State* L);

// Miniwindow text and fonts
extern int L_WindowFont(lua_State* L);
extern int L_WindowText(lua_State* L);
extern int L_WindowTextWidth(lua_State* L);
extern int L_WindowFontInfo(lua_State* L);
extern int L_WindowFontList(lua_State* L);

// Miniwindow image operations
extern int L_WindowLoadImage(lua_State* L);
extern int L_WindowDrawImage(lua_State* L);
extern int L_WindowBlendImage(lua_State* L);
extern int L_WindowImageFromWindow(lua_State* L);
extern int L_WindowImageInfo(lua_State* L);
extern int L_WindowImageList(lua_State* L);
extern int L_WindowWrite(lua_State* L);
extern int L_WindowGetImageAlpha(lua_State* L);
extern int L_WindowDrawImageAlpha(lua_State* L);
extern int L_WindowMergeImageAlpha(lua_State* L);
extern int L_WindowTransformImage(lua_State* L);
extern int L_WindowFilter(lua_State* L);

// Miniwindow hotspots
extern int L_WindowAddHotspot(lua_State* L);
extern int L_WindowDeleteHotspot(lua_State* L);
extern int L_WindowDeleteAllHotspots(lua_State* L);
extern int L_WindowHotspotTooltip(lua_State* L);
extern int L_WindowDragHandler(lua_State* L);
extern int L_WindowMenu(lua_State* L);
extern int L_WindowHotspotInfo(lua_State* L);
extern int L_WindowMoveHotspot(lua_State* L);
extern int L_WindowScrollwheelHandler(lua_State* L);

// Pixel manipulation functions (standalone helpers)
extern int L_BlendPixel(lua_State* L);
extern int L_FilterPixel(lua_State* L);

// Forward declaration of constants registration (defined in lua_constants.cpp)
extern void register_lua_constants(lua_State* L);

// Forward declaration of array functions registration (defined in world_arrays.cpp)
extern void register_array_functions(lua_State* L, int worldTable);

/**
 * RegisterLuaRoutines - Main Lua API registration function
 *
 * Creates the "world" table, registers all Lua API functions,
 * registers constant tables, and sets up global aliases for
 * backward compatibility with legacy plugins.
 *
 * Called by ScriptEngine::openLua() during Lua state initialization.
 *
 * @param L Lua state
 * @return 0 on success
 */
int RegisterLuaRoutines(lua_State* L)
{
    // Register functions in world table
    static const luaL_Reg worldlib[] = {
        // Output functions
        {"Note", L_Note},
        {"Trim", L_Trim},
        {"ColourNote", L_ColourNote},
        {"ColourTell", L_ColourTell},
        {"Tell", L_Tell},
        {"ANSI", L_ANSI},
        {"AnsiNote", L_AnsiNote},
        {"Hyperlink", L_Hyperlink},
        {"Simulate", L_Simulate},

        // Info bar functions
        {"Info", L_Info},
        {"InfoClear", L_InfoClear},
        {"InfoColour", L_InfoColour},
        {"InfoColor", L_InfoColour}, // American spelling alias
        {"InfoBackground", L_InfoBackground},
        {"InfoFont", L_InfoFont},
        {"ShowInfoBar", L_ShowInfoBar},

        // Network functions
        {"Send", L_Send},
        {"SendNoEcho", L_SendNoEcho},
        {"Connect", L_Connect},
        {"Disconnect", L_Disconnect},
        {"IsConnected", L_IsConnected},

        // Variable functions
        {"GetVariable", L_GetVariable},
        {"SetVariable", L_SetVariable},
        {"DeleteVariable", L_DeleteVariable},
        {"GetVariableList", L_GetVariableList},

        // World info functions
        {"GetInfo", L_GetInfo},
        {"GetWorldName", L_GetWorldName},
        {"GetOption", L_GetOption},
        {"SetOption", L_SetOption},
        {"GetAlphaOption", L_GetAlphaOption},
        {"SetAlphaOption", L_SetAlphaOption},
        {"SetStatus", L_SetStatus},
        {"Repaint", L_Repaint},
        {"TextRectangle", L_TextRectangle},
        {"SetBackgroundImage", L_SetBackgroundImage},
        {"SetScroll", L_SetScroll},
        {"GetCommand", L_GetCommand},
        {"SetCommand", L_SetCommand},
        {"SetCommandSelection", L_SetCommandSelection},
        {"SetCommandWindowHeight", L_SetCommandWindowHeight},
        {"GetLineCount", L_GetLineCount},
        {"GetSentBytes", L_GetSentBytes},
        {"GetReceivedBytes", L_GetReceivedBytes},
        {"GetConnectDuration", L_GetConnectDuration},
        {"WorldAddress", L_WorldAddress},
        {"WorldPort", L_WorldPort},
        {"WorldName", L_WorldName},
        {"Version", L_Version},
        {"GetLinesInBufferCount", L_GetLinesInBufferCount},
        {"GetSelectionStartLine", L_GetSelectionStartLine},
        {"GetSelectionEndLine", L_GetSelectionEndLine},
        {"GetSelectionStartColumn", L_GetSelectionStartColumn},
        {"GetSelectionEndColumn", L_GetSelectionEndColumn},
        {"GetSysColor", L_GetSysColor},
        {"GetSystemMetrics", L_GetSystemMetrics},
        {"GetDeviceCaps", L_GetDeviceCaps},
        {"GetFrame", L_GetFrame},
        {"GetAlphaOptionList", L_GetAlphaOptionList},
        {"GetOptionList", L_GetOptionList},
        {"GetGlobalOptionList", L_GetGlobalOptionList},
        {"Queue", L_Queue},
        {"DiscardQueue", L_DiscardQueue},

        // Color functions
        {"GetNormalColour", L_GetNormalColour},
        {"GetBoldColour", L_GetBoldColour},
        {"SetNormalColour", L_SetNormalColour},
        {"SetBoldColour", L_SetBoldColour},
        {"GetCustomColourText", L_GetCustomColourText},
        {"GetCustomColourBackground", L_GetCustomColourBackground},
        {"SetCustomColourText", L_SetCustomColourText},
        {"SetCustomColourBackground", L_SetCustomColourBackground},
        {"SetCustomColourName", L_SetCustomColourName},
        {"PickColour", L_PickColour},
        {"AdjustColour", L_AdjustColour},

        // Trace/Echo/Speedwalk functions
        {"GetTrace", L_GetTrace},
        {"SetTrace", L_SetTrace},
        {"GetEchoInput", L_GetEchoInput},
        {"SetEchoInput", L_SetEchoInput},
        {"GetSpeedWalkDelay", L_GetSpeedWalkDelay},
        {"SetSpeedWalkDelay", L_SetSpeedWalkDelay},
        {"EvaluateSpeedwalk", L_EvaluateSpeedwalk},
        {"ReverseSpeedwalk", L_ReverseSpeedwalk},
        {"RemoveBacktracks", L_RemoveBacktracks},
        {"ColourNameToRGB", L_ColourNameToRGB},
        {"RGBColourToName", L_RGBColourToName},

        // Trigger functions
        {"AddTrigger", L_AddTrigger},
        {"DeleteTrigger", L_DeleteTrigger},
        {"EnableTrigger", L_EnableTrigger},
        {"GetTrigger", L_GetTrigger},
        {"GetTriggerInfo", L_GetTriggerInfo},
        {"GetTriggerList", L_GetTriggerList},
        {"IsTrigger", L_IsTrigger},
        {"EnableTriggerGroup", L_EnableTriggerGroup},
        {"DeleteTriggerGroup", L_DeleteTriggerGroup},
        {"DeleteTemporaryTriggers", L_DeleteTemporaryTriggers},
        {"GetTriggerOption", L_GetTriggerOption},
        {"SetTriggerOption", L_SetTriggerOption},
        {"AddTriggerEx", L_AddTriggerEx},
        {"StopEvaluatingTriggers", L_StopEvaluatingTriggers},

        // Alias functions
        {"AddAlias", L_AddAlias},
        {"DeleteAlias", L_DeleteAlias},
        {"EnableAlias", L_EnableAlias},
        {"GetAlias", L_GetAlias},
        {"GetAliasInfo", L_GetAliasInfo},
        {"GetAliasList", L_GetAliasList},
        {"IsAlias", L_IsAlias},
        {"EnableAliasGroup", L_EnableAliasGroup},
        {"DeleteAliasGroup", L_DeleteAliasGroup},
        {"DeleteTemporaryAliases", L_DeleteTemporaryAliases},
        {"GetAliasOption", L_GetAliasOption},
        {"SetAliasOption", L_SetAliasOption},

        // Timer functions
        {"AddTimer", L_AddTimer},
        {"DeleteTimer", L_DeleteTimer},
        {"EnableTimer", L_EnableTimer},
        {"GetTimer", L_GetTimer},
        {"GetTimerInfo", L_GetTimerInfo},
        {"IsTimer", L_IsTimer},
        {"GetTimerList", L_GetTimerList},
        {"ResetTimer", L_ResetTimer},
        {"ResetTimers", L_ResetTimers},
        {"DoAfter", L_DoAfter},
        {"DoAfterNote", L_DoAfterNote},
        {"DoAfterSpeedWalk", L_DoAfterSpeedWalk},
        {"DoAfterSpecial", L_DoAfterSpecial},
        {"EnableTimerGroup", L_EnableTimerGroup},
        {"DeleteTimerGroup", L_DeleteTimerGroup},
        {"DeleteTemporaryTimers", L_DeleteTemporaryTimers},
        {"GetTimerOption", L_GetTimerOption},
        {"SetTimerOption", L_SetTimerOption},

        // Utility functions
        {"Hash", L_Hash},
        {"Base64Encode", L_Base64Encode},
        {"Base64Decode", L_Base64Decode},
        {"Trim", L_Trim},
        {"GetUniqueNumber", L_GetUniqueNumber},
        {"GetUniqueID", L_GetUniqueID},
        {"CreateGUID", L_CreateGUID},
        {"StripANSI", L_StripANSI},
        {"FixupEscapeSequences", L_FixupEscapeSequences},
        {"FixupHTML", L_FixupHTML},
        {"MakeRegularExpression", L_MakeRegularExpression},
        {"Execute", L_Execute},
        {"SetStatus", L_SetStatus},
        {"GetGlobalOption", L_GetGlobalOption},
        {"SetCursor", L_SetCursor},
        {"Accelerator", L_Accelerator},
        {"AcceleratorList", L_AcceleratorList},
        {"AcceleratorTo", L_AcceleratorTo},
        {"Activate", L_Activate},
        {"ActivateClient", L_ActivateClient},
        {"GetWorldID", L_GetWorldID},
        {"GetWorldList", L_GetWorldList},
        {"GetWorldIdList", L_GetWorldIdList},
        {"GetUdpPort", L_GetUdpPort},
        {"UdpSend", L_UdpSend},
        {"UdpListen", L_UdpListen},
        {"UdpPortList", L_UdpPortList},
        {"SpellCheck", L_SpellCheck},
        {"SpellCheckDlg", L_SpellCheckDlg},
        {"SpellCheckCommand", L_SpellCheckCommand},
        {"AddSpellCheckWord", L_AddSpellCheckWord},
        {"Metaphone", L_Metaphone},
        {"ResetIP", L_ResetIP},
        {"EditDistance", L_EditDistance},
        {"OpenBrowser", L_OpenBrowser},
        {"ChangeDir", L_ChangeDir},
        {"TranslateDebug", L_TranslateDebug},
        {"ImportXML", L_ImportXML},
        {"ExportXML", L_ExportXML},
        {"EnableGroup", L_EnableGroup},
        {"DeleteGroup", L_DeleteGroup},
        {"GetClipboard", L_GetClipboard},
        {"SetClipboard", L_SetClipboard},
        {"ErrorDesc", L_ErrorDesc},
        {"Replace", L_Replace},
        {"Save", L_Save},
        {"GetLineInfo", L_GetLineInfo},
        {"GetStyleInfo", L_GetStyleInfo},
        {"GetRecentLines", L_GetRecentLines},
        {"Menu", L_Menu},
        {"NoteColour", L_NoteColour},
        {"NoteColourFore", L_NoteColourFore},
        {"NoteColourBack", L_NoteColourBack},
        {"NoteColourRGB", L_NoteColourRGB},
        {"NoteColourName", L_NoteColourName},
        {"GetNoteColour", L_NoteColour}, // alias
        {"GetNoteColourFore", L_GetNoteColourFore},
        {"GetNoteColourBack", L_GetNoteColourBack},
        {"SetNoteColour", L_SetNoteColour},
        {"SetNoteColourFore", L_SetNoteColourFore},
        {"SetNoteColourBack", L_SetNoteColourBack},
        {"NoteStyle", L_NoteStyle},
        {"GetNoteStyle", L_GetNoteStyle},
        {"NoteHr", L_NoteHr},
        {"PasteCommand", L_PasteCommand},
        {"GetCommandList", L_GetCommandList},
        {"SelectCommand", L_SelectCommand},
        {"GetQueue", L_GetQueue},
        {"ShiftTabCompleteItem", L_ShiftTabCompleteItem},
        {"GetTriggerWildcard", L_GetTriggerWildcard},
        {"GetAliasWildcard", L_GetAliasWildcard},
        {"Trace", L_Trace},
        {"TraceOut", L_TraceOut},
        {"Debug", L_Debug},

        // World notes functions
        {"GetNotes", L_GetNotes},
        {"SetNotes", L_SetNotes},

        // Command history functions
        {"DeleteCommandHistory", L_DeleteCommandHistory},
        {"PushCommand", L_PushCommand},

        // Document state functions
        {"SetChanged", L_SetChanged},

        // Logging functions
        {"OpenLog", L_OpenLog},
        {"CloseLog", L_CloseLog},
        {"WriteLog", L_WriteLog},
        {"FlushLog", L_FlushLog},
        {"IsLogOpen", L_IsLogOpen},
        {"GetLogInput", L_GetLogInput},
        {"SetLogInput", L_SetLogInput},
        {"GetLogNotes", L_GetLogNotes},
        {"SetLogNotes", L_SetLogNotes},
        {"GetLogOutput", L_GetLogOutput},
        {"SetLogOutput", L_SetLogOutput},
        {"LogSend", L_LogSend},

        // Random number functions
        {"MtRand", L_MtRand},
        {"MtSrand", L_MtSrand},

        // Network info functions
        {"GetHostAddress", L_GetHostAddress},
        {"GetHostName", L_GetHostName},

        // Script timing functions
        {"GetScriptTime", L_GetScriptTime},

        // UI Control functions
        {"FlashIcon", L_FlashIcon},
        {"Redraw", L_Redraw},
        {"Pause", L_Pause},
        {"SetTitle", L_SetTitle},
        {"SetMainTitle", L_SetMainTitle},
        {"GetMainWindowPosition", L_GetMainWindowPosition},
        {"GetWorldWindowPosition", L_GetWorldWindowPosition},
        {"GetWorldWindowPositionX", L_GetWorldWindowPositionX},
        {"MoveMainWindow", L_MoveMainWindow},
        {"MoveWorldWindow", L_MoveWorldWindow},
        {"MoveWorldWindowX", L_MoveWorldWindowX},
        {"SetBackgroundColour", L_SetBackgroundColour},
        {"SetOutputFont", L_SetOutputFont},
        {"SetInputFont", L_SetInputFont},
        {"SetWorldWindowStatus", L_SetWorldWindowStatus},
        {"SetForegroundImage", L_SetForegroundImage},
        {"SetFrameBackgroundColour", L_SetFrameBackgroundColour},
        {"SetToolBarPosition", L_SetToolBarPosition},

        // Database functions
        {"DatabaseOpen", L_DatabaseOpen},
        {"DatabaseClose", L_DatabaseClose},
        {"DatabasePrepare", L_DatabasePrepare},
        {"DatabaseStep", L_DatabaseStep},
        {"DatabaseFinalize", L_DatabaseFinalize},
        {"DatabaseExec", L_DatabaseExec},
        {"DatabaseColumns", L_DatabaseColumns},
        {"DatabaseColumnType", L_DatabaseColumnType},
        {"DatabaseReset", L_DatabaseReset},
        {"DatabaseChanges", L_DatabaseChanges},
        {"DatabaseTotalChanges", L_DatabaseTotalChanges},
        {"DatabaseError", L_DatabaseError},
        {"DatabaseColumnName", L_DatabaseColumnName},
        {"DatabaseColumnText", L_DatabaseColumnText},
        {"DatabaseColumnValue", L_DatabaseColumnValue},
        {"DatabaseColumnNames", L_DatabaseColumnNames},
        {"DatabaseColumnValues", L_DatabaseColumnValues},
        {"DatabaseGetField", L_DatabaseGetField},
        {"DatabaseInfo", L_DatabaseInfo},
        {"DatabaseLastInsertRowid", L_DatabaseLastInsertRowid},
        {"DatabaseList", L_DatabaseList},

        // Plugin functions
        {"GetPluginID", L_GetPluginID},
        {"GetPluginName", L_GetPluginName},
        {"GetPluginList", L_GetPluginList},
        {"IsPluginInstalled", L_IsPluginInstalled},
        {"GetPluginInfo", L_GetPluginInfo},
        {"LoadPlugin", L_LoadPlugin},
        {"ReloadPlugin", L_ReloadPlugin},
        {"UnloadPlugin", L_UnloadPlugin},
        {"EnablePlugin", L_EnablePlugin},
        {"CallPlugin", L_CallPlugin},
        {"PluginSupports", L_PluginSupports},
        {"BroadcastPlugin", L_BroadcastPlugin},
        {"SendPkt", L_SendPkt},
        {"SaveState", L_SaveState},
        {"GetPluginVariable", L_GetPluginVariable},
        {"GetPluginVariableList", L_GetPluginVariableList},
        {"GetPluginTriggerList", L_GetPluginTriggerList},
        {"GetPluginAliasList", L_GetPluginAliasList},
        {"GetPluginTimerList", L_GetPluginTimerList},
        {"GetPluginTriggerInfo", L_GetPluginTriggerInfo},
        {"GetPluginAliasInfo", L_GetPluginAliasInfo},
        {"GetPluginTimerInfo", L_GetPluginTimerInfo},
        {"GetPluginTriggerOption", L_GetPluginTriggerOption},
        {"GetPluginAliasOption", L_GetPluginAliasOption},
        {"GetPluginTimerOption", L_GetPluginTimerOption},
        {"AddFont", L_AddFont},

        // Miniwindow functions
        {"WindowCreate", L_WindowCreate},
        {"WindowShow", L_WindowShow},
        {"WindowPosition", L_WindowPosition},
        {"WindowSetZOrder", L_WindowSetZOrder},
        {"WindowDelete", L_WindowDelete},
        {"WindowInfo", L_WindowInfo},
        {"WindowResize", L_WindowResize},

        // Miniwindow drawing primitives
        {"WindowRectOp", L_WindowRectOp},
        {"WindowCircleOp", L_WindowCircleOp},
        {"WindowLine", L_WindowLine},
        {"WindowPolygon", L_WindowPolygon},
        {"WindowGradient", L_WindowGradient},
        {"WindowSetPixel", L_WindowSetPixel},
        {"WindowGetPixel", L_WindowGetPixel},
        {"WindowArc", L_WindowArc},
        {"WindowBezier", L_WindowBezier},

        // Miniwindow text and fonts
        {"WindowFont", L_WindowFont},
        {"WindowText", L_WindowText},
        {"WindowTextWidth", L_WindowTextWidth},
        {"WindowFontInfo", L_WindowFontInfo},
        {"WindowFontList", L_WindowFontList},

        // Miniwindow image operations
        {"WindowLoadImage", L_WindowLoadImage},
        {"WindowDrawImage", L_WindowDrawImage},
        {"WindowBlendImage", L_WindowBlendImage},
        {"WindowImageFromWindow", L_WindowImageFromWindow},
        {"WindowImageInfo", L_WindowImageInfo},
        {"WindowImageList", L_WindowImageList},
        {"WindowWrite", L_WindowWrite},
        {"WindowGetImageAlpha", L_WindowGetImageAlpha},
        {"WindowDrawImageAlpha", L_WindowDrawImageAlpha},
        {"WindowMergeImageAlpha", L_WindowMergeImageAlpha},
        {"WindowTransformImage", L_WindowTransformImage},
        {"WindowFilter", L_WindowFilter},

        // Pixel manipulation functions (standalone helpers)
        {"BlendPixel", L_BlendPixel},
        {"FilterPixel", L_FilterPixel},

        // Miniwindow hotspots
        {"WindowAddHotspot", L_WindowAddHotspot},
        {"WindowDeleteHotspot", L_WindowDeleteHotspot},
        {"WindowDeleteAllHotspots", L_WindowDeleteAllHotspots},
        {"WindowHotspotTooltip", L_WindowHotspotTooltip},
        {"WindowDragHandler", L_WindowDragHandler},
        {"WindowMenu", L_WindowMenu},
        {"WindowHotspotInfo", L_WindowHotspotInfo},
        {"WindowMoveHotspot", L_WindowMoveHotspot},
        {"WindowScrollwheelHandler", L_WindowScrollwheelHandler},

        // Sound functions
        {"PlaySound", L_PlaySound},
        {"StopSound", L_StopSound},
        {"Sound", L_Sound},
        {"GetSoundStatus", L_GetSoundStatus},

        // Notepad functions
        {"SendToNotepad", L_SendToNotepad},
        {"AppendToNotepad", L_AppendToNotepad},
        {"ReplaceNotepad", L_ReplaceNotepad},
        {"ActivateNotepad", L_ActivateNotepad},
        {"CloseNotepad", L_CloseNotepad},
        {"GetNotepadText", L_GetNotepadText},
        {"GetNotepadLength", L_GetNotepadLength},
        {"GetNotepadList", L_GetNotepadList},
        {"SaveNotepad", L_SaveNotepad},
        {"NotepadFont", L_NotepadFont},
        {"NotepadColour", L_NotepadColour},
        {"NotepadReadOnly", L_NotepadReadOnly},
        {"NotepadSaveMethod", L_NotepadSaveMethod},
        {"MoveNotepadWindow", L_MoveNotepadWindow},
        {"GetNotepadWindowPosition", L_GetNotepadWindowPosition},

        {nullptr, nullptr} // Sentinel
    };

    // Register all functions in the "world" table (Lua 5.1)
    luaL_register(L, "world", worldlib);

    // The world table is now on top of the stack - register array functions
    int worldTable = lua_gettop(L);
    register_array_functions(L, worldTable);

    // Register constant tables (error_code, trigger_flag, sendto, timer_flag, miniwin,
    // extended_colours)
    register_lua_constants(L);

    // ========== Register ALL World Functions as Globals for MUSHclient Compatibility ==========
    //
    // The original MUSHclient registers ALL functions as both:
    //   1. In the "world" table: world.ColourNote(...)
    //   2. As globals: ColourNote(...)
    //
    // Most plugins call functions directly without the "world." prefix.
    // We iterate over the world table and register each function as a global.

    // Get the world table (it's still on the stack from luaL_register)
    lua_getglobal(L, "world");
    lua_pushnil(L); // First key for iteration
    while (lua_next(L, -2) != 0) {
        // Stack: world table, key, value
        if (lua_isfunction(L, -1)) {
            // Get the function name (key is at -2)
            const char* name = lua_tostring(L, -2);
            if (name != nullptr) {
                // Push the function again (it will be consumed by setglobal)
                lua_pushvalue(L, -1);
                lua_setglobal(L, name);
            }
        }
        // Pop value, keep key for next iteration
        lua_pop(L, 1);
    }
    lua_pop(L, 1); // Pop world table

    // ========== Additional Global Registrations ==========
    //
    // Some functions need explicit global registration because they're not in the world table
    // or need special handling.

    // Register GetInfo as a global function (for compatibility with legacy plugins)
    // Plugins often call GetInfo(60) directly instead of world.GetInfo(60)
    lua_pushcfunction(L, L_GetInfo);
    lua_setglobal(L, "GetInfo");

    // Register color functions as globals (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetNormalColour);
    lua_setglobal(L, "GetNormalColour");

    lua_pushcfunction(L, L_GetBoldColour);
    lua_setglobal(L, "GetBoldColour");

    // Register GetPluginID as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetPluginID);
    lua_setglobal(L, "GetPluginID");

    // Register DoAfterSpecial as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_DoAfterSpecial);
    lua_setglobal(L, "DoAfterSpecial");

    // Register DoAfter as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_DoAfter);
    lua_setglobal(L, "DoAfter");

    // Register GetPluginVariable as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetPluginVariable);
    lua_setglobal(L, "GetPluginVariable");

    // Register GetPluginInfo as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetPluginInfo);
    lua_setglobal(L, "GetPluginInfo");

    // Register GetOption as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetOption);
    lua_setglobal(L, "GetOption");

    // Register GetVariable as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetVariable);
    lua_setglobal(L, "GetVariable");

    // Register SetVariable as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_SetVariable);
    lua_setglobal(L, "SetVariable");

    // Register AddFont as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_AddFont);
    lua_setglobal(L, "AddFont");

    // Register CallPlugin as global function (for inter-plugin communication)
    lua_pushcfunction(L, L_CallPlugin);
    lua_setglobal(L, "CallPlugin");

    // Register BroadcastPlugin as global function (for inter-plugin communication)
    lua_pushcfunction(L, L_BroadcastPlugin);
    lua_setglobal(L, "BroadcastPlugin");

    // Register SendPkt as global function (for raw telnet packet sending)
    lua_pushcfunction(L, L_SendPkt);
    lua_setglobal(L, "SendPkt");

    // Register ColourNameToRGB as global function (for color name to RGB conversion)
    lua_pushcfunction(L, L_ColourNameToRGB);
    lua_setglobal(L, "ColourNameToRGB");

    // Register RGBColourToName as global function (for RGB to color name conversion)
    lua_pushcfunction(L, L_RGBColourToName);
    lua_setglobal(L, "RGBColourToName");

    // Register SetCursor as global function (for changing mouse cursor)
    lua_pushcfunction(L, L_SetCursor);
    lua_setglobal(L, "SetCursor");

    // Register WorldName as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetWorldName);
    lua_setglobal(L, "WorldName");

    // Register AddTimer as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_AddTimer);
    lua_setglobal(L, "AddTimer");

    // Register DeleteTimer as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_DeleteTimer);
    lua_setglobal(L, "DeleteTimer");

    // Register Note as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Note);
    lua_setglobal(L, "Note");

    // Override print to use Note (so print() outputs to the client window, not stdout)
    lua_pushcfunction(L, L_Note);
    lua_setglobal(L, "print");

    // Register Trim as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Trim);
    lua_setglobal(L, "Trim");

    // Register Hash as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Hash);
    lua_setglobal(L, "Hash");

    // Register Base64Encode as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Base64Encode);
    lua_setglobal(L, "Base64Encode");

    // Register Base64Decode as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Base64Decode);
    lua_setglobal(L, "Base64Decode");

    // Register SetOption as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_SetOption);
    lua_setglobal(L, "SetOption");

    // Register SetStatus as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_SetStatus);
    lua_setglobal(L, "SetStatus");

    // Register Repaint as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Repaint);
    lua_setglobal(L, "Repaint");

    // Register TextRectangle as global function
    lua_pushcfunction(L, L_TextRectangle);
    lua_setglobal(L, "TextRectangle");

    // Register SetBackgroundImage as global function (stub for aard_layout compatibility)
    lua_pushcfunction(L, L_SetBackgroundImage);
    lua_setglobal(L, "SetBackgroundImage");

    // Register GetCommand as global function (stub for aard_layout compatibility)
    lua_pushcfunction(L, L_GetCommand);
    lua_setglobal(L, "GetCommand");

    // Register SetCommandWindowHeight as global function (stub for aard_layout compatibility)
    lua_pushcfunction(L, L_SetCommandWindowHeight);
    lua_setglobal(L, "SetCommandWindowHeight");

    // Register SetScroll as global function (stub for aard_layout compatibility)
    lua_pushcfunction(L, L_SetScroll);
    lua_setglobal(L, "SetScroll");

    // Register SaveState as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_SaveState);
    lua_setglobal(L, "SaveState");

    // Register Save as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Save);
    lua_setglobal(L, "Save");

    // Register EnableTimer as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_EnableTimer);
    lua_setglobal(L, "EnableTimer");

    // Register EnableTrigger as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_EnableTrigger);
    lua_setglobal(L, "EnableTrigger");

    // Register GetTriggerOption as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_GetTriggerOption);
    lua_setglobal(L, "GetTriggerOption");

    // Register SetTriggerOption as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_SetTriggerOption);
    lua_setglobal(L, "SetTriggerOption");

    // Register Accelerator functions as globals (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_Accelerator);
    lua_setglobal(L, "Accelerator");

    lua_pushcfunction(L, L_AcceleratorTo);
    lua_setglobal(L, "AcceleratorTo");

    lua_pushcfunction(L, L_AcceleratorList);
    lua_setglobal(L, "AcceleratorList");

    // Register EnableTriggerGroup as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_EnableTriggerGroup);
    lua_setglobal(L, "EnableTriggerGroup");

    // Register EnableAlias as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_EnableAlias);
    lua_setglobal(L, "EnableAlias");

    // Register EnableAliasGroup as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_EnableAliasGroup);
    lua_setglobal(L, "EnableAliasGroup");

    // Register IsConnected as global function (for compatibility with legacy plugins)
    lua_pushcfunction(L, L_IsConnected);
    lua_setglobal(L, "IsConnected");

    // Register miniwindow functions as globals (for compatibility with legacy plugins)
    // Miniwindow creation
    lua_pushcfunction(L, L_WindowCreate);
    lua_setglobal(L, "WindowCreate");
    lua_pushcfunction(L, L_WindowShow);
    lua_setglobal(L, "WindowShow");
    lua_pushcfunction(L, L_WindowPosition);
    lua_setglobal(L, "WindowPosition");
    lua_pushcfunction(L, L_WindowSetZOrder);
    lua_setglobal(L, "WindowSetZOrder");
    lua_pushcfunction(L, L_WindowDelete);
    lua_setglobal(L, "WindowDelete");
    lua_pushcfunction(L, L_WindowInfo);
    lua_setglobal(L, "WindowInfo");
    lua_pushcfunction(L, L_WindowResize);
    lua_setglobal(L, "WindowResize");

    // Drawing primitives
    lua_pushcfunction(L, L_WindowRectOp);
    lua_setglobal(L, "WindowRectOp");
    lua_pushcfunction(L, L_WindowCircleOp);
    lua_setglobal(L, "WindowCircleOp");
    lua_pushcfunction(L, L_WindowLine);
    lua_setglobal(L, "WindowLine");
    lua_pushcfunction(L, L_WindowPolygon);
    lua_setglobal(L, "WindowPolygon");
    lua_pushcfunction(L, L_WindowGradient);
    lua_setglobal(L, "WindowGradient");
    lua_pushcfunction(L, L_WindowSetPixel);
    lua_setglobal(L, "WindowSetPixel");
    lua_pushcfunction(L, L_WindowGetPixel);
    lua_setglobal(L, "WindowGetPixel");

    // Text and fonts
    lua_pushcfunction(L, L_WindowFont);
    lua_setglobal(L, "WindowFont");
    lua_pushcfunction(L, L_WindowText);
    lua_setglobal(L, "WindowText");
    lua_pushcfunction(L, L_WindowTextWidth);
    lua_setglobal(L, "WindowTextWidth");
    lua_pushcfunction(L, L_WindowFontInfo);
    lua_setglobal(L, "WindowFontInfo");
    lua_pushcfunction(L, L_WindowFontList);
    lua_setglobal(L, "WindowFontList");

    // Image operations
    lua_pushcfunction(L, L_WindowLoadImage);
    lua_setglobal(L, "WindowLoadImage");
    lua_pushcfunction(L, L_WindowDrawImage);
    lua_setglobal(L, "WindowDrawImage");
    lua_pushcfunction(L, L_WindowBlendImage);
    lua_setglobal(L, "WindowBlendImage");
    lua_pushcfunction(L, L_WindowImageFromWindow);
    lua_setglobal(L, "WindowImageFromWindow");
    lua_pushcfunction(L, L_WindowImageInfo);
    lua_setglobal(L, "WindowImageInfo");
    lua_pushcfunction(L, L_WindowImageList);
    lua_setglobal(L, "WindowImageList");

    // Hotspots
    lua_pushcfunction(L, L_WindowAddHotspot);
    lua_setglobal(L, "WindowAddHotspot");
    lua_pushcfunction(L, L_WindowDragHandler);
    lua_setglobal(L, "WindowDragHandler");
    lua_pushcfunction(L, L_WindowMenu);
    lua_setglobal(L, "WindowMenu");
    lua_pushcfunction(L, L_WindowHotspotInfo);
    lua_setglobal(L, "WindowHotspotInfo");
    lua_pushcfunction(L, L_WindowMoveHotspot);
    lua_setglobal(L, "WindowMoveHotspot");
    lua_pushcfunction(L, L_WindowScrollwheelHandler);
    lua_setglobal(L, "WindowScrollwheelHandler");

    // Register utils module
    luaopen_utils(L);

    // Register rex PCRE regex library
    luaopen_rex(L);

    return 0;
}

/**
 * getLuaFunctionNames - Get list of all Lua function names for Shift+Tab completion
 *
 * Dynamically queries the Lua state to collect function names from:
 * - world.* table (MUSHclient API functions)
 * - string.*, table.*, math.*, os.*, io.* (Lua standard libraries)
 * - coroutine.*, debug.*, bit.* (additional Lua libraries)
 * - Global functions (pairs, ipairs, type, etc.)
 *
 * Used for Shift+Tab completion in the command input.
 * Based on original MUSHclient's Shift+Tab function completion feature.
 */
QStringList getLuaFunctionNames(lua_State* L)
{
    QStringList result;

    if (!L) {
        return result;
    }

    // Tables to query: first entry is "world" (no prefix), rest have prefixes
    static const char* tables[] = {"world",     // MUSHclient API - no prefix
                                   "string",    // string library
                                   "table",     // table library
                                   "math",      // math library
                                   "os",        // os library
                                   "io",        // io library
                                   "coroutine", // coroutine library
                                   "debug",     // debug library
                                   "bit",       // bit operations (LuaBitOp)
                                   "package",   // package library
                                   "rex",       // PCRE regex library (if available)
                                   "lpeg",      // LPeg library (if available)
                                   "lfs",       // LuaFileSystem (if available)
                                   nullptr};

    // Query each table for functions
    for (int i = 0; tables[i] != nullptr; ++i) {
        lua_getglobal(L, tables[i]);
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                // Only include functions (and for world table, all entries)
                if (lua_isfunction(L, -1)) {
                    const char* name = lua_tostring(L, -2);
                    if (name) {
                        if (i == 0) {
                            // world.* functions: no prefix
                            result << QString::fromUtf8(name);
                        } else {
                            // Other libraries: prefix with table name
                            result << QStringLiteral("%1.%2").arg(tables[i], name);
                        }
                    }
                }
                lua_pop(L, 1); // pop value, keep key for next iteration
            }
        }
        lua_pop(L, 1); // pop the table
    }

    // Query global functions (pairs, ipairs, type, etc.)
    // These are functions directly in _G that aren't in a table
    static const char* globalFunctions[] = {
        "assert", "collectgarbage", "dofile",       "error",      "getfenv",  "getmetatable",
        "ipairs", "load",           "loadfile",     "loadstring", "next",     "pairs",
        "pcall",  "print",          "rawequal",     "rawget",     "rawset",   "require",
        "select", "setfenv",        "setmetatable", "tonumber",   "tostring", "type",
        "unpack", "xpcall",         nullptr};

    for (int i = 0; globalFunctions[i] != nullptr; ++i) {
        lua_getglobal(L, globalFunctions[i]);
        if (lua_isfunction(L, -1)) {
            result << QString::fromLatin1(globalFunctions[i]);
        }
        lua_pop(L, 1);
    }

    // Also query constant tables (sendto, error_code, trigger_flag, etc.)
    static const char* constTables[] = {"sendto",     "error_code", "trigger_flag",
                                        "alias_flag", "miniwin",    nullptr};

    for (int i = 0; constTables[i] != nullptr; ++i) {
        lua_getglobal(L, constTables[i]);
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                const char* name = lua_tostring(L, -2);
                if (name) {
                    result << QStringLiteral("%1.%2").arg(constTables[i], name);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    result.sort(Qt::CaseInsensitive);
    return result;
}
