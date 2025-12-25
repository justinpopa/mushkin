#ifndef MXP_TYPES_H
#define MXP_TYPES_H

#include <QColor>
#include <QList>
#include <QMap>
#include <QString>

// Forward declarations
class WorldDocument;

// ========== MXP ACTION CONSTANTS ==========
// Based on mxp/mxp.h from original MUSHclient
// These define what action to take when an MXP element is opened

enum MXP_Action {
    MXP_ACTION_NONE = -1, // No action (custom element container)
    MXP_ACTION_SEND = 0,  // <send href="command">
    MXP_ACTION_BOLD,      // <b> or <bold>
    MXP_ACTION_UNDERLINE, // <u> or <underline>
    MXP_ACTION_ITALIC,    // <i> or <italic>
    MXP_ACTION_COLOR,     // <color fore=red back=blue>
    MXP_ACTION_VERSION,   // <version> - send client version
    MXP_ACTION_FONT,      // <font face=Arial size=12>
    MXP_ACTION_SOUND,     // <sound fname="file.wav">
    MXP_ACTION_USER,      // <user> - send username
    MXP_ACTION_PASSWORD,  // <password> - send password
    MXP_ACTION_RELOCATE,  // Causes a new connect to open
    MXP_ACTION_FRAME,     // Frame definition
    MXP_ACTION_DEST,      // Destination frame
    MXP_ACTION_IMAGE,     // <image fname="pic.png">
    MXP_ACTION_FILTER,    // Sound/image filter
    MXP_ACTION_HYPERLINK, // <a href="url"> (secure)
    MXP_ACTION_BR,        // <br> - hard line break (secure)
    MXP_ACTION_H1,        // <h1> - Level 1 heading (secure)
    MXP_ACTION_H2,        // <h2> - Level 2 heading (secure)
    MXP_ACTION_H3,        // <h3> - Level 3 heading (secure)
    MXP_ACTION_H4,        // <h4> - Level 4 heading (secure)
    MXP_ACTION_H5,        // <h5> - Level 5 heading (secure)
    MXP_ACTION_H6,        // <h6> - Level 6 heading (secure)
    MXP_ACTION_HR,        // <hr> - horizontal rule (secure)
    MXP_ACTION_NOBR,      // Non-breaking newline
    MXP_ACTION_P,         // <p> - paragraph break (secure)
    MXP_ACTION_STRIKE,    // <strike> - strikethrough
    MXP_ACTION_SCRIPT,    // Client script (secure)
    MXP_ACTION_SMALL,     // <small> - small text
    MXP_ACTION_TT,        // <tt> - non-proportional font
    MXP_ACTION_UL,        // <ul> - unordered list
    MXP_ACTION_OL,        // <ol> - ordered list
    MXP_ACTION_LI,        // <li> - list item
    MXP_ACTION_SAMP,      // <samp> - sample text
    MXP_ACTION_CENTER,    // <center> - center text
    MXP_ACTION_HIGH,      // <high> - highlight text
    MXP_ACTION_VAR,       // <var> - set variable
    MXP_ACTION_AFK,       // <afk> - away from keyboard time

    // Recent additions
    MXP_ACTION_GAUGE,  // <gauge> - progress bar
    MXP_ACTION_STAT,   // <stat> - status bar
    MXP_ACTION_EXPIRE, // <expire> - timed text

    // Non-standard yet
    MXP_ACTION_RESET,            // <reset> - close all open tags
    MXP_ACTION_MXP,              // <mxp> - MXP command
    MXP_ACTION_SUPPORT,          // <support> - what commands we support
    MXP_ACTION_OPTION,           // Client options set
    MXP_ACTION_RECOMMEND_OPTION, // Server sets option

    // Pueblo tags
    MXP_ACTION_PRE,      // <pre> - preformatted text
    MXP_ACTION_BODY,     // <body>
    MXP_ACTION_HEAD,     // <head>
    MXP_ACTION_HTML,     // <html>
    MXP_ACTION_TITLE,    // <title>
    MXP_ACTION_IMG,      // <img> - Pueblo image tag
    MXP_ACTION_XCH_PAGE, // Pueblo exchange page
    MXP_ACTION_XCH_PANE, // Pueblo exchange pane

    MXP_ACTION_COUNT // Total number of actions
};

// ========== MXP TAG FLAGS ==========
// Based on mxp/mxp.h from original MUSHclient
// These flags control tag behavior and security

