#ifndef PLUGIN_WIZARD_H
#define PLUGIN_WIZARD_H

#include <QWizard>
#include <QWizardPage>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

// Forward declarations
class WorldDocument;
class Trigger;
class Alias;
class Timer;
class Variable;

/**
 * PluginWizard - 8-page wizard for creating plugins from world items
 *
 * This wizard guides users through creating a plugin XML file by:
 * - Page 1: Entering metadata (name, ID, author, purpose, version, etc.)
 * - Page 2: Adding description and optional help alias
 * - Pages 3-5: Selecting triggers, aliases, timers from world
 * - Page 6: Selecting variables and state saving options
 * - Page 7: Adding/editing script code
 * - Page 8: Adding comments
 *
 * On completion, generates a plugin XML file and optionally removes
 * selected items from the world.
 *
 * MFC Equivalent: CPluginWizardSheet (PluginWizardSheet.cpp)
 */

// ============================================================================
// Page 1: Plugin Metadata
// ============================================================================
class PluginWizardPage1 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage1(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage1() override = default;

    // QWizardPage interface
    bool validatePage() override;
    void initializePage() override;

private:
    WorldDocument* m_doc;

    // UI elements
    QLineEdit* m_nameEdit;
    QLineEdit* m_idEdit;
    QPushButton* m_generateIdButton;
    QLineEdit* m_authorEdit;
    QLineEdit* m_purposeEdit;
    QLineEdit* m_versionEdit;
    QLineEdit* m_dateWrittenEdit;
    QDoubleSpinBox* m_requiresSpin;
    QCheckBox* m_removeItemsCheck;
};

// ============================================================================
// Page 2: Description and Help Alias
// ============================================================================
class PluginWizardPage2 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage2(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage2() override = default;

    // QWizardPage interface
    bool validatePage() override;

private slots:
    void onEditDescription();
    void onGenerateHelpToggled(bool checked);

private:
    WorldDocument* m_doc;

    // UI elements
    QTextEdit* m_descriptionEdit;
    QPushButton* m_editButton;
    QCheckBox* m_generateHelpCheck;
    QLineEdit* m_helpAliasEdit;
    QLabel* m_helpAliasLabel;
};

// ============================================================================
// Page 3: Triggers Selection
// ============================================================================
class PluginWizardPage3 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage3(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage3() override = default;

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onSelectAll();
    void onSelectNone();
    void onHeaderClicked(int column);

private:
    WorldDocument* m_doc;

    // UI elements
    QTableWidget* m_triggerTable;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    enum Column {
        COL_NAME = 0,
        COL_MATCH,
        COL_SEND,
        COL_GROUP,
        COL_COUNT
    };
};

// ============================================================================
// Page 4: Aliases Selection
// ============================================================================
class PluginWizardPage4 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage4(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage4() override = default;

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onSelectAll();
    void onSelectNone();
    void onHeaderClicked(int column);

private:
    WorldDocument* m_doc;

    // UI elements
    QTableWidget* m_aliasTable;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    enum Column {
        COL_NAME = 0,
        COL_MATCH,
        COL_SEND,
        COL_GROUP,
        COL_COUNT
    };
};

// ============================================================================
// Page 5: Timers Selection
// ============================================================================
class PluginWizardPage5 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage5(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage5() override = default;

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onSelectAll();
    void onSelectNone();
    void onHeaderClicked(int column);

private:
    WorldDocument* m_doc;

    // UI elements
    QTableWidget* m_timerTable;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    enum Column {
        COL_NAME = 0,
        COL_TIME,
        COL_SEND,
        COL_GROUP,
        COL_COUNT
    };
};

// ============================================================================
// Page 6: Variables Selection
// ============================================================================
class PluginWizardPage6 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage6(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage6() override = default;

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onSelectAll();
    void onSelectNone();
    void onHeaderClicked(int column);

private:
    WorldDocument* m_doc;

    // UI elements
    QTableWidget* m_variableTable;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;
    QCheckBox* m_saveStateCheck;

    // Sorting state
    int m_lastColumn;
    bool m_reverseSort;

    enum Column {
        COL_NAME = 0,
        COL_CONTENTS,
        COL_COUNT
    };
};

// ============================================================================
// Page 7: Script Editor
// ============================================================================
class PluginWizardPage7 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage7(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage7() override = default;

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onEditScript();

private:
    WorldDocument* m_doc;

    // UI elements
    QTextEdit* m_scriptEdit;
    QComboBox* m_languageCombo;
    QPushButton* m_editButton;
    QCheckBox* m_includeConstantsCheck;
};

// ============================================================================
// Page 8: Comments
// ============================================================================
class PluginWizardPage8 : public QWizardPage {
    Q_OBJECT

public:
    explicit PluginWizardPage8(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizardPage8() override = default;

    // QWizardPage interface
    bool validatePage() override;

private slots:
    void onEditComments();

private:
    WorldDocument* m_doc;

    // UI elements
    QTextEdit* m_commentsEdit;
    QPushButton* m_editButton;
};

// ============================================================================
// Main Wizard Class
// ============================================================================
class PluginWizard : public QWizard {
    Q_OBJECT

public:
    explicit PluginWizard(WorldDocument* doc, QWidget* parent = nullptr);
    ~PluginWizard() override = default;

    // Page IDs
    enum PageId {
        PAGE_METADATA = 0,
        PAGE_DESCRIPTION,
        PAGE_TRIGGERS,
        PAGE_ALIASES,
        PAGE_TIMERS,
        PAGE_VARIABLES,
        PAGE_SCRIPT,
        PAGE_COMMENTS
    };

    // Override accept to generate plugin XML
    void accept() override;

private:
    QString generatePluginXml();
    bool savePluginXml(const QString& xml);
    void removeItemsFromWorld();

    WorldDocument* m_doc;
    QString m_outputFilename;
};

#endif // PLUGIN_WIZARD_H
