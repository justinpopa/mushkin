#include "global_preferences_dialog.h"
#include "../../storage/global_options.h"
#include "../main_window.h"

#include "logging.h"
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

// Helper function to format font info with weight/italic
static QString formatFontInfo(const QFont& font)
{
    QString info = QString("%1, %2pt").arg(font.family()).arg(font.pointSize());

    QStringList styles;
    if (font.bold()) {
        styles << "Bold";
    }
    if (font.italic()) {
        styles << "Italic";
    }

    if (!styles.isEmpty()) {
        info += ", " + styles.join(" ");
    }

    return info;
}

GlobalPreferencesDialog::GlobalPreferencesDialog(QWidget* parent)
    : QDialog(parent), m_categoryList(nullptr), m_contentStack(nullptr), m_buttonBox(nullptr)
{
    setupUi();
    loadSettings();
}

GlobalPreferencesDialog::~GlobalPreferencesDialog()
{
    // Qt handles cleanup of child widgets
}

void GlobalPreferencesDialog::setupUi()
{
    setWindowTitle("Preferences");
    setMinimumSize(700, 500);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create horizontal layout for sidebar + content
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Setup sidebar and content
    setupSidebar();
    createPages();

    contentLayout->addWidget(m_categoryList);
    contentLayout->addWidget(m_contentStack, 1); // Content takes more space

    mainLayout->addLayout(contentLayout);

    // Create button box with OK, Cancel, Apply
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                           QDialogButtonBox::Apply,
                                       Qt::Horizontal, this);

    // Connect button signals
    connect(m_buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &GlobalPreferencesDialog::onOkClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &GlobalPreferencesDialog::onCancelClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &GlobalPreferencesDialog::onApplyClicked);

    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);
}

void GlobalPreferencesDialog::setupSidebar()
{
    m_categoryList = new QListWidget(this);
    m_categoryList->setMaximumWidth(150);
    m_categoryList->setMinimumWidth(120);

    // Add categories (matching MUSHclient tabs)
    m_categoryList->addItem("Worlds");      // P1
    m_categoryList->addItem("General");     // P2
    m_categoryList->addItem("Defaults");    // P9
    m_categoryList->addItem("Notepad");     // P10
    m_categoryList->addItem("Plugins");     // P12
    m_categoryList->addItem("Lua Scripts"); // P13
    m_categoryList->addItem("Closing");     // P3
    m_categoryList->addItem("Logging");     // P5
    m_categoryList->addItem("Timers");      // P6
    m_categoryList->addItem("Activity");    // P7
    m_categoryList->addItem("Tray Icon");   // P11

    // Select first item by default
    m_categoryList->setCurrentRow(0);

    // Connect selection change
    connect(m_categoryList, &QListWidget::currentRowChanged, this,
            &GlobalPreferencesDialog::onCategoryChanged);
}

void GlobalPreferencesDialog::createPages()
{
    m_contentStack = new QStackedWidget(this);

    // Create and add pages (matching MUSHclient tabs)
    m_contentStack->addWidget(createWorldsPage());     // Worlds (P1)
    m_contentStack->addWidget(createGeneralPage());    // General (P2)
    m_contentStack->addWidget(createDefaultsPage());   // Defaults (P9)
    m_contentStack->addWidget(createNotepadPage());    // Notepad (P10)
    m_contentStack->addWidget(createPluginsPage());    // Plugins (P12)
    m_contentStack->addWidget(createLuaScriptsPage()); // Lua Scripts (P13)
    m_contentStack->addWidget(createClosingPage());    // Closing (P3)
    m_contentStack->addWidget(createLoggingPage());    // Logging (P5)
    m_contentStack->addWidget(createTimersPage());     // Timers (P6)
    m_contentStack->addWidget(createActivityPage());   // Activity (P7)
    m_contentStack->addWidget(createTrayIconPage());   // Tray Icon (P11)

    // Show first page
    m_contentStack->setCurrentIndex(0);
}

QWidget* GlobalPreferencesDialog::createWorldsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Startup worlds (open on startup) ===
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* worldsHeader = new QLabel("<b>Startup worlds (open on startup)</b>");
    headerLayout->addWidget(worldsHeader);
    headerLayout->addStretch();
    // World count label (will be updated)
    QLabel* worldCount = new QLabel("0 worlds");
    worldCount->setStyleSheet("color: gray;");
    headerLayout->addWidget(worldCount);
    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(8);

    // World list
    m_worldList = new QListWidget();
    m_worldList->setMinimumHeight(200);
    mainLayout->addWidget(m_worldList, 1);

    // Update count when list changes
    connect(m_worldList->model(), &QAbstractItemModel::rowsInserted, [worldCount, this]() {
        int count = m_worldList->count();
        worldCount->setText(QString("%1 world%2").arg(count).arg(count == 1 ? "" : "s"));
    });
    connect(m_worldList->model(), &QAbstractItemModel::rowsRemoved, [worldCount, this]() {
        int count = m_worldList->count();
        worldCount->setText(QString("%1 world%2").arg(count).arg(count == 1 ? "" : "s"));
    });

    mainLayout->addSpacing(8);

    // Buttons at bottom (horizontal layout)
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addWorld = new QPushButton("Add...");
    m_addWorld->setMinimumWidth(100);
    connect(m_addWorld, &QPushButton::clicked, [this]() {
        QString file =
            QFileDialog::getOpenFileName(this, "Select World File", m_worldDirectory->text(),
                                         "World Files (*.mcl *.MCL);;All Files (*)");
        if (!file.isEmpty()) {
            m_worldList->addItem(file);
        }
    });
    buttonLayout->addWidget(m_addWorld);

    m_removeWorld = new QPushButton("Remove");
    m_removeWorld->setMinimumWidth(100);
    connect(m_removeWorld, &QPushButton::clicked, [this]() {
        int row = m_worldList->currentRow();
        if (row >= 0) {
            delete m_worldList->takeItem(row);
        }
    });
    buttonLayout->addWidget(m_removeWorld);

    m_moveWorldUp = new QPushButton("Move Up");
    m_moveWorldUp->setMinimumWidth(100);
    connect(m_moveWorldUp, &QPushButton::clicked, [this]() {
        int row = m_worldList->currentRow();
        if (row > 0) {
            QListWidgetItem* item = m_worldList->takeItem(row);
            m_worldList->insertItem(row - 1, item);
            m_worldList->setCurrentRow(row - 1);
        }
    });
    buttonLayout->addWidget(m_moveWorldUp);

    m_moveWorldDown = new QPushButton("Move Down");
    m_moveWorldDown->setMinimumWidth(100);
    connect(m_moveWorldDown, &QPushButton::clicked, [this]() {
        int row = m_worldList->currentRow();
        if (row >= 0 && row < m_worldList->count() - 1) {
            QListWidgetItem* item = m_worldList->takeItem(row);
            m_worldList->insertItem(row + 1, item);
            m_worldList->setCurrentRow(row + 1);
        }
    });
    buttonLayout->addWidget(m_moveWorldDown);

    m_addCurrentWorld = new QPushButton("Add Current World");
    m_addCurrentWorld->setMinimumWidth(120);
    connect(m_addCurrentWorld, &QPushButton::clicked, [this]() {
        // TODO(ui): Query active WorldDocument file path from MainWindow for display.
        qWarning() << "Add Current World not yet implemented";
    });
    buttonLayout->addWidget(m_addCurrentWorld);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    mainLayout->addSpacing(16);

    // === Default World File Directory (at bottom) ===
    QHBoxLayout* worldDirLayout = new QHBoxLayout();
    m_browseWorldDir = new QPushButton("Default World File Directory...");
    m_browseWorldDir->setMinimumWidth(150);
    connect(m_browseWorldDir, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Default World Directory",
                                                        m_worldDirectory->text());
        if (!dir.isEmpty()) {
            m_worldDirectory->setText(dir);
        }
    });
    worldDirLayout->addWidget(m_browseWorldDir);
    worldDirLayout->addStretch();
    mainLayout->addLayout(worldDirLayout);

    mainLayout->addSpacing(4);

    // Path display
    m_worldDirectory = new QLineEdit();
    m_worldDirectory->setReadOnly(true);
    m_worldDirectory->setFrame(false);
    m_worldDirectory->setStyleSheet("QLineEdit { background: transparent; color: gray; }");
    mainLayout->addWidget(m_worldDirectory);

    return page;
}

