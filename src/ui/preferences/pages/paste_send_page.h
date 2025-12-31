#ifndef PASTE_SEND_PAGE_H
#define PASTE_SEND_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QTabWidget;

/**
 * PasteSendPage - Paste and Send File settings
 *
 * Configure how text is sent when pasting from clipboard
 * or sending a file to the MUD.
 */
class PasteSendPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit PasteSendPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Paste / Send"); }
    QString pageDescription() const override
    {
        return tr("Configure paste and send file options for sending text to the MUD.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void markChanged();

  private:
    void setupUi();
    QWidget* createPasteTab();
    QWidget* createSendFileTab();

    // Paste to World settings
    QLineEdit* m_pastePreambleEdit;
    QLineEdit* m_pastePostambleEdit;
    QLineEdit* m_pasteLinePreambleEdit;
    QLineEdit* m_pasteLinePostambleEdit;
    QSpinBox* m_pasteDelaySpin;
    QSpinBox* m_pasteDelayPerLinesSpin;
    QCheckBox* m_pasteCommentedSoftcodeCheck;
    QCheckBox* m_pasteEchoCheck;
    QCheckBox* m_pasteConfirmCheck;

    // Send File settings
    QLineEdit* m_filePreambleEdit;
    QLineEdit* m_filePostambleEdit;
    QLineEdit* m_fileLinePreambleEdit;
    QLineEdit* m_fileLinePostambleEdit;
    QSpinBox* m_fileDelaySpin;
    QSpinBox* m_fileDelayPerLinesSpin;
    QCheckBox* m_fileCommentedSoftcodeCheck;
    QCheckBox* m_fileEchoCheck;
    QCheckBox* m_fileConfirmCheck;

    // Track changes
    bool m_hasChanges = false;
};

#endif // PASTE_SEND_PAGE_H
