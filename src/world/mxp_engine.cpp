/**
 * mxp_engine.cpp - MXP (MUD eXtension Protocol) engine
 *
 * Extracted from world_mxp.cpp and world_protocol.cpp.
 * All WorldDocument::MXP_*() and WorldDocument::Phase_MXP_*() implementations
 * are now MXPEngine:: methods. WorldDocument accesses MXP state through
 * m_mxpEngine->field or m_mxpEngine->method().
 *
 * References:
 * - Original: mxp (all .cpp files) from MUSHclient
 * - Spec: http://www.zuggsoft.com/zmud/mxp.htm
 */

#include "mxp_engine.h"
#include "../text/action.h"
#include "../text/line.h"
#include "../text/style.h"
#include "../utils/logging.h"
#include "telnet_parser.h"
#include "world_document.h"
#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>
#include <memory>

// ========================================================================
// Constructor / Destructor
// ========================================================================

MXPEngine::MXPEngine(WorldDocument& doc) : m_doc(doc)
{
}

MXPEngine::~MXPEngine()
{
    CleanupMXP();
}

// ========================================================================
// MXP Initialization
// ========================================================================

/**
 * InitializeMXPElements - Initialize the built-in atomic MXP elements
 *
 * Based on: mxp/mxpDefs.cpp
 *
 * Loads all 50+ standard MXP elements into m_atomicElementMap.
 * These are the tags that MUDs can use without definition.
 */
void MXPEngine::InitializeMXPElements()
{
    // Clear any existing elements (unique_ptr handles deletion)
    m_atomicElementMap.clear();

    // Define built-in MXP elements
    // Format: {name, flags, action, supported_args}

    struct MXP_ElementDef {
        const char* name;
        int flags;
        int action;
        const char* args;
    };

    static const MXP_ElementDef elements[] = {
        // Basic text styling (secure - work in all modes)
        {"b", TAG_MXP, MXP_ACTION_BOLD, ""},
        {"bold", TAG_MXP, MXP_ACTION_BOLD, ""},
        {"i", TAG_MXP, MXP_ACTION_ITALIC, ""},
        {"italic", TAG_MXP, MXP_ACTION_ITALIC, ""},
        {"u", TAG_MXP, MXP_ACTION_UNDERLINE, ""},
        {"underline", TAG_MXP, MXP_ACTION_UNDERLINE, ""},
        {"em", TAG_MXP, MXP_ACTION_ITALIC, ""},   // em = italic (original: mxpinit.cpp:34)
        {"strong", TAG_MXP, MXP_ACTION_BOLD, ""}, // strong = bold (original: mxpinit.cpp:41)
        {"s", TAG_MXP, MXP_ACTION_STRIKE, ""},    // short alias for strike
        {"strike", TAG_MXP, MXP_ACTION_STRIKE, ""},
        {"strikeout", TAG_MXP, MXP_ACTION_STRIKE, ""},
        {"small", TAG_MXP, MXP_ACTION_SMALL, ""},
        {"tt", TAG_MXP, MXP_ACTION_TT, ""},
        {"h", TAG_MXP, MXP_ACTION_HIGH, ""}, // short alias for high
        {"high", TAG_MXP, MXP_ACTION_HIGH, ""},

        // Paragraph and formatting (secure)
        {"p", TAG_MXP | TAG_COMMAND, MXP_ACTION_P, ""},
        {"br", TAG_MXP | TAG_COMMAND, MXP_ACTION_BR, ""},
        {"nobr", TAG_MXP, MXP_ACTION_NOBR, ""},
        {"sbr", TAG_MXP | TAG_COMMAND, MXP_ACTION_BR, ""},
        {"hr", TAG_MXP | TAG_COMMAND, MXP_ACTION_HR, ""},

        // Headings (secure)
        {"h1", TAG_MXP, MXP_ACTION_H1, ""},
        {"h2", TAG_MXP, MXP_ACTION_H2, ""},
        {"h3", TAG_MXP, MXP_ACTION_H3, ""},
        {"h4", TAG_MXP, MXP_ACTION_H4, ""},
        {"h5", TAG_MXP, MXP_ACTION_H5, ""},
        {"h6", TAG_MXP, MXP_ACTION_H6, ""},

        // Lists (secure)
        {"ul", TAG_MXP, MXP_ACTION_UL, ""},
        {"ol", TAG_MXP, MXP_ACTION_OL, ""},
        {"li", TAG_MXP, MXP_ACTION_LI, ""},

        // Color and font (secure)
        {"c", TAG_MXP, MXP_ACTION_COLOR, "fore,back"}, // short alias for color
        {"color", TAG_MXP, MXP_ACTION_COLOR, "fore,back"},
        {"font", TAG_MXP, MXP_ACTION_FONT, "face,size,color,back"},

        // Links and actions (hyperlink is secure, send requires open mode)
        {"a", TAG_MXP, MXP_ACTION_HYPERLINK, "href,xch_cmd,xch_hint"},
        {"send", TAG_MXP, MXP_ACTION_SEND, "href,hint,prompt"}, // secure (no TAG_OPEN)

        // Media (open mode required for security)
        {"sound", TAG_COMMAND | TAG_MXP, MXP_ACTION_SOUND, "fname,v,l,p,t,u"},   // secure
        {"music", TAG_COMMAND | TAG_MXP, MXP_ACTION_SOUND, "fname,v,l,p,t,u,c"}, // secure
        {"filter", TAG_MXP | TAG_NOT_IMP, MXP_ACTION_FILTER, ""}, // sound/image filter
        {"image", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_IMAGE,
         "fname,url,t,h,w,hspace,vspace,align"},

        // Gauges and stats (secure)
        {"gauge", TAG_MXP, MXP_ACTION_GAUGE, "entity,max,caption,color"},
        {"stat", TAG_MXP, MXP_ACTION_STAT, "entity,max,caption"},

        // Special commands (mostly secure)
        {"version", TAG_MXP | TAG_COMMAND, MXP_ACTION_VERSION, ""},
        {"support", TAG_MXP, MXP_ACTION_SUPPORT, ""},
        {"option", TAG_MXP | TAG_COMMAND, MXP_ACTION_OPTION, ""},
        {"recommend_option", TAG_MXP | TAG_COMMAND, MXP_ACTION_RECOMMEND_OPTION, ""},
        {"expire", TAG_MXP, MXP_ACTION_EXPIRE, "name"},
        {"v", TAG_MXP | TAG_COMMAND, MXP_ACTION_VAR, ""}, // short alias for var
        {"var", TAG_MXP | TAG_COMMAND, MXP_ACTION_VAR, ""},

        // User/password (open mode)
        {"user", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_USER, ""},
        {"username", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_USER, ""},
        {"password", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_PASSWORD, ""},
        {"pass", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_PASSWORD, ""},
        {"relocate", TAG_OPEN | TAG_COMMAND | TAG_MXP | TAG_NOT_IMP, MXP_ACTION_RELOCATE, ""},

        // Frame operations (secure - not fully implemented)
        {"frame", TAG_MXP | TAG_NOT_IMP, MXP_ACTION_FRAME,
         "name,action,title,internal,align,left,top,width,height,scrolling,floating"},
        {"dest", TAG_MXP | TAG_NOT_IMP, MXP_ACTION_DEST, "name,x,y,eol"},

        // Scripting (secure)
        {"script", TAG_MXP, MXP_ACTION_SCRIPT, ""},

        // Misc (secure)
        {"center", TAG_MXP, MXP_ACTION_CENTER, ""},
        {"samp", TAG_MXP, MXP_ACTION_SAMP, ""},
        {"afk", TAG_MXP | TAG_COMMAND, MXP_ACTION_AFK, ""},

        // Pueblo-specific tags
        {"pre", TAG_PUEBLO, MXP_ACTION_PRE, ""},
        {"body", TAG_PUEBLO | TAG_NO_RESET, MXP_ACTION_BODY, ""},
        {"head", TAG_PUEBLO | TAG_NO_RESET, MXP_ACTION_HEAD, ""},
        {"html", TAG_PUEBLO | TAG_NO_RESET, MXP_ACTION_HTML, ""},
        {"title", TAG_PUEBLO, MXP_ACTION_TITLE, ""},
        {"img", TAG_PUEBLO | TAG_COMMAND, MXP_ACTION_IMG,
         "src,fname,url,t,h,w,hspace,vspace,align"},
        {"xch_page", TAG_PUEBLO | TAG_COMMAND, MXP_ACTION_XCH_PAGE, ""},
        {"xch_pane", TAG_PUEBLO | TAG_COMMAND | TAG_NOT_IMP, MXP_ACTION_XCH_PANE, ""},

        // Special MXP commands
        {"reset", TAG_MXP | TAG_COMMAND, MXP_ACTION_RESET, ""},
        {"mxp", TAG_MXP | TAG_COMMAND, MXP_ACTION_MXP, ""},

        {nullptr, 0, 0, nullptr} // Sentinel
    };

    // Load elements into map
    for (int i = 0; elements[i].name; i++) {
        auto elem = std::make_unique<AtomicElement>();
        elem->strName = QString::fromLatin1(elements[i].name);
        elem->iFlags = elements[i].flags;
        elem->iAction = elements[i].action;
        elem->strArgs = QString::fromLatin1(elements[i].args);

        QString key = elem->strName.toLower();
        m_atomicElementMap.emplace(key, std::move(elem));
    }

    qCDebug(lcMXP) << "Initialized" << m_atomicElementMap.size() << "MXP elements";
}

/**
 * InitializeMXPEntities - Initialize standard HTML entities
 *
 * Based on: mxp/mxpDefs.cpp
 *
 * Loads common HTML entities like &lt;, &gt;, &amp;, etc.
 */
