/**
 * world_notepad.cpp - WorldDocument notepad management methods
 *
 * Implements notepad window tracking and lifecycle management for WorldDocument.
 * Notepads are MDI child windows that display text (logs, stats, help, etc.)
 */

#include "lua_api/lua_common.h"
#include "notepad_widget.h"
#include "world_document.h"
#include <QColor>
#include <QMdiSubWindow>
#include <QString>

/**
 * RegisterNotepad - Register a notepad window with this world
 *
 * Called by NotepadWidget constructor to add itself to the world's notepad list.
 * This allows the world to track all its notepads and find them by title.
 *
 * @param notepad Pointer to notepad widget to register (must not be nullptr)
 */
void WorldDocument::RegisterNotepad(NotepadWidget* notepad)
{
    if (!notepad) {
        return;
    }

    // Add to list if not already present
    if (!m_notepadList.contains(notepad)) {
        m_notepadList.append(notepad);
    }
}

/**
 * UnregisterNotepad - Unregister a notepad window from this world
 *
 * Called by NotepadWidget destructor to remove itself from the world's notepad list.
 * This prevents dangling pointers when notepads are closed.
 *
 * @param notepad Pointer to notepad widget to unregister
 */
void WorldDocument::UnregisterNotepad(NotepadWidget* notepad)
{
    if (!notepad) {
        return;
    }

    // Remove from list
    m_notepadList.removeAll(notepad);
}

/**
 * FindNotepad - Find a notepad by title (case-insensitive)
 *
 * Searches through all registered notepads for one with a matching title.
 * This is used by Lua API functions like AppendToNotepad to find the target window.
 *
 * @param title Window title to search for (case-insensitive)
 * @return Pointer to notepad if found, nullptr otherwise
 */
NotepadWidget* WorldDocument::FindNotepad(const QString& title)
{
    // Search for notepad with matching title (case-insensitive)
    for (NotepadWidget* notepad : m_notepadList) {
        if (notepad && notepad->m_strTitle.compare(title, Qt::CaseInsensitive) == 0) {
            return notepad;
        }
    }

    // Not found
    return nullptr;
}

/**
 * CreateNotepadWindow - Create a new notepad window
 *
 * Creates a new NotepadWidget with the specified title and contents.
 * The widget will be registered with this world and a signal will be emitted
 * so the UI layer can wrap it in an MDI subwindow.
 *
 * @param title Window title
 * @param contents Initial text contents
 * @return Pointer to newly created notepad widget
 */
NotepadWidget* WorldDocument::CreateNotepadWindow(const QString& title, const QString& contents)
{
    // Create notepad widget
    // Note: The widget's parent is set to nullptr here - the MDI subwindow will
    // be the actual parent after the UI layer wraps it
    NotepadWidget* notepad = new NotepadWidget(this, title, contents, nullptr);

    // Emit signal so UI layer can wrap in MDI subwindow
    emit notepadCreated(notepad);

    return notepad;
}

// ========== Notepad Operations (Lua API) ==========

/**
 * SendToNotepad - Create or replace notepad contents
 *
 * If notepad exists, replaces all text. If not, creates new notepad.
 *
 * @param title Notepad window title
 * @param contents New text content
 * @return true on success
 */
bool WorldDocument::SendToNotepad(const QString& title, const QString& contents)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (notepad) {
        // Notepad exists - replace contents
        notepad->ReplaceText(contents);
    } else {
        // Create new notepad
        notepad = CreateNotepadWindow(title, contents);
    }

    return (notepad != nullptr);
}

/**
 * AppendToNotepad - Append text to notepad
 *
 * If notepad doesn't exist, creates it with the text.
 *
 * @param title Notepad window title
 * @param contents Text to append
 * @return true on success
 */
bool WorldDocument::AppendToNotepad(const QString& title, const QString& contents)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (notepad) {
        // Append to existing notepad
        notepad->AppendText(contents);
    } else {
        // Create new notepad
        notepad = CreateNotepadWindow(title, contents);
    }

    return (notepad != nullptr);
}

/**
 * ReplaceNotepad - Replace notepad contents
 *
 * Only works if notepad already exists.
 *
 * @param title Notepad window title
 * @param contents New text content
 * @return true if notepad found and replaced
 */
bool WorldDocument::ReplaceNotepad(const QString& title, const QString& contents)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return false;
    }

    notepad->ReplaceText(contents);
    return true;
}

/**
 * ActivateNotepad - Bring notepad window to front
 *
 * @param title Notepad window title
 * @return true if notepad found
 */
bool WorldDocument::ActivateNotepad(const QString& title)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad || !notepad->m_pMdiSubWindow) {
        return false;
    }

    // Activate the MDI subwindow
    notepad->m_pMdiSubWindow->raise();
    notepad->m_pMdiSubWindow->setFocus();

    return true;
}

