#ifndef STYLE_H
#define STYLE_H

#include <QColor>
#include <QtGlobal>
#include <memory>

// Forward declaration
class Action;

// ========== COLOUR TYPE CONSTANTS (OtherTypes.h) ==========
// These are stored in the COLOURTYPE bits (0x0300) of iFlags

inline constexpr quint16 COLOUR_ANSI =
    0x0000; // ANSI colour from ANSI table (m_normalcolour/m_boldcolour)
inline constexpr quint16 COLOUR_CUSTOM =
    0x0100; // Custom colour from custom table (m_customtext/m_customback)
inline constexpr quint16 COLOUR_RGB = 0x0200;      // RGB colour directly in iForeColour/iBackColour
inline constexpr quint16 COLOUR_RESERVED = 0x0300; // Reserved for future use

// ========== ACTION TYPE CONSTANTS (OtherTypes.h) ==========
// These are stored in the ACTIONTYPE bits (0x0C00) of iFlags

inline constexpr quint16 ACTION_NONE = 0x0000;      // No action
inline constexpr quint16 ACTION_SEND = 0x0400;      // Send strAction to MUD
inline constexpr quint16 ACTION_HYPERLINK = 0x0800; // HTTP:// or mailto: link
inline constexpr quint16 ACTION_PROMPT = 0x0C00; // Send strAction to command window (prompt user)

// ========== STYLE FLAG BITS (OtherTypes.h) ==========
// These define visual text styling and special behaviors.
// Used in bitwise OR/AND operations on quint16 iFlags — inline constexpr, not enum class.

inline constexpr quint16 NORMAL = 0x0000;    // Mnemonic way of clearing all attributes
inline constexpr quint16 HILITE = 0x0001;    // Bold text
inline constexpr quint16 UNDERLINE = 0x0002; // Underlined text
inline constexpr quint16 BLINK = 0x0004; // Italic (blink is rarely used, so repurposed for italic)
inline constexpr quint16 INVERSE = 0x0008;    // Swap foreground/background colors
inline constexpr quint16 CHANGED = 0x0010;    // Colour has been changed by a trigger
inline constexpr quint16 STRIKEOUT = 0x0020;  // Strike-through text
inline constexpr quint16 COLOURTYPE = 0x0300; // Mask for color type bits (2 bits)
inline constexpr quint16 ACTIONTYPE = 0x0C00; // Mask for action type bits (2 bits)
inline constexpr quint16 STYLE_BITS = 0x0FFF; // Everything except START_TAG
inline constexpr quint16 START_TAG = 0x1000; // This style starts an MXP tag (strAction is tag name)
inline constexpr quint16 TEXT_STYLE =
    0x002F; // Text styling flags: bold, underline, italic, inverse, strikeout

// ========== POPUP MENU DELIMITER (OtherTypes.h) ==========
// Used for separating multiple menu items in action hints
// Example: <send "cmd1|cmd2|cmd3" hint="Menu|Item 1|Item 2|Item 3">

inline constexpr char POPUP_DELIMITER[] = "|";

// ========== ANSI COLOR INDICES ==========
// These are indices into the color lookup tables, not RGB values!
// Must match original MUSHclient: enum { BLACK=0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE }
// Note: ANSI_* versions are defined as enum in world_document.h

inline constexpr int WHITE = 7;
inline constexpr int BLACK = 0;

/**
 * Style - Text style run
 *
 * Source: OtherTypes.h (original MUSHclient)
 *
 * WHY THIS EXISTS:
 * Represents one contiguous run of same-styled text.
 * Example: "Hello world" where "Hello" is red and "world" is blue
 *          would be represented by 2 Style objects.
 *
 * Each Style describes:
 * - How many bytes of text it covers (iLength)
 * - Visual styling (bold, underline, italic, etc.) (iFlags)
 * - Colors (iForeColour, iBackColour)
 * - Optional action/hyperlink (pAction)
 *
 * MEMORY MANAGEMENT:
 * - Style objects are stored in Line's styleList using unique_ptr
 * - pAction is a shared_ptr, so it's automatically reference counted
 * - No manual memory management required
 */
class Style {
  public:
    /**
     * Constructor
     *
     * Source: OtherTypes.h (original MUSHclient)
     *
     * Initializes Style with default values:
     * - White foreground, black background
     * - Zero length (will be set when added to line)
     * - No flags (normal text)
     * - No action
     */
    Style();

    /**
     * Destructor
     *
     * Source: OtherTypes.h (original MUSHclient)
     *
     * Releases the Action reference if one is held.
     */
    ~Style();

    // Public members (same as original for direct access)
    quint16 iLength;                 // How many bytes (characters) this style affects
    quint16 iFlags;                  // Style bits (see defines above)
    QRgb iForeColour;                // Foreground color (interpretation depends on COLOURTYPE bits)
    QRgb iBackColour;                // Background color (interpretation depends on COLOURTYPE bits)
    std::shared_ptr<Action> pAction; // Action/hyperlink pointer (or nullptr)
};

#endif // STYLE_H