void MXPEngine::InitializeMXPEntities()
{
    // Clear any existing entities (unique_ptr handles deletion)
    m_entityMap.clear();

    struct MXP_EntityDef {
        const char* name;
        quint32 codepoint;
    };

    static const MXP_EntityDef entities[] = {
        // Basic HTML entities
        {"lt", '<'},
        {"gt", '>'},
        {"amp", '&'},
        {"quot", '"'},
        {"apos", '\''},

        // ISO-8859-1 Latin characters (160–255)
        {"nbsp", 0xA0},   // Non-breaking space
        {"iexcl", 0xA1},  // ¡
        {"cent", 0xA2},   // ¢
        {"pound", 0xA3},  // £
        {"curren", 0xA4}, // ¤
        {"yen", 0xA5},    // ¥
        {"brvbar", 0xA6}, // ¦
        {"sect", 0xA7},   // §
        {"uml", 0xA8},    // ¨
        {"copy", 0xA9},   // ©
        {"ordf", 0xAA},   // ª
        {"laquo", 0xAB},  // «
        {"not", 0xAC},    // ¬
        {"shy", 0xAD},    // Soft hyphen
        {"reg", 0xAE},    // ®
        {"macr", 0xAF},   // ¯
        {"deg", 0xB0},    // °
        {"plusmn", 0xB1}, // ±
        {"sup2", 0xB2},   // ²
        {"sup3", 0xB3},   // ³
        {"acute", 0xB4},  // ´
        {"micro", 0xB5},  // µ
        {"para", 0xB6},   // ¶
        {"middot", 0xB7}, // ·
        {"cedil", 0xB8},  // ¸
        {"sup1", 0xB9},   // ¹
        {"ordm", 0xBA},   // º
        {"raquo", 0xBB},  // »
        {"frac14", 0xBC}, // ¼
        {"frac12", 0xBD}, // ½
        {"frac34", 0xBE}, // ¾
        {"iquest", 0xBF}, // ¿
        {"Agrave", 0xC0}, // À
        {"Aacute", 0xC1}, // Á
        {"Acirc", 0xC2},  // Â
        {"Atilde", 0xC3}, // Ã
        {"Auml", 0xC4},   // Ä
        {"Aring", 0xC5},  // Å
        {"AElig", 0xC6},  // Æ
        {"Ccedil", 0xC7}, // Ç
        {"Egrave", 0xC8}, // È
        {"Eacute", 0xC9}, // É
        {"Ecirc", 0xCA},  // Ê
        {"Euml", 0xCB},   // Ë
        {"Igrave", 0xCC}, // Ì
        {"Iacute", 0xCD}, // Í
        {"Icirc", 0xCE},  // Î
        {"Iuml", 0xCF},   // Ï
        {"ETH", 0xD0},    // Ð
        {"Ntilde", 0xD1}, // Ñ
        {"Ograve", 0xD2}, // Ò
        {"Oacute", 0xD3}, // Ó
        {"Ocirc", 0xD4},  // Ô
        {"Otilde", 0xD5}, // Õ
        {"Ouml", 0xD6},   // Ö
        {"times", 0xD7},  // ×
        {"Oslash", 0xD8}, // Ø
        {"Ugrave", 0xD9}, // Ù
        {"Uacute", 0xDA}, // Ú
        {"Ucirc", 0xDB},  // Û
        {"Uuml", 0xDC},   // Ü
        {"Yacute", 0xDD}, // Ý
        {"THORN", 0xDE},  // Þ
        {"szlig", 0xDF},  // ß
        {"agrave", 0xE0}, // à
        {"aacute", 0xE1}, // á
        {"acirc", 0xE2},  // â
        {"atilde", 0xE3}, // ã
        {"auml", 0xE4},   // ä
        {"aring", 0xE5},  // å
        {"aelig", 0xE6},  // æ
        {"ccedil", 0xE7}, // ç
        {"egrave", 0xE8}, // è
        {"eacute", 0xE9}, // é
        {"ecirc", 0xEA},  // ê
        {"euml", 0xEB},   // ë
        {"igrave", 0xEC}, // ì
        {"iacute", 0xED}, // í
        {"icirc", 0xEE},  // î
        {"iuml", 0xEF},   // ï
        {"eth", 0xF0},    // ð
        {"ntilde", 0xF1}, // ñ
        {"ograve", 0xF2}, // ò
        {"oacute", 0xF3}, // ó
        {"ocirc", 0xF4},  // ô
        {"otilde", 0xF5}, // õ
        {"ouml", 0xF6},   // ö
        {"divide", 0xF7}, // ÷
        {"oslash", 0xF8}, // ø
        {"ugrave", 0xF9}, // ù
        {"uacute", 0xFA}, // ú
        {"ucirc", 0xFB},  // û
        {"uuml", 0xFC},   // ü
        {"yacute", 0xFD}, // ý
        {"thorn", 0xFE},  // þ
        {"yuml", 0xFF},   // ÿ

        // HTML 4 special characters
        {"OElig", 0x0152},  // Œ
        {"oelig", 0x0153},  // œ
        {"Scaron", 0x0160}, // Š
        {"scaron", 0x0161}, // š
        {"Yuml", 0x0178},   // Ÿ
        {"fnof", 0x0192},   // ƒ
        {"circ", 0x02C6},   // ˆ
        {"tilde", 0x02DC},  // ˜

        // Greek letters
        {"Alpha", 0x0391},    // Α
        {"Beta", 0x0392},     // Β
        {"Gamma", 0x0393},    // Γ
        {"Delta", 0x0394},    // Δ
        {"Epsilon", 0x0395},  // Ε
        {"Zeta", 0x0396},     // Ζ
        {"Eta", 0x0397},      // Η
        {"Theta", 0x0398},    // Θ
        {"Iota", 0x0399},     // Ι
        {"Kappa", 0x039A},    // Κ
        {"Lambda", 0x039B},   // Λ
        {"Mu", 0x039C},       // Μ
        {"Nu", 0x039D},       // Ν
        {"Xi", 0x039E},       // Ξ
        {"Omicron", 0x039F},  // Ο
        {"Pi", 0x03A0},       // Π
        {"Rho", 0x03A1},      // Ρ
        {"Sigma", 0x03A3},    // Σ
        {"Tau", 0x03A4},      // Τ
        {"Upsilon", 0x03A5},  // Υ
        {"Phi", 0x03A6},      // Φ
        {"Chi", 0x03A7},      // Χ
        {"Psi", 0x03A8},      // Ψ
        {"Omega", 0x03A9},    // Ω
        {"alpha", 0x03B1},    // α
        {"beta", 0x03B2},     // β
        {"gamma", 0x03B3},    // γ
        {"delta", 0x03B4},    // δ
        {"epsilon", 0x03B5},  // ε
        {"zeta", 0x03B6},     // ζ
        {"eta", 0x03B7},      // η
        {"theta", 0x03B8},    // θ
        {"iota", 0x03B9},     // ι
        {"kappa", 0x03BA},    // κ
        {"lambda", 0x03BB},   // λ
        {"mu", 0x03BC},       // μ
        {"nu", 0x03BD},       // ν
        {"xi", 0x03BE},       // ξ
        {"omicron", 0x03BF},  // ο
        {"pi", 0x03C0},       // π
        {"rho", 0x03C1},      // ρ
        {"sigmaf", 0x03C2},   // ς
        {"sigma", 0x03C3},    // σ
        {"tau", 0x03C4},      // τ
        {"upsilon", 0x03C5},  // υ
        {"phi", 0x03C6},      // φ
        {"chi", 0x03C7},      // χ
        {"psi", 0x03C8},      // ψ
        {"omega", 0x03C9},    // ω
        {"thetasym", 0x03D1}, // ϑ
        {"upsih", 0x03D2},    // ϒ
        {"piv", 0x03D6},      // ϖ

        // General punctuation and typographic characters
        {"ensp", 0x2002},   // En space
        {"emsp", 0x2003},   // Em space
        {"thinsp", 0x2009}, // Thin space
        {"zwnj", 0x200C},   // Zero-width non-joiner
        {"zwj", 0x200D},    // Zero-width joiner
        {"lrm", 0x200E},    // Left-to-right mark
        {"rlm", 0x200F},    // Right-to-left mark
        {"ndash", 0x2013},  // –
        {"mdash", 0x2014},  // —
        {"lsquo", 0x2018},  // '
        {"rsquo", 0x2019},  // '
        {"sbquo", 0x201A},  // ‚
        {"ldquo", 0x201C},  // "
        {"rdquo", 0x201D},  // "
        {"bdquo", 0x201E},  // „
        {"dagger", 0x2020}, // †
        {"Dagger", 0x2021}, // ‡
        {"bull", 0x2022},   // •
        {"hellip", 0x2026}, // …
        {"permil", 0x2030}, // ‰
        {"prime", 0x2032},  // ′
        {"Prime", 0x2033},  // ″
        {"lsaquo", 0x2039}, // ‹
        {"rsaquo", 0x203A}, // ›
        {"oline", 0x203E},  // ‾
        {"frasl", 0x2044},  // ⁄
        {"euro", 0x20AC},   // €
        {"trade", 0x2122},  // ™

        // Arrows
        {"larr", 0x2190}, // ←
        {"uarr", 0x2191}, // ↑
        {"rarr", 0x2192}, // →
        {"darr", 0x2193}, // ↓
        {"harr", 0x2194}, // ↔
        {"lArr", 0x21D0}, // ⇐
        {"uArr", 0x21D1}, // ⇑
        {"rArr", 0x21D2}, // ⇒
        {"dArr", 0x21D3}, // ⇓
        {"hArr", 0x21D4}, // ⇔

        // Mathematical operators
        {"forall", 0x2200}, // ∀
        {"part", 0x2202},   // ∂
        {"exist", 0x2203},  // ∃
        {"empty", 0x2205},  // ∅
        {"nabla", 0x2207},  // ∇
        {"isin", 0x2208},   // ∈
        {"notin", 0x2209},  // ∉
        {"ni", 0x220B},     // ∋
        {"prod", 0x220F},   // ∏
        {"sum", 0x2211},    // ∑
        {"minus", 0x2212},  // −
        {"lowast", 0x2217}, // ∗
        {"radic", 0x221A},  // √
        {"prop", 0x221D},   // ∝
        {"infin", 0x221E},  // ∞
        {"ang", 0x2220},    // ∠
        {"and", 0x2227},    // ∧
        {"or", 0x2228},     // ∨
        {"cap", 0x2229},    // ∩
        {"cup", 0x222A},    // ∪
        {"int", 0x222B},    // ∫
        {"there4", 0x2234}, // ∴
        {"sim", 0x223C},    // ∼
        {"cong", 0x2245},   // ≅
        {"asymp", 0x2248},  // ≈
        {"ne", 0x2260},     // ≠
        {"equiv", 0x2261},  // ≡
        {"le", 0x2264},     // ≤
        {"ge", 0x2265},     // ≥
        {"sub", 0x2282},    // ⊂
        {"sup", 0x2283},    // ⊃
        {"nsub", 0x2284},   // ⊄
        {"sube", 0x2286},   // ⊆
        {"supe", 0x2287},   // ⊇
        {"oplus", 0x2295},  // ⊕
        {"otimes", 0x2297}, // ⊗
        {"perp", 0x22A5},   // ⊥
        {"sdot", 0x22C5},   // ⋅
        {"lceil", 0x2308},  // ⌈
        {"rceil", 0x2309},  // ⌉
        {"lfloor", 0x230A}, // ⌊
        {"rfloor", 0x230B}, // ⌋
        {"lang", 0x2329},   // 〈
        {"rang", 0x232A},   // 〉
        {"loz", 0x25CA},    // ◊

        // Suit symbols
        {"spades", 0x2660}, // ♠
        {"clubs", 0x2663},  // ♣
        {"hearts", 0x2665}, // ♥
        {"diams", 0x2666},  // ♦

        {nullptr, 0} // Sentinel
    };

    // Load entities into map
    for (int i = 0; entities[i].name; i++) {
        auto entity = std::make_unique<MXPEntity>();
        entity->strName = QString::fromLatin1(entities[i].name);
        entity->iCodepoint = entities[i].codepoint;

        QString key = entity->strName;
        m_entityMap.emplace(key, std::move(entity));
    }

    qCDebug(lcMXP) << "Initialized" << m_entityMap.size() << "MXP entities";
}

/**
 * CleanupMXP - Free all MXP resources
 *
 * Called when world is closed or MXP is turned off.
 * unique_ptr maps/lists handle deletion automatically on clear().
 */
void MXPEngine::CleanupMXP()
{
    // Clearing unique_ptr containers destroys all owned objects automatically
    m_atomicElementMap.clear();
    m_customElementMap.clear();
    m_entityMap.clear();
    m_customEntityMap.clear();
    m_activeTagList.clear();

    qCDebug(lcMXP) << "Cleaned up MXP resources";
}

// ========================================================================
// MXP Lifecycle
// ========================================================================

/**
 * MXP_On - Turn on MXP (MUD eXtension Protocol)
 *
 * Port from: mxp/mxpOnOff.cpp
 *
 * @param manual If true, preserve existing custom elements/entities/active tags
 *               (user toggled MXP off then on again). If false (default), clear
 *               custom definitions — we assume a fresh connection.
 *
 * Original: MXP_On(bPueblo, bManual) — when bManual is true, skips clearing
 *           m_CustomElementMap, m_ActiveTagList, m_CustomEntityMap.
 */
void MXPEngine::MXP_On(bool manual)
{
    // Do nothing if already on
    if (m_bMXP) {
        return;
    }

    qCDebug(lcWorld) << "MXP turned on";

    // Basic MXP initialization
    m_bMXP = true;
    m_bPuebloActive = false; // Not Pueblo mode
    m_bMXP_script = false;
    m_bPreMode = false;
    m_iListMode = eNoList;
    m_iListCount = 0;
    m_iMXPerrors = 0;
    m_iMXPtags = 0;
    m_iMXPentities = 0;

    // If NOT a manual toggle, reset mode to open and clear custom definitions.
    // Original: mxpOnOff.cpp:136-155 — the !bManual branch resets mode and
    //           deletes custom elements, active tags, and custom entities.
    if (!manual) {
        m_iMXP_mode = eMXP_open;
        m_iMXP_defaultMode = eMXP_open;

        // Clear custom element/entity definitions and active tag stack.
        // Built-in maps (m_atomicElementMap, m_entityMap) are re-initialized below.
        m_customElementMap.clear();
        m_activeTagList.clear();
        m_customEntityMap.clear();
    }

    // (Re-)initialize built-in atomic elements and standard HTML entities.
    // These are statically defined and safe to reload on every non-manual start.
    InitializeMXPElements();
    InitializeMXPEntities();

    // TODO(mxp): Fire OnMXP_Start script callback and ON_PLUGIN_MXP_START plugin notification.
}

/**
 * MXP_Off - Turn off MXP or reset it
 *
 * Port from: mxp/mxpOnOff.cpp
 *
 * @param force If true, completely turn off MXP. If false, just reset to normal behavior.
 */
void MXPEngine::MXP_Off(bool force)
{
    // Do nothing else if already off
    if (!m_bMXP && !force) {
        return;
    }

    // Reset ANSI colors to default (original: MXP_Off calls InterpretANSIcode(0))
    m_doc.InterpretANSIcode(0);

    if (force) {
        qCDebug(lcWorld) << "Closing down MXP";
    }

    // Reset MXP state
    m_bInParagraph = false;
    m_bMXP_script = false;
    m_bPreMode = false;
    m_iListMode = eNoList;
    m_iListCount = 0;

    // Close ALL tags including secure ones (original: mxpOnOff.cpp:34 calls MXP_CloseAllTags)
    MXP_CloseOpenTags(true);

    if (force) {
        // Change back to open mode.
        // NOTE: Do NOT call CleanupMXP() here. The original MXP_Off(bCompletely=true)
        // does NOT clear custom elements/entities — that only happens in MXP_On(manual=false).
        // Clearing them here would destroy server-defined definitions that should survive
        // a manual off→on toggle (M175).
        MXP_mode_change(eMXP_open);

        // If in MXP collection phase, reset to Phase::NONE
        if (m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_ELEMENT ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_COMMENT ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_QUOTE ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_ENTITY ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_ROOM_NAME ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_ROOM_DESCRIPTION ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_ROOM_EXITS ||
            m_doc.m_telnetParser->m_phase == Phase::HAVE_MXP_WELCOME) {
            m_doc.m_telnetParser->m_phase = Phase::NONE;
        }

        if (m_bPuebloActive) {
            qCDebug(lcWorld) << "Pueblo turned off";
        } else {
            qCDebug(lcWorld) << "MXP turned off";
        }

        m_bPuebloActive = false;
        m_bMXP = false;

        // TODO(mxp): Fire OnMXP_Stop script callback and ON_PLUGIN_MXP_STOP plugin notification.
    }
}

/**
 * MXP_mode_change - Change MXP line security mode
 *
 * Port from: mxp/mxpMode.cpp
 *
 * @param mode New MXP mode (-1 means use default after newline)
 */
void MXPEngine::MXP_mode_change(int mode)
{
    // Change to default mode if -1
    if (mode == -1) {
        mode = m_iMXP_defaultMode;
    }

    // Debug output for mode changes
    const char* modeNames[] = {
        "open",                 // 0
        "secure",               // 1
        "locked",               // 2
        "reset",                // 3
        "secure next tag only", // 4
        "permanently open",     // 5
        "permanently secure",   // 6
        "permanently locked"    // 7
    };

    if (mode != m_iMXP_mode &&
        (mode == eMXP_perm_open || mode == eMXP_perm_secure || mode == eMXP_perm_locked ||
         m_iMXP_mode == eMXP_perm_open || m_iMXP_mode == eMXP_perm_secure ||
         m_iMXP_mode == eMXP_perm_locked)) {
        QString oldMode = (m_iMXP_mode >= 0 && m_iMXP_mode < 8)
                              ? QString(modeNames[m_iMXP_mode])
                              : QString("unknown mode %1").arg(m_iMXP_mode);
        QString newMode = (mode >= 0 && mode < 8) ? QString(modeNames[mode])
                                                  : QString("unknown mode %1").arg(mode);
        qCDebug(lcWorld) << "MXP mode change from" << oldMode << "to" << newMode;
    }

    // Close open tags when transitioning from OPEN to non-OPEN mode
    // Original: mxpMode.cpp:78-81
    if (MXP_Open() && mode != eMXP_open && mode != eMXP_perm_open) {
        MXP_CloseOpenTags();
    }

    // Set default mode based on new mode
    switch (mode) {
        case eMXP_open:
        case eMXP_secure:
        case eMXP_locked:
            m_iMXP_defaultMode = eMXP_open; // These make open the default
            break;

        case eMXP_secure_once:
            m_iMXP_previousMode = m_iMXP_mode; // Remember previous mode
            break;

        case eMXP_perm_open:
        case eMXP_perm_secure:
        case eMXP_perm_locked:
            m_iMXP_defaultMode = mode; // Permanent mode becomes the default
            break;
    }

    // Set the new mode
    m_iMXP_mode = mode;
}

