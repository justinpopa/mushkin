#include "input_view.h"
#include "../../automation/plugin.h"
#include "../../utils/font_utils.h"
#include "../../world/color_utils.h"
#include "../../world/lua_api/lua_registration.h"
#include "../../world/script_engine.h"
#include "dialogs/complete_word_dialog.h"
#include "../text/line.h"
#include "../world/world_document.h"
#include "logging.h"
#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <cctype>

/**
 * InputView Constructor
 *
 * Command History
 * Configurable Appearance
 * Auto-resize
 *
 * Creates a multi-line input widget connected to a WorldDocument for command history.
 */
InputView::InputView(WorldDocument* doc, QWidget* parent)
    : QPlainTextEdit(parent), m_doc(doc), m_savedCommand(), m_bChanged(false)
{
    // Set placeholder text
    setPlaceholderText("Type commands here...");

    // Configure for single-line appearance by default (auto-resize will adjust)
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);

    // Connect textChanged signal to track user modifications
    connect(this, &QPlainTextEdit::textChanged, this, &InputView::onTextChanged);

    // Apply configurable font/colors from document
    applyInputSettings();

    // Set initial height
    updateHeight();

    qCDebug(lcUI) << "InputView created (QPlainTextEdit-based)";
}

/**
 * InputView Destructor
 */
InputView::~InputView()
{
    // Nothing to clean up (don't own m_doc)
}

/**
 * setCursorPosition - Set cursor position (compatibility with QLineEdit API)
 *
 * Uses QTextCursor to position the cursor in the text.
 */
void InputView::setCursorPosition(int pos)
{
    // Clamp position to valid range to avoid Qt warnings
    int textLen = toPlainText().length();
    pos = qBound(0, pos, textLen);

    QTextCursor cursor = textCursor();
    cursor.setPosition(pos);
    setTextCursor(cursor);
}

