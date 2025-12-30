#ifndef INPUT_VIEW_H
#define INPUT_VIEW_H

#include <QPlainTextEdit>
#include <QString>

#include "world/view_interfaces.h"

// Forward declarations
class WorldDocument;

/**
 * InputView - Custom multi-line input widget with command history navigation
 *
 * Command History & Input Enhancements
 * Command Input Widget Enhancements
 *
 * This widget extends QPlainTextEdit to provide:
 * - Command history recall via Up/Down arrow keys
 * - Auto-resize based on content (matching original MUSHclient)
 * - Tab completion
 * - Configurable font/colors
 *
 * Based on CSendView from sendvw.h/sendvw.cpp in original MUSHclient.
 *
 * Key features:
 * - Up arrow: Recall previous command from history
 * - Down arrow: Recall next command (or restore what you were typing)
 * - Enter: Send command (not insert newline)
 * - Shift+Enter: Insert newline
 * - Saves partial command when starting to browse history
 * - Restores partial command when you press Down past the end
 * - Ctrl+L: Clear input field
 * - Escape: Clear input (if m_bEscapeDeletesInput enabled)
 * - Configurable font/colors from WorldDocument settings
 * - Auto-resize based on line count (1-100 lines)
 * - Signal emission for plugin notifications
 * - Configurable clear-after-send behavior
 */
class InputView : public QPlainTextEdit, public IInputView {
    Q_OBJECT

  public:
    explicit InputView(WorldDocument* doc, QWidget* parent = nullptr);
    ~InputView() override;

    // ========== IInputView Interface Implementation ==========
    QString inputText() const override { return toPlainText(); }
    void setInputText(const QString& text) override { setPlainText(text); }
    int cursorPosition() const override { return textCursor().position(); }
    void setCursorPosition(int pos) override;
    void setSelection(int start, int length) override;
    void selectAll() override { QPlainTextEdit::selectAll(); }
    void clearInput() override { clear(); }

    // ========== QLineEdit Compatibility Methods ==========
    /**
     * text - Get the current text (compatibility with QLineEdit API)
     */
    QString text() const { return toPlainText(); }

    /**
     * setText - Set the current text (compatibility with QLineEdit API)
     */
    void setText(const QString& text) { setPlainText(text); }

    /**
     * hasSelectedText - Check if text is selected (compatibility with QLineEdit API)
     */
    bool hasSelectedText() const
    {
        return textCursor().hasSelection();
    }

    /**
     * selectedText - Get selected text (compatibility with QLineEdit API)
     */
    QString selectedText() const
    {
        return textCursor().selectedText();
    }

    /**
     * applyInputSettings - Apply font and color settings from document
     *
     * Configurable font/colors
     *
     * Updates widget appearance based on document settings:
     * - m_input_font_name, m_input_font_height
     * - m_input_text_colour, m_input_background_colour
     */
    void applyInputSettings();

    /**
     * handleTabCompletion - Tab completion for commands (Tab key)
     *
     * Tab Completion (IDENTICAL to original MUSHclient)
     *
     * Searches OUTPUT BUFFER (recent MUD lines) and default list for words
     * starting with the current partial word under cursor.
     *
     * Based on CSendView::OnKeysTab() from sendvw.cpp
     *
     * NOTE: Made public for testing purposes.
     */
    void handleTabCompletion();

    /**
     * handleShiftTabCompletion - Show function/word completion dialog (Shift+Tab)
     *
     * Shows CompleteWordDialog with Lua functions and extra items
     * added via ShiftTabCompleteItem().
     *
     * Based on CSendView::OnKeysTab() Shift+Tab handling from sendvw.cpp
     */
    void handleShiftTabCompletion();

    /**
     * Public command history navigation methods
     * (for menu item access)
     */
    void previousCommand()
    {
        recallPreviousCommand();
    }
    void nextCommand()
    {
        recallNextCommand();
    }

  protected:
    /**
     * keyPressEvent - Handle keyboard input
     *
     * Intercepts Up/Down arrows for history navigation, Enter for sending.
     * Passes other keys to QPlainTextEdit::keyPressEvent().
     */
    void keyPressEvent(QKeyEvent* event) override;

  private:
    /**
     * recallPreviousCommand - Navigate backward in history (Up arrow)
     *
     * Saves current text on first navigation, then moves backward through
     * command history.
     */
    void recallPreviousCommand();

    /**
     * recallNextCommand - Navigate forward in history (Down arrow)
     *
     * Moves forward through history. When reaching the end, restores
     * the text that was being typed before browsing started.
     */
    void recallNextCommand();

    /**
     * recallPartialPrevious - Search backwards for partial match (Alt+Up)
     *
     * Partial command matching
     *
     * Searches backwards in history for commands starting with the
     * current input text.
     */
    void recallPartialPrevious();

    /**
     * recallPartialNext - Search forwards for partial match (Alt+Down)
     *
     * Partial command matching
     *
     * Searches forwards in history for commands starting with the
     * current partial match text.
     */
    void recallPartialNext();

    /**
     * recallFirstCommand - Jump to first (oldest) command (Ctrl+Up)
     *
     * Jump to history edges
     */
    void recallFirstCommand();

    /**
     * recallLastCommand - Jump to last (newest) command (Ctrl+Down)
     *
     * Jump to history edges
     */
    void recallLastCommand();

    /**
     * TabCompleteOneLine - Scan one line for tab completion match
     *
     * Tab Completion (IDENTICAL to original)
     *
     * Based on CSendView::TabCompleteOneLine() from sendvw.cpp
     *
     * Character-by-character scan of strLine looking for a word that:
     * - Starts with strWord (case-insensitive prefix match)
     * - Is longer than strWord (no same-length matches)
     *
     * If match found: replaces text[nStartChar..nEndChar] with match
     * Returns true if match found and replaced, false otherwise.
     *
     * @param nStartChar - Start position of word to replace
     * @param nEndChar - End position of word to replace
     * @param strWord - Partial word to match (already lowercased)
     * @param strLine - Line to search for matching words
     * @return true if match found and completed, false otherwise
     */
    bool TabCompleteOneLine(int nStartChar, int nEndChar, const QString& strWord,
                            const QString& strLine);

    /**
     * updateHeight - Auto-resize based on content line count
     *
     * Based on CSendView auto-resize from sendvw.cpp
     *
     * Resizes height based on:
     * - m_bAutoResizeCommandWindow (enabled?)
     * - m_iAutoResizeMinimumLines (minimum height)
     * - m_iAutoResizeMaximumLines (maximum height)
     * - Current line count
     */
    void updateHeight();

    // Data members
    WorldDocument* m_doc;        // Document (not owned)
    QString m_savedCommand;      // Text being typed before browsing history
    bool m_bChanged;             // True if user has typed since last history navigation
    QString m_strPartialCommand; // Partial command for Alt+Up/Down matching

  signals:
    /**
     * commandTextChanged - Emitted when command text changes
     *
     * Plugin notification
     *
     * This signal is emitted whenever the user modifies the input text,
     * allowing plugins to react to command changes in real-time.
     */
    void commandTextChanged(const QString& text);

    /**
     * commandEntered - Emitted when Enter is pressed (command ready to send)
     */
    void commandEntered();

  private slots:
    /**
     * onTextChanged - Track when user types
     *
     * Sets m_bChanged = true when user modifies the text.
     * Also emits commandTextChanged signal for plugins.
     * Triggers auto-resize.
     */
    void onTextChanged();
};

#endif // INPUT_VIEW_H