// ========================================================================
// MXP Phase Handlers
// ========================================================================

/**
 * Phase_MXP_ELEMENT - Collect MXP element characters
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters for an MXP element starting with '<' until '>'.
 * Handles quotes and comments within elements.
 */
void MXPEngine::Phase_MXP_ELEMENT(unsigned char c)
{
    switch (c) {
        case '>':
            // End of element - process it
            MXP_collected_element();
            m_doc.m_telnetParser->m_phase = Phase::NONE;
            break;

        case '<':
            // Shouldn't have < inside <
            qCWarning(lcMXP) << "Got \"<\" inside \"<\" - discarding previous element";
            m_strMXPstring.clear(); // Start again
            break;

        case '\'':
        case '\"':
            // Quote inside element - switch to quote mode
            m_cMXPquoteTerminator = c;
            m_doc.m_telnetParser->m_phase = Phase::HAVE_MXP_QUOTE;
            m_strMXPstring += static_cast<char>(c); // Collect this character
            break;

        case '-':
            // May be a comment? Check for <!--
            m_strMXPstring += static_cast<char>(c); // Collect this character
            if (m_strMXPstring.left(3) == "!--") {
                m_doc.m_telnetParser->m_phase = Phase::HAVE_MXP_COMMENT;
            }
            break;

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}

/**
 * Phase_MXP_COMMENT - Collect MXP comment
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters within an MXP comment <!-- ... -->
 */
void MXPEngine::Phase_MXP_COMMENT(unsigned char c)
{
    switch (c) {
        case '>':
            // End of comment if preceded by --
            if (m_strMXPstring.right(2) == "--") {
                // Discard comment, switch phase back to none
                m_doc.m_telnetParser->m_phase = Phase::NONE;
                break;
            }
            // If > is not preceded by -- then just collect it
            // FALL THROUGH

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}

/**
 * Phase_MXP_QUOTE - Collect quoted string within MXP element
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters within quotes inside an MXP element.
 */
void MXPEngine::Phase_MXP_QUOTE(unsigned char c)
{
    // Closing quote? Change phase back to element collection
    if (c == m_cMXPquoteTerminator) {
        m_doc.m_telnetParser->m_phase = Phase::HAVE_MXP_ELEMENT;
    }

    m_strMXPstring += static_cast<char>(c); // Collect this character
}

/**
 * Phase_MXP_ENTITY - Collect MXP entity
 *
 * Port from: mxp/mxp_phases.cpp
 *
 * Collects characters for an MXP entity starting with '&' until ';'.
 */
void MXPEngine::Phase_MXP_ENTITY(unsigned char c)
{
    switch (c) {
        case ';':
            // End of entity - process it
            m_doc.m_telnetParser->m_phase = Phase::NONE;
            MXP_collected_entity();
            break;

        case '&':
            // Shouldn't have & inside &
            qCWarning(lcMXP) << "Got \"&\" inside \"&\" - discarding previous entity";
            m_strMXPstring.clear(); // Start again
            break;

        case '<':
            // Shouldn't have < inside &, but we are now collecting an element
            qCWarning(lcMXP) << "Got \"<\" inside \"&\" - switching to element collection";
            m_doc.m_telnetParser->m_phase = Phase::HAVE_MXP_ELEMENT;
            m_strMXPstring.clear(); // Start again
            break;

        default:
            // Any other character, add to string
            m_strMXPstring += static_cast<char>(c);
            break;
    }
}

// ========================================================================
// MXP Element Lookup
// ========================================================================

/**
 * MXP_FindAtomicElement - Look up a built-in MXP element
 *
 * @param name Element name (case-insensitive)
 * @return Pointer to element, or nullptr if not found
 */
AtomicElement* MXPEngine::MXP_FindAtomicElement(const QString& name)
{
    auto it = m_atomicElementMap.find(name.toLower());
    return it != m_atomicElementMap.end() ? it->second.get() : nullptr;
}

/**
 * MXP_FindCustomElement - Look up a user-defined MXP element
 *
 * @param name Element name (case-insensitive)
 * @return Pointer to element, or nullptr if not found
 */
CustomElement* MXPEngine::MXP_FindCustomElement(const QString& name)
{
    auto it = m_customElementMap.find(name.toLower());
    return it != m_customElementMap.end() ? it->second.get() : nullptr;
}

// ========================================================================
// MXP Parsing - Argument Extraction
// ========================================================================

/**
 * ParseMXPTag - Parse MXP tag string into name and arguments
 *
 * Based on: mxp/mxpOpenAtomic.cpp argument parsing
 *
 * @param tagString The tag content (without < >)
 * @param tagName Output: The tag name
 * @param args Output: List of arguments
 */
void MXPEngine::ParseMXPTag(const QString& tagString, QString& tagName, MXPArgumentList& args)
{
    // Clear any existing arguments (unique_ptr handles deletion)
    args.clear();

    QString str = tagString.trimmed();
    if (str.isEmpty()) {
        return;
    }

    // Extract tag name (first word)
    int spacePos = str.indexOf(QRegularExpression("\\s"));
    if (spacePos == -1) {
        // No arguments, just tag name
        tagName = str;
        return;
    }

    tagName = str.left(spacePos);
    QString argString = str.mid(spacePos + 1).trimmed();

    if (argString.isEmpty()) {
        return;
    }

    // Parse arguments
    // Format: name=value name='value' name="value" keyword
    int pos = 0;
    int argPosition = 1;

    while (pos < argString.length()) {
        // Skip whitespace
        while (pos < argString.length() && argString[pos].isSpace()) {
            pos++;
        }

        if (pos >= argString.length()) {
            break;
        }

        // Look for name=value or positional value
        int eqPos = argString.indexOf('=', pos);
        int nextSpace = argString.indexOf(QRegularExpression("\\s"), pos);

        // If no '=' before next space, it's a positional argument or keyword
        if (eqPos == -1 || (nextSpace != -1 && nextSpace < eqPos)) {
            // Positional argument (no name=value format)
            QString value;
            bool wasQuoted = false;

            // Check if this is a quoted value
            QChar quoteChar = argString[pos];
            if (quoteChar == '"' || quoteChar == '\'') {
                // Quoted positional value
                wasQuoted = true;
                pos++; // Skip opening quote
                int startPos = pos;

                // Find closing quote
                while (pos < argString.length() && argString[pos] != quoteChar) {
                    // Handle escaped quotes
                    if (argString[pos] == '\\' && pos + 1 < argString.length()) {
                        pos++; // Skip escape char
                    }
                    pos++;
                }

                value = argString.mid(startPos, pos - startPos);

                if (pos < argString.length()) {
                    pos++; // Skip closing quote
                }
            } else {
                // Unquoted positional value
                int endPos = nextSpace == -1 ? argString.length() : nextSpace;
                value = argString.mid(pos, endPos - pos).trimmed();
                pos = endPos;
            }

            // Create argument even if value is empty (for empty definitions like "'' EMPTY")
            // But skip empty unquoted values (whitespace only)
            bool isKeyword = !value.isEmpty() && value.toUpper() == value &&
                             (value == "OPEN" || value == "EMPTY" || value == "DELETE" ||
                              value == "ADD" || value == "REMOVE");

            // Create argument for keywords or any quoted value (even if empty)
            if (isKeyword || wasQuoted || !value.isEmpty()) {
                auto arg = std::make_unique<MXPArgument>();

                if (isKeyword) {
                    // It's a keyword flag
                    arg->strName = value;
                    arg->bKeyword = true;
                } else {
                    // It's a positional value (may be empty)
                    arg->strValue = value;
                    arg->bKeyword = false;
                }
                arg->iPosition = argPosition++;
                args.push_back(std::move(arg));
            }

            continue;
        }

        // Extract name
        QString name = argString.mid(pos, eqPos - pos).trimmed();
        pos = eqPos + 1;

        // Skip whitespace after =
        while (pos < argString.length() && argString[pos].isSpace()) {
            pos++;
        }

        if (pos >= argString.length()) {
            break;
        }

        // Extract value (handle quotes)
        QString value;
        QChar quoteChar = argString[pos];

        if (quoteChar == '"' || quoteChar == '\'') {
            // Quoted value
            pos++; // Skip opening quote
            int startPos = pos;

            // Find closing quote
            while (pos < argString.length() && argString[pos] != quoteChar) {
                // Handle escaped quotes
                if (argString[pos] == '\\' && pos + 1 < argString.length()) {
                    pos++; // Skip escape char
                }
                pos++;
            }

            value = argString.mid(startPos, pos - startPos);

            if (pos < argString.length()) {
                pos++; // Skip closing quote
            }
        } else {
            // Unquoted value (read until space)
            int startPos = pos;
            while (pos < argString.length() && !argString[pos].isSpace()) {
                pos++;
            }
            value = argString.mid(startPos, pos - startPos);
        }

        // Create argument
        if (!name.isEmpty()) {
            auto arg = std::make_unique<MXPArgument>();
            arg->strName = name;
            arg->strValue = value;
            arg->iPosition = argPosition++;
            args.push_back(std::move(arg));
        }
    }
}

/**
 * GetMXPArgument - Retrieve argument value by name
 *
 * @param args Argument list
 * @param name Argument name (case-insensitive)
 * @return Argument value, or empty string if not found
 */
QString MXPEngine::GetMXPArgument(MXPArgumentList& args, const QString& name, int position)
{
    MXPArgument* positionalMatch = nullptr;

    for (const auto& arg : args) {
        if (arg->bKeyword) {
            continue; // keywords are not arguments (original: mxputils.cpp:210-211)
        }
        // Name match takes priority
        if (!name.isEmpty() && arg->strName.compare(name, Qt::CaseInsensitive) == 0) {
            arg->bUsed = true;
            return arg->strValue;
        }
        // Remember positional match as fallback (original: mxputils.cpp:221-222)
        if (position > 0 && arg->iPosition == position) {
            positionalMatch = arg.get();
        }
    }

    // Fall back to positional match if no name match found
    if (positionalMatch) {
        positionalMatch->bUsed = true;
        return positionalMatch->strValue;
    }

    return QString();
}

/**
 * MXP_GetArgument - Convenient wrapper for GetMXPArgument
 *
 * Swaps the parameter order to make it more natural to call: MXP_GetArgument("href", args)
 */
QString MXPEngine::MXP_GetArgument(const QString& name, MXPArgumentList& args, int position)
{
    return GetMXPArgument(args, name, position);
}

/**
 * MXP_HasArgument - Check if an argument exists (keyword or named)
 *
 * Used to check for boolean arguments like "prompt" in <send prompt>
 */
bool MXPEngine::MXP_HasArgument(const QString& name, MXPArgumentList& args)
{
    for (const auto& arg : args) {
        if (arg->strName.compare(name, Qt::CaseInsensitive) == 0) {
            arg->bUsed = true;
            return true;
        }
    }
    return false;
}

// ========================================================================
// MXP Element Collection
// ========================================================================

/**
 * MXP_collected_element - Process a completed MXP element
 *
 * Based on: mxp/mxpProcess.cpp
 *
 * Called when we've collected a complete element: <...>
 * This determines if it's an opening tag, closing tag, or definition.
 */
void MXPEngine::MXP_collected_element()
{
    QString str = m_strMXPstring.trimmed();
    m_strMXPstring.clear();

    if (str.isEmpty()) {
        qCDebug(lcMXP) << "Empty MXP element";
        m_iMXPerrors++;
        return;
    }

    QChar firstChar = str[0];

    // Check for comment: <!-- -->
    if (str.startsWith("!--")) {
        // Already handled by comment collection state
        return;
    }

    // Check for definition: <!ELEMENT> or <!ENTITY>
    if (firstChar == '!') {
        MXP_Definition(str.mid(1).trimmed());
        return;
    }

    // Check for closing tag: </bold>
    if (firstChar == '/') {
        MXP_EndTag(str.mid(1).trimmed());
        return;
    }

    // Opening tag: <bold> or <send href='cmd'>
    MXP_StartTag(str);
}

/**
 * MXP_collected_entity - Process a completed MXP entity
 *
 * Based on: mxp/mxpProcess.cpp entity handling
 *
 * Called when we've collected a complete entity: &...;
 * Replaces the entity with its text value.
 */
void MXPEngine::MXP_collected_entity()
{
    QString entityName = m_strMXPstring;
    m_strMXPstring.clear();

    QString replacement = MXP_GetEntity(entityName);

    if (!replacement.isNull()) {
        // Inject replacement text back into processing
        // This is done by adding it to the current line
        for (int i = 0; i < replacement.length(); i++) {
            m_doc.AddToLine(replacement[i].toLatin1());
        }
        m_iMXPentities++;
    } else {
        // Unknown entity - show as-is
        QString entityStr = QString("&%1;").arg(entityName);
        for (int i = 0; i < entityStr.length(); i++) {
            m_doc.AddToLine(entityStr[i].toLatin1());
        }
        m_iMXPerrors++;
    }
}

// ========================================================================
// MXP Entity Resolution
// ========================================================================

/**
 * MXP_GetEntity - Resolve entity name to text
 *
 * Based on: mxp/mxpEntity.cpp
 *
 * Handles:
 * - Numeric entities: &#65; (decimal), &#x41; (hex)
 * - Named entities: &lt;, &gt;, &amp;, etc.
 * - Custom entities: defined by <!ENTITY>
 *
 * @param entityName Entity name (without & and ;)
 * @return Replacement text, or null QString if unknown
 */
QString MXPEngine::MXP_GetEntity(const QString& entityName)
{
    // Check for numeric entity: &#65; or &#x41;
    if (entityName.startsWith('#')) {
        bool ok;
        quint32 codepoint;

        if (entityName.length() > 1 && (entityName[1] == 'x' || entityName[1] == 'X')) {
            // Hex: &#x41;
            codepoint = entityName.mid(2).toUInt(&ok, 16);
        } else {
            // Decimal: &#65;
            codepoint = entityName.mid(1).toUInt(&ok, 10);
        }

        if (!ok || codepoint > 0x10FFFF) {
            qCDebug(lcMXP) << "Invalid numeric entity:" << entityName;
            return QString(); // Invalid
        }

        // Check for disallowed control characters
        if (codepoint < 32 && codepoint != 9 && codepoint != 10 && codepoint != 13) {
            qCDebug(lcMXP) << "Disallowed control character entity:" << entityName;
            return QString(); // Disallowed
        }

        // Use QString::fromUcs4 to properly handle high codepoints (U+10000 and above)
        // QChar can only hold 16-bit values (U+0000 to U+FFFF)
        return QString::fromUcs4(reinterpret_cast<const char32_t*>(&codepoint), 1);
    }

    // Custom entity lookup is case-insensitive: keys are stored lowercased by
    // MXP_DefineEntity (line: strName = remaining.left(spacePos).toLower()).
    // Original: mxpEntities.cpp:64-66 — MXP_GetEntity lowercases the name before
    // looking up m_CustomEntityMap.
    const QString lowerEntityName = entityName.toLower();

    // Check custom entities first (they can override)
    {
        auto it = m_customEntityMap.find(lowerEntityName);
        if (it != m_customEntityMap.end()) {
            const MXPEntity* entity = it->second.get();
            // Custom entities use strValue (can be multi-character)
            if (!entity->strValue.isEmpty()) {
                return entity->strValue;
            }
            // Fallback to iCodepoint for backward compatibility
            return QString::fromUcs4(reinterpret_cast<const char32_t*>(&entity->iCodepoint), 1);
        }
    }

    // Check standard entities (these always use iCodepoint).
    // Standard entities use the original (non-lowercased) name since HTML entities
    // like &lt; &gt; &amp; etc. are already lowercase by convention.
    {
        auto it = m_entityMap.find(entityName);
        if (it != m_entityMap.end()) {
            const MXPEntity* entity = it->second.get();
            return QString::fromUcs4(reinterpret_cast<const char32_t*>(&entity->iCodepoint), 1);
        }
    }

    qCDebug(lcMXP) << "Unknown entity:" << entityName;
    return QString(); // Unknown
}

// ========================================================================
// MXP Tag Processing
// ========================================================================

/**
 * MXP_StartTag - Process an opening MXP tag
 *
 * Based on: mxpStart.cpp::MXP_StartTag
 *
 * Validates security, expands custom elements, and executes actions.
 *
 * @param tagString Tag content (without < >)
 */
void MXPEngine::MXP_StartTag(const QString& tagString)
{
    QString tagName;
    MXPArgumentList args;

    ParseMXPTag(tagString, tagName, args);

    if (tagName.isEmpty()) {
        qCWarning(lcMXP) << "Empty tag name";
        m_iMXPerrors++;
        return;
    }

    tagName = tagName.toLower(); // Case-insensitive

    // Check if current mode is secure
    bool bSecure = (m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure);

    // Restore mode (cancel secure-once) — original: MXP_Restore_Mode in mxpMode.cpp
    if (m_iMXP_mode == eMXP_secure_once) {
        m_iMXP_mode = m_iMXP_previousMode;
    }

    // Count tags
    m_iMXPtags++;

    // Look up element (atomic or custom)
    AtomicElement* pAtomicElement = MXP_FindAtomicElement(tagName);
    CustomElement* pCustomElement = nullptr;
    bool bOpen = false;
    bool bCommand = false;
    bool bNoReset = false;

    if (pAtomicElement) {
        // Atomic element found
        bOpen = (pAtomicElement->iFlags & TAG_OPEN) != 0;
        bCommand = (pAtomicElement->iFlags & TAG_COMMAND) != 0;
        bNoReset = (pAtomicElement->iFlags & TAG_NO_RESET) != 0;
    } else {
        // Try custom element
        pCustomElement = MXP_FindCustomElement(tagName);
        if (!pCustomElement) {
            qCWarning(lcMXP) << "Unknown MXP element:" << tagName;
            m_iMXPerrors++;
            return;
        }
        bOpen = pCustomElement->bOpen;
        bCommand = pCustomElement->bCommand;
    }

    // SECURITY CHECK: Tags WITHOUT TAG_OPEN are secure-only; reject in non-secure mode
    // (original: mxpStart.cpp:140-148 — if (!bOpen && !bSecure) reject)
    if (!bOpen && !bSecure) {
        qCWarning(lcMXP) << "Secure MXP tag ignored when not in secure mode:" << tagName;
        m_iMXPerrors++;
        return;
    }

    // Expand entities in arguments
    for (const auto& arg : args) {
        if (arg->strValue.contains('&')) {
            QString expanded;
            int pos = 0;
            while (pos < arg->strValue.length()) {
                if (arg->strValue[pos] == '&') {
                    int semiPos = arg->strValue.indexOf(';', pos + 1);
                    if (semiPos == -1) {
                        qCWarning(lcMXP) << "No closing ; in argument:" << arg->strValue;
                        m_iMXPerrors++;
                        break;
                    }
                    QString entityName = arg->strValue.mid(pos + 1, semiPos - pos - 1);

                    // Special case: &text; means the tag's body text (handled later)
                    if (entityName == "text") {
                        expanded += "&text;";
                    } else {
                        expanded += MXP_GetEntity(entityName);
                    }
                    pos = semiPos + 1;
                } else {
                    expanded += arg->strValue[pos];
                    pos++;
                }
            }
            arg->strValue = expanded;
        }
    }

    // Track non-command tags in active tag list
    if (!bCommand) {
        auto tag = std::make_unique<ActiveTag>();
        tag->strName = tagName;
        tag->bSecure = bSecure;
        tag->bNoReset = bNoReset;
        tag->iAction = pAtomicElement ? pAtomicElement->iAction : MXP_ACTION_NONE;
        m_activeTagList.push_back(std::move(tag));

        // Warn if too many outstanding tags
        if (m_activeTagList.size() % 100 == 0) {
            qCWarning(lcMXP) << "Many outstanding MXP tags:" << m_activeTagList.size();
        }
    }

    // Execute element
    if (pAtomicElement) {
        // Execute atomic element action
        MXP_ExecuteAction(pAtomicElement, args);
    } else if (pCustomElement) {
        // Expand custom element to atomic elements
        for (const auto& item : pCustomElement->elementItemList) {
            if (!item->pAtomicElement)
                continue;

            // Merge custom element attributes with item arguments
            MXPArgumentList expandedArgs;

            // Add custom element's default attributes
            for (const auto& attr : pCustomElement->attributeList) {
                auto newArg = std::make_unique<MXPArgument>();
                *newArg = *attr; // Copy
                expandedArgs.push_back(std::move(newArg));
            }

            // Add item-specific arguments
            for (const auto& itemArg : item->argumentList) {
                auto newArg = std::make_unique<MXPArgument>();
                *newArg = *itemArg; // Copy
                expandedArgs.push_back(std::move(newArg));
            }

            // Override with user-provided arguments
            for (const auto& userArg : args) {
                // Find if this argument was already added
                bool found = false;
                for (const auto& arg : expandedArgs) {
                    if (arg->strName.compare(userArg->strName, Qt::CaseInsensitive) == 0) {
                        arg->strValue = userArg->strValue;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto newArg = std::make_unique<MXPArgument>();
                    *newArg = *userArg; // Copy
                    expandedArgs.push_back(std::move(newArg));
                }
            }

            // Execute the atomic element
            MXP_ExecuteAction(item->pAtomicElement, expandedArgs);
            // expandedArgs cleared automatically by unique_ptr when it goes out of scope
        }
    }
    // args cleared automatically by unique_ptr when it goes out of scope
}

/**
 * MXP_EndTag - Process a closing MXP tag
 *
 * Based on: mxpEnd.cpp::MXP_EndTag
 *
 * Finds matching opening tag and closes all tags up to and including it.
 * Validates security constraints during closing.
 *
 * @param tagString Tag content (without </ >)
 */
void MXPEngine::MXP_EndTag(const QString& tagString)
{
    // Check if current mode is secure
    bool bSecure = (m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure);

    // Restore mode (cancel secure-once) — original: MXP_Restore_Mode in mxpMode.cpp
    if (m_iMXP_mode == eMXP_secure_once) {
        m_iMXP_mode = m_iMXP_previousMode;
    }

    QString strName = tagString.trimmed().toLower(); // Case-insensitive

    if (strName.isEmpty()) {
        qCWarning(lcMXP) << "Empty closing tag name";
        m_iMXPerrors++;
        return;
    }

    // Check for inappropriate arguments in closing tag
    if (tagString.contains(' ') || tagString.contains('\t')) {
        qCWarning(lcMXP) << "Closing tag has inappropriate arguments:" << tagString;
    }

    // Find the matching opening tag in active tag list
    // Search backwards (most recent first)
    const ActiveTag* pMatchingTag = nullptr;
    int matchIndex = -1;

    for (int i = m_activeTagList.size() - 1; i >= 0; i--) {
        const ActiveTag* pTag = m_activeTagList[i].get();

        if (pTag->strName == strName) {
            pMatchingTag = pTag;
            matchIndex = i;
            break;
        }

        // Check we don't cross over secure tags when finding it
        if (!bSecure && pTag->bSecure) {
            qCWarning(lcMXP) << "Cannot close tag <" << strName << "> - blocked by secure tag <"
                             << pTag->strName << ">";
            return;
        }
    }

    if (!pMatchingTag) {
        qCDebug(lcMXP) << "Closing tag </" << strName << "> has no corresponding opening tag";
        return;
    }

    // Security check: Can't close secure tags from non-secure mode
    if (!bSecure && pMatchingTag->bSecure) {
        qCWarning(lcMXP) << "Cannot close tag <" << strName << "> - it was opened in secure mode";
        return;
    }

    // Close all tags from the end up to and including the matching tag
    // This handles improperly nested tags: <b><i></b></i> - </b> also closes <i>
    while (static_cast<int>(m_activeTagList.size()) > matchIndex) {
        // Move last element out and pop; unique_ptr destructor cleans up at end of scope
        auto owned = std::move(m_activeTagList.back());
        m_activeTagList.pop_back();
        QString closingName = owned->strName;

        if (closingName != strName) {
            qCDebug(lcMXP) << "Closing out-of-sequence tag:" << closingName;
        }

        // Execute end action for this tag
        MXP_EndAction(owned->iAction);
        // owned goes out of scope here — ActiveTag is destroyed automatically

        if (closingName == strName) {
            break; // Found the tag we're supposed to close
        }
    }
}

/**
 * MXP_Definition - Process MXP definition
 */
void MXPEngine::MXP_Definition(const QString& defString)
{
    // Definitions require secure mode (original: mxpDefs.cpp:30-42)
    bool bSecure = (m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure ||
                    m_iMXP_mode == eMXP_secure_once);

    // Restore mode (cancel secure-once)
    if (m_iMXP_mode == eMXP_secure_once) {
        m_iMXP_mode = m_iMXP_previousMode;
    }

    if (!bSecure) {
        qCWarning(lcMXP) << "MXP definition ignored when not in secure mode: <!" << defString
                         << ">";
        m_iMXPerrors++;
        return;
    }

    if (defString.startsWith("ELEMENT", Qt::CaseInsensitive)) {
        MXP_DefineElement(defString.mid(7).trimmed());
    } else if (defString.startsWith("ENTITY", Qt::CaseInsensitive)) {
        MXP_DefineEntity(defString.mid(6).trimmed());
    } else {
        qCDebug(lcMXP) << "Unknown definition:" << defString;
    }
}

/**
 * MXP_DefineElement - Define custom element
 *
 * Based on: mxpDefs.cpp::MXP_Element
 *
 * @param defString Element definition (after "ELEMENT")
 */
void MXPEngine::MXP_DefineElement(const QString& defString)
{
    QString tagName;
    MXPArgumentList args;

    // Parse the entire definition as if it's a tag with arguments
    ParseMXPTag(defString, tagName, args);

    if (tagName.isEmpty()) {
        qCWarning(lcMXP) << "Missing element name in definition:" << defString;
        m_iMXPerrors++;
        return;
    }

    QString strName = tagName.toLower(); // Case-insensitive

    // Check if trying to redefine built-in element
    if (MXP_FindAtomicElement(strName)) {
        qCWarning(lcMXP) << "Cannot redefine built-in element:" << strName;
        m_iMXPerrors++;
        return;
    }

    // Check for DELETE keyword
    bool bDelete = false;
    for (const auto& arg : args) {
        if (arg->bKeyword && arg->strName.compare("DELETE", Qt::CaseInsensitive) == 0) {
            bDelete = true;
            break;
        }
    }

    // Delete old element if it exists (unique_ptr destructor handles cleanup)
    if (m_customElementMap.count(strName) > 0) {
        if (!bDelete) {
            qCDebug(lcMXP) << "Replacing custom element:" << strName;
        }
        m_customElementMap.erase(strName);
    }

    if (bDelete) {
        qCDebug(lcMXP) << "Deleted custom element:" << strName;
        return; // All done (args cleared automatically by unique_ptr)
    }

    // Create new element (use unique_ptr locally to handle error-path cleanup)
    auto pElementOwned = std::make_unique<CustomElement>();
    CustomElement* pElement = pElementOwned.get();
    pElement->strName = strName;

    // Parse keywords
    for (const auto& arg : args) {
        if (!arg->bKeyword)
            continue;

        QString keyword = arg->strName.toUpper();
        if (keyword == "OPEN") {
            pElement->bOpen = true;
        } else if (keyword == "EMPTY") {
            pElement->bCommand = true;
        }
    }

    // Get definition (first positional argument): '<color &col;><send>'
    // For EMPTY elements, definition can be empty string: "br '' EMPTY"
    QString definition;
    bool hasDefinitionArg = false;
    for (const auto& arg : args) {
        if (!arg->bKeyword && arg->strName.isEmpty()) {
            definition = arg->strValue; // May be empty for EMPTY elements
            hasDefinitionArg = true;
            break;
        }
    }

    if (!hasDefinitionArg) {
        qCWarning(lcMXP) << "Missing definition in element:" << strName;
        m_iMXPerrors++;
        return;
    }

    // Parse atomic elements in the definition
    // Format: '<color &col;><send href="&cmd;">'
    // Skip parsing if definition is empty (for EMPTY elements)
    int pos = 0;
    while (!definition.isEmpty() && pos < definition.length()) {
        // Skip whitespace
        while (pos < definition.length() && definition[pos].isSpace()) {
            pos++;
        }

        if (pos >= definition.length())
            break;

        // Expect '<'
        if (definition[pos] != '<') {
            qCWarning(lcMXP) << "Expected '<' in element definition:" << definition;
            m_iMXPerrors++;
            // pElementOwned unique_ptr cleans up on return
            return;
        }

        pos++; // Skip '<'
        int startPos = pos;

        // Find matching '>' (handle quotes)
        bool inQuote = false;
        QChar quoteChar;
        while (pos < definition.length()) {
            QChar c = definition[pos];

            if (!inQuote && (c == '\'' || c == '"')) {
                inQuote = true;
                quoteChar = c;
            } else if (inQuote && c == quoteChar) {
                inQuote = false;
            } else if (!inQuote && c == '<') {
                qCWarning(lcMXP) << "Unexpected '<' in element definition:" << definition;
                m_iMXPerrors++;
                // pElementOwned unique_ptr cleans up on return
                return;
            } else if (!inQuote && c == '>') {
                break;
            }

            pos++;
        }

        if (pos >= definition.length()) {
            qCWarning(lcMXP) << "Missing '>' in element definition:" << definition;
            m_iMXPerrors++;
            // pElementOwned unique_ptr cleans up on return
            return;
        }

        // Extract atomic element tag
        QString atomTag = definition.mid(startPos, pos - startPos);
        pos++; // Skip '>'

        // Parse the atomic element
        QString atomName;
        MXPArgumentList atomArgs;
        ParseMXPTag(atomTag, atomName, atomArgs);

        atomName = atomName.toLower();

        // Check for invalid tags
        if (atomName.startsWith('/')) {
            qCWarning(lcMXP) << "Element definitions cannot close elements:" << atomName;
            m_iMXPerrors++;
            // atomArgs cleared automatically by unique_ptr
            continue;
        }

        if (atomName.startsWith('!')) {
            qCWarning(lcMXP) << "Element definitions cannot define elements:" << atomName;
            m_iMXPerrors++;
            // atomArgs cleared automatically by unique_ptr
            continue;
        }

        // Look up atomic element
        AtomicElement* atomicElem = MXP_FindAtomicElement(atomName);
        if (!atomicElem) {
            qCWarning(lcMXP) << "Unknown atomic element in definition:" << atomName;
            m_iMXPerrors++;
            // atomArgs cleared automatically by unique_ptr
            continue;
        }

        // Create element item
        auto item = std::make_unique<ElementItem>();
        item->pAtomicElement = atomicElem;
        item->argumentList = std::move(atomArgs); // Transfer ownership
        pElement->elementItemList.push_back(std::move(item));
    }

    // Parse ATT= (attributes for this element)
    QString attValue = GetMXPArgument(args, "ATT");
    if (!attValue.isEmpty()) {
        // Prepend dummy tag name since ParseMXPTag expects "tagname arg1=val1 arg2=val2"
        QString dummyTag;
        ParseMXPTag("_dummy " + attValue, dummyTag, pElement->attributeList);
    }

    // Parse TAG= (line tag number 20-99)
    QString tagValue = GetMXPArgument(args, "TAG");
    if (!tagValue.isEmpty()) {
        bool ok;
        int tagNum = tagValue.toInt(&ok);
        if (ok && tagNum >= 20 && tagNum <= 99) {
            pElement->iTag = tagNum;
        }
    }

    // Parse FLAG= (variable name to set)
    QString flagValue = GetMXPArgument(args, "FLAG");
    if (!flagValue.isEmpty()) {
        // Handle "set varname" format
        if (flagValue.startsWith("set ", Qt::CaseInsensitive)) {
            flagValue = flagValue.mid(4).trimmed();
        }

        // Replace spaces with underscores
        flagValue.replace(' ', '_');

        pElement->strFlag = flagValue;
    }

    // Add to custom element map (transfer ownership via unique_ptr)
    std::size_t elementCount = pElement->elementItemList.size();
    m_customElementMap.emplace(strName, std::move(pElementOwned));
    qCDebug(lcMXP) << "Defined custom element:" << strName << "with" << elementCount
                   << "atomic elements";
    // args cleared automatically by unique_ptr
}

/**
 * MXP_DefineEntity - Define custom entity
 *
 * Based on: mxpDefs.cpp::MXP_Entity
 *
 * @param defString Entity definition (after "ENTITY")
 */
void MXPEngine::MXP_DefineEntity(const QString& defString)
{
    QString remaining = defString.trimmed();

    // Extract entity name (first word)
    int spacePos = remaining.indexOf(QRegularExpression("\\s"));
    if (spacePos == -1) {
        qCWarning(lcMXP) << "Missing entity value in definition:" << defString;
        m_iMXPerrors++;
        return;
    }

    QString strName = remaining.left(spacePos).toLower(); // Case-insensitive
    remaining = remaining.mid(spacePos + 1).trimmed();

    // Check if trying to redefine built-in entity
    if (m_entityMap.count(strName) > 0) {
        qCWarning(lcMXP) << "Cannot redefine built-in entity:" << strName;
        m_iMXPerrors++;
        return;
    }

    // Check for DELETE keyword
    if (remaining.startsWith("DELETE", Qt::CaseInsensitive)) {
        // Remove custom entity (unique_ptr destructor handles cleanup)
        if (m_customEntityMap.erase(strName) > 0) {
            qCDebug(lcMXP) << "Deleted custom entity:" << strName;
        }
        return;
    }

    // Extract value (handle quotes)
    QString strValue;
    if (remaining.startsWith('"') || remaining.startsWith('\'')) {
        QChar quote = remaining[0];
        int closeQuote = remaining.indexOf(quote, 1);
        if (closeQuote == -1) {
            qCWarning(lcMXP) << "No closing quote in entity definition:" << defString;
            m_iMXPerrors++;
            return;
        }
        strValue = remaining.mid(1, closeQuote - 1);
    } else {
        // Unquoted value (first word)
        int nextSpace = remaining.indexOf(QRegularExpression("\\s"));
        if (nextSpace == -1) {
            strValue = remaining;
        } else {
            strValue = remaining.left(nextSpace);
        }
    }

    // Expand any entities embedded in the value (e.g., &lt; &gt;)
    QString strFixedValue;
    int pos = 0;
    while (pos < strValue.length()) {
        if (strValue[pos] == '&') {
            // Find entity
            int semiPos = strValue.indexOf(';', pos + 1);
            if (semiPos == -1) {
                qCWarning(lcMXP) << "No closing ; in entity argument:" << strValue;
                m_iMXPerrors++;
                return;
            }

            QString entityName = strValue.mid(pos + 1, semiPos - pos - 1);
            QString entityValue = MXP_GetEntity(entityName);
            strFixedValue += entityValue;
            pos = semiPos + 1;
        } else {
            strFixedValue += strValue[pos];
            pos++;
        }
    }

    // Delete old entity if it exists (unique_ptr destructor handles cleanup)
    if (m_customEntityMap.count(strName) > 0) {
        qCDebug(lcMXP) << "Replacing custom entity:" << strName;
    }

    // Create new entity
    auto entity = std::make_unique<MXPEntity>();
    entity->strName = strName;
    entity->strValue = strFixedValue;

    // For single-character entities, also set iCodepoint for compatibility
    if (strFixedValue.length() == 1) {
        entity->iCodepoint = strFixedValue[0].unicode();
    } else {
        entity->iCodepoint = 0; // Multi-character entity
    }

    m_customEntityMap.emplace(strName, std::move(entity));
    qCDebug(lcMXP) << "Defined custom entity:" << strName << "=" << strFixedValue;

    // TODO(mxp): Fire ON_PLUGIN_MXP_SETENTITY plugin callback when MXP entity is set.
}

/**
 * MXP_ExecuteAction - Execute MXP element action
 *
 * Based on: mxpProcess.cpp:execute_element() from original MUSHclient
 *
 * Executes the action associated with an MXP element. This is called when
 * an opening tag is encountered and should apply the element's effect.
 * Many actions modify WorldDocument output state (colors, style flags) via m_doc.
 */
void MXPEngine::MXP_ExecuteAction(AtomicElement* elem, MXPArgumentList& args)
{
    if (!elem) {
        return;
    }

    int action = elem->iAction;

    // Pueblo compatibility: <A xch_cmd="..."> converts hyperlink to SEND action
    // (original: mxpOpenAtomic.cpp:97-106)
    if (action == MXP_ACTION_HYPERLINK) {
        QString xchCmd = MXP_GetArgument("xch_cmd", args);
        if (!xchCmd.isEmpty()) {
            m_bPuebloActive = true;
            action = MXP_ACTION_SEND;
        }
    }

    qCDebug(lcMXP) << "Execute action:" << action << "(" << elem->strName << ")";

    switch (action) {
        // ========== TEXT FORMATTING ==========
        case MXP_ACTION_BOLD:
            m_doc.m_iFlags |= HILITE;
            qCDebug(lcMXP) << "Bold: set HILITE flag";
            break;

        case MXP_ACTION_ITALIC:
            m_doc.m_iFlags |= BLINK; // BLINK is repurposed for italic
            qCDebug(lcMXP) << "Italic: set BLINK flag";
            break;

        case MXP_ACTION_UNDERLINE:
            m_doc.m_iFlags |= UNDERLINE;
            qCDebug(lcMXP) << "Underline: set UNDERLINE flag";
            break;

        case MXP_ACTION_STRIKE:
            m_doc.m_iFlags |= STRIKEOUT;
            qCDebug(lcMXP) << "Strikeout: set STRIKEOUT flag";
            break;

        case MXP_ACTION_SMALL:
        case MXP_ACTION_TT:
        case MXP_ACTION_SAMP:
            // These are text style hints that don't map directly to flags
            // May be used for font selection in the future
            qCDebug(lcMXP) << "Text style hint:" << elem->strName;
            break;

        case MXP_ACTION_HIGH:
            // High intensity - typically makes colors brighter
            // This is handled by color system, not style flags
            qCDebug(lcMXP) << "High intensity";
            break;

        // ========== HEADINGS ==========
        case MXP_ACTION_H1:
        case MXP_ACTION_H2:
        case MXP_ACTION_H3:
        case MXP_ACTION_H4:
        case MXP_ACTION_H5:
        case MXP_ACTION_H6:
            // TODO(mxp): Apply heading style (bold + scaled font size) to output runs.
            qCDebug(lcMXP) << "Heading action:" << elem->strName;
            break;

        // ========== LINE BREAKS / STRUCTURE ==========
        case MXP_ACTION_BR:
            // Hard line break - flush current line and start new one
            m_doc.StartNewLine(true, 0);
            qCDebug(lcMXP) << "Line break: started new line";
            break;

        case MXP_ACTION_HR:
            // Horizontal rule - mark line with HORIZ_RULE flag
            // Wrap up previous line if it has content
            if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0) {
                m_doc.StartNewLine(true, 0);
            }
            // Mark this line as a horizontal rule
            if (m_doc.m_currentLine) {
                m_doc.m_currentLine->flags |= HORIZ_RULE;
            }
            // Finish the HR line
            m_doc.StartNewLine(true, 0);
            qCDebug(lcMXP) << "Horizontal rule: set HORIZ_RULE flag on line";
            break;

        case MXP_ACTION_P:
            // Paragraph mode - enables text wrapping, newlines are suppressed
            m_doc.m_cLastChar = 0;
            m_bInParagraph = true;
            qCDebug(lcMXP) << "Paragraph: enabled paragraph mode";
            break;

        case MXP_ACTION_NOBR:
            // Non-breaking mode (suppress line breaks)
            m_bMXP_nobr = true;
            qCDebug(lcMXP) << "Enable nobr mode";
            break;

        case MXP_ACTION_PRE:
            // Preformatted text (preserve whitespace)
            m_bMXP_preformatted = true;
            qCDebug(lcMXP) << "Enable preformatted mode";
            break;

        // ========== COLOR ==========
        case MXP_ACTION_COLOR: {
            // <color fore=red back=blue>
            // Original (mxpOpenAtomic.cpp:124-149): checks m_bIgnoreMXPcolourChanges before
            // applying both foreground and background colour changes.
            if (m_doc.m_bIgnoreMXPcolourChanges)
                break;

            QString fore = MXP_GetArgument("fore", args);
            QString back = MXP_GetArgument("back", args);

            // Set RGB color mode upfront (mirrors original: always clears COLOURTYPE)
            m_doc.m_iFlags = (m_doc.m_iFlags & ~COLOURTYPE) | COLOUR_RGB;

            if (!fore.isEmpty()) {
                QRgb fgColor = MXP_GetColor(fore);
                m_doc.m_iForeColour = (int)fgColor;
                qCDebug(lcMXP) << "Set foreground color:" << fore << "="
                               << QString::number(fgColor, 16);
            }

            if (!back.isEmpty()) {
                QRgb bgColor = MXP_GetColor(back);
                m_doc.m_iBackColour = (int)bgColor;
                qCDebug(lcMXP) << "Set background color:" << back << "="
                               << QString::number(bgColor, 16);
            }
            break;
        }

        // ========== FONT ==========
        case MXP_ACTION_FONT: {
            // <font color=Red,Blink back=blue face=Arial size=12>
            // Original (mxpOpenAtomic.cpp:226-285): parses comma-separated color+style list
            QString face = MXP_GetArgument("face", args);

            // Set RGB color mode
            m_doc.m_iFlags = (m_doc.m_iFlags & ~COLOURTYPE) | COLOUR_RGB;

            // Color argument: comma-separated list of color name and/or style keywords
            QString colorArg = MXP_GetArgument("color", args);
            if (colorArg.isEmpty()) {
                colorArg = MXP_GetArgument("fgcolor", args); // Pueblo fallback
            }

            if (!colorArg.isEmpty()) {
                QStringList items = colorArg.split(',', Qt::SkipEmptyParts);
                for (const QString& item : items) {
                    QString trimmed = item.trimmed().toLower();
                    if (trimmed == "blink" || trimmed == "italic") {
                        m_doc.m_iFlags |= BLINK;
                    } else if (trimmed == "underline") {
                        m_doc.m_iFlags |= UNDERLINE;
                    } else if (trimmed == "bold") {
                        m_doc.m_iFlags |= HILITE;
                    } else if (trimmed == "inverse") {
                        m_doc.m_iFlags |= INVERSE;
                    } else if (!m_doc.m_bIgnoreMXPcolourChanges) {
                        m_doc.m_iForeColour = static_cast<QRgb>(MXP_GetColor(item.trimmed()));
                    }
                }
            }

            // Background color
            QString backArg = MXP_GetArgument("back", args);
            if (backArg.isEmpty()) {
                backArg = MXP_GetArgument("bgcolor", args); // Pueblo fallback
            }
            if (!backArg.isEmpty() && !m_doc.m_bIgnoreMXPcolourChanges) {
                m_doc.m_iBackColour = static_cast<QRgb>(MXP_GetColor(backArg));
            }

            // face/size: not fully supported (need MXP style stack)
            if (!face.isEmpty()) {
                qCDebug(lcMXP) << "Font face:" << face << "(needs style stack)";
            }

            // consume size argument to avoid unused-argument warnings
            (void)MXP_GetArgument("size", args);
            break;
        }

        // ========== INTERACTIVE ELEMENTS ==========
        case MXP_ACTION_SEND: {
            // <send href="command" hint="tooltip" prompt>
            // Pueblo fallback: xch_cmd if href empty, xch_hint if hint empty
            // (original: mxpOpenAtomic.cpp:172-205)
            QString href = MXP_GetArgument("href", args);
            if (href.isEmpty())
                href = MXP_GetArgument("xch_cmd", args);
            QString hint = MXP_GetArgument("hint", args);
            if (hint.isEmpty())
                hint = MXP_GetArgument("xch_hint", args);
            bool prompt = MXP_HasArgument("prompt", args);

            // Store link info for text that follows
            m_strMXP_link = href;
            m_strMXP_hint = hint;
            m_bMXP_link_prompt = prompt;

            // Set action type bits: cancel old, set SEND or PROMPT
            // (original: mxpOpenAtomic.cpp:175-179)
            m_doc.m_iFlags &= ~ACTIONTYPE;
            m_doc.m_iFlags |= prompt ? ACTION_PROMPT : ACTION_SEND;

            // Apply underline if world option is set (original: mxpOpenAtomic.cpp:181-182)
            if (m_doc.m_bUnderlineHyperlinks)
                m_doc.m_iFlags |= UNDERLINE;

            // Apply custom link color if configured (original: mxpOpenAtomic.cpp:184-191)
            if (m_doc.m_bUseCustomLinkColour) {
                m_doc.m_iForeColour = static_cast<int>(m_doc.m_colors.hyperlink_colour);
                m_doc.m_iFlags = (m_doc.m_iFlags & ~COLOURTYPE) | COLOUR_RGB;
            }

            qCDebug(lcMXP) << "Begin send link:" << href << "hint:" << hint << "prompt:" << prompt;
            break;
        }

        case MXP_ACTION_HYPERLINK: {
            // <a href="http://url">
            // (original: mxpOpenAtomic.cpp:207-224)
            QString href = MXP_GetArgument("href", args);
            QString hint = MXP_GetArgument("hint", args);

            // Store hyperlink info for text that follows
            m_strMXP_link = href;
            m_strMXP_hint = hint;
            m_bMXP_link_prompt = false;

            // Set action type bits and unconditionally apply underline
            // (original: mxpOpenAtomic.cpp:213-214 — <a> always underlines)
            m_doc.m_iFlags &= ~ACTIONTYPE;
            m_doc.m_iFlags |= ACTION_HYPERLINK | UNDERLINE;

            // Apply custom link color if configured (original: mxpOpenAtomic.cpp:216-222)
            if (m_doc.m_bUseCustomLinkColour) {
                m_doc.m_iForeColour = static_cast<int>(m_doc.m_colors.hyperlink_colour);
                m_doc.m_iFlags = (m_doc.m_iFlags & ~COLOURTYPE) | COLOUR_RGB;
            }

            qCDebug(lcMXP) << "Begin hyperlink:" << href << "hint:" << hint;
            break;
        }

        // ========== MEDIA ==========
        case MXP_ACTION_SOUND: {
            // <sound fname="file.wav" v=100 l=1 p=50 t=category u="http://url">
            // <music fname="file.mp3" v=100 l=-1 p=50 t=category u="http://url" c=1>
            // MXP spec: fname=filename, v=volume(1-100), l=loops(-1=infinite,1=once),
            //           p=priority, t=type/category, u=base URL for download.
            // MUSIC also has c=continue (don't restart if same file already playing).
            // Original: TAG_NOT_IMP in reference; behavioural spec from MXP standard.
            QString fname = MXP_GetArgument("fname", args);
            QString volumeStr = MXP_GetArgument("v", args);
            QString loopsStr = MXP_GetArgument("l", args);
            // Priority is not implemented — no sound priority queue in Mushkin.
            (void)MXP_GetArgument("p", args);
            // Type/category is informational only.
            QString typeStr = MXP_GetArgument("t", args);
            if (!typeStr.isEmpty())
                qCDebug(lcMXP) << "Sound type/category:" << typeStr;
            QString url = MXP_GetArgument("u", args);
            // c=continue is MUSIC-only; consume so it isn't flagged as unused.
            (void)MXP_GetArgument("c", args);

            QString soundFile = fname.isEmpty() ? url : fname;
            if (soundFile.isEmpty()) {
                qCWarning(lcMXP) << "Sound tag has no fname or URL";
                break;
            }

            // v=1-100 (MXP spec, 100=full); convert to Mushkin API scale (-100..0).
            int vol = volumeStr.isEmpty() ? 100 : qBound(1, volumeStr.toInt(), 100);
            double volumeApi = static_cast<double>(vol) - 100.0; // 100→0.0, 1→-99.0

            // l=loops: -1=infinite, 1=once (default), N>1=N times.
            int loopCount = loopsStr.isEmpty() ? 1 : loopsStr.toInt();
            bool loop = (loopCount < 0 || loopCount > 1);

            // <music> uses the dedicated music buffer (1); <sound> auto-selects (0).
            bool isMusic = (elem->strName.compare("music", Qt::CaseInsensitive) == 0);
            qint16 buffer = isMusic ? 1 : 0;

            qCDebug(lcMXP) << (isMusic ? "MXP MUSIC" : "MXP SOUND") << "file:" << soundFile
                           << "volume:" << vol << "loops:" << loopCount << "url:" << url;

            m_doc.PlayMSPSound(soundFile, url, loop, volumeApi, buffer);
            break;
        }

        case MXP_ACTION_IMAGE:
        case MXP_ACTION_IMG: {
            // <image fname="pic.png" url="http://..." align=left h=100 w=100>
            // We are a text client — display a clickable placeholder link instead.
            // Original (mxpOpenAtomic.cpp:723-796): when url is present, appends fname to
            // url, outputs "[url+fname]" as a hyperlink on its own line, then restores style.
            // Pueblo also checks "src" as a fallback for url.
            QString fname = MXP_GetArgument("fname", args);
            QString url = MXP_GetArgument("url", args);
            if (url.isEmpty())
                url = MXP_GetArgument("src", args); // Pueblo fallback
            // Consume remaining recognised args to suppress unused-arg warnings
            (void)MXP_GetArgument("align", args);
            (void)MXP_GetArgument("h", args);
            (void)MXP_GetArgument("w", args);
            (void)MXP_GetArgument("hspace", args);
            (void)MXP_GetArgument("vspace", args);
            (void)MXP_GetArgument("t", args);
            (void)MXP_GetArgument("ismap", args);
            (void)MXP_GetArgument("xch_mode", args);

            if (!url.isEmpty()) {
                // Append filename to URL as the original does (original:770 strArgument +=
                // strFilename)
                QString linkTarget = url + fname;

                // Ensure image link appears on its own line (original:755-756)
                if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0)
                    m_doc.StartNewLine(true, 0);

                // Save current style
                quint16 savedFlags = m_doc.m_iFlags;
                QRgb savedFore = m_doc.m_iForeColour;
                QRgb savedBack = m_doc.m_iBackColour;
                auto savedAction = m_doc.m_currentAction;

                // Set hyperlink style (original:762-776)
                m_doc.m_currentAction =
                    std::make_shared<Action>(linkTarget, linkTarget, QString(), &m_doc);
                m_doc.m_iFlags &= ~ACTIONTYPE;
                m_doc.m_iFlags |= ACTION_HYPERLINK;
                if (m_doc.m_bUnderlineHyperlinks)
                    m_doc.m_iFlags |= UNDERLINE;
                if (m_doc.m_bUseCustomLinkColour) {
                    m_doc.m_iForeColour = static_cast<int>(m_doc.m_colors.hyperlink_colour);
                    m_doc.m_iFlags = (m_doc.m_iFlags & ~COLOURTYPE) | COLOUR_RGB;
                }

                // Output "[url+fname]" as clickable text (original:778-780)
                QByteArray linkBytes = QString("[%1]").arg(linkTarget).toUtf8();
                m_doc.AddToLine(linkBytes.constData(), linkBytes.size());

                // Restore style (original:787-793)
                m_doc.m_iFlags = savedFlags;
                m_doc.m_iForeColour = savedFore;
                m_doc.m_iBackColour = savedBack;
                m_doc.m_currentAction = savedAction;

                // Start a new line after the image link (original:786)
                m_doc.StartNewLine(true, 0);

                qCDebug(lcMXP) << "Image placeholder link:" << linkTarget;
            } else if (!fname.isEmpty()) {
                // No URL — just log; can't make a clickable link without a URL
                qCDebug(lcMXP) << "Image: fname=" << fname << "(no URL; no placeholder output)";
            }
            break;
        }

        // ========== SERVER COMMANDS ==========
        case MXP_ACTION_VERSION: {
            // Send client version to server (original: mxpOpenAtomic.cpp:288-304)
            // Format: ESC[1z<VERSION MXP="0.4" CLIENT=Mushkin VERSION="x.y.z" REGISTERED=YES>\r\n
            QString versionResponse =
                QString("\x1B[1z<VERSION MXP=\"0.4\" CLIENT=Mushkin VERSION=\"%1\" "
                        "REGISTERED=YES>\r\n")
                    .arg(QCoreApplication::applicationVersion());
            QByteArray data = versionResponse.toUtf8();
            m_doc.SendPacket(
                std::span<const unsigned char>(reinterpret_cast<const unsigned char*>(data.data()),
                                               static_cast<size_t>(data.size())));
            qCDebug(lcMXP) << "Sent version response:" << versionResponse.trimmed();
            break;
        }

        case MXP_ACTION_USER:
            // Send username to server
            // Not implemented: MXP USER tag — automatic credential sending. Security risk; omitted
            // by design.
            qCDebug(lcMXP) << "Send username (not implemented)";
            break;

        case MXP_ACTION_PASSWORD:
            // Send password to server (masked input)
            // Not implemented: MXP PASSWORD tag — automatic credential sending. Security risk;
            // omitted by design.
            qCDebug(lcMXP) << "Send password (not implemented)";
            break;

        case MXP_ACTION_RELOCATE: {
            // <relocate server="mud.example.com" port="4000">
            QString server = MXP_GetArgument("server", args);
            QString port = MXP_GetArgument("port", args);

            // Not implemented: MXP RELOCATE tag — server-initiated redirect. Would need connection
            // manager support.
            qCDebug(lcMXP) << "Relocate to:" << server << ":" << port << "(not implemented)";
            break;
        }

        // ========== ADVANCED ELEMENTS ==========
        case MXP_ACTION_GAUGE: {
            // <gauge entity="hp" max="100" caption="Health" color="red">
            QString entity = MXP_GetArgument("entity", args);
            QString maxStr = MXP_GetArgument("max", args);
            QString caption = MXP_GetArgument("caption", args);
            QString color = MXP_GetArgument("color", args);

            if (entity.isEmpty()) {
                qCWarning(lcMXP) << "Gauge has no entity name";
                break;
            }

            // Create or update gauge in map
            MXPGauge& gauge = m_gaugeMap[entity];
            gauge.entity = entity;
            gauge.caption = caption.isEmpty() ? entity : caption;
            gauge.color = color;
            gauge.max = maxStr.isEmpty() ? 100 : maxStr.toInt();
            gauge.isGauge = true;
            // Current value will be captured from text between tags in MXP_EndAction

            qCDebug(lcMXP) << "Gauge created/updated: entity=" << entity << "max=" << gauge.max
                           << "caption=" << gauge.caption << "color=" << color
                           << "(value will be captured from tag content)";
            break;
        }

        case MXP_ACTION_STAT: {
            // <stat entity="hp" max="100" caption="Health">
            QString entity = MXP_GetArgument("entity", args);
            QString maxStr = MXP_GetArgument("max", args);
            QString caption = MXP_GetArgument("caption", args);

            if (entity.isEmpty()) {
                qCWarning(lcMXP) << "Stat has no entity name";
                break;
            }

            // Create or update stat in map
            MXPGauge& stat = m_gaugeMap[entity];
            stat.entity = entity;
            stat.caption = caption.isEmpty() ? entity : caption;
            stat.max = maxStr.isEmpty() ? 100 : maxStr.toInt();
            stat.isGauge = false; // Mark as stat, not gauge
            // Current value will be captured from text between tags in MXP_EndAction

            qCDebug(lcMXP) << "Stat created/updated: entity=" << entity << "max=" << stat.max
                           << "caption=" << stat.caption
                           << "(value will be captured from tag content)";
            break;
        }

        case MXP_ACTION_EXPIRE: {
            // <expire name="banner">This text expires in 5 seconds</expire>
            QString name = MXP_GetArgument("name", args);
            QString when = MXP_GetArgument("when", args);

            // Not implemented: MXP EXPIRE tag — timed text removal. Rarely used by MUDs.
            qCDebug(lcMXP) << "Expire:" << name << "when:" << when;
            break;
        }

        case MXP_ACTION_VAR: {
            // <var name="hp">100</var> or <var name="hp" value="100">
            QString name = MXP_GetArgument("name", args);
            QString value = MXP_GetArgument("value", args);

            // TODO(mxp): Set MXP variable in m_mxpVariableMap from VAR tag content.
            qCDebug(lcMXP) << "Set variable:" << name << "=" << value;
            break;
        }

        case MXP_ACTION_AFK:
            // <afk> - mark player as away from keyboard
            // Not implemented: MXP AFK tag — idle time tracking. Rarely used by MUDs.
            qCDebug(lcMXP) << "AFK marker (not implemented)";
            break;

        // ========== PROTOCOL CONTROL ==========
        case MXP_ACTION_RESET:
            // <reset> - reset MXP state: close open tags, reset ANSI formatting,
            // and reset paragraph/pre mode.
            // Original: mxpOpenAtomic.cpp:615-616 — calls MXP_Off() (bCompletely=false),
            // which also calls InterpretANSIcode(0), resets bInParagraph/bPreMode/listMode,
            // and then calls MXP_CloseAllTags(). This is more than just closing open tags.
            MXP_Off(false);
            break;

        case MXP_ACTION_MXP: {
            // <mxp off> — server requests permanent MXP disable (original:
            // mxpOpenAtomic.cpp:620-659) Only "off" is implemented; mode-change keywords
            // (default_open, etc.) are commented out in the original too.
            if (MXP_HasArgument("off", args)) {
                qCDebug(lcMXP) << "MXP off: disabling MXP completely";
                MXP_Off(true);
            } else {
                qCDebug(lcMXP) << "MXP command (no recognised keyword)";
            }
            break;
        }

        case MXP_ACTION_SUPPORT: {
            // <support> - report supported tags (original: mxpOpenAtomic.cpp:326-462)
            QString supports;

            if (args.empty()) {
                // No args: report all supported elements and their sub-attributes
                // (original: lists +elem for each implemented element, then +elem.attr
                // for each attribute in pElement->strArgs)
                for (const auto& [name, elem] : m_atomicElementMap) {
                    if (elem->iFlags & TAG_NOT_IMP) {
                        supports += QString("-%1 ").arg(name);
                    } else {
                        supports += QString("+%1 ").arg(name);
                        // Also list each supported sub-attribute (original: 345-355)
                        if (!elem->strArgs.isEmpty()) {
                            const QStringList subAttrs =
                                elem->strArgs.split(',', Qt::SkipEmptyParts);
                            for (const QString& attr : subAttrs) {
                                supports += QString("+%1.%2 ").arg(name, attr.trimmed());
                            }
                        }
                    }
                }
            } else {
                // Report on specific requested elements, with dot-notation sub-attribute
                // and wildcard support (original: mxpOpenAtomic.cpp:361-451).
                // Arguments are positional: strValue holds the full "elem" or "elem.attr"
                // token (original: pArgument->strValue).
                for (const auto& arg : args) {
                    if (arg->bKeyword)
                        continue;
                    // Use strValue for positional args (the full "elem" or "elem.attr" token)
                    QString argValue = arg->strValue.isEmpty() ? arg->strName : arg->strValue;
                    argValue = argValue.toLower();

                    const QStringList parts = argValue.split('.', Qt::SkipEmptyParts);
                    if (parts.isEmpty() || parts.size() > 2) {
                        qCWarning(lcMXP) << "Invalid <support> argument:" << argValue;
                        continue;
                    }

                    const QString tagName = parts[0];
                    auto it = m_atomicElementMap.find(tagName);
                    if (it == m_atomicElementMap.end() || (it->second->iFlags & TAG_NOT_IMP)) {
                        // Tag unknown or not supported
                        supports += QString("-%1 ").arg(tagName);
                        continue;
                    }

                    if (parts.size() == 1) {
                        // Just "elem" — report supported
                        supports += QString("+%1 ").arg(tagName);
                        continue;
                    }

                    // "elem.subtag" or "elem.*"
                    const QString subtag = parts[1];
                    const QStringList subAttrs = it->second->strArgs.split(',', Qt::SkipEmptyParts);
                    if (subtag == "*") {
                        // Wildcard: list all sub-attributes (original: 411-424)
                        for (const QString& attr : subAttrs) {
                            supports += QString("+%1.%2 ").arg(tagName, attr.trimmed());
                        }
                    } else {
                        // Specific sub-attribute lookup (original: 437-449)
                        bool found = false;
                        for (const QString& attr : subAttrs) {
                            if (attr.trimmed().compare(subtag, Qt::CaseInsensitive) == 0) {
                                found = true;
                                break;
                            }
                        }
                        supports += found ? QString("+%1.%2 ").arg(tagName, subtag)
                                          : QString("-%1.%2 ").arg(tagName, subtag);
                    }
                }
            }

            {
                QString response = QString("\x1B[1z<SUPPORTS %1>\r\n").arg(supports.trimmed());
                QByteArray data = response.toUtf8();
                m_doc.SendPacket(std::span<const unsigned char>(
                    reinterpret_cast<const unsigned char*>(data.data()),
                    static_cast<size_t>(data.size())));
                qCDebug(lcMXP) << "Sent support response:" << supports.trimmed();
            }
            break;
        }

        case MXP_ACTION_OPTION: {
            // Client option set
            QString option = MXP_GetArgument("option", args);
            QString value = MXP_GetArgument("value", args);

            // Not implemented: MXP OPTION/RECOMMEND_OPTION — server-controlled client settings.
            // Omitted for security.
            qCDebug(lcMXP) << "Set option:" << option << "=" << value;
            break;
        }

        case MXP_ACTION_RECOMMEND_OPTION: {
            // Server recommends option
            QString option = MXP_GetArgument("option", args);
            QString value = MXP_GetArgument("value", args);

            // Not implemented: MXP OPTION/RECOMMEND_OPTION — server-controlled client settings.
            // Omitted for security.
            qCDebug(lcMXP) << "Recommend option:" << option << "=" << value;
            break;
        }

        // ========== LISTS ==========
        case MXP_ACTION_UL:
            // Unordered list (original: mxpOpenAtomic.cpp:692-695)
            m_iMXP_list_depth++;
            m_iListMode = eUnorderedList;
            m_iListCount = 0;
            qCDebug(lcMXP) << "Start unordered list (depth:" << m_iMXP_list_depth << ")";
            break;

        case MXP_ACTION_OL:
            // Ordered list (original: mxpOpenAtomic.cpp:696-699)
            m_iMXP_list_depth++;
            m_iListMode = eOrderedList;
            m_iListCount = 0;
            qCDebug(lcMXP) << "Start ordered list (depth:" << m_iMXP_list_depth << ")";
            break;

        case MXP_ACTION_LI: {
            // List item — output bullet or number prefix (original: mxpOpenAtomic.cpp:700-710)
            // Wrap up previous line if necessary
            if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0)
                m_doc.StartNewLine(true, 0);
            QString listItem = " * ";
            if (m_iListMode == eOrderedList)
                listItem = QString(" %1. ").arg(++m_iListCount);
            m_doc.AddToLine(listItem.toLatin1().constData());
            qCDebug(lcMXP) << "List item:" << listItem.trimmed();
            break;
        }

        // ========== ALIGNMENT ==========
        case MXP_ACTION_CENTER:
            m_bMXP_centered = true;
            break;

        // ========== FRAMES (NOT IMPLEMENTED) ==========
        case MXP_ACTION_FRAME:
        case MXP_ACTION_DEST:
            qCDebug(lcMXP) << "Frame action (not implemented)";
            break;

        // ========== PUEBLO TAGS (MOSTLY IGNORED) ==========
        case MXP_ACTION_BODY:
        case MXP_ACTION_HEAD:
        case MXP_ACTION_HTML:
        case MXP_ACTION_TITLE:
        case MXP_ACTION_XCH_PAGE:
        case MXP_ACTION_XCH_PANE:
            qCDebug(lcMXP) << "Pueblo tag:" << elem->strName << "(ignored)";
            break;

        // ========== FILTER (NOT IMPLEMENTED) ==========
        case MXP_ACTION_FILTER:
            qCDebug(lcMXP) << "Filter action (not implemented)";
            break;

        // ========== SCRIPT ==========
        case MXP_ACTION_SCRIPT: {
            // <script>lua code</script> or <script language="lua">
            QString language = MXP_GetArgument("language", args);
            // Not implemented: MXP SCRIPT tag — server-sent code execution. Disabled for security.
            qCDebug(lcMXP) << "Script action (not implemented, security risk)";
            break;
        }

        default:
            qCWarning(lcMXP) << "Unknown MXP action:" << action;
            break;
    }
}