void InputView::setSelection(int start, int length)
{
    int textLen = toPlainText().length();
    start = qBound(0, start, textLen);
    int end = qBound(start, start + length, textLen);

    QTextCursor cursor = textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

/**
 * onTextChanged - Track when user types
 *
 * Command History
 * Plugin notification
 * Auto-resize
 *
 * Sets m_bChanged = true when the user modifies the text.
 * This flag is used to detect when to save the "partial command" before
 * browsing history.
 *
 * Also emits commandTextChanged signal for plugin notifications.
 * Triggers auto-resize based on content.
 */
void InputView::onTextChanged()
{
    m_bChanged = true;

    // Notify plugins of command text change
    emit commandTextChanged(toPlainText());

    // Auto-resize based on content
    updateHeight();
}

/**
 * updateHeight - Auto-resize based on content line count
 *
 * Based on CSendView auto-resize from sendvw.cpp
 */
void InputView::updateHeight()
{
    if (!m_doc || !m_doc->m_bAutoResizeCommandWindow) {
        return;
    }

    // Get current line count
    int lineCount = document()->blockCount();

    // Clamp to minimum/maximum
    int minLines = qMax(1, static_cast<int>(m_doc->m_iAutoResizeMinimumLines));
    int maxLines = qMax(minLines, static_cast<int>(m_doc->m_iAutoResizeMaximumLines));

    if (lineCount < minLines) {
        lineCount = minLines;
    }

    // Check if current height exceeds maximum - if so, let user manage it
    QFontMetrics fm(font());
    int lineHeight = fm.lineSpacing();
    int currentHeight = height();
    int maxHeight =
        lineHeight * maxLines + contentsMargins().top() + contentsMargins().bottom() + 4;

    if (currentHeight > maxHeight) {
        // User has manually resized larger than max, don't interfere
        return;
    }

    if (lineCount > maxLines) {
        lineCount = maxLines;
    }

    // Calculate new height: font height * lines + margins
    int newHeight =
        lineHeight * lineCount + contentsMargins().top() + contentsMargins().bottom() + 4;

    // Only resize if height actually changed
    if (newHeight != currentHeight) {
        setFixedHeight(newHeight);
        qCDebug(lcUI) << "Auto-resize: lines=" << document()->blockCount()
                      << "height=" << newHeight;
    }
}

/**
 * keyPressEvent - Handle keyboard input
 *
 * Command History Navigation
 * Enter to send
 *
 * Based on CSendView::PreTranslateMessage() from sendvw.cpp
 *
 * Intercepts:
 * - Up/Down arrows for history navigation
 * - Enter to send command (emit commandEntered)
 * - Shift+Enter to insert newline
 * Other keys are passed to QPlainTextEdit for normal handling.
 */
void InputView::keyPressEvent(QKeyEvent* event)
{
    if (!m_doc) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    Qt::KeyboardModifiers modifiers = event->modifiers();
    bool hasCtrl = modifiers & Qt::ControlModifier;
    bool hasAlt = modifiers & Qt::AltModifier;
    bool hasShift = modifiers & Qt::ShiftModifier;

    // Enter key handling
    // Original MUSHclient: Enter ALWAYS sends command (no modifier check)
    // Based on sendvw.cpp - OnChar intercepts VK_RETURN unconditionally
    // Multi-line input is supported (ES_MULTILINE) but newlines must be pasted
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit commandEntered();
        return;
    }

    // Up arrow with modifiers
    // Original MUSHclient: only navigate history if cursor at start OR end of text
    // Based on sendvw.cpp
    if (event->key() == Qt::Key_Up) {
        if (m_doc->m_bArrowsChangeHistory) {
            QTextCursor cursor = textCursor();
            int pos = cursor.position();
            int textLen = toPlainText().length();

            // Only traverse if at start (pos 0) or end of text
            if (pos == 0 || pos == textLen) {
                if (hasCtrl) {
                    recallFirstCommand();
                    return;
                } else if (hasAlt && m_doc->m_bAltArrowRecallsPartial) {
                    recallPartialPrevious();
                    return;
                } else if (!hasCtrl && !hasAlt) {
                    recallPreviousCommand();
                    return;
                }
            }
        }
    }

    // Down arrow with modifiers
    // Original MUSHclient: only navigate history if cursor at start OR end of text
    // Based on sendvw.cpp
    else if (event->key() == Qt::Key_Down) {
        if (m_doc->m_bArrowsChangeHistory) {
            QTextCursor cursor = textCursor();
            int pos = cursor.position();
            int textLen = toPlainText().length();

            // Only traverse if at start (pos 0) or end of text
            if (pos == 0 || pos == textLen) {
                if (hasCtrl) {
                    recallLastCommand();
                    return;
                } else if (hasAlt && m_doc->m_bAltArrowRecallsPartial) {
                    recallPartialNext();
                    return;
                } else if (!hasCtrl && !hasAlt) {
                    recallNextCommand();
                    return;
                }
            }
        }
    }

    // Ctrl+L: Clear input
    else if (event->key() == Qt::Key_L && hasCtrl) {
        clear();
        return;
    }

    // Escape: Clear input (if enabled)
    else if (event->key() == Qt::Key_Escape) {
        if (m_doc->m_bEscapeDeletesInput) {
            clear();
            return;
        }
    }

    // Shift+Tab: Show function/completion menu
    else if (event->key() == Qt::Key_Tab && hasShift && !hasCtrl && !hasAlt) {
        handleShiftTabCompletion();
        return;
    }

    // Tab: Tab completion (if no modifiers)
    else if (event->key() == Qt::Key_Tab && !hasCtrl && !hasAlt && !hasShift) {
        handleTabCompletion();
        return;
    }

    // All other keys: pass to base class
    QPlainTextEdit::keyPressEvent(event);
}

/**
 * recallPreviousCommand - Navigate backward in history (Up arrow)
 */
void InputView::recallPreviousCommand()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    if (m_bChanged) {
        m_savedCommand = toPlainText();
        m_bChanged = false;
        qCDebug(lcUI) << "Saved command:" << m_savedCommand;
    }

    if (m_doc->m_historyPosition > 0) {
        m_doc->m_historyPosition--;
    }

    if (m_doc->m_historyPosition >= 0 &&
        m_doc->m_historyPosition < m_doc->m_commandHistory.count()) {
        QString command = m_doc->m_commandHistory[m_doc->m_historyPosition];
        setPlainText(command);
        // Move cursor to end
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        qCDebug(lcUI) << "Recalled:" << command << "(pos" << m_doc->m_historyPosition << ")";
    }
}

/**
 * recallNextCommand - Navigate forward in history (Down arrow)
 */
void InputView::recallNextCommand()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    m_doc->m_historyPosition++;

    if (m_doc->m_historyPosition >= m_doc->m_commandHistory.count()) {
        setPlainText(m_savedCommand);
        m_savedCommand.clear();
        m_bChanged = false;
        m_doc->m_historyPosition = m_doc->m_commandHistory.count();
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        qCDebug(lcUI) << "Reached end of history, restored saved command";
        return;
    }

    QString command = m_doc->m_commandHistory[m_doc->m_historyPosition];
    setPlainText(command);
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    qCDebug(lcUI) << "Recalled:" << command << "(pos" << m_doc->m_historyPosition << ")";
}

