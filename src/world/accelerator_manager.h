/**
 * accelerator_manager.h - Keyboard accelerator (hotkey) management
 *
 * Provides functionality for registering and handling keyboard shortcuts
 * that execute scripts or send commands to the MUD.
 *
 * Based on accelerators.cpp from original MUSHclient.
 */

#ifndef ACCELERATOR_MANAGER_H
#define ACCELERATOR_MANAGER_H

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QVector>

class WorldDocument;
class QShortcut;
class QWidget;

/**
 * Source of an accelerator registration
 */
enum class AcceleratorSource {
    User,   // User-defined via UI (persisted to XML)
    Script, // Registered by world script at runtime
    Plugin  // Registered by a plugin at runtime
};

/**
 * Accelerator entry - stores info about a single keyboard shortcut
 */
struct AcceleratorEntry {
    QString keyString;        // Original key string (e.g., "Ctrl+F5")
    QKeySequence keySeq;      // Qt key sequence
    QString action;           // Script/command to execute
    int sendTo;               // Where to send (sendto constants)
    QString pluginId;         // Plugin that registered this (empty if world script)
    AcceleratorSource source; // Where this accelerator came from
    bool enabled;             // Whether the accelerator is active
    QShortcut* shortcut;      // Qt shortcut object (nullptr if not active)

    AcceleratorEntry()
        : sendTo(0), source(AcceleratorSource::Script), enabled(true), shortcut(nullptr)
    {
    }
};

/**
 * AcceleratorManager - manages keyboard shortcuts for a world
 *
 * Handles:
 * - Parsing key strings like "Ctrl+F5", "Alt+Shift+A", "PageUp"
 * - Registering shortcuts with Qt
 * - Executing actions when shortcuts are triggered
 * - Listing and removing accelerators
 */
class AcceleratorManager : public QObject {
    Q_OBJECT

  public:
    explicit AcceleratorManager(WorldDocument* doc, QObject* parent = nullptr);
    ~AcceleratorManager() override;

    /**
     * Register an accelerator
     *
     * @param keyString Key combination (e.g., "Ctrl+F5", "Alt+A", "PageDown")
     * @param action Script or command to execute
     * @param sendTo Where to send (eSendToExecute, eSendToWorld, etc.)
     * @param pluginId Plugin ID if registered by a plugin (empty otherwise)
     * @return Error code (eOK on success)
     */
    int addAccelerator(const QString& keyString, const QString& action, int sendTo,
                       const QString& pluginId = QString());

    /**
     * Remove an accelerator
     *
     * @param keyString Key combination to remove
     * @return true if removed, false if not found
     */
    bool removeAccelerator(const QString& keyString);

    /**
     * Remove all accelerators for a plugin
     *
     * @param pluginId Plugin ID whose accelerators to remove
     */
    void removePluginAccelerators(const QString& pluginId);

    /**
     * Get list of all accelerators
     *
     * @return List of accelerator entries
     */
    QVector<AcceleratorEntry> acceleratorList() const;

    /**
     * Check if an accelerator exists
     *
     * @param keyString Key combination to check
     * @return true if exists
     */
    bool hasAccelerator(const QString& keyString) const;

    /**
     * Get an accelerator by key string
     *
     * @param keyString Key combination
     * @return Pointer to entry or nullptr if not found
     */
    const AcceleratorEntry* getAccelerator(const QString& keyString) const;

    /**
     * Add a user-defined accelerator (persisted to XML)
     *
     * @param keyString Key combination
     * @param action Script or command to execute
     * @param sendTo Where to send
     * @return Error code (eOK on success)
     */
    int addKeyBinding(const QString& keyString, const QString& action, int sendTo);

    /**
     * Remove a key binding
     *
     * @param keyString Key combination to remove
     * @return true if removed
     */
    bool removeKeyBinding(const QString& keyString);

    /**
     * Get list of key bindings (persisted shortcuts from macros, keypad, accelerators)
     *
     * @return List of key binding entries
     */
    QVector<AcceleratorEntry> keyBindingList() const;

    /**
     * Enable or disable an accelerator
     *
     * @param keyString Key combination
     * @param enabled Whether to enable
     * @return true if found and updated
     */
    bool setAcceleratorEnabled(const QString& keyString, bool enabled);

    /**
     * Find conflicts - keys with multiple bindings
     *
     * @return Map of key strings to list of conflicting entries
     */
    QHash<QString, QVector<AcceleratorEntry>> findConflicts() const;

    /**
     * Set the parent widget for shortcuts
     *
     * Must be called before accelerators will work.
     * Typically call with the WorldWidget.
     */
    void setParentWidget(QWidget* widget);

    /**
     * Parse a key string into a Qt key sequence
     *
     * Supports formats like:
     * - "Ctrl+F5", "Alt+Shift+A", "Ctrl+Alt+Delete"
     * - Function keys: "F1" through "F24"
     * - Navigation: "PageUp", "PageDown", "Home", "End", etc.
     * - Numpad: "Numpad0" through "Numpad9"
     *
     * @param keyString Key string to parse
     * @param outKeySeq Output key sequence
     * @return true on success, false on parse error
     */
    static bool parseKeyString(const QString& keyString, QKeySequence& outKeySeq);

    /**
     * Convert a key sequence back to a key string
     *
     * @param keySeq Key sequence to convert
     * @return Key string representation
     */
    static QString keySequenceToString(const QKeySequence& keySeq);

  signals:
    /**
     * Emitted when an accelerator is triggered
     *
     * @param action The action/script to execute
     * @param sendTo Where to send
     */
    void acceleratorTriggered(const QString& action, int sendTo);

  private slots:
    void onShortcutActivated();

  private:
    void activateShortcut(AcceleratorEntry& entry);
    void deactivateShortcut(AcceleratorEntry& entry);

    WorldDocument* m_doc;
    QWidget* m_parentWidget;

    // Map from normalized key string to accelerator entry
    QHash<QString, AcceleratorEntry> m_accelerators;

    // Static key name mappings
    static QHash<QString, Qt::Key> s_keyNameMap;
    static void initKeyNameMap();
};

#endif // ACCELERATOR_MANAGER_H
