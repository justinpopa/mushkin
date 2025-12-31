#ifndef KEYPAD_PAGE_H
#define KEYPAD_PAGE_H

#include "../preferences_page_base.h"

class QLineEdit;
class QCheckBox;

#define KEYPAD_MAX_ITEMS 30

/**
 * KeypadPage - Numeric keypad configuration
 *
 * Configure the numeric keypad for speedwalking and
 * other quick command entry.
 */
class KeypadPage : public PreferencesPageBase {
    Q_OBJECT

  public:
    explicit KeypadPage(WorldDocument* doc, QWidget* parent = nullptr);

    QString pageName() const override { return tr("Keypad"); }
    QString pageDescription() const override
    {
        return tr("Configure numeric keypad for speedwalking and quick commands.");
    }

    void loadSettings() override;
    void saveSettings() override;
    bool hasChanges() const override;

  private slots:
    void markChanged();

  private:
    void setupUi();

    // Enable checkbox
    QCheckBox* m_enableCheck;

    // Keypad command edits (30 items)
    QLineEdit* m_keypadEdits[KEYPAD_MAX_ITEMS];

    // Track changes
    bool m_hasChanges = false;
};

#endif // KEYPAD_PAGE_H