#define TAG_OPEN 0x01     // Tag is "open" (insecure) - requires open mode
#define TAG_COMMAND 0x02  // Tag is self-closing (no </tag> needed)
#define TAG_PUEBLO 0x04   // Tag is Pueblo-only
#define TAG_MXP 0x08      // Tag is MXP-only
#define TAG_NO_RESET 0x10 // Not closed by <reset> (e.g., <body>)
#define TAG_NOT_IMP 0x20  // Not implemented (for <supports> tag)

// ========== ATOMIC ELEMENT STRUCTURE ==========
// Based on mxp/mxp.h from original MUSHclient
// Defines a built-in MXP element

struct AtomicElement {
    QString strName; // Element name: "bold", "send", "color"
    int iFlags;      // TAG_* flags above
    int iAction;     // MXP_ACTION_* constant
    QString strArgs; // Supported arguments: "href,hint,prompt"
};

// ========== ARGUMENT STRUCTURE ==========
// Based on OtherTypes.h from original MUSHclient
// Represents one argument to an MXP tag

struct MXPArgument {
    QString strName;  // Argument name: "href", "hint", "fore"
    QString strValue; // Argument value: "go north", "red"
    int iPosition;    // 1-based position in argument list
    bool bKeyword;    // True if keyword argument (OPEN, EMPTY)
    bool bUsed;       // Marked true when argument is used

    MXPArgument() : iPosition(0), bKeyword(false), bUsed(false)
    {
    }
};

typedef QList<MXPArgument*> MXPArgumentList;

// ========== ELEMENT ITEM STRUCTURE ==========
// Based on OtherTypes.h from original MUSHclient
// One atomic element reference in a custom element's expansion

struct ElementItem {
    AtomicElement* pAtomicElement; // Points to built-in element
    MXPArgumentList argumentList;  // Arguments for this element

    ElementItem() : pAtomicElement(nullptr)
    {
    }
    ~ElementItem()
    {
        qDeleteAll(argumentList);
    }
};

typedef QList<ElementItem*> ElementItemList;

// ========== CUSTOM ELEMENT STRUCTURE ==========
// Based on OtherTypes.h from original MUSHclient
// A user-defined MXP element

struct CustomElement {
    QString strName;                 // Element name: "hp", "boldtext"
    ElementItemList elementItemList; // Expansion: [<color red>, <b>]
    MXPArgumentList attributeList;   // ATT='col=red max=100'
    int iTag;                        // TAG=20 (for line tagging)
    QString strFlag;                 // FLAG='hp_var' (set variable)
    bool bOpen;                      // OPEN (requires open mode)
    bool bCommand;                   // EMPTY (no closing tag)

    CustomElement() : iTag(0), bOpen(false), bCommand(false)
    {
    }

    ~CustomElement()
    {
        qDeleteAll(elementItemList);
        qDeleteAll(attributeList);
    }
};

// ========== ENTITY STRUCTURE ==========
// Based on OtherTypes.h from original MUSHclient
// Reusing color structure for entities (maps name to Unicode codepoint)

struct MXPEntity {
    QString strName;    // Entity name: "lt", "gt", "amp"
    quint32 iCodepoint; // Unicode codepoint value (for single-char entities)
    QString strValue;   // String value (for multi-char custom entities)

    MXPEntity() : iCodepoint(0)
    {
    }
};

// ========== ACTIVE TAG STRUCTURE ==========
// Based on MXP implementation notes
// Tracks unclosed MXP tags for proper nesting validation

struct ActiveTag {
    QString strName; // Tag name: "bold", "color"
    bool bSecure;    // Was opened in secure mode?
    bool bNoReset;   // Protected from <reset>?
    int iAction;     // MXP_ACTION_* for this tag

    ActiveTag() : bSecure(false), bNoReset(false), iAction(-1)
    {
    }
};

// ========== TYPE ALIASES ==========

typedef QMap<QString, AtomicElement*> AtomicElementMap;
typedef QMap<QString, CustomElement*> CustomElementMap;
typedef QMap<QString, MXPEntity*> MXPEntityMap;
typedef QList<ActiveTag*> ActiveTagList;

// ========== MXP GAUGE/STAT TRACKING ==========
// Structure to track gauge and stat elements
struct MXPGauge {
    QString entity;  // Variable name (e.g., "hp", "mana")
    QString caption; // Display caption (e.g., "Health", "Mana")
    QString color;   // Color for gauge
    int max;         // Maximum value
    int current;     // Current value
    bool isGauge;    // true = gauge (progress bar), false = stat (numeric)

    MXPGauge() : max(100), current(0), isGauge(true)
    {
    }
};

typedef QMap<QString, MXPGauge> MXPGaugeMap;

#endif // MXP_TYPES_H