/**
 * applyInputSettings - Apply font and color settings from document
 */
void InputView::applyInputSettings()
{
    if (!m_doc) {
        return;
    }

    // Apply font settings (uses createMUSHclientFont for Windows-compatible sizing on macOS)
    QFont inputFont = createMUSHclientFont(m_doc->m_input_font_name, m_doc->m_input_font_height);
    inputFont.setItalic(m_doc->m_input_font_italic);
    inputFont.setWeight(static_cast<QFont::Weight>(m_doc->m_input_font_weight));
    setFont(inputFont);

    // Apply color settings using palette (more reliable than stylesheet with themes)
    // Colors are stored in BGR format (Windows COLORREF), convert to QColor
    QColor textColor = bgrToQColor(m_doc->m_input_text_colour);
    QColor bgColor = bgrToQColor(m_doc->m_input_background_colour);

    // Use QPalette instead of stylesheet for better theme compatibility
    QPalette pal = palette();
    pal.setColor(QPalette::Text, textColor);
    pal.setColor(QPalette::Base, bgColor);
    pal.setColor(QPalette::PlaceholderText, textColor.darker(150));
    setPalette(pal);
    setAutoFillBackground(true);

    // Update height after font change
    updateHeight();

    qCDebug(lcUI) << "Applied input settings: font=" << inputFont.family() << inputFont.pointSize()
                  << "fg=" << textColor.name() << "bg=" << bgColor.name();
}

/**
 * recallPartialPrevious - Search backwards for partial match (Alt+Up)
 */
void InputView::recallPartialPrevious()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    QString strCommand = toPlainText();
    bool bAtBottom = false;

    if (m_bChanged) {
        m_strPartialCommand = strCommand;
        bAtBottom = true;
        m_bChanged = false;
        qCDebug(lcUI) << "NEW m_strPartialCommand =" << m_strPartialCommand;
    }

    if (m_strPartialCommand.isEmpty()) {
        recallPreviousCommand();
        return;
    }

    QString strText;
    int searchPos;

    if (bAtBottom) {
        searchPos = m_doc->m_commandHistory.count() - 1;
    } else {
        searchPos = (m_doc->m_historyPosition > 0) ? m_doc->m_historyPosition - 1
                                                   : m_doc->m_commandHistory.count() - 1;
    }

    while (searchPos >= 0) {
        strText = m_doc->m_commandHistory[searchPos];

        bool bPrefixMatch = strText.left(m_strPartialCommand.length())
                                .compare(m_strPartialCommand, Qt::CaseInsensitive) == 0;
        bool bExactMatch = strText.compare(m_strPartialCommand, Qt::CaseInsensitive) == 0;

        if (bPrefixMatch && !bExactMatch) {
            m_doc->m_historyPosition = searchPos;
            setPlainText(strText);
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End);
            setTextCursor(cursor);
            m_doc->m_iHistoryStatus = (searchPos == 0) ? eAtTop : eInMiddle;
            qCDebug(lcUI) << "Found partial match:" << strText << "(pos" << searchPos << ")";
            return;
        }

        searchPos--;
    }

    setPlainText("");
    m_strPartialCommand.clear();
    m_doc->m_historyPosition = -1;
    m_doc->m_iHistoryStatus = eAtTop;
    qCDebug(lcUI) << "Reached top, cleared partial command";
}

/**
 * recallPartialNext - Search forwards for partial match (Alt+Down)
 */
