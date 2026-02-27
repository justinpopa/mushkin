#pragma once

// mxp_engine.h
// MXPEngine — Companion object that owns all MXP protocol state and methods.
// WorldDocument creates and owns this via std::unique_ptr<MXPEngine>.
//
// This follows the same pattern as TelnetParser extraction.
// MXPEngine holds a WorldDocument& back-reference and accesses WorldDocument
// members (colors, style flags, line buffer) through it.

#include "mxp_types.h"
#include <QColor>
#include <QList>
#include <QMap>
#include <QString>
#include <cstdint>

class WorldDocument; // forward declaration

// ========== MXP Line Security Mode Constants ==========
// Converted from #define (world_document.h) to inline constexpr.
// IMPORTANT: These values must match the ANSI escape code values sent by servers.
inline constexpr int MXP_MODE_OPEN = 0;        // Open mode (all tags allowed)
inline constexpr int MXP_MODE_SECURE = 1;      // Secure mode (limited tags)
inline constexpr int MXP_MODE_LOCKED = 2;      // Locked mode (no tags)
inline constexpr int MXP_MODE_RESET = 3;       // MXP reset (close all open tags)
inline constexpr int MXP_MODE_SECURE_ONCE = 4; // Next tag is secure only
inline constexpr int MXP_MODE_PERM_OPEN = 5;   // Permanent open mode
inline constexpr int MXP_MODE_PERM_SECURE = 6; // Permanent secure mode
inline constexpr int MXP_MODE_PERM_LOCKED = 7; // Permanent locked mode
// Room and welcome modes (10-19)
inline constexpr int MXP_MODE_ROOM_NAME = 10;        // Line is room name
inline constexpr int MXP_MODE_ROOM_DESCRIPTION = 11; // Line is room description
inline constexpr int MXP_MODE_ROOM_EXITS = 12;       // Line is room exits
inline constexpr int MXP_MODE_WELCOME = 19;          // Welcome text

// ========== MXPEngine Class ==========
class MXPEngine {
  public:
    explicit MXPEngine(WorldDocument& doc);
    ~MXPEngine();

    // Non-copyable, non-movable
    MXPEngine(const MXPEngine&) = delete;
    MXPEngine& operator=(const MXPEngine&) = delete;
    MXPEngine(MXPEngine&&) = delete;
    MXPEngine& operator=(MXPEngine&&) = delete;

    // ========== Lifecycle ==========
    void MXP_On();                    // Activate MXP
    void MXP_Off(bool force = false); // Deactivate or reset MXP
    void MXP_mode_change(int mode);   // Change MXP line security mode

    // ========== Initialization / Cleanup ==========
    void InitializeMXPElements(); // Load built-in atomic MXP elements
    void InitializeMXPEntities(); // Load standard HTML entities
    void CleanupMXP();            // Free all MXP heap resources

    // ========== Phase Handlers (called from WorldDocument::ProcessIncomingByte) ==========
    void Phase_MXP_ELEMENT(unsigned char c);
    void Phase_MXP_COMMENT(unsigned char c);
    void Phase_MXP_QUOTE(unsigned char c);
    void Phase_MXP_ENTITY(unsigned char c);

    // ========== Element/Entity Collection ==========
    void MXP_collected_element(); // Process completed <element>
    void MXP_collected_entity();  // Process completed &entity;

    // ========== Tag Processing ==========
    void MXP_StartTag(const QString& tagString);   // Process opening tag
    void MXP_EndTag(const QString& tagName);       // Process closing tag
    void MXP_Definition(const QString& defString); // Process <!ELEMENT> or <!ENTITY>

    // ========== Element/Entity Definition ==========
    void MXP_DefineElement(const QString& defString); // Define custom element
    void MXP_DefineEntity(const QString& defString);  // Define custom entity

    // ========== Lookup ==========
    AtomicElement* MXP_FindAtomicElement(const QString& name); // Look up built-in element
    CustomElement* MXP_FindCustomElement(const QString& name); // Look up custom element
    QString MXP_GetEntity(const QString& entityName);          // Resolve &entity; to text

    // ========== Action Execution ==========
    void MXP_ExecuteAction(AtomicElement* elem, MXPArgumentList& args); // Execute element action
    void MXP_EndAction(int action);                                     // Reverse element action
    QRgb MXP_GetColor(const QString& colorSpec); // Resolve color name/#RRGGBB