QWidget* GlobalPreferencesDialog::createGeneralPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Theme ===
    QLabel* themeHeader = new QLabel("<b>Theme</b>");
    mainLayout->addWidget(themeHeader);
    mainLayout->addSpacing(8);

    QHBoxLayout* themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel("Application theme:"));
    themeLayout->addSpacing(10);
    m_themeMode = new QComboBox();
    m_themeMode->addItem("Light");
    m_themeMode->addItem("Dark");
    m_themeMode->addItem("System");
    m_themeMode->setToolTip("Choose the application color scheme.\n"
                            "System follows your operating system's theme setting.");
    themeLayout->addWidget(m_themeMode);
    themeLayout->addStretch();
    mainLayout->addLayout(themeLayout);

    mainLayout->addSpacing(16);

    // === Worlds ===
    QLabel* worldsHeader = new QLabel("<b>Worlds</b>");
    mainLayout->addWidget(worldsHeader);
    mainLayout->addSpacing(8);

    m_autoConnectWorlds = new QCheckBox("Auto connect to world on open");
    m_autoConnectWorlds->setChecked(true);
    mainLayout->addWidget(m_autoConnectWorlds);
    mainLayout->addSpacing(4);

    m_reconnectOnDisconnect = new QCheckBox("Reconnect on disconnect");
    mainLayout->addWidget(m_reconnectOnDisconnect);
    mainLayout->addSpacing(4);

    m_openWorldsMaximized = new QCheckBox("Open world windows maximised");
    mainLayout->addWidget(m_openWorldsMaximized);
    mainLayout->addSpacing(4);

    m_notifyIfCannotConnect = new QCheckBox("Notify me when connection broken");
    m_notifyIfCannotConnect->setChecked(true);
    mainLayout->addWidget(m_notifyIfCannotConnect);
    mainLayout->addSpacing(4);

    m_notifyOnDisconnect = new QCheckBox("Notify me if unable to connect");
    m_notifyOnDisconnect->setChecked(true);
    mainLayout->addWidget(m_notifyOnDisconnect);

    mainLayout->addSpacing(16);

    // === Behavior ===
    QLabel* behaviorHeader = new QLabel("<b>Behavior</b>");
    mainLayout->addWidget(behaviorHeader);
    mainLayout->addSpacing(8);

    m_allTypingToCommandWindow = new QCheckBox("All typing goes to command window");
    m_allTypingToCommandWindow->setChecked(true);
    mainLayout->addWidget(m_allTypingToCommandWindow);
    mainLayout->addSpacing(4);

    m_altKeyActivatesMenu = new QCheckBox("ALT key does not activate menu bar");
    mainLayout->addWidget(m_altKeyActivatesMenu);
    mainLayout->addSpacing(4);

    m_fixedFontForEditing =
        new QCheckBox("Use fixed space font when editing triggers/aliases/timers");
    m_fixedFontForEditing->setChecked(true);
    mainLayout->addWidget(m_fixedFontForEditing);
    mainLayout->addSpacing(4);

    m_f1Macro = new QCheckBox("F1, F2, etc. are macros");
    mainLayout->addWidget(m_f1Macro);
    mainLayout->addSpacing(4);

    m_regexpMatchEmpty = new QCheckBox("Regular expressions match on an empty string");
    m_regexpMatchEmpty->setChecked(true);
    mainLayout->addWidget(m_regexpMatchEmpty);
    mainLayout->addSpacing(4);

    m_triggerRemoveCheck = new QCheckBox("Confirm before removing triggers/aliases/timers");
    m_triggerRemoveCheck->setChecked(true);
    mainLayout->addWidget(m_triggerRemoveCheck);
    mainLayout->addSpacing(4);

    m_errorNotificationToOutput = new QCheckBox("Show error notifications in output window");
    m_errorNotificationToOutput->setChecked(true);
    mainLayout->addWidget(m_errorNotificationToOutput);

    mainLayout->addSpacing(16);

    // === Delimiters ===
    QLabel* delimitersHeader = new QLabel("<b>Delimiters</b>");
    mainLayout->addWidget(delimitersHeader);
    mainLayout->addSpacing(8);

    QHBoxLayout* tabCompletionLayout = new QHBoxLayout();
    tabCompletionLayout->addWidget(new QLabel("Tab completion:"));
    tabCompletionLayout->addSpacing(10);
    m_wordDelimiters = new QLineEdit(".,()[]\"'");
    m_wordDelimiters->setMaximumWidth(300);
    tabCompletionLayout->addWidget(m_wordDelimiters);
    tabCompletionLayout->addStretch();
    mainLayout->addLayout(tabCompletionLayout);
    mainLayout->addSpacing(8);

    QHBoxLayout* dblClickLayout = new QHBoxLayout();
    dblClickLayout->addWidget(new QLabel("Double-click:"));
    dblClickLayout->addSpacing(10);
    m_wordDelimitersDblClick = new QLineEdit(".,()[]\"'");
    m_wordDelimitersDblClick->setMaximumWidth(300);
    dblClickLayout->addWidget(m_wordDelimitersDblClick);
    dblClickLayout->addStretch();
    mainLayout->addLayout(dblClickLayout);

    mainLayout->addSpacing(16);

    // === Display ===
    QLabel* displayHeader = new QLabel("<b>Display</b>");
    mainLayout->addWidget(displayHeader);
    mainLayout->addSpacing(8);

    QHBoxLayout* windowTabsLayout = new QHBoxLayout();
    windowTabsLayout->addWidget(new QLabel("Window tabs:"));
    windowTabsLayout->addSpacing(10);
    m_windowTabsStyle = new QComboBox();
    m_windowTabsStyle->addItem("None");
    m_windowTabsStyle->addItem("Top");
    m_windowTabsStyle->addItem("Bottom");
    windowTabsLayout->addWidget(m_windowTabsStyle);
    windowTabsLayout->addStretch();
    mainLayout->addLayout(windowTabsLayout);
    mainLayout->addSpacing(8);

    QHBoxLayout* localeLayout = new QHBoxLayout();
    localeLayout->addWidget(new QLabel("Locale code:"));
    localeLayout->addSpacing(10);
    m_localeCode = new QLineEdit("EN");
    m_localeCode->setMaximumWidth(60);
    localeLayout->addWidget(m_localeCode);
    localeLayout->addStretch();
    mainLayout->addLayout(localeLayout);

    mainLayout->addSpacing(8);

    m_autoExpandConfig = new QCheckBox("Auto-expand config screens");
    m_autoExpandConfig->setChecked(true);
    mainLayout->addWidget(m_autoExpandConfig);
    mainLayout->addSpacing(4);

    m_colourGradientConfig = new QCheckBox("Use colour gradient in config screens");
    m_colourGradientConfig->setChecked(true);
    mainLayout->addWidget(m_colourGradientConfig);
    mainLayout->addSpacing(4);

    m_bleedBackground = new QCheckBox("Bleed background colour to edge");
    mainLayout->addWidget(m_bleedBackground);
    mainLayout->addSpacing(4);

    m_smoothScrolling = new QCheckBox("Smoother scrolling");
    mainLayout->addWidget(m_smoothScrolling);
    mainLayout->addSpacing(4);

    m_smootherScrolling = new QCheckBox("Very smooth scrolling");
    mainLayout->addWidget(m_smootherScrolling);
    mainLayout->addSpacing(4);

    m_showGridLinesInListViews = new QCheckBox("Show grid lines in list views");
    m_showGridLinesInListViews->setChecked(true);
    mainLayout->addWidget(m_showGridLinesInListViews);
    mainLayout->addSpacing(4);

    m_flatToolbars = new QCheckBox("Flat toolbars");
    m_flatToolbars->setChecked(true);
    mainLayout->addWidget(m_flatToolbars);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createDefaultsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Fonts ===
    QLabel* fontsHeader = new QLabel("<b>Fonts</b>");
    mainLayout->addWidget(fontsHeader);
    mainLayout->addSpacing(8);

    // Output font
    QHBoxLayout* outputFontLayout = new QHBoxLayout();
    outputFontLayout->addWidget(new QLabel("Default output font:"));
    outputFontLayout->addSpacing(10);
    m_defaultOutputFontLabel = new QLabel("FixedSys, 9pt");
    m_defaultOutputFontLabel->setMinimumWidth(200);
    outputFontLayout->addWidget(m_defaultOutputFontLabel);
    m_defaultOutputFontButton = new QPushButton("Change...");
    connect(m_defaultOutputFontButton, &QPushButton::clicked, [this]() {
        bool ok;
        QFont font =
            QFontDialog::getFont(&ok, m_defaultOutputFont, this, "Choose Default Output Font");
        if (ok) {
            m_defaultOutputFont = font;
            m_defaultOutputFontLabel->setText(formatFontInfo(font));
        }
    });
    outputFontLayout->addWidget(m_defaultOutputFontButton);
    outputFontLayout->addStretch();
    mainLayout->addLayout(outputFontLayout);
    mainLayout->addSpacing(8);

    // Input font
    QHBoxLayout* inputFontLayout = new QHBoxLayout();
    inputFontLayout->addWidget(new QLabel("Default input font:"));
    inputFontLayout->addSpacing(10);
    m_defaultInputFontLabel = new QLabel("FixedSys, 9pt");
    m_defaultInputFontLabel->setMinimumWidth(200);
    inputFontLayout->addWidget(m_defaultInputFontLabel);
    m_defaultInputFontButton = new QPushButton("Change...");
    connect(m_defaultInputFontButton, &QPushButton::clicked, [this]() {
        bool ok;
        QFont font =
            QFontDialog::getFont(&ok, m_defaultInputFont, this, "Choose Default Input Font");
        if (ok) {
            m_defaultInputFont = font;
            m_defaultInputFontLabel->setText(formatFontInfo(font));
        }
    });
    inputFontLayout->addWidget(m_defaultInputFontButton);
    inputFontLayout->addStretch();
    mainLayout->addLayout(inputFontLayout);
    mainLayout->addSpacing(8);

    // Fixed pitch font
    QHBoxLayout* fixedFontLayout = new QHBoxLayout();
    fixedFontLayout->addWidget(new QLabel("Fixed pitch font:"));
    fixedFontLayout->addSpacing(10);
    m_fixedPitchFontLabel = new QLabel("FixedSys, 9pt");
    m_fixedPitchFontLabel->setMinimumWidth(200);
    fixedFontLayout->addWidget(m_fixedPitchFontLabel);
    m_fixedPitchFontButton = new QPushButton("Change...");
    connect(m_fixedPitchFontButton, &QPushButton::clicked, [this]() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, m_fixedPitchFont, this, "Choose Fixed Pitch Font");
        if (ok) {
            m_fixedPitchFont = font;
            m_fixedPitchFontLabel->setText(formatFontInfo(font));
        }
    });
    fixedFontLayout->addWidget(m_fixedPitchFontButton);
    fixedFontLayout->addStretch();
    mainLayout->addLayout(fixedFontLayout);

    mainLayout->addSpacing(16);

    // === Import Files ===
    QLabel* importHeader = new QLabel("<b>Default Import Files</b>");
    mainLayout->addWidget(importHeader);
    mainLayout->addSpacing(8);

    QLabel* importInfo =
        new QLabel("<i>These files are automatically imported when creating new worlds.</i>");
    importInfo->setWordWrap(true);
    mainLayout->addWidget(importInfo);
    mainLayout->addSpacing(8);

    // Helper lambda for creating file path rows
    auto createFileRow = [this, mainLayout](const QString& label, QLineEdit** lineEdit,
                                            QPushButton** browseBtn, const QString& filter) {
        QHBoxLayout* layout = new QHBoxLayout();
        layout->addWidget(new QLabel(label + ":"));
        layout->addSpacing(10);
        *lineEdit = new QLineEdit();
        (*lineEdit)->setMaximumWidth(350);
        layout->addWidget(*lineEdit);
        *browseBtn = new QPushButton("Browse...");
        connect(*browseBtn, &QPushButton::clicked, [this, lineEdit, filter]() {
            QString file = QFileDialog::getOpenFileName(this, "Select " + filter + " File",
                                                        (*lineEdit)->text(), filter);
            if (!file.isEmpty()) {
                (*lineEdit)->setText(file);
            }
        });
        layout->addWidget(*browseBtn);
        layout->addStretch();
        mainLayout->addLayout(layout);
        mainLayout->addSpacing(4);
    };

    createFileRow("Aliases", &m_defaultAliasesFile, &m_browseAliasesFile, "Alias Files (*.xml)");
    createFileRow("Triggers", &m_defaultTriggersFile, &m_browseTriggersFile,
                  "Trigger Files (*.xml)");
    createFileRow("Timers", &m_defaultTimersFile, &m_browseTimersFile, "Timer Files (*.xml)");
    createFileRow("Macros", &m_defaultMacrosFile, &m_browseMacrosFile, "Macro Files (*.xml)");
    createFileRow("Colours", &m_defaultColoursFile, &m_browseColoursFile, "Colour Files (*.xml)");

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createNotepadPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Notepad ===
    QLabel* notepadHeader = new QLabel("<b>Notepad</b>");
    mainLayout->addWidget(notepadHeader);
    mainLayout->addSpacing(8);

    m_notepadWordWrap = new QCheckBox("Word wrap");
    m_notepadWordWrap->setChecked(true);
    mainLayout->addWidget(m_notepadWordWrap);

    m_tabInsertsTab = new QCheckBox("Tab inserts tab in multi-line dialogs");
    mainLayout->addWidget(m_tabInsertsTab);

    mainLayout->addSpacing(16);

    // Notepad font
    QHBoxLayout* notepadFontLayout = new QHBoxLayout();
    notepadFontLayout->addWidget(new QLabel("Notepad font:"));
    notepadFontLayout->addSpacing(10);
    m_notepadFontLabel = new QLabel("Courier, 10pt");
    m_notepadFontLabel->setMinimumWidth(200);
    notepadFontLayout->addWidget(m_notepadFontLabel);
    m_notepadFontButton = new QPushButton("Change...");
    connect(m_notepadFontButton, &QPushButton::clicked, [this]() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, m_notepadFont, this, "Choose Notepad Font");
        if (ok) {
            m_notepadFont = font;
            m_notepadFontLabel->setText(formatFontInfo(font));
        }
    });
    notepadFontLayout->addWidget(m_notepadFontButton);
    notepadFontLayout->addStretch();
    mainLayout->addLayout(notepadFontLayout);

    mainLayout->addSpacing(16);

    // Colors
    QHBoxLayout* backColorLayout = new QHBoxLayout();
    backColorLayout->addWidget(new QLabel("Background color:"));
    backColorLayout->addSpacing(10);
    m_notepadBackColourButton = new QPushButton();
    m_notepadBackColourButton->setFixedSize(80, 25);
    connect(m_notepadBackColourButton, &QPushButton::clicked, [this]() {
        QColor color =
            QColorDialog::getColor(QColor(m_notepadBackColour), this, "Choose Background Color");
        if (color.isValid()) {
            m_notepadBackColour = color.rgb();
            updateColorButton(m_notepadBackColourButton, m_notepadBackColour);
        }
    });
    backColorLayout->addWidget(m_notepadBackColourButton);
    backColorLayout->addStretch();
    mainLayout->addLayout(backColorLayout);
    mainLayout->addSpacing(8);

    QHBoxLayout* textColorLayout = new QHBoxLayout();
    textColorLayout->addWidget(new QLabel("Text color:"));
    textColorLayout->addSpacing(10);
    m_notepadTextColourButton = new QPushButton();
    m_notepadTextColourButton->setFixedSize(80, 25);
    connect(m_notepadTextColourButton, &QPushButton::clicked, [this]() {
        QColor color =
            QColorDialog::getColor(QColor(m_notepadTextColour), this, "Choose Text Color");
        if (color.isValid()) {
            m_notepadTextColour = color.rgb();
            updateColorButton(m_notepadTextColourButton, m_notepadTextColour);
        }
    });
    textColorLayout->addWidget(m_notepadTextColourButton);
    textColorLayout->addStretch();
    mainLayout->addLayout(textColorLayout);

    mainLayout->addSpacing(16);

    // Quote string
    QHBoxLayout* quoteLayout = new QHBoxLayout();
    quoteLayout->addWidget(new QLabel("Quote string:"));
    quoteLayout->addSpacing(10);
    m_notepadQuoteString = new QLineEdit("> ");
    m_notepadQuoteString->setMaximumWidth(200);
    quoteLayout->addWidget(m_notepadQuoteString);
    quoteLayout->addStretch();
    mainLayout->addLayout(quoteLayout);

    mainLayout->addSpacing(16);

    // === Parenthesis Matching ===
    QLabel* parenHeader = new QLabel("<b>Parenthesis Matching</b>");
    mainLayout->addWidget(parenHeader);
    mainLayout->addSpacing(8);

    m_parenMatchNestBraces = new QCheckBox("Nest braces like '(', ')', '[', ']', '{', '}'");
    mainLayout->addWidget(m_parenMatchNestBraces);
    mainLayout->addSpacing(4);

    m_parenMatchBackslashEscapes = new QCheckBox("Backslash escapes following character");
    mainLayout->addWidget(m_parenMatchBackslashEscapes);
    mainLayout->addSpacing(4);

    m_parenMatchPercentEscapes = new QCheckBox("Percent sign escapes following character");
    mainLayout->addWidget(m_parenMatchPercentEscapes);
    mainLayout->addSpacing(4);

    m_parenMatchSingleQuotes = new QCheckBox("Single quotes delimit strings");
    mainLayout->addWidget(m_parenMatchSingleQuotes);
    mainLayout->addSpacing(4);

    m_parenMatchDoubleQuotes = new QCheckBox("Double quotes delimit strings");
    mainLayout->addWidget(m_parenMatchDoubleQuotes);
    mainLayout->addSpacing(4);

    m_parenMatchEscapeSingleQuotes = new QCheckBox("Backslash escapes single quotes");
    mainLayout->addWidget(m_parenMatchEscapeSingleQuotes);
    mainLayout->addSpacing(4);

    m_parenMatchEscapeDoubleQuotes = new QCheckBox("Backslash escapes double quotes");
    mainLayout->addWidget(m_parenMatchEscapeDoubleQuotes);

    mainLayout->addSpacing(16);

    QLabel* infoLabel = new QLabel("<i>These settings apply to notepad windows.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createPluginsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Global plugins (load into each world) ===
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* pluginsHeader = new QLabel("<b>Global plugins (load into each world)</b>");
    headerLayout->addWidget(pluginsHeader);
    headerLayout->addStretch();
    // Plugin count label (will be updated)
    QLabel* pluginCount = new QLabel("0 plugins");
    pluginCount->setStyleSheet("color: gray;");
    headerLayout->addWidget(pluginCount);
    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(8);

    // Plugin list
    m_pluginList = new QListWidget();
    m_pluginList->setMinimumHeight(200);
    mainLayout->addWidget(m_pluginList, 1);

    // Update count when list changes
    connect(m_pluginList->model(), &QAbstractItemModel::rowsInserted, [pluginCount, this]() {
        int count = m_pluginList->count();
        pluginCount->setText(QString("%1 plugin%2").arg(count).arg(count == 1 ? "" : "s"));
    });
    connect(m_pluginList->model(), &QAbstractItemModel::rowsRemoved, [pluginCount, this]() {
        int count = m_pluginList->count();
        pluginCount->setText(QString("%1 plugin%2").arg(count).arg(count == 1 ? "" : "s"));
    });

    mainLayout->addSpacing(8);

    // Buttons at bottom (horizontal layout)
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addPlugin = new QPushButton("Add...");
    m_addPlugin->setMinimumWidth(100);
    connect(m_addPlugin, &QPushButton::clicked, [this]() {
        QString file =
            QFileDialog::getOpenFileName(this, "Select Plugin", m_pluginsDirectory->text(),
                                         "Plugin Files (*.xml *.dll *.so *.dylib);;All Files (*)");
        if (!file.isEmpty()) {
            m_pluginList->addItem(file);
        }
    });
    buttonLayout->addWidget(m_addPlugin);

    m_removePlugin = new QPushButton("Remove");
    m_removePlugin->setMinimumWidth(100);
    connect(m_removePlugin, &QPushButton::clicked, [this]() {
        int row = m_pluginList->currentRow();
        if (row >= 0) {
            delete m_pluginList->takeItem(row);
        }
    });
    buttonLayout->addWidget(m_removePlugin);

    m_movePluginUp = new QPushButton("Move Up");
    m_movePluginUp->setMinimumWidth(100);
    connect(m_movePluginUp, &QPushButton::clicked, [this]() {
        int row = m_pluginList->currentRow();
        if (row > 0) {
            QListWidgetItem* item = m_pluginList->takeItem(row);
            m_pluginList->insertItem(row - 1, item);
            m_pluginList->setCurrentRow(row - 1);
        }
    });
    buttonLayout->addWidget(m_movePluginUp);

    m_movePluginDown = new QPushButton("Move Down");
    m_movePluginDown->setMinimumWidth(100);
    connect(m_movePluginDown, &QPushButton::clicked, [this]() {
        int row = m_pluginList->currentRow();
        if (row >= 0 && row < m_pluginList->count() - 1) {
            QListWidgetItem* item = m_pluginList->takeItem(row);
            m_pluginList->insertItem(row + 1, item);
            m_pluginList->setCurrentRow(row + 1);
        }
    });
    buttonLayout->addWidget(m_movePluginDown);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    mainLayout->addSpacing(16);

    // === Plugins Directory (at bottom) ===
    QHBoxLayout* pluginsDirLayout = new QHBoxLayout();
    m_browsePluginsDir = new QPushButton("Plugins Directory...");
    m_browsePluginsDir->setMinimumWidth(150);
    connect(m_browsePluginsDir, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Plugins Directory",
                                                        m_pluginsDirectory->text());
        if (!dir.isEmpty()) {
            m_pluginsDirectory->setText(dir);
        }
    });
    pluginsDirLayout->addWidget(m_browsePluginsDir);
    pluginsDirLayout->addStretch();
    mainLayout->addLayout(pluginsDirLayout);

    mainLayout->addSpacing(4);

    // Path display
    m_pluginsDirectory = new QLineEdit();
    m_pluginsDirectory->setReadOnly(true);
    m_pluginsDirectory->setFrame(false);
    m_pluginsDirectory->setStyleSheet("QLineEdit { background: transparent; color: gray; }");
    mainLayout->addWidget(m_pluginsDirectory);

    mainLayout->addSpacing(8);

    // State files directory (bonus: not in original UI but useful to expose)
    QHBoxLayout* stateDirLayout = new QHBoxLayout();
    m_browseStateFilesDir = new QPushButton("State Files Directory...");
    m_browseStateFilesDir->setMinimumWidth(150);
    connect(m_browseStateFilesDir, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select State Files Directory",
                                                        m_stateFilesDirectory->text());
        if (!dir.isEmpty()) {
            m_stateFilesDirectory->setText(dir);
        }
    });
    stateDirLayout->addWidget(m_browseStateFilesDir);
    stateDirLayout->addStretch();
    mainLayout->addLayout(stateDirLayout);

    mainLayout->addSpacing(4);

    // State files path display
    m_stateFilesDirectory = new QLineEdit();
    m_stateFilesDirectory->setReadOnly(true);
    m_stateFilesDirectory->setFrame(false);
    m_stateFilesDirectory->setStyleSheet("QLineEdit { background: transparent; color: gray; }");
    mainLayout->addWidget(m_stateFilesDirectory);

    return page;
}