void InputView::recallPartialNext()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    QString strCommand = toPlainText();
    bool bAtTop = false;

    if (m_bChanged) {
        m_strPartialCommand = strCommand;
        bAtTop = true;
        m_bChanged = false;
        qCDebug(lcUI) << "NEW m_strPartialCommand =" << m_strPartialCommand;
    }

    if (m_strPartialCommand.isEmpty()) {
        recallNextCommand();
        return;
    }

    QString strText;
    int searchPos;

    if (bAtTop) {
        searchPos = 0;
    } else {
        searchPos = (m_doc->m_historyPosition >= 0 &&
                     m_doc->m_historyPosition < m_doc->m_commandHistory.count() - 1)
                        ? m_doc->m_historyPosition + 1
                        : 0;
    }

    while (searchPos < m_doc->m_commandHistory.count()) {
        strText = m_doc->m_commandHistory[searchPos];

        bool bPrefixMatch = strText.left(m_strPartialCommand.length())
                                .compare(m_strPartialCommand, Qt::CaseInsensitive) == 0;
        bool bExactMatch = strText.compare(m_strPartialCommand, Qt::CaseInsensitive) == 0;

        if (bPrefixMatch && !bExactMatch) {
            m_doc->m_historyPosition = searchPos;
            setPlainText(strText);
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End);
            setTextCursor(cursor);
            m_doc->m_iHistoryStatus =
                (searchPos == m_doc->m_commandHistory.count() - 1) ? eAtBottom : eInMiddle;
            qCDebug(lcUI) << "Found partial match:" << strText << "(pos" << searchPos << ")";
            return;
        }

        searchPos++;
    }

    setPlainText("");
    m_strPartialCommand.clear();
    m_doc->m_historyPosition = m_doc->m_commandHistory.count();
    m_doc->m_iHistoryStatus = eAtBottom;
    qCDebug(lcUI) << "Reached bottom, cleared partial command";
}

/**
 * recallFirstCommand - Jump to first (oldest) command (Ctrl+Up)
 */
void InputView::recallFirstCommand()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    m_doc->m_historyPosition = 0;
    setPlainText(m_doc->m_commandHistory[0]);
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    m_doc->m_iHistoryStatus = eAtTop;
    m_strPartialCommand.clear();
    qCDebug(lcUI) << "Jumped to first command:" << m_doc->m_commandHistory[0];
}

/**
 * recallLastCommand - Jump to last (newest) command (Ctrl+Down)
 */
void InputView::recallLastCommand()
{
    if (m_doc->m_commandHistory.isEmpty()) {
        return;
    }

    m_doc->m_historyPosition = m_doc->m_commandHistory.count() - 1;
    setPlainText(m_doc->m_commandHistory[m_doc->m_historyPosition]);
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    m_doc->m_iHistoryStatus = eAtBottom;
    m_strPartialCommand.clear();
    qCDebug(lcUI) << "Jumped to last command:" << m_doc->m_commandHistory[m_doc->m_historyPosition];
}

/**
 * TabCompleteOneLine - Scan one line for tab completion match
 */
bool InputView::TabCompleteOneLine(int nStartChar, int nEndChar, const QString& strWord,
                                   const QString& strLine)
{
    if (!m_doc) {
        return false;
    }

    QByteArray lineBytes = strLine.toUtf8();
    const char* p = lineBytes.constData();

    while (*p) {
        while (*p && !isalnum(static_cast<unsigned char>(*p))) {
            p++;
        }

        if (*p == 0) {
            break;
        }

        const char* wordStart = p;
        QString candidateWord;

        while (*p) {
            char c = *p;
            candidateWord += c;
            p++;

            if (*p == 0 || isspace(static_cast<unsigned char>(*p))) {
                break;
            }

            QByteArray delimBytes = m_doc->m_strWordDelimiters.toUtf8();
            if (strchr(delimBytes.constData(), *p) != nullptr) {
                break;
            }
        }

        QString candidateLower = candidateWord.toLower();
        if (candidateLower.startsWith(strWord)) {
            if (candidateWord.length() > strWord.length()) {
                QString sReplacement = candidateWord;

                if (m_doc->m_bLowerCaseTabCompletion) {
                    sReplacement = sReplacement.toLower();
                }

                if (m_doc->m_bTabCompletionSpace) {
                    sReplacement += " ";
                }

                // Notify plugins of tab completion
                // Note: Original MUSHclient uses SendToAllPluginCallbacksRtn which allows
                // plugins to modify sReplacement. We notify but don't support modification yet.
                m_doc->SendToAllPluginCallbacks(ON_PLUGIN_TABCOMPLETE, sReplacement, false);

                // Replace using QTextCursor
                QTextCursor cursor = textCursor();
                int textLen = toPlainText().length();
                int clampedStart = qBound(0, nStartChar, textLen);
                int clampedEnd = qBound(0, nEndChar, textLen);
                cursor.setPosition(clampedStart);
                cursor.setPosition(clampedEnd, QTextCursor::KeepAnchor);
                cursor.insertText(sReplacement);
                setTextCursor(cursor);

                qCDebug(lcUI) << "Tab completion: matched" << candidateWord << "->" << sReplacement;
                return true;
            }
        }
    }

    return false;
}