/**
 * MXP_EndAction - End MXP element action
 *
 * Based on: mxpProcess.cpp from original MUSHclient
 *
 * Reverses the effect of an MXP element when its closing tag is encountered.
 */
void MXPEngine::MXP_EndAction(int action)
{
    qCDebug(lcMXP) << "End action:" << action;

    switch (action) {
        // ========== TEXT FORMATTING ==========
        case MXP_ACTION_BOLD:
            m_doc.m_iFlags &= ~HILITE;
            qCDebug(lcMXP) << "End bold: clear HILITE flag";
            break;

        case MXP_ACTION_ITALIC:
            m_doc.m_iFlags &= ~BLINK; // BLINK is repurposed for italic
            qCDebug(lcMXP) << "End italic: clear BLINK flag";
            break;

        case MXP_ACTION_UNDERLINE:
            m_doc.m_iFlags &= ~UNDERLINE;
            qCDebug(lcMXP) << "End underline: clear UNDERLINE flag";
            break;

        case MXP_ACTION_STRIKE:
            m_doc.m_iFlags &= ~STRIKEOUT;
            qCDebug(lcMXP) << "End strikeout: clear STRIKEOUT flag";
            break;

        case MXP_ACTION_SMALL:
        case MXP_ACTION_TT:
        case MXP_ACTION_SAMP:
        case MXP_ACTION_HIGH:
            // Style hints don't have persistent state
            qCDebug(lcMXP) << "End text style hint";
            break;

        // ========== HEADINGS ==========
        case MXP_ACTION_H1:
        case MXP_ACTION_H2:
        case MXP_ACTION_H3:
        case MXP_ACTION_H4:
        case MXP_ACTION_H5:
        case MXP_ACTION_H6:
            // TODO(mxp): Pop heading style from MXP style stack (requires style stack
            // implementation).
            qCDebug(lcMXP) << "End heading";
            break;

        // ========== STRUCTURAL (no end action needed) ==========
        case MXP_ACTION_BR:
        case MXP_ACTION_HR:
            // Self-closing, no end action
            break;

        case MXP_ACTION_P:
            // End paragraph mode
            m_bInParagraph = false;
            qCDebug(lcMXP) << "End paragraph: disabled paragraph mode";
            break;

        case MXP_ACTION_NOBR:
            m_bMXP_nobr = false;
            break;

        case MXP_ACTION_PRE:
            m_bMXP_preformatted = false;
            break;

        // ========== COLOR ==========
        case MXP_ACTION_COLOR:
            // TODO(mxp): Pop color state from MXP style stack (requires style stack
            // implementation).
            qCDebug(lcMXP) << "End color";
            break;

        // ========== FONT ==========
        case MXP_ACTION_FONT:
            // TODO(mxp): Pop font state from MXP style stack (requires style stack implementation).
            qCDebug(lcMXP) << "End font";
            break;

        // ========== INTERACTIVE ELEMENTS ==========
        case MXP_ACTION_SEND:
            // Clear send link state and undo style side-effects applied on open
            m_strMXP_link.clear();
            m_strMXP_hint.clear();
            m_bMXP_link_prompt = false;
            m_doc.m_iFlags &= ~(ACTIONTYPE | UNDERLINE);
            qCDebug(lcMXP) << "End send link";
            break;

        case MXP_ACTION_HYPERLINK:
            // Clear hyperlink state and undo style side-effects applied on open
            m_strMXP_link.clear();
            m_strMXP_hint.clear();
            m_doc.m_iFlags &= ~(ACTIONTYPE | UNDERLINE);
            qCDebug(lcMXP) << "End hyperlink";
            break;

        // ========== LISTS ==========
        case MXP_ACTION_UL:
        case MXP_ACTION_OL:
            // Reset list mode and wrap up the last item (original: mxpCloseAtomic.cpp:126-139)
            m_iListMode = eNoList;
            m_iListCount = 0;
            if (m_iMXP_list_depth > 0) {
                m_iMXP_list_depth--;
            }
            if (m_doc.m_currentLine && m_doc.m_currentLine->len() > 0)
                m_doc.StartNewLine(true, 0);
            qCDebug(lcMXP) << "End list (depth:" << m_iMXP_list_depth << ")";
            break;

        case MXP_ACTION_LI:
            // List item end (usually implicit)
            break;

        // ========== ALIGNMENT ==========
        case MXP_ACTION_CENTER:
            m_bMXP_centered = false;
            break;

        // ========== ADVANCED ELEMENTS ==========
        case MXP_ACTION_GAUGE:
        case MXP_ACTION_STAT:
            // These capture text between tags
            // TODO(mxp): Update gauge/stat display with captured text from GAUGE/STAT tag.
            qCDebug(lcMXP) << "End gauge/stat";
            break;

        case MXP_ACTION_EXPIRE:
            // Not implemented: MXP EXPIRE close — paired with EXPIRE open tag (also not
            // implemented).
            qCDebug(lcMXP) << "End expire";
            break;

        case MXP_ACTION_VAR:
            // TODO(mxp): Set MXP variable to captured text content (paired with VAR open tag).
            qCDebug(lcMXP) << "End var";
            break;

        // ========== SCRIPT ==========
        case MXP_ACTION_SCRIPT:
            // Not implemented: MXP SCRIPT close — server-sent code execution disabled for security.
            qCDebug(lcMXP) << "End script (not executed)";
            break;

        // ========== MEDIA (no end action) ==========
        case MXP_ACTION_SOUND:
        case MXP_ACTION_IMAGE:
        case MXP_ACTION_IMG:
            // These are typically empty or self-closing
            break;

        // ========== SERVER COMMANDS (no end action) ==========
        case MXP_ACTION_VERSION:
        case MXP_ACTION_USER:
        case MXP_ACTION_PASSWORD:
        case MXP_ACTION_RELOCATE:
            // These execute immediately
            break;

        // ========== PROTOCOL CONTROL (no end action) ==========
        case MXP_ACTION_RESET:
        case MXP_ACTION_MXP:
        case MXP_ACTION_SUPPORT:
        case MXP_ACTION_OPTION:
        case MXP_ACTION_RECOMMEND_OPTION:
        case MXP_ACTION_AFK:
            // These are commands, not containers
            break;

        // ========== FRAMES (NOT IMPLEMENTED) ==========
        case MXP_ACTION_FRAME:
        case MXP_ACTION_DEST:
            break;

        // ========== PUEBLO TAGS ==========
        case MXP_ACTION_BODY:
        case MXP_ACTION_HEAD:
        case MXP_ACTION_HTML:
        case MXP_ACTION_TITLE:
        case MXP_ACTION_XCH_PAGE:
        case MXP_ACTION_XCH_PANE:
            // Ignored
            break;

        // ========== FILTER (NOT IMPLEMENTED) ==========
        case MXP_ACTION_FILTER:
            break;

        case MXP_ACTION_NONE:
            // Custom element container with no action
            break;

        default:
            qCWarning(lcMXP) << "Unknown end action:" << action;
            break;
    }
}