    // ========== Tag Stack ==========
    void MXP_CloseOpenTags();                  // Close all unclosed tags
    void MXP_CloseTag(const QString& tagName); // Close specific tag (internal)

    // ========== Mode Helpers ==========
    bool MXP_Open() const;   // True if current mode allows open (unsecure) tags
    bool MXP_Secure() const; // True if current mode allows secure tags

    // ========== Argument Parsing ==========
    void ParseMXPTag(const QString& tagString, QString& tagName,
                     MXPArgumentList& args); // Parse tag string into name + args
    QString GetMXPArgument(MXPArgumentList& args,
                           const QString& name); // Get arg value by name (marks as used)
    QString MXP_GetArgument(const QString& name,
                            MXPArgumentList& args); // Convenience wrapper (swapped params)
    bool MXP_HasArgument(const QString& name,
                         MXPArgumentList& args); // Check if argument exists (marks as used)

    // ========== State Accessors ==========
    bool isActive() const
    {
        return m_bMXP;
    }
    bool isPuebloActive() const
    {
        return m_bPuebloActive;
    }
    qint64 mxpErrors() const
    {
        return m_iMXPerrors;
    }
    qint64 mxpTags() const
    {
        return m_iMXPtags;
    }
    qint64 mxpEntities() const
    {
        return m_iMXPentities;
    }

    // ========== Runtime State (MXP engine state, not user config) ==========
    // These are direct-access public members matching the original WorldDocument layout.
    // The Lua API and WorldDocument forwarding wrappers read/write these directly.

    bool m_bMXP = false;             // Using MXP now?
    bool m_bPuebloActive = false;    // Pueblo mode active
    QString m_iPuebloLevel;          // e.g., "1.0", "1.10"
    bool m_bPreMode = false;         // <PRE> tag active
    qint32 m_iMXP_mode = 0;          // Current tag security mode
    qint32 m_iMXP_defaultMode = 0;   // Default mode after newline
    qint32 m_iMXP_previousMode = 0;  // Previous mode (for secure_once)
    bool m_bInParagraph = false;     // Discard newlines (wrap)
    bool m_bMXP_script = false;      // In script collection
    bool m_bSuppressNewline = false; // Newline doesn't start new line

    // MXP formatting state
    bool m_bMXP_nobr = false;         // <nobr> — non-breaking mode active
    bool m_bMXP_preformatted = false; // <pre> — preformatted text mode
    bool m_bMXP_centered = false;     // <center> — centered text mode
    QString m_strMXP_link;            // Current <send> or <a> href
    QString m_strMXP_hint;            // Current link hint/tooltip
    bool m_bMXP_link_prompt = false;  // <send prompt> — show prompt before sending
    qint32 m_iMXP_list_depth = 0;     // Current list nesting depth
    qint32 m_iMXP_list_counter = 0;   // Counter for ordered lists
    qint32 m_iListMode = 0;           // What sort of list displaying
    qint32 m_iListCount = 0;          // For ordered list

    // Parsing buffers
    QString m_strMXPstring;         // Currently collecting MXP string
    QString m_strMXPtagcontents;    // Stuff inside tag
    char m_cMXPquoteTerminator = 0; // ' or "

    // MXP data structures (raw pointer maps — C library interop pattern, freed via qDeleteAll)
    AtomicElementMap m_atomicElementMap; // Built-in MXP elements
    CustomElementMap m_customElementMap; // User-defined elements
    MXPEntityMap m_entityMap;            // Standard HTML entities
    MXPEntityMap m_customEntityMap;      // User-defined entities
    ActiveTagList m_activeTagList;       // Stack of unclosed tags
    MXPGaugeMap m_gaugeMap;              // Track gauges and stats by entity name

    // Statistics
    QString m_strPuebloMD5; // Pueblo hash string
    qint64 m_iMXPerrors = 0;
    qint64 m_iMXPtags = 0;
    qint64 m_iMXPentities = 0;

    // Script handler DISPIDs (Lua flags for MXP callbacks)
    qint32 m_dispidOnMXP_Start = 0;
    qint32 m_dispidOnMXP_Stop = 0;
    qint32 m_dispidOnMXP_OpenTag = 0;
    qint32 m_dispidOnMXP_CloseTag = 0;
    qint32 m_dispidOnMXP_SetVariable = 0;
    qint32 m_dispidOnMXP_Error = 0;

  private:
    WorldDocument& m_doc; // back-reference (non-owning)
};
