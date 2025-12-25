/**
 * accelerator_manager.cpp - Keyboard accelerator (hotkey) management
 *
 * Based on accelerators.cpp and methods_accelerators.cpp from original MUSHclient.
 */

#include "accelerator_manager.h"
#include "world_document.h"
#include <QShortcut>
#include <QWidget>

// Static key name map
QHash<QString, Qt::Key> AcceleratorManager::s_keyNameMap;

void AcceleratorManager::initKeyNameMap()
{
    if (!s_keyNameMap.isEmpty()) {
        return; // Already initialized
    }

    // Letters A-Z
    for (char c = 'A'; c <= 'Z'; c++) {
        s_keyNameMap[QString(c)] = static_cast<Qt::Key>(Qt::Key_A + (c - 'A'));
    }

    // Numbers 0-9
    for (char c = '0'; c <= '9'; c++) {
        s_keyNameMap[QString(c)] = static_cast<Qt::Key>(Qt::Key_0 + (c - '0'));
    }

    // Function keys F1-F35 (Qt supports up to F35)
    for (int i = 1; i <= 35; i++) {
        s_keyNameMap[QString("F%1").arg(i)] = static_cast<Qt::Key>(Qt::Key_F1 + (i - 1));
    }

    // Numpad keys
    for (int i = 0; i <= 9; i++) {
        s_keyNameMap[QString("Numpad%1").arg(i)] = static_cast<Qt::Key>(Qt::Key_0 + i);
    }

    // Navigation keys
    s_keyNameMap["Home"] = Qt::Key_Home;
    s_keyNameMap["End"] = Qt::Key_End;
    s_keyNameMap["PageUp"] = Qt::Key_PageUp;
    s_keyNameMap["PageDown"] = Qt::Key_PageDown;
    s_keyNameMap["Up"] = Qt::Key_Up;
    s_keyNameMap["Down"] = Qt::Key_Down;
    s_keyNameMap["Left"] = Qt::Key_Left;
    s_keyNameMap["Right"] = Qt::Key_Right;

    // Editing keys
    s_keyNameMap["Insert"] = Qt::Key_Insert;
    s_keyNameMap["Delete"] = Qt::Key_Delete;
    s_keyNameMap["Backspace"] = Qt::Key_Backspace;
    s_keyNameMap["Enter"] = Qt::Key_Return;
    s_keyNameMap["Return"] = Qt::Key_Return;
    s_keyNameMap["Tab"] = Qt::Key_Tab;
    s_keyNameMap["Esc"] = Qt::Key_Escape;
    s_keyNameMap["Escape"] = Qt::Key_Escape;
    s_keyNameMap["Space"] = Qt::Key_Space;

    // Numpad operators
    s_keyNameMap["Add"] = Qt::Key_Plus;
    s_keyNameMap["Subtract"] = Qt::Key_Minus;
    s_keyNameMap["Multiply"] = Qt::Key_Asterisk;
    s_keyNameMap["Divide"] = Qt::Key_Slash;
    s_keyNameMap["Decimal"] = Qt::Key_Period;
    s_keyNameMap["Separator"] = Qt::Key_Comma; // Numpad separator (locale-dependent)

    // Lock keys
    s_keyNameMap["Pause"] = Qt::Key_Pause;
    s_keyNameMap["Print"] = Qt::Key_Print;
    s_keyNameMap["PrintScreen"] = Qt::Key_Print;
    s_keyNameMap["Snapshot"] = Qt::Key_Print; // Windows name for PrintScreen
    s_keyNameMap["ScrollLock"] = Qt::Key_ScrollLock;
    s_keyNameMap["Scroll"] = Qt::Key_ScrollLock; // Original MUSHclient name
    s_keyNameMap["NumLock"] = Qt::Key_NumLock;
    s_keyNameMap["Numlock"] = Qt::Key_NumLock;
    s_keyNameMap["CapsLock"] = Qt::Key_CapsLock;
    s_keyNameMap["Capital"] = Qt::Key_CapsLock; // Original MUSHclient name

    // Special/system keys
    s_keyNameMap["Help"] = Qt::Key_Help;
    s_keyNameMap["Clear"] = Qt::Key_Clear;
    s_keyNameMap["Menu"] = Qt::Key_Menu;
    s_keyNameMap["Apps"] = Qt::Key_Menu;
    s_keyNameMap["Select"] = Qt::Key_Select;
    s_keyNameMap["Execute"] = Qt::Key_Execute;
    s_keyNameMap["Play"] = Qt::Key_Play;
    s_keyNameMap["Zoom"] = Qt::Key_Zoom;
    s_keyNameMap["Cancel"] = Qt::Key_Cancel;
    s_keyNameMap["Sleep"] = Qt::Key_Sleep;

    // Left/Right modifier variants (for compatibility)
    s_keyNameMap["LShift"] = Qt::Key_Shift;
    s_keyNameMap["RShift"] = Qt::Key_Shift;
    s_keyNameMap["LControl"] = Qt::Key_Control;
    s_keyNameMap["RControl"] = Qt::Key_Control;
    s_keyNameMap["LMenu"] = Qt::Key_Alt; // Windows "Menu" = Alt
    s_keyNameMap["RMenu"] = Qt::Key_Alt;
    s_keyNameMap["LWin"] = Qt::Key_Meta;
    s_keyNameMap["RWin"] = Qt::Key_Meta;
    s_keyNameMap["Control"] = Qt::Key_Control;
    s_keyNameMap["Shift"] = Qt::Key_Shift;

    // Asian input method keys (IME)
    s_keyNameMap["Kana"] = Qt::Key_Kana_Lock;
    s_keyNameMap["Hangul"] = Qt::Key_Hangul;
    s_keyNameMap["Hangeul"] = Qt::Key_Hangul; // Alternate spelling
    s_keyNameMap["Hanja"] = Qt::Key_Hangul_Hanja;
    s_keyNameMap["Junja"] = Qt::Key_Hangul_Jeonja;
    s_keyNameMap["Kanji"] = Qt::Key_Kanji;
    s_keyNameMap["Convert"] = Qt::Key_Henkan;      // IME convert
    s_keyNameMap["NonConvert"] = Qt::Key_Muhenkan; // IME non-convert
    s_keyNameMap["Final"] = Qt::Key_Hangul_End;
    s_keyNameMap["ModeChange"] = Qt::Key_Mode_switch;
    s_keyNameMap["Accept"] = Qt::Key_Yes; // Closest Qt equivalent
    s_keyNameMap["ProcessKey"] = Qt::Key_Multi_key;

    // Punctuation - original MUSHclient names
    s_keyNameMap[";"] = Qt::Key_Semicolon;
    s_keyNameMap["Plus"] = Qt::Key_Plus;
    s_keyNameMap[","] = Qt::Key_Comma;
    s_keyNameMap["-"] = Qt::Key_Minus;
    s_keyNameMap["."] = Qt::Key_Period;
    s_keyNameMap["/"] = Qt::Key_Slash;
    s_keyNameMap["`"] = Qt::Key_QuoteLeft;
    s_keyNameMap["["] = Qt::Key_BracketLeft;
    s_keyNameMap["\\"] = Qt::Key_Backslash;
    s_keyNameMap["]"] = Qt::Key_BracketRight;
    s_keyNameMap["'"] = Qt::Key_Apostrophe;

    // Punctuation - descriptive names
    s_keyNameMap["Comma"] = Qt::Key_Comma;
    s_keyNameMap["Period"] = Qt::Key_Period;
    s_keyNameMap["Semicolon"] = Qt::Key_Semicolon;
    s_keyNameMap["Quote"] = Qt::Key_Apostrophe;
    s_keyNameMap["BracketLeft"] = Qt::Key_BracketLeft;
    s_keyNameMap["BracketRight"] = Qt::Key_BracketRight;
    s_keyNameMap["Backslash"] = Qt::Key_Backslash;
    s_keyNameMap["Slash"] = Qt::Key_Slash;
    s_keyNameMap["Minus"] = Qt::Key_Minus;
    s_keyNameMap["Equal"] = Qt::Key_Equal;
    s_keyNameMap["Grave"] = Qt::Key_QuoteLeft;
    s_keyNameMap["Tilde"] = Qt::Key_AsciiTilde;

    // Media keys (bonus - not in original but useful)
    s_keyNameMap["MediaPlay"] = Qt::Key_MediaPlay;
    s_keyNameMap["MediaStop"] = Qt::Key_MediaStop;
    s_keyNameMap["MediaPause"] = Qt::Key_MediaPause;
    s_keyNameMap["MediaNext"] = Qt::Key_MediaNext;
    s_keyNameMap["MediaPrevious"] = Qt::Key_MediaPrevious;
    s_keyNameMap["VolumeUp"] = Qt::Key_VolumeUp;
    s_keyNameMap["VolumeDown"] = Qt::Key_VolumeDown;
    s_keyNameMap["VolumeMute"] = Qt::Key_VolumeMute;
}

