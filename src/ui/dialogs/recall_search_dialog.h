#ifndef RECALL_SEARCH_DIALOG_H
#define RECALL_SEARCH_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

// Forward declarations
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QPushButton;
class WorldDocument;

/**
 * RecallSearchDialog - Configure buffer search/recall
 *
 * The Recall feature searches through the output buffer and displays
 * matching lines in a separate notepad window. This is useful for:
 * - Finding all lines matching a pattern (e.g., "damage")
 * - Extracting commands from history
 * - Filtering game output by type
 * - Reviewing recent quest dialogue
 *
 * Based on: RecallSearchDlg.h/cpp in original MUSHclient
 * Accessed via: View → Recall (Ctrl+R)
 */
class RecallSearchDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc World document to search
     * @param parent Parent widget
     */
    explicit RecallSearchDialog(WorldDocument* doc, QWidget* parent = nullptr);
    ~RecallSearchDialog() override;

    // Getters for search parameters
    QString searchText() const
    {
        return m_searchText;
    }
    bool matchCase() const
    {
        return m_matchCase;
    }
    bool useRegex() const
    {
        return m_useRegex;
    }
    bool includeOutput() const
    {
        return m_includeOutput;
    }
    bool includeCommands() const
    {
        return m_includeCommands;
    }
    bool includeNotes() const
    {
        return m_includeNotes;
    }
    int lineCount() const
    {
        return m_lineCount;
    }
    QString linePreamble() const
    {
        return m_linePreamble;
    }

  private slots:
    void onOkClicked();
    void onCancelClicked();
    void onRegexpHelpClicked();

  private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    WorldDocument* m_doc;

    // UI components
    QComboBox* m_searchTextCombo;
    QCheckBox* m_matchCaseCheck;
    QCheckBox* m_useRegexCheck;
    QCheckBox* m_includeOutputCheck;
    QCheckBox* m_includeCommandsCheck;
    QCheckBox* m_includeNotesCheck;
    QSpinBox* m_lineCountSpin;
    QLineEdit* m_linePreambleEdit;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Search parameters (results)
    QString m_searchText;
    bool m_matchCase;
    bool m_useRegex;
    bool m_includeOutput;
    bool m_includeCommands;
    bool m_includeNotes;
    int m_lineCount;
    QString m_linePreamble;

    // Search history
    QStringList m_searchHistory;
};

#endif // RECALL_SEARCH_DIALOG_H
