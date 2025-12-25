#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QComboBox;
class QCheckBox;
class QRadioButton;
class QPushButton;
class WorldDocument;

/**
 * FindDialog - Search for text in output buffer
 *
 * Find/Search Dialog
 *
 * Features:
 * - Search forward/backward from current position
 * - Case-sensitive/insensitive search
 * - Regular expression support
 * - Search history (remembers previous searches)
 * - Wrap-around at buffer end
 *
 * Based on original MUSHclient's CFindDlg (FindDlg.h/cpp)
 */
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~FindDialog() override;

    // Get last search parameters (for Find Next)
    QString lastSearchText() const { return m_lastSearchText; }
    bool lastMatchCase() const { return m_lastMatchCase; }
    bool lastUseRegex() const { return m_lastUseRegex; }
    bool lastSearchForward() const { return m_lastSearchForward; }
    int lastFoundLine() const { return m_lastFoundLine; }
    int lastFoundChar() const { return m_lastFoundChar; }

private slots:
    void findNext();
    void findPrevious();
    void closeDialog();
    void updateMatchCounter();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    bool performSearch(bool forward);
    int countAllMatches();

    WorldDocument* m_doc;

    // UI Components
    QComboBox* m_searchText;
    QCheckBox* m_matchCase;
    QCheckBox* m_useRegex;
    QRadioButton* m_searchForward;
    QRadioButton* m_searchBackward;
    QPushButton* m_findButton;
    QPushButton* m_findPrevButton;
    QPushButton* m_closeButton;
    class QLabel* m_matchCounterLabel;

    // Search state
    QStringList m_searchHistory;
    QString m_lastSearchText;
    bool m_lastMatchCase;
    bool m_lastUseRegex;
    bool m_lastSearchForward;
    int m_lastFoundLine;
    int m_lastFoundChar;
    int m_totalMatches;
    int m_currentMatchIndex;
};

#endif // FIND_DIALOG_H