/**
 * CloseNotepad - Close notepad window
 *
 * @param title Notepad window title
 * @param querySave If true, prompt user to save changes
 * @return eOK on success, eNoSuchNotepad if not found
 */
qint32 WorldDocument::CloseNotepad(const QString& title, bool querySave)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    // For now, just close without save prompt
    // TODO: Implement save prompt if querySave is true and content modified
    if (notepad->m_pMdiSubWindow) {
        notepad->m_pMdiSubWindow->close();
    }

    return eOK;
}

/**
 * SaveNotepad - Save notepad contents to file
 *
 * @param title Notepad window title
 * @param filename File path to save to
 * @param replaceExisting If false, fail if file exists
 * @return eOK on success, error code otherwise
 */
qint32 WorldDocument::SaveNotepad(const QString& title, const QString& filename,
                                  bool replaceExisting)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    bool success = notepad->SaveToFile(filename, replaceExisting);
    return success ? eOK : eFileNotOpened;
}

/**
 * GetNotepadList - Get list of notepad titles
 *
 * @param includeAllWorlds If true, include all worlds' notepads (not implemented yet)
 * @return List of notepad window titles
 */
QStringList WorldDocument::GetNotepadList(bool includeAllWorlds)
{
    QStringList titles;

    // For now, only return this world's notepads
    // TODO: If includeAllWorlds is true, get notepads from all worlds
    for (NotepadWidget* notepad : m_notepadList) {
        if (notepad) {
            titles.append(notepad->m_strTitle);
        }
    }

    return titles;
}

/**
 * NotepadFont - Set notepad font
 *
 * @param title Notepad window title
 * @param name Font name
 * @param size Font size in points
 * @param style Style flags (1=bold, 2=italic, 4=underline, 8=strikeout)
 * @param charset Character set (Windows charset)
 * @return eOK on success, eNoSuchNotepad if not found
 */
qint32 WorldDocument::NotepadFont(const QString& title, const QString& name, qint32 size,
                                  qint32 style, qint32 charset)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    notepad->SetFont(name, size, style, charset);
    return eOK;
}

/**
 * NotepadColour - Set notepad text and background colors
 *
 * @param title Notepad window title
 * @param textColour Text color (color name or #RRGGBB)
 * @param backColour Background color (color name or #RRGGBB)
 * @return eOK on success, eNoSuchNotepad if not found
 */
qint32 WorldDocument::NotepadColour(const QString& title, const QString& textColour,
                                    const QString& backColour)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    // Parse colors - QColor handles both names and #RRGGBB format
    QColor textColor(textColour);
    QColor backColor(backColour);

    if (!textColor.isValid() || !backColor.isValid()) {
        return eInvalidColourName;
    }

    notepad->SetColours(textColor.rgb(), backColor.rgb());
    return eOK;
}

/**
 * NotepadReadOnly - Set notepad read-only mode
 *
 * @param title Notepad window title
 * @param readOnly True to make read-only, false to make editable
 * @return eOK on success, eNoSuchNotepad if not found
 */
qint32 WorldDocument::NotepadReadOnly(const QString& title, bool readOnly)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    notepad->SetReadOnly(readOnly);
    return eOK;
}

/**
 * NotepadSaveMethod - Set notepad auto-save method
 *
 * @param title Notepad window title
 * @param method Save method (0=ask, 1=always, 2=never)
 * @return eOK on success, eNoSuchNotepad if not found
 */
qint32 WorldDocument::NotepadSaveMethod(const QString& title, qint32 method)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad) {
        return eNoSuchNotepad;
    }

    notepad->m_iSaveOnChange = method;
    return eOK;
}

/**
 * MoveNotepadWindow - Move and resize notepad window
 *
 * @param title Notepad window title
 * @param left Left position
 * @param top Top position
 * @param width Width
 * @param height Height
 * @return true if notepad found and moved
 */
bool WorldDocument::MoveNotepadWindow(const QString& title, qint32 left, qint32 top, qint32 width,
                                      qint32 height)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad || !notepad->m_pMdiSubWindow) {
        return false;
    }

    // Set MDI window geometry
    notepad->m_pMdiSubWindow->setGeometry(left, top, width, height);

    return true;
}

/**
 * GetNotepadWindowPosition - Get notepad window position
 *
 * Returns position as "left,top,width,height" string (matching MUSHclient format)
 *
 * @param title Notepad window title
 * @return Position string, or empty string if not found
 */
QString WorldDocument::GetNotepadWindowPosition(const QString& title)
{
    NotepadWidget* notepad = FindNotepad(title);

    if (!notepad || !notepad->m_pMdiSubWindow) {
        return QString();
    }

    // Get MDI window geometry
    QRect geometry = notepad->m_pMdiSubWindow->geometry();

    // Format as "left,top,width,height" (matching MUSHclient)
    return QString("%1,%2,%3,%4")
        .arg(geometry.left())
        .arg(geometry.top())
        .arg(geometry.width())
        .arg(geometry.height());
}
