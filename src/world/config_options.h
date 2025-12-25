// config_options.h
// Options tables for world serialization
//
// These tables provide metadata for all configurable world options,
// allowing generic serialization to/from XML without hardcoding each field.
//
// Ported from: doc.h (original MUSHclient)

#ifndef CONFIG_OPTIONS_H
#define CONFIG_OPTIONS_H

// ============================================================================
// NUMERIC OPTION FLAGS
// ============================================================================

// Special value types
#define OPT_CUSTOM_COLOUR        0x000001    // colour number (add 1 to colour to save, subtract 1 to load)
#define OPT_RGB_COLOUR           0x000002    // colour is RGB colour
#define OPT_DOUBLE               0x000004    // option is a double

// Update notifications - trigger UI/state updates when changed
#define OPT_UPDATE_VIEWS         0x000100    // if changed, update all views
#define OPT_UPDATE_INPUT_FONT    0x000200    // if changed, update input font
#define OPT_UPDATE_OUTPUT_FONT   0x000400    // if changed, update output font
#define OPT_FIX_OUTPUT_BUFFER    0x000800    // if changed, rework output buffer size
#define OPT_FIX_WRAP_COLUMN      0x001000    // if changed, wrap column has changed
#define OPT_FIX_SPEEDWALK_DELAY  0x002000    // if changed, speedwalk delay has changed
#define OPT_USE_MXP              0x004000    // if changed, use_mxp has changed
#define OPT_FIX_INPUT_WRAP       0x1000000   // if changed, input wrapping changed
#define OPT_FIX_TOOLTIP_VISIBLE  0x2000000   // if changed, tooltip visibility changed
#define OPT_FIX_TOOLTIP_START    0x4000000   // if changed, tooltip delay changed

// Access control flags
#define OPT_PLUGIN_CANNOT_READ   0x100000    // plugin may not read its value
#define OPT_PLUGIN_CANNOT_WRITE  0x200000    // plugin may not write its value
#define OPT_PLUGIN_CANNOT_RW     0x300000    // plugin may not read or write its value
#define OPT_CANNOT_WRITE         0x400000    // cannot be changed by any script
#define OPT_SERVER_CAN_WRITE     0x800000    // CAN be changed by <recommend_option> tag

// ============================================================================
// ALPHA (STRING) OPTION FLAGS
// ============================================================================

#define OPT_MULTLINE             0x000001    // multi-line option (eg. world notes)
#define OPT_KEEP_SPACES          0x000002    // preserve leading/trailing spaces
#define OPT_PASSWORD             0x000004    // use base 64 encoding
#define OPT_COMMAND_STACK        0x000008    // this is the command stack character
#define OPT_WORLD_ID             0x000010    // this is the world ID

// Alpha options can also use the update flags from numeric options:
// OPT_UPDATE_VIEWS, OPT_UPDATE_INPUT_FONT, OPT_UPDATE_OUTPUT_FONT, etc.

// ============================================================================
// OPTION TABLE STRUCTURES
// ============================================================================

// Metadata for one numeric/boolean configuration option
// Used for debug.options, MXP <option> tag, and XML serialization
typedef struct
{
    const char* pName;      // name, eg. "logoutput"
    double iDefault;        // original (default) value
    int iOffset;            // offset in WorldDocument (use offsetof macro)
    int iLength;            // length of field (1, 2, 4, or 8 bytes)
    double iMinimum;        // minimum size it can be
    double iMaximum;        // maximum size it can be, if both zero, assume boolean
    int iFlags;             // flags: colours, access control, update triggers, etc.
} tConfigurationNumericOption;

// Metadata for one string configuration option
// Used for XML load/save
typedef struct
{
    const char* pName;      // name, eg. "server"
    const char* sDefault;   // original (default) value
    int iOffset;            // offset in WorldDocument (use offsetof macro)
    int iFlags;             // flags: multiline, spaces, password, etc.
} tConfigurationAlphaOption;

// ============================================================================
// GLOBAL TABLES (defined in config_options.cpp)
// ============================================================================

// Table of all numeric/boolean options (173 entries + NULL terminator)
extern const tConfigurationNumericOption OptionsTable[];

// Table of all string options (69 entries + NULL terminator)
extern const tConfigurationAlphaOption AlphaOptionsTable[];

#endif // CONFIG_OPTIONS_H
