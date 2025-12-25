/**
 * world_mxp.cpp - MXP (MUD eXtension Protocol) implementation
 *
 * This file implements the complete MXP protocol parsing and execution,
 * based on the original MUSHclient MXP implementation.
 *
 * References:
 * - Original: mxp (all .cpp files) from MUSHclient
 * - Spec: http://www.zuggsoft.com/zmud/mxp.htm
 */

#include "../text/action.h"
#include "../text/line.h"
#include "../text/style.h"
#include "../utils/logging.h"
#include "world_document.h"
#include <QDebug>
#include <QRegularExpression>

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
void WorldDocument::InitializeMXPElements()
{
    // Clear any existing elements
    qDeleteAll(m_atomicElementMap);
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
        {"strike", TAG_MXP, MXP_ACTION_STRIKE, ""},
        {"strikeout", TAG_MXP, MXP_ACTION_STRIKE, ""},
        {"small", TAG_MXP, MXP_ACTION_SMALL, ""},
        {"tt", TAG_MXP, MXP_ACTION_TT, ""},
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
        {"color", TAG_MXP, MXP_ACTION_COLOR, "fore,back"},
        {"font", TAG_MXP, MXP_ACTION_FONT, "face,size,color,back"},

        // Links and actions (hyperlink is secure, send requires open mode)
        {"a", TAG_MXP, MXP_ACTION_HYPERLINK, "href"},
        {"send", TAG_OPEN | TAG_MXP, MXP_ACTION_SEND, "href,hint,prompt"},

        // Media (open mode required for security)
        {"sound", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_SOUND, "fname,v,l,p,t,u"},
        {"music", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_SOUND, "fname,v,l,p,t,u,c"},
        {"image", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_IMAGE,
         "fname,url,t,h,w,hspace,vspace,align"},

        // Gauges and stats (secure)
        {"gauge", TAG_MXP, MXP_ACTION_GAUGE, "entity,max,caption,color"},
        {"stat", TAG_MXP, MXP_ACTION_STAT, "entity,max,caption"},

        // Special commands (mostly secure)
        {"version", TAG_MXP | TAG_COMMAND, MXP_ACTION_VERSION, ""},
        {"support", TAG_MXP, MXP_ACTION_SUPPORT, ""},
        {"expire", TAG_MXP, MXP_ACTION_EXPIRE, "name"},
        {"var", TAG_MXP | TAG_COMMAND, MXP_ACTION_VAR, ""},

        // User/password (open mode)
        {"user", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_USER, ""},
        {"password", TAG_OPEN | TAG_COMMAND | TAG_MXP, MXP_ACTION_PASSWORD, ""},

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

        // Special MXP commands
        {"reset", TAG_MXP | TAG_COMMAND, MXP_ACTION_RESET, ""},
        {"mxp", TAG_MXP | TAG_COMMAND, MXP_ACTION_MXP, ""},

        {nullptr, 0, 0, nullptr} // Sentinel
    };

    // Load elements into map
    for (int i = 0; elements[i].name; i++) {
        AtomicElement* elem = new AtomicElement();
        elem->strName = QString::fromLatin1(elements[i].name);
        elem->iFlags = elements[i].flags;
        elem->iAction = elements[i].action;
        elem->strArgs = QString::fromLatin1(elements[i].args);

        m_atomicElementMap.insert(elem->strName.toLower(), elem);
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
void WorldDocument::InitializeMXPEntities()
{
    // Clear any existing entities
    qDeleteAll(m_entityMap);
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
        {"nbsp", 0xA0}, // Non-breaking space

        // Common symbols
        {"copy", 0xA9},    // ©
        {"reg", 0xAE},     // ®
        {"trade", 0x2122}, // ™
        {"euro", 0x20AC},  // €
        {"pound", 0xA3},   // £
        {"yen", 0xA5},     // ¥
        {"cent", 0xA2},    // ¢

        // Math symbols
        {"times", 0xD7},  // ×
        {"divide", 0xF7}, // ÷
        {"plusmn", 0xB1}, // ±
        {"deg", 0xB0},    // °
        {"frac12", 0xBD}, // ½
        {"frac14", 0xBC}, // ¼
        {"frac34", 0xBE}, // ¾

        // Arrows
        {"larr", 0x2190}, // ←
        {"uarr", 0x2191}, // ↑
        {"rarr", 0x2192}, // →
        {"darr", 0x2193}, // ↓

        // Misc
        {"hearts", 0x2665}, // ♥
        {"clubs", 0x2663},  // ♣
        {"spades", 0x2660}, // ♠
        {"diams", 0x2666},  // ♦

        {nullptr, 0} // Sentinel
    };

    // Load entities into map
    for (int i = 0; entities[i].name; i++) {
        MXPEntity* entity = new MXPEntity();
        entity->strName = QString::fromLatin1(entities[i].name);
        entity->iCodepoint = entities[i].codepoint;

        m_entityMap.insert(entity->strName, entity);
    }

    qCDebug(lcMXP) << "Initialized" << m_entityMap.size() << "MXP entities";
}