/**
 * handleTabCompletion - Tab completion for commands (Tab key)
 */
void InputView::handleTabCompletion()
{
    if (!m_doc) {
        return;
    }

    QString strCurrent = toPlainText();
    QTextCursor cursor = textCursor();
    int nEndChar = cursor.position();

    if (strCurrent.isEmpty() || nEndChar <= 0) {
        return;
    }

    int nStartChar;
    QByteArray delimBytes = m_doc->m_strWordDelimiters.toUtf8();

    for (nStartChar = nEndChar - 1; nStartChar >= 0; nStartChar--) {
        QChar c = strCurrent[nStartChar];
        unsigned char uc = c.toLatin1();

        if (c.isSpace() || strchr(delimBytes.constData(), uc) != nullptr) {
            break;
        }
    }
    nStartChar++;

    QString sWord = strCurrent.mid(nStartChar, nEndChar - nStartChar);
    sWord = sWord.toLower();

    if (sWord.isEmpty()) {
        return;
    }

    qCDebug(lcUI) << "Tab completion: searching for" << sWord << "from pos" << nStartChar << "to"
                  << nEndChar;

    if (TabCompleteOneLine(nStartChar, nEndChar, sWord, m_doc->m_strTabCompletionDefaults)) {
        return;
    }

    int iCount = 0;

    for (int i = m_doc->m_lineList.count() - 1; i >= 0; i--) {
        Line* pLine = m_doc->m_lineList[i];
        QString strLine = QString::fromUtf8(pLine->text(), pLine->len());

        if (++iCount > static_cast<int>(m_doc->m_iTabCompletionLines)) {
            qCDebug(lcUI) << "Hit line limit" << m_doc->m_iTabCompletionLines;
            break;
        }

        if (TabCompleteOneLine(nStartChar, nEndChar, sWord, strLine)) {
            return;
        }
    }

    qCDebug(lcUI) << "No tab completion match found for:" << sWord;
}

/**
 * handleShiftTabCompletion - Show function/word completion dialog (Shift+Tab)
 */
void InputView::handleShiftTabCompletion()
{
    if (!m_doc) {
        return;
    }

    QTextCursor cursor = textCursor();
    int cursorPos = cursor.position();
    QString inputText = toPlainText();

    if (inputText.isEmpty()) {
        return;
    }

    int nStartChar = cursorPos;
    while (nStartChar > 0) {
        QChar c = inputText.at(nStartChar - 1);
        if (m_doc->m_strWordDelimiters.contains(c) || c.isSpace()) {
            break;
        }
        nStartChar--;
    }

    int nEndChar = cursorPos;
    while (nEndChar < inputText.length()) {
        QChar c = inputText.at(nEndChar);
        if (m_doc->m_strWordDelimiters.contains(c) || c.isSpace()) {
            break;
        }
        nEndChar++;
    }

    QString filterWord = inputText.mid(nStartChar, nEndChar - nStartChar).toLower();

    CompleteWordDialog dialog(this);

    QStringList extraItems;
    for (const QString& item : m_doc->m_ExtraShiftTabCompleteItems) {
        extraItems << item;
    }
    dialog.setExtraItems(extraItems);

    // Add Lua function names if functions mode is enabled
    if (m_doc->m_bTabCompleteFunctions && m_doc->m_ScriptEngine) {
        dialog.setItems(getLuaFunctionNames(m_doc->m_ScriptEngine->L));
    }

    dialog.setLuaMode(true);
    dialog.setFunctionsMode(m_doc->m_bTabCompleteFunctions);
    dialog.setFilter(filterWord);

    QPoint cursorScreenPos = mapToGlobal(QPoint(cursorRect().x(), cursorRect().bottom()));
    dialog.setPosition(cursorScreenPos);

    if (dialog.exec() == QDialog::Accepted && !dialog.selectedItem().isEmpty()) {
        QString replacement = dialog.selectedItem();

        // Replace using QTextCursor
        QTextCursor replaceCursor = textCursor();
        int textLen = toPlainText().length();
        int clampedStart = qBound(0, nStartChar, textLen);
        int clampedEnd = qBound(0, nEndChar, textLen);
        replaceCursor.setPosition(clampedStart);
        replaceCursor.setPosition(clampedEnd, QTextCursor::KeepAnchor);
        replaceCursor.insertText(replacement);
        setTextCursor(replaceCursor);

        qCDebug(lcUI) << "Shift+Tab completion: selected" << replacement;
    }
}