QWidget* GlobalPreferencesDialog::createLuaScriptsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Lua Script ===
    QLabel* scriptHeader = new QLabel("<b>Lua Initialization Script</b>");
    mainLayout->addWidget(scriptHeader);
    mainLayout->addSpacing(8);

    QLabel* infoLabel = new QLabel("<i>This Lua script runs when MUSHclient starts. Use it to "
                                   "define global functions or configure the Lua environment.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(8);

    m_luaScript = new QPlainTextEdit();
    m_luaScript->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_luaScript->setPlaceholderText("-- Enter Lua code here\n-- This script runs at startup");
    mainLayout->addWidget(m_luaScript, 1); // Takes remaining space

    mainLayout->addSpacing(8);

    m_enablePackageLibrary = new QCheckBox("Enable Lua 'package' library");
    mainLayout->addWidget(m_enablePackageLibrary);

    return page;
}

QWidget* GlobalPreferencesDialog::createClosingPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Confirmations ===
    QLabel* confirmHeader = new QLabel("<b>Confirmations</b>");
    mainLayout->addWidget(confirmHeader);
    mainLayout->addSpacing(8);

    m_confirmBeforeClosingMushclient = new QCheckBox("Confirm before closing MUSHclient");
    mainLayout->addWidget(m_confirmBeforeClosingMushclient);
    mainLayout->addSpacing(4);

    m_confirmBeforeClosingWorld = new QCheckBox("Confirm before closing world");
    m_confirmBeforeClosingWorld->setChecked(true);
    mainLayout->addWidget(m_confirmBeforeClosingWorld);
    mainLayout->addSpacing(4);

    m_confirmBeforeClosingMXPdebug = new QCheckBox("Confirm before closing MXP debug window");
    mainLayout->addWidget(m_confirmBeforeClosingMXPdebug);
    mainLayout->addSpacing(4);

    m_confirmBeforeSavingVariables = new QCheckBox("Confirm before saving variables");
    m_confirmBeforeSavingVariables->setChecked(true);
    mainLayout->addWidget(m_confirmBeforeSavingVariables);

    mainLayout->addSpacing(16);

    QLabel* infoLabel =
        new QLabel("<i>These settings control when confirmation dialogs are shown.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createLoggingPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Logging ===
    QLabel* loggingHeader = new QLabel("<b>Logging</b>");
    mainLayout->addWidget(loggingHeader);
    mainLayout->addSpacing(8);

    m_autoLogWorld = new QCheckBox("Auto-log worlds");
    mainLayout->addWidget(m_autoLogWorld);
    mainLayout->addSpacing(4);

    m_appendToLogFiles = new QCheckBox("Append to log files");
    mainLayout->addWidget(m_appendToLogFiles);
    mainLayout->addSpacing(4);

    m_confirmLogFileClose = new QCheckBox("Confirm before closing log file");
    m_confirmLogFileClose->setChecked(true);
    mainLayout->addWidget(m_confirmLogFileClose);

    mainLayout->addSpacing(16);

    // Log directory
    QHBoxLayout* logDirLayout = new QHBoxLayout();
    logDirLayout->addWidget(new QLabel("Log directory:"));
    logDirLayout->addSpacing(10);
    m_logDirectory = new QLineEdit();
    m_logDirectory->setMaximumWidth(400);
    logDirLayout->addWidget(m_logDirectory);
    m_browseLogDir = new QPushButton("Browse...");
    connect(m_browseLogDir, &QPushButton::clicked, [this]() {
        QString dir =
            QFileDialog::getExistingDirectory(this, "Select Log Directory", m_logDirectory->text());
        if (!dir.isEmpty()) {
            m_logDirectory->setText(dir);
        }
    });
    logDirLayout->addWidget(m_browseLogDir);
    logDirLayout->addStretch();
    mainLayout->addLayout(logDirLayout);

    mainLayout->addSpacing(16);

    QLabel* infoLabel = new QLabel("<i>Log files are saved to the specified directory.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createTimersPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Timer Interval ===
    QLabel* timerHeader = new QLabel("<b>Timer Interval</b>");
    mainLayout->addWidget(timerHeader);
    mainLayout->addSpacing(8);

    QHBoxLayout* timerLayout = new QHBoxLayout();
    timerLayout->addWidget(new QLabel("Global timer interval (seconds):"));
    timerLayout->addSpacing(10);

    m_timerInterval = new QSpinBox();
    m_timerInterval->setRange(0, 120);
    m_timerInterval->setSpecialValueText("Disabled");
    m_timerInterval->setToolTip("Timer interval in seconds (0 to disable, 1-120 seconds). "
                                "Controls how often timers are checked.");
    timerLayout->addWidget(m_timerInterval);

    timerLayout->addStretch();
    mainLayout->addLayout(timerLayout);

    mainLayout->addSpacing(16);

    QLabel* infoLabel = new QLabel(
        "<i>The global timer interval controls how often MUSHclient checks timers. "
        "Set to 0 to disable global timer checking, or 1-120 seconds for the check interval.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createActivityPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Activity Window ===
    QLabel* activityHeader = new QLabel("<b>Activity Window</b>");
    mainLayout->addWidget(activityHeader);
    mainLayout->addSpacing(8);

    m_openActivityWindow = new QCheckBox("Open activity window on startup");
    mainLayout->addWidget(m_openActivityWindow);

    mainLayout->addSpacing(16);

    // Refresh interval
    QHBoxLayout* intervalLayout = new QHBoxLayout();
    intervalLayout->addWidget(new QLabel("Refresh interval (seconds):"));
    intervalLayout->addSpacing(10);

    m_activityRefreshInterval = new QSpinBox();
    m_activityRefreshInterval->setRange(1, 300);
    m_activityRefreshInterval->setValue(15);
    m_activityRefreshInterval->setToolTip(
        "How often to update the activity window (1-300 seconds)");
    intervalLayout->addWidget(m_activityRefreshInterval);

    intervalLayout->addStretch();
    mainLayout->addLayout(intervalLayout);

    mainLayout->addSpacing(16);

    // Refresh type
    QLabel* refreshTypeHeader = new QLabel("<b>Update Activity Window</b>");
    mainLayout->addWidget(refreshTypeHeader);
    mainLayout->addSpacing(8);

    m_refreshOnActivity = new QRadioButton("On activity (new lines from world)");
    mainLayout->addWidget(m_refreshOnActivity);
    mainLayout->addSpacing(4);

    m_refreshPeriodically = new QRadioButton("Periodically (at refresh interval)");
    mainLayout->addWidget(m_refreshPeriodically);
    mainLayout->addSpacing(4);

    m_refreshBoth = new QRadioButton("Both (on activity and periodically)");
    m_refreshBoth->setChecked(true); // Default
    mainLayout->addWidget(m_refreshBoth);

    mainLayout->addSpacing(16);

    // Button bar style
    QHBoxLayout* buttonStyleLayout = new QHBoxLayout();
    buttonStyleLayout->addWidget(new QLabel("Button bar style:"));
    buttonStyleLayout->addSpacing(10);

    m_activityButtonBarStyle = new QComboBox();
    m_activityButtonBarStyle->addItem("Style 0");
    m_activityButtonBarStyle->addItem("Style 1");
    m_activityButtonBarStyle->addItem("Style 2");
    m_activityButtonBarStyle->addItem("Style 3");
    m_activityButtonBarStyle->addItem("Style 4");
    m_activityButtonBarStyle->addItem("Style 5");
    m_activityButtonBarStyle->setToolTip("Visual style for activity window toolbar buttons");
    buttonStyleLayout->addWidget(m_activityButtonBarStyle);

    buttonStyleLayout->addStretch();
    mainLayout->addLayout(buttonStyleLayout);

    mainLayout->addSpacing(16);

    QLabel* infoLabel = new QLabel(
        "<i>The activity window shows all open worlds with their status, new line counts, "
        "and connection duration. Useful for monitoring multiple worlds in MDI mode.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

QWidget* GlobalPreferencesDialog::createTrayIconPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(0);

    // === Icon Placement ===
    QLabel* placementHeader = new QLabel("<b>Icon Placement</b>");
    mainLayout->addWidget(placementHeader);
    mainLayout->addSpacing(8);

    QHBoxLayout* placementLayout = new QHBoxLayout();
    placementLayout->addWidget(new QLabel("Show icon in:"));
    placementLayout->addSpacing(10);

    m_iconPlacement = new QComboBox();
    m_iconPlacement->addItem("Taskbar only");          // 0
    m_iconPlacement->addItem("System tray only");      // 1
    m_iconPlacement->addItem("Both taskbar and tray"); // 2
    m_iconPlacement->setToolTip(
        "Choose where to display the application icon.\n"
        "System tray allows MUSHclient to run minimized in the notification area.");
    placementLayout->addWidget(m_iconPlacement);

    placementLayout->addStretch();
    mainLayout->addLayout(placementLayout);

    mainLayout->addSpacing(16);

    // === Tray Icon Type ===
    QLabel* iconTypeHeader = new QLabel("<b>Tray Icon</b>");
    mainLayout->addWidget(iconTypeHeader);
    mainLayout->addSpacing(8);

    m_useMushclientIcon = new QRadioButton("Use MUSHclient icon");
    m_useMushclientIcon->setChecked(true);
    m_useMushclientIcon->setToolTip("Use the default MUSHclient application icon");
    mainLayout->addWidget(m_useMushclientIcon);
    mainLayout->addSpacing(4);

    m_useCustomIcon = new QRadioButton("Use custom icon:");
    m_useCustomIcon->setToolTip("Use a custom icon file (.ico, .png, .svg)");
    mainLayout->addWidget(m_useCustomIcon);
    mainLayout->addSpacing(4);

    // Custom icon file selection (indented under radio button)
    QHBoxLayout* iconFileLayout = new QHBoxLayout();
    iconFileLayout->setContentsMargins(24, 0, 0, 0); // Indent

    m_customIconFile = new QLineEdit();
    m_customIconFile->setPlaceholderText("Select custom icon file...");
    m_customIconFile->setEnabled(false); // Enabled when custom icon selected
    iconFileLayout->addWidget(m_customIconFile, 1);

    m_browseIconFile = new QPushButton("Browse...");
    m_browseIconFile->setEnabled(false); // Enabled when custom icon selected
    connect(m_browseIconFile, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getOpenFileName(
            this, "Select Icon File", QString(), "Icon Files (*.ico *.png *.svg);;All Files (*)");
        if (!file.isEmpty()) {
            m_customIconFile->setText(file);
        }
    });
    iconFileLayout->addWidget(m_browseIconFile);

    mainLayout->addLayout(iconFileLayout);

    // Enable/disable custom icon controls based on radio selection
    connect(m_useCustomIcon, &QRadioButton::toggled, [this](bool checked) {
        m_customIconFile->setEnabled(checked);
        m_browseIconFile->setEnabled(checked);
    });

    mainLayout->addSpacing(16);

    QLabel* infoLabel =
        new QLabel("<i>The system tray icon allows MUSHclient to run in the background "
                   "while minimized to the notification area. Right-click the tray icon "
                   "for a context menu.</i>");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    return page;
}

void GlobalPreferencesDialog::loadSettings()
{
    auto& opts = GlobalOptions::instance();

    // === Worlds Page ===
    // Default to relative path like original MUSHclient (resolves relative to working directory)
    m_worldDirectory->setText(opts.defaultWorldFileDirectory());

    // World list
    for (const QString& world : opts.worldList()) {
        m_worldList->addItem(world);
    }

    // === General Page ===
    m_themeMode->setCurrentIndex(opts.themeMode());
    m_autoConnectWorlds->setChecked(opts.autoConnectWorlds());
    m_reconnectOnDisconnect->setChecked(opts.reconnectOnLinkFailure());
    m_openWorldsMaximized->setChecked(opts.openWorldsMaximized());
    m_notifyIfCannotConnect->setChecked(opts.notifyIfCannotConnect());
    m_notifyOnDisconnect->setChecked(opts.notifyOnDisconnect());

    m_allTypingToCommandWindow->setChecked(opts.allTypingToCommandWindow());
    m_altKeyActivatesMenu->setChecked(opts.disableKeyboardMenuActivation());
    m_fixedFontForEditing->setChecked(opts.fixedFontForEditing());
    m_f1Macro->setChecked(opts.f1Macro());
    m_regexpMatchEmpty->setChecked(opts.regexpMatchEmpty());
    m_triggerRemoveCheck->setChecked(opts.triggerRemoveCheck());
    m_errorNotificationToOutput->setChecked(opts.errorNotificationToOutputWindow());

    m_wordDelimiters->setText(opts.wordDelimiters());
    m_wordDelimitersDblClick->setText(opts.wordDelimitersDblClick());

    m_windowTabsStyle->setCurrentIndex(opts.windowTabsStyle());
    m_localeCode->setText(opts.locale());

    m_autoExpandConfig->setChecked(opts.autoExpandConfig());
    m_colourGradientConfig->setChecked(opts.colourGradientConfig());
    m_bleedBackground->setChecked(opts.bleedBackground());
    m_smoothScrolling->setChecked(opts.smoothScrolling());
    m_smootherScrolling->setChecked(opts.smootherScrolling());
    m_showGridLinesInListViews->setChecked(opts.showGridLinesInListViews());
    m_flatToolbars->setChecked(opts.flatToolbars());

    // === Closing Page ===
    m_confirmBeforeClosingMushclient->setChecked(opts.confirmBeforeClosingMushclient());
    m_confirmBeforeClosingWorld->setChecked(opts.confirmBeforeClosingWorld());
    m_confirmBeforeClosingMXPdebug->setChecked(opts.confirmBeforeClosingMXPdebug());
    m_confirmBeforeSavingVariables->setChecked(opts.confirmBeforeSavingVariables());

    // === Logging Page ===
    m_autoLogWorld->setChecked(opts.autoLogWorld());
    m_appendToLogFiles->setChecked(opts.appendToLogFiles());
    m_confirmLogFileClose->setChecked(opts.confirmLogFileClose());
    // Default to relative path like original MUSHclient
    m_logDirectory->setText(opts.defaultLogFileDirectory());

    // === Plugins Page ===
    // Default to relative paths like original MUSHclient
    m_pluginsDirectory->setText(opts.pluginsDirectory());
    m_stateFilesDirectory->setText(opts.stateFilesDirectory());

    // Plugin list
    for (const QString& plugin : opts.globalPluginList()) {
        m_pluginList->addItem(plugin);
    }

    // === Notepad Page ===
    m_notepadWordWrap->setChecked(opts.notepadWordWrap());
    m_tabInsertsTab->setChecked(opts.tabInsertsTab());

    // Notepad font
    m_notepadFont.setFamily(opts.notepadFontName());
    m_notepadFont.setPointSize(opts.notepadFontHeight());
    m_notepadFontLabel->setText(formatFontInfo(m_notepadFont));

    m_notepadBackColour = opts.notepadBackColour();
    m_notepadTextColour = opts.notepadTextColour();
    m_notepadQuoteString->setText(opts.notepadQuoteString());
    updateColorButton(m_notepadBackColourButton, m_notepadBackColour);
    updateColorButton(m_notepadTextColourButton, m_notepadTextColour);

    // Paren matching flags (default: 0x0061 = NEST_BRACES | BACKSLASH_ESCAPES | PERCENT_ESCAPES)
    int parenFlags = opts.parenMatchFlags();
    m_parenMatchNestBraces->setChecked(parenFlags & 0x0001);
    m_parenMatchSingleQuotes->setChecked(parenFlags & 0x0002);
    m_parenMatchDoubleQuotes->setChecked(parenFlags & 0x0004);
    m_parenMatchEscapeSingleQuotes->setChecked(parenFlags & 0x0008);
    m_parenMatchEscapeDoubleQuotes->setChecked(parenFlags & 0x0010);
    m_parenMatchBackslashEscapes->setChecked(parenFlags & 0x0020);
    m_parenMatchPercentEscapes->setChecked(parenFlags & 0x0040);

    // === Lua Scripts Page ===
    m_luaScript->setPlainText(opts.luaScript());
    m_enablePackageLibrary->setChecked(opts.enablePackageLibrary());

    // === Timers Page ===
    m_timerInterval->setValue(opts.timerInterval());

    // === Activity Page ===
    m_openActivityWindow->setChecked(opts.openActivityWindow());
    m_activityRefreshInterval->setValue(opts.activityWindowRefreshInterval());
    int refreshType = opts.activityWindowRefreshType();
    if (refreshType == 0)
        m_refreshOnActivity->setChecked(true);
    else if (refreshType == 1)
        m_refreshPeriodically->setChecked(true);
    else
        m_refreshBoth->setChecked(true);
    m_activityButtonBarStyle->setCurrentIndex(opts.activityButtonBarStyle());

    // === Tray Icon Page ===
    m_iconPlacement->setCurrentIndex(opts.iconPlacement()); // Default: taskbar only
    int trayIcon = opts.trayIcon();
    if (trayIcon == 10) {
        // Custom icon
        m_useCustomIcon->setChecked(true);
    } else {
        m_useMushclientIcon->setChecked(true);
    }
    m_customIconFile->setText(opts.trayIconFileName());

    // === Defaults Page ===
    // Output font
    m_defaultOutputFont.setFamily(opts.defaultOutputFont());
    m_defaultOutputFont.setPointSize(opts.defaultOutputFontHeight());
    m_defaultOutputFontLabel->setText(formatFontInfo(m_defaultOutputFont));

    // Input font
    m_defaultInputFont.setFamily(opts.defaultInputFont());
    m_defaultInputFont.setPointSize(opts.defaultInputFontHeight());
    m_defaultInputFont.setWeight(static_cast<QFont::Weight>(opts.defaultInputFontWeight()));
    m_defaultInputFont.setItalic(opts.defaultInputFontItalic() != 0);
    m_defaultInputFontLabel->setText(formatFontInfo(m_defaultInputFont));

    // Fixed pitch font
    m_fixedPitchFont.setFamily(opts.fixedPitchFont());
    m_fixedPitchFont.setPointSize(opts.fixedPitchFontSize());
    m_fixedPitchFontLabel->setText(formatFontInfo(m_fixedPitchFont));

    // Import files
    m_defaultAliasesFile->setText(opts.defaultAliasesFile());
    m_defaultTriggersFile->setText(opts.defaultTriggersFile());
    m_defaultTimersFile->setText(opts.defaultTimersFile());
    m_defaultMacrosFile->setText(opts.defaultMacrosFile());
    m_defaultColoursFile->setText(opts.defaultColoursFile());

    qCDebug(lcDialog) << "GlobalPreferencesDialog::loadSettings() - loaded from QSettings";
}

void GlobalPreferencesDialog::saveSettings()
{
    auto& opts = GlobalOptions::instance();

    // === Worlds Page ===
    opts.setDefaultWorldFileDirectory(m_worldDirectory->text());

    // World list
    QStringList worlds;
    for (int i = 0; i < m_worldList->count(); ++i) {
        worlds.append(m_worldList->item(i)->text());
    }
    opts.setWorldList(worlds);

    // === General Page ===
    opts.setThemeMode(m_themeMode->currentIndex());
    opts.setAutoConnectWorlds(m_autoConnectWorlds->isChecked());
    opts.setReconnectOnLinkFailure(m_reconnectOnDisconnect->isChecked());
    opts.setOpenWorldsMaximized(m_openWorldsMaximized->isChecked());
    opts.setNotifyIfCannotConnect(m_notifyIfCannotConnect->isChecked());
    opts.setNotifyOnDisconnect(m_notifyOnDisconnect->isChecked());

    opts.setAllTypingToCommandWindow(m_allTypingToCommandWindow->isChecked());
    opts.setDisableKeyboardMenuActivation(m_altKeyActivatesMenu->isChecked());
    opts.setFixedFontForEditing(m_fixedFontForEditing->isChecked());
    opts.setF1Macro(m_f1Macro->isChecked());
    opts.setRegexpMatchEmpty(m_regexpMatchEmpty->isChecked());
    opts.setTriggerRemoveCheck(m_triggerRemoveCheck->isChecked());
    opts.setErrorNotificationToOutputWindow(m_errorNotificationToOutput->isChecked());

    opts.setWordDelimiters(m_wordDelimiters->text());
    opts.setWordDelimitersDblClick(m_wordDelimitersDblClick->text());

    opts.setWindowTabsStyle(m_windowTabsStyle->currentIndex());
    opts.setLocale(m_localeCode->text());

    opts.setAutoExpandConfig(m_autoExpandConfig->isChecked());
    opts.setColourGradientConfig(m_colourGradientConfig->isChecked());
    opts.setBleedBackground(m_bleedBackground->isChecked());
    opts.setSmoothScrolling(m_smoothScrolling->isChecked());
    opts.setSmootherScrolling(m_smootherScrolling->isChecked());
    opts.setShowGridLinesInListViews(m_showGridLinesInListViews->isChecked());
    opts.setFlatToolbars(m_flatToolbars->isChecked());

    // === Closing Page ===
    opts.setConfirmBeforeClosingMushclient(m_confirmBeforeClosingMushclient->isChecked());
    opts.setConfirmBeforeClosingWorld(m_confirmBeforeClosingWorld->isChecked());
    opts.setConfirmBeforeClosingMXPdebug(m_confirmBeforeClosingMXPdebug->isChecked());
    opts.setConfirmBeforeSavingVariables(m_confirmBeforeSavingVariables->isChecked());

    // === Logging Page ===
    opts.setAutoLogWorld(m_autoLogWorld->isChecked());
    opts.setAppendToLogFiles(m_appendToLogFiles->isChecked());
    opts.setConfirmLogFileClose(m_confirmLogFileClose->isChecked());
    opts.setDefaultLogFileDirectory(m_logDirectory->text());

    // === Plugins Page ===
    opts.setPluginsDirectory(m_pluginsDirectory->text());
    opts.setStateFilesDirectory(m_stateFilesDirectory->text());

    // Plugin list
    QStringList plugins;
    for (int i = 0; i < m_pluginList->count(); ++i) {
        plugins.append(m_pluginList->item(i)->text());
    }
    opts.setGlobalPluginList(plugins);

    // === Notepad Page ===
    opts.setNotepadWordWrap(m_notepadWordWrap->isChecked());
    opts.setTabInsertsTab(m_tabInsertsTab->isChecked());
    opts.setNotepadFontName(m_notepadFont.family());
    opts.setNotepadFontHeight(m_notepadFont.pointSize());
    opts.setNotepadBackColour(m_notepadBackColour);
    opts.setNotepadTextColour(m_notepadTextColour);
    opts.setNotepadQuoteString(m_notepadQuoteString->text());

    // Paren matching flags
    int parenFlags = 0;
    if (m_parenMatchNestBraces->isChecked())
        parenFlags |= 0x0001;
    if (m_parenMatchSingleQuotes->isChecked())
        parenFlags |= 0x0002;
    if (m_parenMatchDoubleQuotes->isChecked())
        parenFlags |= 0x0004;
    if (m_parenMatchEscapeSingleQuotes->isChecked())
        parenFlags |= 0x0008;
    if (m_parenMatchEscapeDoubleQuotes->isChecked())
        parenFlags |= 0x0010;
    if (m_parenMatchBackslashEscapes->isChecked())
        parenFlags |= 0x0020;
    if (m_parenMatchPercentEscapes->isChecked())
        parenFlags |= 0x0040;
    opts.setParenMatchFlags(parenFlags);

    // === Lua Scripts Page ===
    opts.setLuaScript(m_luaScript->toPlainText());
    opts.setEnablePackageLibrary(m_enablePackageLibrary->isChecked());

    // === Timers Page ===
    opts.setTimerInterval(m_timerInterval->value());

    // === Activity Page ===
    opts.setOpenActivityWindow(m_openActivityWindow->isChecked());
    opts.setActivityWindowRefreshInterval(m_activityRefreshInterval->value());
    int refreshType = 2; // Default: eRefreshBoth
    if (m_refreshOnActivity->isChecked())
        refreshType = 0; // eRefreshOnActivity
    else if (m_refreshPeriodically->isChecked())
        refreshType = 1; // eRefreshPeriodically
    else
        refreshType = 2; // eRefreshBoth
    opts.setActivityWindowRefreshType(refreshType);
    opts.setActivityButtonBarStyle(m_activityButtonBarStyle->currentIndex());

    // === Tray Icon Page ===
    opts.setIconPlacement(m_iconPlacement->currentIndex());
    opts.setTrayIcon(m_useCustomIcon->isChecked() ? 10 : 0);
    opts.setTrayIconFileName(m_customIconFile->text());

    // === Defaults Page ===
    // Output font
    opts.setDefaultOutputFont(m_defaultOutputFont.family());
    opts.setDefaultOutputFontHeight(m_defaultOutputFont.pointSize());

    // Input font
    opts.setDefaultInputFont(m_defaultInputFont.family());
    opts.setDefaultInputFontHeight(m_defaultInputFont.pointSize());
    opts.setDefaultInputFontWeight(static_cast<int>(m_defaultInputFont.weight()));
    opts.setDefaultInputFontItalic(m_defaultInputFont.italic() ? 1 : 0);

    // Fixed pitch font
    opts.setFixedPitchFont(m_fixedPitchFont.family());
    opts.setFixedPitchFontSize(m_fixedPitchFont.pointSize());

    // Import files
    opts.setDefaultAliasesFile(m_defaultAliasesFile->text());
    opts.setDefaultTriggersFile(m_defaultTriggersFile->text());
    opts.setDefaultTimersFile(m_defaultTimersFile->text());
    opts.setDefaultMacrosFile(m_defaultMacrosFile->text());
    opts.setDefaultColoursFile(m_defaultColoursFile->text());

    opts.save();

    qCDebug(lcDialog) << "GlobalPreferencesDialog::saveSettings() - saved to QSettings";
}

void GlobalPreferencesDialog::applySettings()
{
    saveSettings();

    // Apply theme changes to main window
    MainWindow* mainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (mainWindow) {
        mainWindow->applyTheme();
    }

    qCDebug(lcDialog) << "GlobalPreferencesDialog::applySettings() - settings applied";
}

void GlobalPreferencesDialog::onOkClicked()
{
    qCDebug(lcDialog) << "GlobalPreferencesDialog: OK clicked";
    applySettings();
    accept(); // Close dialog with accepted status
}

void GlobalPreferencesDialog::onCancelClicked()
{
    qCDebug(lcDialog) << "GlobalPreferencesDialog: Cancel clicked";
    reject(); // Close dialog with rejected status
}

void GlobalPreferencesDialog::onApplyClicked()
{
    qCDebug(lcDialog) << "GlobalPreferencesDialog: Apply clicked";
    applySettings();
    // Don't close the dialog
}

void GlobalPreferencesDialog::onCategoryChanged(int index)
{
    m_contentStack->setCurrentIndex(index);
}

void GlobalPreferencesDialog::updateColorButton(QPushButton* button, QRgb color)
{
    QColor qcolor(color);
    QString style = QString("background-color: %1; color: %2;")
                        .arg(qcolor.name())
                        .arg(qcolor.lightness() > 128 ? "black" : "white");
    button->setStyleSheet(style);
    button->setText(qcolor.name());
}