/**
 * CleanupMXP - Free all MXP resources
 *
 * Called when world is closed or MXP is turned off.
 */
void WorldDocument::CleanupMXP()
{
    // Clean up atomic elements
    qDeleteAll(m_atomicElementMap);
    m_atomicElementMap.clear();

    // Clean up custom elements
    qDeleteAll(m_customElementMap);
    m_customElementMap.clear();

    // Clean up entities
    qDeleteAll(m_entityMap);
    m_entityMap.clear();

    qDeleteAll(m_customEntityMap);
    m_customEntityMap.clear();

    // Clean up active tags
    qDeleteAll(m_activeTagList);
    m_activeTagList.clear();

    qCDebug(lcMXP) << "Cleaned up MXP resources";
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
AtomicElement* WorldDocument::MXP_FindAtomicElement(const QString& name)
{
    return m_atomicElementMap.value(name.toLower(), nullptr);
}

/**
 * MXP_FindCustomElement - Look up a user-defined MXP element
 *
 * @param name Element name (case-insensitive)
 * @return Pointer to element, or nullptr if not found
 */
CustomElement* WorldDocument::MXP_FindCustomElement(const QString& name)
{
    return m_customElementMap.value(name.toLower(), nullptr);
}

// ========================================================================
// MXP Parsing - Argument Extraction
// ========================================================================

/**
 * ParseMXPTag - Parse MXP tag string into name and arguments
 *
 * Based on: mxp/mxpOpenAtomic.cpp argument parsing
 *
 * Parses strings like:
 *   "send href='go north' hint=\"Click me\""
 * Into:
 *   tagName = "send"
 *   args = [{name="href", value="go north"}, {name="hint", value="Click me"}]
 *
 * @param tagString The tag content (without < >)
 * @param tagName Output: The tag name
 * @param args Output: List of arguments
 */
void WorldDocument::ParseMXPTag(const QString& tagString, QString& tagName, MXPArgumentList& args)
{
    // Clean up any existing arguments
    qDeleteAll(args);
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
                MXPArgument* arg = new MXPArgument();

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
                args.append(arg);
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
            MXPArgument* arg = new MXPArgument();
            arg->strName = name;
            arg->strValue = value;
            arg->iPosition = argPosition++;
            args.append(arg);
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
QString WorldDocument::GetMXPArgument(const MXPArgumentList& args, const QString& name)
{
    for (MXPArgument* arg : args) {
        if (arg->strName.compare(name, Qt::CaseInsensitive) == 0) {
            arg->bUsed = true;
            return arg->strValue;
        }
    }
    return QString();
}

/**
 * MXP_GetArgument - Convenient wrapper for GetMXPArgument
 *
 * This is a convenience function that swaps the parameter order
 * to make it more natural to call: MXP_GetArgument("href", args)
 */
QString WorldDocument::MXP_GetArgument(const QString& name, const MXPArgumentList& args)
{
    return GetMXPArgument(args, name);
}

/**
 * MXP_HasArgument - Check if an argument exists (keyword or named)
 *
 * Used to check for boolean arguments like "prompt" in <send prompt>
 */
bool WorldDocument::MXP_HasArgument(const QString& name, const MXPArgumentList& args)
{
    for (MXPArgument* arg : args) {
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
void WorldDocument::MXP_collected_element()
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
void WorldDocument::MXP_collected_entity()
{
    QString entityName = m_strMXPstring;
    m_strMXPstring.clear();

    QString replacement = MXP_GetEntity(entityName);

    if (!replacement.isNull()) {
        // Inject replacement text back into processing
        // This is done by adding it to the current line
        for (int i = 0; i < replacement.length(); i++) {
            AddToLine(replacement[i].toLatin1());
        }
        m_iMXPentities++;
    } else {
        // Unknown entity - show as-is
        QString entityStr = QString("&%1;").arg(entityName);
        for (int i = 0; i < entityStr.length(); i++) {
            AddToLine(entityStr[i].toLatin1());
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
QString WorldDocument::MXP_GetEntity(const QString& entityName)
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

    // Check custom entities first (they can override)
    MXPEntity* entity = m_customEntityMap.value(entityName, nullptr);
    if (entity) {
        // Custom entities use strValue (can be multi-character)
        if (!entity->strValue.isEmpty()) {
            return entity->strValue;
        }
        // Fallback to iCodepoint for backward compatibility
        return QString::fromUcs4(reinterpret_cast<const char32_t*>(&entity->iCodepoint), 1);
    }

    // Check standard entities (these always use iCodepoint)
    entity = m_entityMap.value(entityName, nullptr);
    if (entity) {
        return QString::fromUcs4(reinterpret_cast<const char32_t*>(&entity->iCodepoint), 1);
    }

    qCDebug(lcMXP) << "Unknown entity:" << entityName;
    return QString(); // Unknown
}

// ========================================================================
// MXP Tag Processing - Stubs (TO BE FULLY IMPLEMENTED)
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
void WorldDocument::MXP_StartTag(const QString& tagString)
{
    QString tagName;
    MXPArgumentList args;

    ParseMXPTag(tagString, tagName, args);

    if (tagName.isEmpty()) {
        qCWarning(lcMXP) << "Empty tag name";
        m_iMXPerrors++;
        qDeleteAll(args);
        return;
    }

    tagName = tagName.toLower(); // Case-insensitive

    // Check if current mode is secure
    bool bSecure = (m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure);

    // Restore mode (cancel secure-once)
    if (m_iMXP_mode == eMXP_perm_secure) {
        m_iMXP_previousMode = eMXP_secure;
        m_iMXP_mode = eMXP_secure;
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
            qDeleteAll(args);
            return;
        }
        bOpen = pCustomElement->bOpen;
        bCommand = pCustomElement->bCommand;
    }

    // SECURITY CHECK: Tags WITH TAG_OPEN require open mode (blocked in secure mode)
    if (bOpen && bSecure) {
        qCWarning(lcMXP) << "Open-mode tag blocked in secure mode:" << tagName;
        m_iMXPerrors++;
        qDeleteAll(args);
        return;
    }

    // Expand entities in arguments
    for (MXPArgument* arg : args) {
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
        ActiveTag* pTag = new ActiveTag();
        pTag->strName = tagName;
        pTag->bSecure = bSecure;
        pTag->bNoReset = bNoReset;
        pTag->iAction = pAtomicElement ? pAtomicElement->iAction : MXP_ACTION_NONE;
        m_activeTagList.append(pTag);

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
        for (ElementItem* item : pCustomElement->elementItemList) {
            if (!item->pAtomicElement)
                continue;

            // Merge custom element attributes with item arguments
            MXPArgumentList expandedArgs;

            // Add custom element's default attributes
            for (MXPArgument* attr : pCustomElement->attributeList) {
                MXPArgument* newArg = new MXPArgument();
                *newArg = *attr; // Copy
                expandedArgs.append(newArg);
            }

            // Add item-specific arguments
            for (MXPArgument* itemArg : item->argumentList) {
                MXPArgument* newArg = new MXPArgument();
                *newArg = *itemArg; // Copy
                expandedArgs.append(newArg);
            }

            // Override with user-provided arguments
            for (MXPArgument* userArg : args) {
                // Find if this argument was already added
                bool found = false;
                for (MXPArgument* arg : expandedArgs) {
                    if (arg->strName.compare(userArg->strName, Qt::CaseInsensitive) == 0) {
                        arg->strValue = userArg->strValue;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    MXPArgument* newArg = new MXPArgument();
                    *newArg = *userArg; // Copy
                    expandedArgs.append(newArg);
                }
            }

            // Execute the atomic element
            MXP_ExecuteAction(item->pAtomicElement, expandedArgs);

            qDeleteAll(expandedArgs);
        }
    }

    qDeleteAll(args);
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
void WorldDocument::MXP_EndTag(const QString& tagString)
{
    // Check if current mode is secure
    bool bSecure = (m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure);

    // Restore mode (cancel secure-once)
    if (m_iMXP_mode == eMXP_perm_secure) {
        m_iMXP_previousMode = eMXP_secure;
        m_iMXP_mode = eMXP_secure;
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
    ActiveTag* pMatchingTag = nullptr;
    int matchIndex = -1;

    for (int i = m_activeTagList.size() - 1; i >= 0; i--) {
        ActiveTag* pTag = m_activeTagList[i];

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
    while (m_activeTagList.size() > matchIndex) {
        ActiveTag* pTag = m_activeTagList.takeLast();
        QString closingName = pTag->strName;

        if (closingName != strName) {
            qCDebug(lcMXP) << "Closing out-of-sequence tag:" << closingName;
        }

        // Execute end action for this tag
        MXP_EndAction(pTag->iAction);

        delete pTag;

        if (closingName == strName) {
            break; // Found the tag we're supposed to close
        }
    }
}

/**
 * MXP_Definition - Process MXP definition
 *
 * TODO: Full implementation
 */
void WorldDocument::MXP_Definition(const QString& defString)
{
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
 * Syntax: <!ELEMENT hp '<color &col;><send>' ATT='col=red max=100' TAG=20 FLAG='hp_var' OPEN EMPTY>
 *
 * This creates a custom element that expands to atomic elements.
 * When <hp col=blue max=50> is used, it expands to <color blue><send>
 *
 * @param defString Element definition (after "ELEMENT")
 */
void WorldDocument::MXP_DefineElement(const QString& defString)
{
    QString tagName;
    MXPArgumentList args;

    // Parse the entire definition as if it's a tag with arguments
    ParseMXPTag(defString, tagName, args);

    if (tagName.isEmpty()) {
        qCWarning(lcMXP) << "Missing element name in definition:" << defString;
        m_iMXPerrors++;
        qDeleteAll(args);
        return;
    }

    QString strName = tagName.toLower(); // Case-insensitive

    // Check if trying to redefine built-in element
    if (MXP_FindAtomicElement(strName)) {
        qCWarning(lcMXP) << "Cannot redefine built-in element:" << strName;
        m_iMXPerrors++;
        qDeleteAll(args);
        return;
    }

    // Check for DELETE keyword
    bool bDelete = false;
    for (MXPArgument* arg : args) {
        if (arg->bKeyword && arg->strName.compare("DELETE", Qt::CaseInsensitive) == 0) {
            bDelete = true;
            break;
        }
    }

    // Delete old element if it exists
    CustomElement* oldElement = m_customElementMap.value(strName, nullptr);
    if (oldElement) {
        if (!bDelete) {
            qCDebug(lcMXP) << "Replacing custom element:" << strName;
        }
        // Note: CustomElement destructor handles cleanup of elementItemList and attributeList
        delete oldElement;
        m_customElementMap.remove(strName);
    }

    if (bDelete) {
        qCDebug(lcMXP) << "Deleted custom element:" << strName;
        qDeleteAll(args);
        return; // All done
    }

    // Create new element
    CustomElement* pElement = new CustomElement();
    pElement->strName = strName;

    // Parse keywords
    for (MXPArgument* arg : args) {
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
    for (MXPArgument* arg : args) {
        if (!arg->bKeyword && arg->strName.isEmpty()) {
            definition = arg->strValue; // May be empty for EMPTY elements
            hasDefinitionArg = true;
            break;
        }
    }

    if (!hasDefinitionArg) {
        qCWarning(lcMXP) << "Missing definition in element:" << strName;
        m_iMXPerrors++;
        delete pElement;
        qDeleteAll(args);
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
            qDeleteAll(pElement->elementItemList);
            qDeleteAll(pElement->attributeList);
            delete pElement;
            qDeleteAll(args);
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
                qDeleteAll(pElement->elementItemList);
                qDeleteAll(pElement->attributeList);
                delete pElement;
                qDeleteAll(args);
                return;
            } else if (!inQuote && c == '>') {
                break;
            }

            pos++;
        }

        if (pos >= definition.length()) {
            qCWarning(lcMXP) << "Missing '>' in element definition:" << definition;
            m_iMXPerrors++;
            qDeleteAll(pElement->elementItemList);
            qDeleteAll(pElement->attributeList);
            delete pElement;
            qDeleteAll(args);
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
            qDeleteAll(atomArgs);
            continue;
        }

        if (atomName.startsWith('!')) {
            qCWarning(lcMXP) << "Element definitions cannot define elements:" << atomName;
            m_iMXPerrors++;
            qDeleteAll(atomArgs);
            continue;
        }

        // Look up atomic element
        AtomicElement* atomicElem = MXP_FindAtomicElement(atomName);
        if (!atomicElem) {
            qCWarning(lcMXP) << "Unknown atomic element in definition:" << atomName;
            m_iMXPerrors++;
            qDeleteAll(atomArgs);
            continue;
        }

        // Create element item
        ElementItem* item = new ElementItem();
        item->pAtomicElement = atomicElem;
        item->argumentList = atomArgs; // Transfer ownership
        pElement->elementItemList.append(item);
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

    // Add to custom element map
    m_customElementMap.insert(strName, pElement);
    qCDebug(lcMXP) << "Defined custom element:" << strName << "with"
                   << pElement->elementItemList.size() << "atomic elements";

    qDeleteAll(args);
}

/**
 * MXP_DefineEntity - Define custom entity
 *
 * Based on: mxpDefs.cpp::MXP_Entity
 *
 * Syntax: <!ENTITY name "value">
 *         <!ENTITY name 'value'>
 *         <!ENTITY name DELETE>
 *
 * The value can contain other entities which will be expanded.
 * Example: <!ENTITY blah '&lt;Nick&gt;'>
 *
 * @param defString Entity definition (after "ENTITY")
 */
void WorldDocument::MXP_DefineEntity(const QString& defString)
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
    if (m_entityMap.contains(strName)) {
        qCWarning(lcMXP) << "Cannot redefine built-in entity:" << strName;
        m_iMXPerrors++;
        return;
    }

    // Check for DELETE keyword
    if (remaining.startsWith("DELETE", Qt::CaseInsensitive)) {
        // Remove custom entity
        MXPEntity* entity = m_customEntityMap.value(strName, nullptr);
        if (entity) {
            delete entity;
            m_customEntityMap.remove(strName);
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

    // Delete old entity if it exists
    MXPEntity* oldEntity = m_customEntityMap.value(strName, nullptr);
    if (oldEntity) {
        delete oldEntity;
        qCDebug(lcMXP) << "Replacing custom entity:" << strName;
    }

    // Create new entity
    MXPEntity* entity = new MXPEntity();
    entity->strName = strName;
    entity->strValue = strFixedValue;

    // For single-character entities, also set iCodepoint for compatibility
    if (strFixedValue.length() == 1) {
        entity->iCodepoint = strFixedValue[0].unicode();
    } else {
        entity->iCodepoint = 0; // Multi-character entity
    }

    m_customEntityMap.insert(strName, entity);
    qCDebug(lcMXP) << "Defined custom entity:" << strName << "=" << strFixedValue;

    // TODO: Plugin callback (ON_PLUGIN_MXP_SETENTITY)
}

/**
 * MXP_ExecuteAction - Execute MXP element action
 *
 * Based on: mxpProcess.cpp:execute_element() from original MUSHclient
 *
 * Executes the action associated with an MXP element. This is called when
 * an opening tag is encountered and should apply the element's effect.
 */
void WorldDocument::MXP_ExecuteAction(AtomicElement* elem, const MXPArgumentList& args)
{
    if (!elem) {
        return;
    }

    int action = elem->iAction;
    qCDebug(lcMXP) << "Execute action:" << action << "(" << elem->strName << ")";

    switch (action) {
        // ========== TEXT FORMATTING ==========
        case MXP_ACTION_BOLD:
            m_iFlags |= HILITE;
            qCDebug(lcMXP) << "Bold: set HILITE flag";
            break;

        case MXP_ACTION_ITALIC:
            m_iFlags |= BLINK; // BLINK is repurposed for italic
            qCDebug(lcMXP) << "Italic: set BLINK flag";
            break;

        case MXP_ACTION_UNDERLINE:
            m_iFlags |= UNDERLINE;
            qCDebug(lcMXP) << "Underline: set UNDERLINE flag";
            break;

        case MXP_ACTION_STRIKE:
            m_iFlags |= STRIKEOUT;
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
            // TODO: Set heading style (bold + larger size)
            qCDebug(lcMXP) << "Heading action:" << elem->strName;
            break;

        // ========== LINE BREAKS / STRUCTURE ==========
        case MXP_ACTION_BR:
            // Hard line break - flush current line and start new one
            StartNewLine(true, 0);
            qCDebug(lcMXP) << "Line break: started new line";
            break;

        case MXP_ACTION_HR:
            // Horizontal rule - mark line with HORIZ_RULE flag
            // Wrap up previous line if it has content
            if (m_currentLine && m_currentLine->len() > 0) {
                StartNewLine(true, 0);
            }
            // Mark this line as a horizontal rule
            if (m_currentLine) {
                m_currentLine->flags |= HORIZ_RULE;
            }
            // Finish the HR line
            StartNewLine(true, 0);
            qCDebug(lcMXP) << "Horizontal rule: set HORIZ_RULE flag on line";
            break;

        case MXP_ACTION_P:
            // Paragraph mode - enables text wrapping, newlines are suppressed
            m_cLastChar = 0;
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
            QString fore = MXP_GetArgument("fore", args);
            QString back = MXP_GetArgument("back", args);

            if (!fore.isEmpty()) {
                QRgb fgColor = MXP_GetColor(fore);
                // Convert QRgb to int for storage
                m_iForeColour = (int)fgColor;
                // Set RGB color mode
                m_iFlags = (m_iFlags & ~COLOURTYPE) | COLOUR_RGB;
                qCDebug(lcMXP) << "Set foreground color:" << fore << "="
                               << QString::number(fgColor, 16);
            }

            if (!back.isEmpty()) {
                QRgb bgColor = MXP_GetColor(back);
                // Convert QRgb to int for storage
                m_iBackColour = (int)bgColor;
                // Set RGB color mode (only need to set once)
                m_iFlags = (m_iFlags & ~COLOURTYPE) | COLOUR_RGB;
                qCDebug(lcMXP) << "Set background color:" << back << "="
                               << QString::number(bgColor, 16);
            }
            break;
        }

        // ========== FONT ==========
        case MXP_ACTION_FONT: {
            // <font face=Arial size=12 color=red>
            QString face = MXP_GetArgument("face", args);
            QString size = MXP_GetArgument("size", args);
            QString color = MXP_GetArgument("color", args);

            if (!face.isEmpty()) {
                // TODO: Push font face onto style stack
                qCDebug(lcMXP) << "Set font face:" << face;
            }

            if (!size.isEmpty()) {
                // TODO: Push font size onto style stack
                qCDebug(lcMXP) << "Set font size:" << size;
            }

            if (!color.isEmpty()) {
                QRgb fgColor = MXP_GetColor(color);
                // TODO: Push font color onto style stack
                qCDebug(lcMXP) << "Set font color:" << color;
            }
            break;
        }

        // ========== INTERACTIVE ELEMENTS ==========
        case MXP_ACTION_SEND: {
            // <send href="command" hint="tooltip" prompt>
            QString href = MXP_GetArgument("href", args);
            QString hint = MXP_GetArgument("hint", args);
            bool prompt = MXP_HasArgument("prompt", args);

            // Store link info for text that follows
            m_strMXP_link = href;
            m_strMXP_hint = hint;
            m_bMXP_link_prompt = prompt;

            // Mark that we're in a clickable region
            m_iFlags |= 0x0400; // ACTION_SEND flag (part of ACTIONTYPE bits)

            qCDebug(lcMXP) << "Begin send link:" << href << "hint:" << hint << "prompt:" << prompt;
            break;
        }

        case MXP_ACTION_HYPERLINK: {
            // <a href="http://url">
            QString href = MXP_GetArgument("href", args);
            QString hint = MXP_GetArgument("hint", args);

            // Store hyperlink info for text that follows
            m_strMXP_link = href;
            m_strMXP_hint = hint;
            m_bMXP_link_prompt = false;

            // Mark that we're in a hyperlink region
            m_iFlags |= 0x0800; // ACTION_HYPERLINK flag (part of ACTIONTYPE bits)

            qCDebug(lcMXP) << "Begin hyperlink:" << href << "hint:" << hint;
            break;
        }

        // ========== MEDIA ==========
        case MXP_ACTION_SOUND: {
            // <sound fname="file.wav" v=100 l=1 p=50 t=music u="http://url">
            QString fname = MXP_GetArgument("fname", args);
            QString volume = MXP_GetArgument("v", args);
            QString loops = MXP_GetArgument("l", args);
            QString priority = MXP_GetArgument("p", args);
            QString type = MXP_GetArgument("t", args);
            QString url = MXP_GetArgument("u", args);

            if (fname.isEmpty() && url.isEmpty()) {
                qCWarning(lcMXP) << "Sound tag has no fname or URL";
                break;
            }

            // Use Lua sound API
            int vol = volume.isEmpty() ? 100 : volume.toInt();
            int loop = loops.isEmpty() ? 1 : (loops.toInt() == -1 ? -1 : loops.toInt());

            QString soundFile = fname.isEmpty() ? url : fname;
            qCDebug(lcMXP) << "Playing sound:" << soundFile << "volume:" << vol << "loops:" << loop;

            // Play sound using Qt Multimedia system
            // Note: Currently using PlaySoundFile for simplicity.
            // Full implementation would use PlaySound() with buffer management for volume/loop
            // control.
            if (PlaySoundFile(soundFile)) {
                qCDebug(lcMXP) << "Successfully started sound playback";
            } else {
                qCWarning(lcMXP) << "Failed to play sound:" << soundFile;
            }
            break;
        }

        case MXP_ACTION_IMAGE:
        case MXP_ACTION_IMG: {
            // <image fname="pic.png" url="http://..." align=left h=100 w=100>
            // Display inline image in text flow
            QString fname = MXP_GetArgument("fname", args);
            QString url = MXP_GetArgument("url", args);
            QString align = MXP_GetArgument("align", args);
            QString height = MXP_GetArgument("h", args);
            QString width = MXP_GetArgument("w", args);

            // Full implementation would:
            // 1. Load image from fname or download from url
            // 2. Scale to specified width/height
            // 3. Insert image inline in text flow with specified alignment
            // 4. Handle image caching for repeated use
            QString imgSource = fname.isEmpty() ? url : fname;
            qCDebug(lcMXP) << "Image: src=" << imgSource << "align=" << align << "w=" << width
                           << "h=" << height
                           << "(requires text pipeline integration for inline images)";
            break;
        }

        // ========== SERVER COMMANDS ==========
        case MXP_ACTION_VERSION:
            // Send client version to server
            // TODO: Send version string
            qCDebug(lcMXP) << "Send version (not implemented)";
            break;

        case MXP_ACTION_USER:
            // Send username to server
            // TODO: Send from world settings
            qCDebug(lcMXP) << "Send username (not implemented)";
            break;

        case MXP_ACTION_PASSWORD:
            // Send password to server (masked input)
            // TODO: Prompt for password or send from settings
            qCDebug(lcMXP) << "Send password (not implemented)";
            break;

        case MXP_ACTION_RELOCATE: {
            // <relocate server="mud.example.com" port="4000">
            QString server = MXP_GetArgument("server", args);
            QString port = MXP_GetArgument("port", args);

            // TODO: Disconnect and reconnect to new server
            qCDebug(lcMXP) << "Relocate to:" << server << ":" << port << "(not implemented)";
            break;
        }

        // ========== ADVANCED ELEMENTS ==========
        case MXP_ACTION_GAUGE: {
            // <gauge entity="hp" max="100" caption="Health" color="red">
            // Text between tags is current value: <gauge entity="hp" max="100">75</gauge>
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
            // Similar to gauge but stored as numeric stat instead of progress bar
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

            // TODO: Mark text for expiration
            qCDebug(lcMXP) << "Expire:" << name << "when:" << when;
            break;
        }

        case MXP_ACTION_VAR: {
            // <var name="hp">100</var> or <var name="hp" value="100">
            QString name = MXP_GetArgument("name", args);
            QString value = MXP_GetArgument("value", args);

            // TODO: Set MXP variable (stored in m_mxpVariableMap)
            qCDebug(lcMXP) << "Set variable:" << name << "=" << value;
            break;
        }

        case MXP_ACTION_AFK:
            // <afk> - mark player as away from keyboard
            // TODO: Record AFK time
            qCDebug(lcMXP) << "AFK marker (not implemented)";
            break;

        // ========== PROTOCOL CONTROL ==========
        case MXP_ACTION_RESET:
            // <reset> - close all open tags
            MXP_CloseOpenTags();
            break;

        case MXP_ACTION_MXP: {
            // <mxp> - MXP protocol commands
            // This is usually handled separately in line processing
            qCDebug(lcMXP) << "MXP command (not implemented)";
            break;
        }

        case MXP_ACTION_SUPPORT: {
            // <support> - report what we support
            // TODO: Send list of supported tags
            qCDebug(lcMXP) << "Support query (not implemented)";
            break;
        }

        case MXP_ACTION_OPTION: {
            // Client option set
            QString option = MXP_GetArgument("option", args);
            QString value = MXP_GetArgument("value", args);

            // TODO: Set client option
            qCDebug(lcMXP) << "Set option:" << option << "=" << value;
            break;
        }

        case MXP_ACTION_RECOMMEND_OPTION: {
            // Server recommends option
            QString option = MXP_GetArgument("option", args);
            QString value = MXP_GetArgument("value", args);

            // TODO: Consider setting client option
            qCDebug(lcMXP) << "Recommend option:" << option << "=" << value;
            break;
        }

        // ========== LISTS ==========
        case MXP_ACTION_UL:
            // Unordered list
            m_iMXP_list_depth++;
            qCDebug(lcMXP) << "Start unordered list (depth:" << m_iMXP_list_depth << ")";
            break;

        case MXP_ACTION_OL:
            // Ordered list
            m_iMXP_list_depth++;
            m_iMXP_list_counter = 1;
            qCDebug(lcMXP) << "Start ordered list (depth:" << m_iMXP_list_depth << ")";
            break;

        case MXP_ACTION_LI:
            // List item
            // TODO: Insert list item with bullet or number
            qCDebug(lcMXP) << "List item";
            break;

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
            // TODO: Execute script (security check required!)
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
 * For most formatting actions, this means popping state from the style stack.
 */
void WorldDocument::MXP_EndAction(int action)
{
    qCDebug(lcMXP) << "End action:" << action;

    switch (action) {
        // ========== TEXT FORMATTING ==========
        case MXP_ACTION_BOLD:
            m_iFlags &= ~HILITE;
            qCDebug(lcMXP) << "End bold: clear HILITE flag";
            break;

        case MXP_ACTION_ITALIC:
            m_iFlags &= ~BLINK; // BLINK is repurposed for italic
            qCDebug(lcMXP) << "End italic: clear BLINK flag";
            break;

        case MXP_ACTION_UNDERLINE:
            m_iFlags &= ~UNDERLINE;
            qCDebug(lcMXP) << "End underline: clear UNDERLINE flag";
            break;

        case MXP_ACTION_STRIKE:
            m_iFlags &= ~STRIKEOUT;
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
            // TODO: Restore previous heading style
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
            // TODO: Pop color state from style stack
            qCDebug(lcMXP) << "End color";
            break;

        // ========== FONT ==========
        case MXP_ACTION_FONT:
            // TODO: Pop font state from style stack
            qCDebug(lcMXP) << "End font";
            break;

        // ========== INTERACTIVE ELEMENTS ==========
        case MXP_ACTION_SEND:
            // Clear send link state
            m_strMXP_link.clear();
            m_strMXP_hint.clear();
            m_bMXP_link_prompt = false;
            m_iFlags &= ~0x0400; // Clear ACTION_SEND flag
            qCDebug(lcMXP) << "End send link";
            break;

        case MXP_ACTION_HYPERLINK:
            // Clear hyperlink state
            m_strMXP_link.clear();
            m_strMXP_hint.clear();
            m_iFlags &= ~0x0800; // Clear ACTION_HYPERLINK flag
            qCDebug(lcMXP) << "End hyperlink";
            break;

        // ========== LISTS ==========
        case MXP_ACTION_UL:
        case MXP_ACTION_OL:
            if (m_iMXP_list_depth > 0) {
                m_iMXP_list_depth--;
            }
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
            // TODO: Update gauge/stat with captured text
            qCDebug(lcMXP) << "End gauge/stat";
            break;

        case MXP_ACTION_EXPIRE:
            // TODO: Set expiration timer for captured text
            qCDebug(lcMXP) << "End expire";
            break;

        case MXP_ACTION_VAR:
            // TODO: Set variable to captured text
            qCDebug(lcMXP) << "End var";
            break;

        // ========== SCRIPT ==========
        case MXP_ACTION_SCRIPT:
            // TODO: Execute captured script code
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
 *
 * TODO: Full implementation
 */
QRgb WorldDocument::MXP_GetColor(const QString& colorSpec)
{
    // Handle hex color: #FF0000
    if (colorSpec.startsWith('#')) {
        bool ok;
        quint32 rgb = colorSpec.mid(1).toUInt(&ok, 16);
        if (ok) {
            return qRgb((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }
    }

    // Named colors (HTML/CSS basic colors)
    QString lower = colorSpec.toLower();

    if (lower == "black")
        return qRgb(0, 0, 0);
    if (lower == "red")
        return qRgb(255, 0, 0);
    if (lower == "green")
        return qRgb(0, 128, 0);
    if (lower == "yellow")
        return qRgb(255, 255, 0);
    if (lower == "blue")
        return qRgb(0, 0, 255);
    if (lower == "magenta" || lower == "fuchsia")
        return qRgb(255, 0, 255);
    if (lower == "cyan" || lower == "aqua")
        return qRgb(0, 255, 255);
    if (lower == "white")
        return qRgb(255, 255, 255);

    // Extended colors
    if (lower == "gray" || lower == "grey")
        return qRgb(128, 128, 128);
    if (lower == "maroon")
        return qRgb(128, 0, 0);
    if (lower == "olive")
        return qRgb(128, 128, 0);
    if (lower == "navy")
        return qRgb(0, 0, 128);
    if (lower == "purple")
        return qRgb(128, 0, 128);
    if (lower == "teal")
        return qRgb(0, 128, 128);
    if (lower == "silver")
        return qRgb(192, 192, 192);
    if (lower == "lime")
        return qRgb(0, 255, 0);
    if (lower == "orange")
        return qRgb(255, 165, 0);

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
void WorldDocument::MXP_CloseOpenTags()
{
    qCDebug(lcMXP) << "Closing" << m_activeTagList.size() << "open tags";

    // Close tags in reverse order (most recent first)
    while (!m_activeTagList.isEmpty()) {
        ActiveTag* pTag = m_activeTagList.takeLast();

        // Execute end action
        if (!pTag->bNoReset) {
            MXP_EndAction(pTag->iAction);
        }

        delete pTag;
    }
}

/**
 * MXP_CloseTag - Close specific tag by name
 *
 * Based on: mxpEnd.cpp - executes the end action for a tag
 *
 * This is called internally when a tag is being closed to execute its end action.
 * Currently just logs the event - actual action reversal happens in MXP_EndAction.
 *
 * @param tagName Name of the tag being closed
 */
void WorldDocument::MXP_CloseTag(const QString& tagName)
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
 * Open mode allows all tags.
 */
bool WorldDocument::MXP_Open() const
{
    return m_iMXP_mode == eMXP_open || m_iMXP_mode == eMXP_perm_open;
}

/**
 * MXP_Secure - Check if MXP is in secure mode
 *
 * Returns true if current MXP mode allows secure tags.
 * Secure and locked modes allow only secure tags.
 */
bool WorldDocument::MXP_Secure() const
{
    return m_iMXP_mode == eMXP_secure || m_iMXP_mode == eMXP_perm_secure ||
           m_iMXP_mode == eMXP_locked || m_iMXP_mode == eMXP_perm_locked ||
           m_iMXP_mode == eMXP_secure_once;
}