AcceleratorManager::AcceleratorManager(WorldDocument* doc, QObject* parent)
    : QObject(parent), m_doc(doc), m_parentWidget(nullptr)
{
    initKeyNameMap();
}

AcceleratorManager::~AcceleratorManager()
{
    // Clean up all shortcuts
    for (auto& entry : m_accelerators) {
        deactivateShortcut(entry);
    }
}

bool AcceleratorManager::parseKeyString(const QString& keyString, QKeySequence& outKeySeq)
{
    initKeyNameMap();

    // Split by '+' to get modifiers and key
    QStringList parts = keyString.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }

    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::Key key = Qt::Key_unknown;

    for (const QString& part : parts) {
        QString trimmed = part.trimmed();
        QString upper = trimmed.toUpper();

        // Check for modifiers
        if (upper == "CTRL" || upper == "CONTROL") {
            if (modifiers & Qt::ControlModifier) {
                return false; // Duplicate modifier
            }
            modifiers |= Qt::ControlModifier;
        } else if (upper == "ALT") {
            if (modifiers & Qt::AltModifier) {
                return false; // Duplicate modifier
            }
            modifiers |= Qt::AltModifier;
        } else if (upper == "SHIFT") {
            if (modifiers & Qt::ShiftModifier) {
                return false; // Duplicate modifier
            }
            modifiers |= Qt::ShiftModifier;
        } else if (upper == "META" || upper == "WIN" || upper == "CMD") {
            if (modifiers & Qt::MetaModifier) {
                return false; // Duplicate modifier
            }
            modifiers |= Qt::MetaModifier;
        } else {
            // Must be the key itself
            if (key != Qt::Key_unknown) {
                return false; // Already have a key
            }

            // Try case-insensitive lookup (build search key)
            QString searchKey;
            for (auto it = s_keyNameMap.constBegin(); it != s_keyNameMap.constEnd(); ++it) {
                if (it.key().compare(trimmed, Qt::CaseInsensitive) == 0) {
                    key = it.value();
                    break;
                }
            }

            if (key == Qt::Key_unknown) {
                // Try single character
                if (trimmed.length() == 1) {
                    QChar ch = trimmed[0].toUpper();
                    if (ch.isLetter()) {
                        key = static_cast<Qt::Key>(Qt::Key_A + (ch.unicode() - 'A'));
                    } else if (ch.isDigit()) {
                        key = static_cast<Qt::Key>(Qt::Key_0 + (ch.unicode() - '0'));
                    }
                }
            }

            if (key == Qt::Key_unknown) {
                return false; // Unknown key
            }
        }
    }

    if (key == Qt::Key_unknown) {
        return false; // No key specified
    }

    // Create key sequence
    outKeySeq = QKeySequence(static_cast<int>(key) | static_cast<int>(modifiers));
    return true;
}