/**
 * MXP_GetColor - Resolve color name or #RRGGBB
 */
QRgb MXPEngine::MXP_GetColor(const QString& colorSpec)
{
    // Handle hex color: #FF0000
    if (colorSpec.startsWith('#')) {
        bool ok;
        quint32 rgb = colorSpec.mid(1).toUInt(&ok, 16);
        if (ok) {
            return qRgb((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }
    }

    // Named colors — full HTML/CSS color table matching MUSHclient's MXP_colours[]
    // (reference/mushclient-original/mxp/mxpinit.cpp). RGB values are 0xRRGGBB in
    // the original; we extract R, G, B directly.
    // clang-format off
    struct ColorEntry { const char* name; quint8 r; quint8 g; quint8 b; };
    static constexpr ColorEntry kColors[] = {
        {"aliceblue",            0xf0, 0xf8, 0xff},
        {"antiquewhite",         0xfa, 0xeb, 0xd7},
        {"aqua",                 0x00, 0xff, 0xff},
        {"aquamarine",           0x7f, 0xff, 0xd4},
        {"azure",                0xf0, 0xff, 0xff},
        {"beige",                0xf5, 0xf5, 0xdc},
        {"bisque",               0xff, 0xe4, 0xc4},
        {"black",                0x00, 0x00, 0x00},
        {"blanchedalmond",       0xff, 0xeb, 0xcd},
        {"blue",                 0x00, 0x00, 0xff},
        {"blueviolet",           0x8a, 0x2b, 0xe2},
        {"brown",                0xa5, 0x2a, 0x2a},
        {"burlywood",            0xde, 0xb8, 0x87},
        {"cadetblue",            0x5f, 0x9e, 0xa0},
        {"chartreuse",           0x7f, 0xff, 0x00},
        {"chocolate",            0xd2, 0x69, 0x1e},
        {"coral",                0xff, 0x7f, 0x50},
        {"cornflowerblue",       0x64, 0x95, 0xed},
        {"cornsilk",             0xff, 0xf8, 0xdc},
        {"crimson",              0xdc, 0x14, 0x3c},
        {"cyan",                 0x00, 0xff, 0xff},
        {"darkblue",             0x00, 0x00, 0x8b},
        {"darkcyan",             0x00, 0x8b, 0x8b},
        {"darkgoldenrod",        0xb8, 0x86, 0x0b},
        {"darkgray",             0xa9, 0xa9, 0xa9},
        {"darkgrey",             0xa9, 0xa9, 0xa9},
        {"darkgreen",            0x00, 0x64, 0x00},
        {"darkkhaki",            0xbd, 0xb7, 0x6b},
        {"darkmagenta",          0x8b, 0x00, 0x8b},
        {"darkolivegreen",       0x55, 0x6b, 0x2f},
        {"darkorange",           0xff, 0x8c, 0x00},
        {"darkorchid",           0x99, 0x32, 0xcc},
        {"darkred",              0x8b, 0x00, 0x00},
        {"darksalmon",           0xe9, 0x96, 0x7a},
        {"darkseagreen",         0x8f, 0xbc, 0x8f},
        {"darkslateblue",        0x48, 0x3d, 0x8b},
        {"darkslategray",        0x2f, 0x4f, 0x4f},
        {"darkslategrey",        0x2f, 0x4f, 0x4f},
        {"darkturquoise",        0x00, 0xce, 0xd1},
        {"darkviolet",           0x94, 0x00, 0xd3},
        {"deeppink",             0xff, 0x14, 0x93},
        {"deepskyblue",          0x00, 0xbf, 0xff},
        {"dimgray",              0x69, 0x69, 0x69},
        {"dimgrey",              0x69, 0x69, 0x69},
        {"dodgerblue",           0x1e, 0x90, 0xff},
        {"firebrick",            0xb2, 0x22, 0x22},
        {"floralwhite",          0xff, 0xfa, 0xf0},
        {"forestgreen",          0x22, 0x8b, 0x22},
        {"fuchsia",              0xff, 0x00, 0xff},
        {"gainsboro",            0xdc, 0xdc, 0xdc},
        {"ghostwhite",           0xf8, 0xf8, 0xff},
        {"gold",                 0xff, 0xd7, 0x00},
        {"goldenrod",            0xda, 0xa5, 0x20},
        {"gray",                 0x80, 0x80, 0x80},
        {"grey",                 0x80, 0x80, 0x80},
        {"green",                0x00, 0x80, 0x00},
        {"greenyellow",          0xad, 0xff, 0x2f},
        {"honeydew",             0xf0, 0xff, 0xf0},
        {"hotpink",              0xff, 0x69, 0xb4},
        {"indianred",            0xcd, 0x5c, 0x5c},
        {"indigo",               0x4b, 0x00, 0x82},
        {"ivory",                0xff, 0xff, 0xf0},
        {"khaki",                0xf0, 0xe6, 0x8c},
        {"lavender",             0xe6, 0xe6, 0xfa},
        {"lavenderblush",        0xff, 0xf0, 0xf5},
        {"lawngreen",            0x7c, 0xfc, 0x00},
        {"lemonchiffon",         0xff, 0xfa, 0xcd},
        {"lightblue",            0xad, 0xd8, 0xe6},
        {"lightcoral",           0xf0, 0x80, 0x80},
        {"lightcyan",            0xe0, 0xff, 0xff},
        {"lightgoldenrodyellow", 0xfa, 0xfa, 0xd2},
        {"lightgreen",           0x90, 0xee, 0x90},
        {"lightgrey",            0xd3, 0xd3, 0xd3},
        {"lightgray",            0xd3, 0xd3, 0xd3},
        {"lightpink",            0xff, 0xb6, 0xc1},
        {"lightsalmon",          0xff, 0xa0, 0x7a},
        {"lightseagreen",        0x20, 0xb2, 0xaa},
        {"lightskyblue",         0x87, 0xce, 0xfa},
        {"lightslategray",       0x77, 0x88, 0x99},
        {"lightslategrey",       0x77, 0x88, 0x99},
        {"lightsteelblue",       0xb0, 0xc4, 0xde},
        {"lightyellow",          0xff, 0xff, 0xe0},
        {"lime",                 0x00, 0xff, 0x00},
        {"limegreen",            0x32, 0xcd, 0x32},
        {"linen",                0xfa, 0xf0, 0xe6},
        {"magenta",              0xff, 0x00, 0xff},
        {"maroon",               0x80, 0x00, 0x00},
        {"mediumaquamarine",     0x66, 0xcd, 0xaa},
        {"mediumblue",           0x00, 0x00, 0xcd},
        {"mediumorchid",         0xba, 0x55, 0xd3},
        {"mediumpurple",         0x93, 0x70, 0xdb},
        {"mediumseagreen",       0x3c, 0xb3, 0x71},
        {"mediumslateblue",      0x7b, 0x68, 0xee},
        {"mediumspringgreen",    0x00, 0xfa, 0x9a},
        {"mediumturquoise",      0x48, 0xd1, 0xcc},
        {"mediumvioletred",      0xc7, 0x15, 0x85},
        {"midnightblue",         0x19, 0x19, 0x70},
        {"mintcream",            0xf5, 0xff, 0xfa},
        {"mistyrose",            0xff, 0xe4, 0xe1},
        {"moccasin",             0xff, 0xe4, 0xb5},
        {"navajowhite",          0xff, 0xde, 0xad},
        {"navy",                 0x00, 0x00, 0x80},
        {"oldlace",              0xfd, 0xf5, 0xe6},
        {"olive",                0x80, 0x80, 0x00},
        {"olivedrab",            0x6b, 0x8e, 0x23},
        {"orange",               0xff, 0xa5, 0x00},
        {"orangered",            0xff, 0x45, 0x00},
        {"orchid",               0xda, 0x70, 0xd6},
        {"palegoldenrod",        0xee, 0xe8, 0xaa},
        {"palegreen",            0x98, 0xfb, 0x98},
        {"paleturquoise",        0xaf, 0xee, 0xee},
        {"palevioletred",        0xdb, 0x70, 0x93},
        {"papayawhip",           0xff, 0xef, 0xd5},
        {"peachpuff",            0xff, 0xda, 0xb9},
        {"peru",                 0xcd, 0x85, 0x3f},
        {"pink",                 0xff, 0xc0, 0xcb},
        {"plum",                 0xdd, 0xa0, 0xdd},
        {"powderblue",           0xb0, 0xe0, 0xe6},
        {"purple",               0x80, 0x00, 0x80},
        {"rebeccapurple",        0x66, 0x33, 0x99},
        {"red",                  0xff, 0x00, 0x00},
        {"rosybrown",            0xbc, 0x8f, 0x8f},
        {"royalblue",            0x41, 0x69, 0xe1},
        {"saddlebrown",          0x8b, 0x45, 0x13},
        {"salmon",               0xfa, 0x80, 0x72},
        {"sandybrown",           0xf4, 0xa4, 0x60},
        {"seagreen",             0x2e, 0x8b, 0x57},
        {"seashell",             0xff, 0xf5, 0xee},
        {"sienna",               0xa0, 0x52, 0x2d},
        {"silver",               0xc0, 0xc0, 0xc0},
        {"skyblue",              0x87, 0xce, 0xeb},
        {"slateblue",            0x6a, 0x5a, 0xcd},
        {"slategray",            0x70, 0x80, 0x90},
        {"slategrey",            0x70, 0x80, 0x90},
        {"snow",                 0xff, 0xfa, 0xfa},
        {"springgreen",          0x00, 0xff, 0x7f},
        {"steelblue",            0x46, 0x82, 0xb4},
        {"tan",                  0xd2, 0xb4, 0x8c},
        {"teal",                 0x00, 0x80, 0x80},
        {"thistle",              0xd8, 0xbf, 0xd8},
        {"tomato",               0xff, 0x63, 0x47},
        {"turquoise",            0x40, 0xe0, 0xd0},
        {"violet",               0xee, 0x82, 0xee},
        {"wheat",                0xf5, 0xde, 0xb3},
        {"white",                0xff, 0xff, 0xff},
        {"whitesmoke",           0xf5, 0xf5, 0xf5},
        {"yellow",               0xff, 0xff, 0x00},
        {"yellowgreen",          0x9a, 0xcd, 0x32},
    };
    // clang-format on

    QString lower = colorSpec.toLower().trimmed();
    for (const auto& entry : kColors) {
        if (lower == QLatin1StringView(entry.name))
            return qRgb(entry.r, entry.g, entry.b);
    }

    qCDebug(lcMXP) << "Unknown color:" << colorSpec << "- defaulting to white";
    return qRgb(255, 255, 255); // Default white
}

/**
 * MXP_CloseOpenTags - Close all unclosed tags
 *
 * Based on: mxpProcess.cpp - called when MXP is turned off or line ends
 *
 * Closes all tags in the active tag list, executing their end actions.
 * Tags marked with TAG_NO_RESET are protected from this.
 */
void MXPEngine::MXP_CloseOpenTags(bool closeAll)
{
    qCDebug(lcMXP) << "Closing" << (closeAll ? "ALL" : "open") << "tags";

    // Close tags in reverse order (most recent first)
    // Original: MXP_CloseOpenTags (mxpClose.cpp:363-387) stops at secure tags
    //           MXP_CloseAllTags (mxpClose.cpp:389-406) closes all except TAG_NO_RESET
    while (!m_activeTagList.empty()) {
        ActiveTag* tag = m_activeTagList.back().get();

        // Don't close securely-opened tags (unless closeAll is true)
        if (!closeAll && tag->bSecure) {
            return;
        }

        // Move last element out and pop
        auto owned = std::move(m_activeTagList.back());
        m_activeTagList.pop_back();

        // Execute end action
        if (!owned->bNoReset) {
            MXP_EndAction(owned->iAction);
        }
    }
}

/**
 * MXP_CloseTag - Close specific tag by name (internal logging helper)
 *
 * Based on: mxpEnd.cpp - executes the end action for a tag
 *
 * @param tagName Name of the tag being closed
 */
void MXPEngine::MXP_CloseTag(const QString& tagName)
{
    qCDebug(lcMXP) << "Closing tag:" << tagName;
    // Actual end action execution happens in MXP_EndAction
}

// ========================================================================
// MXP Mode Helper Functions
// ========================================================================

/**
 * MXP_Open - Check if MXP is in open mode
 *
 * Returns true if current MXP mode allows open (unsecure) tags.
 */
bool MXPEngine::MXP_Open() const
{
    return m_iMXP_mode == eMXP_open || m_iMXP_mode == eMXP_perm_open;
}

/**
 * MXP_Secure - Check if MXP is in secure mode
 *
 * Returns true if current MXP mode allows secure tags.
 */
bool MXPEngine::MXP_Secure() const
{
    // Original: secure, perm_secure, secure_once. Locked mode = NO tags allowed.
    return m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure ||
           m_iMXP_mode == eMXP_secure_once;
}