QString AcceleratorManager::keySequenceToString(const QKeySequence& keySeq)
{
    // Qt provides a portable string representation
    return keySeq.toString(QKeySequence::PortableText);
}

int AcceleratorManager::addAccelerator(const QString& keyString, const QString& action, int sendTo,
                                       const QString& pluginId)
{
    // Parse the key string
    QKeySequence keySeq;
    if (!parseKeyString(keyString, keySeq)) {
        return 30001; // eBadParameter - invalid key string
    }

    // Normalize key string for storage
    QString normalizedKey = keyString.toUpper().simplified();

    // Remove existing accelerator with same key
    if (m_accelerators.contains(normalizedKey)) {
        deactivateShortcut(m_accelerators[normalizedKey]);
    }

    // If action is empty, just remove the accelerator
    if (action.isEmpty()) {
        m_accelerators.remove(normalizedKey);
        return 0; // eOK
    }

    // Create new entry
    AcceleratorEntry entry;
    entry.keyString = keyString;
    entry.keySeq = keySeq;
    entry.action = action;
    entry.sendTo = sendTo;
    entry.pluginId = pluginId;
    entry.source = pluginId.isEmpty() ? AcceleratorSource::Script : AcceleratorSource::Plugin;
    entry.enabled = true;
    entry.shortcut = nullptr;

    // Store it
    m_accelerators[normalizedKey] = entry;

    // Activate if we have a parent widget
    if (m_parentWidget) {
        activateShortcut(m_accelerators[normalizedKey]);
    }

    return 0; // eOK
}

bool AcceleratorManager::removeAccelerator(const QString& keyString)
{
    QString normalizedKey = keyString.toUpper().simplified();

    if (!m_accelerators.contains(normalizedKey)) {
        return false;
    }

    deactivateShortcut(m_accelerators[normalizedKey]);
    m_accelerators.remove(normalizedKey);
    return true;
}

void AcceleratorManager::removePluginAccelerators(const QString& pluginId)
{
    QList<QString> toRemove;

    for (auto it = m_accelerators.constBegin(); it != m_accelerators.constEnd(); ++it) {
        if (it.value().pluginId == pluginId) {
            toRemove.append(it.key());
        }
    }

    for (const QString& key : toRemove) {
        deactivateShortcut(m_accelerators[key]);
        m_accelerators.remove(key);
    }
}

QVector<AcceleratorEntry> AcceleratorManager::acceleratorList() const
{
    QVector<AcceleratorEntry> list;
    list.reserve(m_accelerators.size());

    for (const auto& entry : m_accelerators) {
        list.append(entry);
    }

    return list;
}

bool AcceleratorManager::hasAccelerator(const QString& keyString) const
{
    QString normalizedKey = keyString.toUpper().simplified();
    return m_accelerators.contains(normalizedKey);
}

const AcceleratorEntry* AcceleratorManager::getAccelerator(const QString& keyString) const
{
    QString normalizedKey = keyString.toUpper().simplified();
    auto it = m_accelerators.constFind(normalizedKey);
    if (it != m_accelerators.constEnd()) {
        return &it.value();
    }
    return nullptr;
}

int AcceleratorManager::addKeyBinding(const QString& keyString, const QString& action, int sendTo)
{
    // Parse the key string
    QKeySequence keySeq;
    if (!parseKeyString(keyString, keySeq)) {
        return 30001; // eBadParameter - invalid key string
    }

    // Normalize key string for storage
    QString normalizedKey = keyString.toUpper().simplified();

    // Remove existing accelerator with same key
    if (m_accelerators.contains(normalizedKey)) {
        deactivateShortcut(m_accelerators[normalizedKey]);
    }

    // If action is empty, just remove the accelerator
    if (action.isEmpty()) {
        m_accelerators.remove(normalizedKey);
        return 0; // eOK
    }

    // Create new entry
    AcceleratorEntry entry;
    entry.keyString = keyString;
    entry.keySeq = keySeq;
    entry.action = action;
    entry.sendTo = sendTo;
    entry.pluginId = QString(); // Key bindings have no plugin
    entry.source = AcceleratorSource::User;
    entry.enabled = true;
    entry.shortcut = nullptr;

    // Store it
    m_accelerators[normalizedKey] = entry;

    // Activate if we have a parent widget
    if (m_parentWidget) {
        activateShortcut(m_accelerators[normalizedKey]);
    }

    return 0; // eOK
}

bool AcceleratorManager::removeKeyBinding(const QString& keyString)
{
    QString normalizedKey = keyString.toUpper().simplified();

    if (!m_accelerators.contains(normalizedKey)) {
        return false;
    }

    // Only remove if it's a key binding (User source)
    if (m_accelerators[normalizedKey].source != AcceleratorSource::User) {
        return false;
    }

    deactivateShortcut(m_accelerators[normalizedKey]);
    m_accelerators.remove(normalizedKey);
    return true;
}

QVector<AcceleratorEntry> AcceleratorManager::keyBindingList() const
{
    QVector<AcceleratorEntry> list;

    for (const auto& entry : m_accelerators) {
        if (entry.source == AcceleratorSource::User) {
            list.append(entry);
        }
    }

    return list;
}

bool AcceleratorManager::setAcceleratorEnabled(const QString& keyString, bool enabled)
{
    QString normalizedKey = keyString.toUpper().simplified();

    if (!m_accelerators.contains(normalizedKey)) {
        return false;
    }

    AcceleratorEntry& entry = m_accelerators[normalizedKey];
    if (entry.enabled == enabled) {
        return true; // Already in desired state
    }

    entry.enabled = enabled;

    if (enabled) {
        // Activate if we have a parent widget
        if (m_parentWidget) {
            activateShortcut(entry);
        }
    } else {
        // Deactivate
        deactivateShortcut(entry);
    }

    return true;
}

QHash<QString, QVector<AcceleratorEntry>> AcceleratorManager::findConflicts() const
{
    // Group accelerators by their key sequence (normalized)
    QHash<QString, QVector<AcceleratorEntry>> conflicts;

    for (const auto& entry : m_accelerators) {
        QString keySeqStr = entry.keySeq.toString(QKeySequence::PortableText);
        conflicts[keySeqStr].append(entry);
    }

    // Remove non-conflicts (entries with only one binding)
    QList<QString> toRemove;
    for (auto it = conflicts.constBegin(); it != conflicts.constEnd(); ++it) {
        if (it.value().size() <= 1) {
            toRemove.append(it.key());
        }
    }
    for (const QString& key : toRemove) {
        conflicts.remove(key);
    }

    return conflicts;
}

void AcceleratorManager::setParentWidget(QWidget* widget)
{
    // Deactivate all existing shortcuts
    for (auto& entry : m_accelerators) {
        deactivateShortcut(entry);
    }

    m_parentWidget = widget;

    // Reactivate with new parent
    if (m_parentWidget) {
        for (auto& entry : m_accelerators) {
            activateShortcut(entry);
        }
    }
}

void AcceleratorManager::activateShortcut(AcceleratorEntry& entry)
{
    if (!m_parentWidget || entry.shortcut || !entry.enabled) {
        return;
    }

    entry.shortcut = new QShortcut(entry.keySeq, m_parentWidget);
    entry.shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Store the key string in the shortcut for retrieval in slot
    entry.shortcut->setProperty("keyString", entry.keyString.toUpper().simplified());

    connect(entry.shortcut, &QShortcut::activated, this, &AcceleratorManager::onShortcutActivated);
}

void AcceleratorManager::deactivateShortcut(AcceleratorEntry& entry)
{
    if (entry.shortcut) {
        delete entry.shortcut;
        entry.shortcut = nullptr;
    }
}

void AcceleratorManager::onShortcutActivated()
{
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    if (!shortcut) {
        return;
    }

    QString keyString = shortcut->property("keyString").toString();
    if (!m_accelerators.contains(keyString)) {
        return;
    }

    const AcceleratorEntry& entry = m_accelerators[keyString];
    emit acceleratorTriggered(entry.action, entry.sendTo);
}
