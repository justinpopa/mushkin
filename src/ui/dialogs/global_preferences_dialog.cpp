#include "global_preferences_dialog.h"
#include "../../storage/database.h"
#include "../../storage/global_options.h"

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
        // TODO: Add logic to get current world file path from main window
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
    Database* db = Database::instance();

    // === Worlds Page ===
    // Default to relative path like original MUSHclient (resolves relative to working directory)
    m_worldDirectory->setText(db->getPreference("DefaultWorldFileDirectory", "./worlds/"));

    // World list
    QString worldList = db->getPreference("WorldList", "");
    if (!worldList.isEmpty()) {
        QStringList worlds = worldList.split("\n", Qt::SkipEmptyParts);
        for (const QString& world : worlds) {
            m_worldList->addItem(world);
        }
    }

    // === General Page ===
    m_autoConnectWorlds->setChecked(db->getPreferenceInt("AutoConnectWorlds", 1) != 0);
    m_reconnectOnDisconnect->setChecked(db->getPreferenceInt("ReconnectOnLinkFailure", 0) != 0);
    m_openWorldsMaximized->setChecked(db->getPreferenceInt("OpenWorldsMaximised", 0) != 0);
    m_notifyIfCannotConnect->setChecked(db->getPreferenceInt("NotifyIfCannotConnect", 1) != 0);
    m_notifyOnDisconnect->setChecked(db->getPreferenceInt("NotifyOnDisconnect", 1) != 0);

    m_allTypingToCommandWindow->setChecked(db->getPreferenceInt("AllTypingToCommandWindow", 1) !=
                                           0);
    m_altKeyActivatesMenu->setChecked(db->getPreferenceInt("DisableKeyboardMenuActivation", 0) !=
                                      0);
    m_fixedFontForEditing->setChecked(db->getPreferenceInt("FixedFontForEditing", 1) != 0);
    m_f1Macro->setChecked(db->getPreferenceInt("F1macro", 0) != 0);
    m_regexpMatchEmpty->setChecked(db->getPreferenceInt("RegexpMatchEmpty", 1) != 0);
    m_triggerRemoveCheck->setChecked(db->getPreferenceInt("TriggerRemoveCheck", 1) != 0);
    m_errorNotificationToOutput->setChecked(
        db->getPreferenceInt("ErrorNotificationToOutputWindow", 1) != 0);

    m_wordDelimiters->setText(db->getPreference("WordDelimiters", ".,()[]\"'"));
    m_wordDelimitersDblClick->setText(db->getPreference("WordDelimitersDblClick", ".,()[]\"'"));

    m_windowTabsStyle->setCurrentIndex(db->getPreferenceInt("WindowTabsStyle", 0));
    m_localeCode->setText(db->getPreference("Locale", "EN"));

    m_autoExpandConfig->setChecked(db->getPreferenceInt("AutoExpandConfig", 1) != 0);
    m_colourGradientConfig->setChecked(db->getPreferenceInt("ColourGradientConfig", 1) != 0);
    m_bleedBackground->setChecked(db->getPreferenceInt("BleedBackground", 0) != 0);
    m_smoothScrolling->setChecked(db->getPreferenceInt("SmoothScrolling", 0) != 0);
    m_smootherScrolling->setChecked(db->getPreferenceInt("SmootherScrolling", 0) != 0);
    m_showGridLinesInListViews->setChecked(db->getPreferenceInt("ShowGridLinesInListViews", 1) !=
                                           0);
    m_flatToolbars->setChecked(db->getPreferenceInt("FlatToolbars", 1) != 0);

    // === Closing Page ===
    m_confirmBeforeClosingMushclient->setChecked(
        db->getPreferenceInt("ConfirmBeforeClosingMushclient", 0) != 0);
    m_confirmBeforeClosingWorld->setChecked(db->getPreferenceInt("ConfirmBeforeClosingWorld", 1) !=
                                            0);
    m_confirmBeforeClosingMXPdebug->setChecked(
        db->getPreferenceInt("ConfirmBeforeClosingMXPdebug", 0) != 0);
    m_confirmBeforeSavingVariables->setChecked(
        db->getPreferenceInt("ConfirmBeforeSavingVariables", 1) != 0);

    // === Logging Page ===
    m_autoLogWorld->setChecked(db->getPreferenceInt("AutoLogWorld", 0) != 0);
    m_appendToLogFiles->setChecked(db->getPreferenceInt("AppendToLogFiles", 0) != 0);
    m_confirmLogFileClose->setChecked(db->getPreferenceInt("ConfirmLogFileClose", 1) != 0);
    // Default to relative path like original MUSHclient
    m_logDirectory->setText(db->getPreference("DefaultLogFileDirectory", "./logs/"));

    // === Plugins Page ===
    // Default to relative paths like original MUSHclient
    m_pluginsDirectory->setText(db->getPreference("PluginsDirectory", "./worlds/plugins/"));
    m_stateFilesDirectory->setText(db->getPreference("StateFilesDirectory", "./worlds/plugins/state/"));

    // Plugin list
    QString pluginList = db->getPreference("PluginList", "");
    if (!pluginList.isEmpty()) {
        QStringList plugins = pluginList.split("\n", Qt::SkipEmptyParts);
        for (const QString& plugin : plugins) {
            m_pluginList->addItem(plugin);
        }
    }

    // === Notepad Page ===
    m_notepadWordWrap->setChecked(db->getPreferenceInt("NotepadWordWrap", 1) != 0);
    m_tabInsertsTab->setChecked(db->getPreferenceInt("TabInsertsTabInMultiLineDialogs", 0) != 0);

    // Notepad font
    QString notepadFontFamily = db->getPreference("NotepadFont", "Courier");
    int notepadFontSize = db->getPreferenceInt("NotepadFontHeight", 10);
    m_notepadFont.setFamily(notepadFontFamily);
    m_notepadFont.setPointSize(notepadFontSize);
    m_notepadFontLabel->setText(formatFontInfo(m_notepadFont));

    m_notepadBackColour = db->getPreferenceInt("NotepadBackColour", 0xFFFFFF);
    m_notepadTextColour = db->getPreferenceInt("NotepadTextColour", 0x000000);
    m_notepadQuoteString->setText(db->getPreference("NotepadQuoteString", "> "));
    updateColorButton(m_notepadBackColourButton, m_notepadBackColour);
    updateColorButton(m_notepadTextColourButton, m_notepadTextColour);

    // Paren matching flags (default: 0x0061 = NEST_BRACES | BACKSLASH_ESCAPES | PERCENT_ESCAPES)
    int parenFlags = db->getPreferenceInt("ParenMatchFlags", 0x0061);
    m_parenMatchNestBraces->setChecked(parenFlags & 0x0001);
    m_parenMatchSingleQuotes->setChecked(parenFlags & 0x0002);
    m_parenMatchDoubleQuotes->setChecked(parenFlags & 0x0004);
    m_parenMatchEscapeSingleQuotes->setChecked(parenFlags & 0x0008);
    m_parenMatchEscapeDoubleQuotes->setChecked(parenFlags & 0x0010);
    m_parenMatchBackslashEscapes->setChecked(parenFlags & 0x0020);
    m_parenMatchPercentEscapes->setChecked(parenFlags & 0x0040);

    // === Lua Scripts Page ===
    m_luaScript->setPlainText(db->getPreference("LuaScript", ""));
    m_enablePackageLibrary->setChecked(db->getPreferenceInt("EnablePackageLibrary", 0) != 0);

    // === Timers Page ===
    m_timerInterval->setValue(db->getPreferenceInt("TimerInterval", 0));

    // === Activity Page ===
    m_openActivityWindow->setChecked(db->getPreferenceInt("OpenActivityWindow", 0) != 0);
    m_activityRefreshInterval->setValue(db->getPreferenceInt("ActivityWindowRefreshInterval", 15));
    int refreshType = db->getPreferenceInt("ActivityWindowRefreshType", 2); // Default: eRefreshBoth
    if (refreshType == 0)
        m_refreshOnActivity->setChecked(true);
    else if (refreshType == 1)
        m_refreshPeriodically->setChecked(true);
    else
        m_refreshBoth->setChecked(true);
    m_activityButtonBarStyle->setCurrentIndex(db->getPreferenceInt("ActivityButtonBarStyle", 0));

    // === Tray Icon Page ===
    m_iconPlacement->setCurrentIndex(
        db->getPreferenceInt("IconPlacement", 0)); // Default: taskbar only
    int trayIcon = db->getPreferenceInt("TrayIcon", 0);
    if (trayIcon == 10) {
        // Custom icon
        m_useCustomIcon->setChecked(true);
    } else {
        m_useMushclientIcon->setChecked(true);
    }
    m_customIconFile->setText(db->getPreference("TrayIconFileName", ""));

    // === Defaults Page ===
    // Output font
    QString outputFontFamily = db->getPreference("DefaultOutputFont", "Courier");
    int outputFontSize = db->getPreferenceInt("DefaultOutputFontHeight", 10);
    m_defaultOutputFont.setFamily(outputFontFamily);
    m_defaultOutputFont.setPointSize(outputFontSize);
    m_defaultOutputFontLabel->setText(formatFontInfo(m_defaultOutputFont));

    // Input font
    QString inputFontFamily = db->getPreference("DefaultInputFont", "Courier");
    int inputFontSize = db->getPreferenceInt("DefaultInputFontHeight", 10);
    int inputFontWeight = db->getPreferenceInt("DefaultInputFontWeight", QFont::Normal);
    bool inputFontItalic = db->getPreferenceInt("DefaultInputFontItalic", 0) != 0;
    m_defaultInputFont.setFamily(inputFontFamily);
    m_defaultInputFont.setPointSize(inputFontSize);
    m_defaultInputFont.setWeight(static_cast<QFont::Weight>(inputFontWeight));
    m_defaultInputFont.setItalic(inputFontItalic);
    m_defaultInputFontLabel->setText(formatFontInfo(m_defaultInputFont));

    // Fixed pitch font
    QString fixedFontFamily = db->getPreference("FixedPitchFont", "Courier");
    int fixedFontSize = db->getPreferenceInt("FixedPitchFontSize", 10);
    m_fixedPitchFont.setFamily(fixedFontFamily);
    m_fixedPitchFont.setPointSize(fixedFontSize);
    m_fixedPitchFontLabel->setText(formatFontInfo(m_fixedPitchFont));

    // Import files
    m_defaultAliasesFile->setText(db->getPreference("DefaultAliasesFile", ""));
    m_defaultTriggersFile->setText(db->getPreference("DefaultTriggersFile", ""));
    m_defaultTimersFile->setText(db->getPreference("DefaultTimersFile", ""));
    m_defaultMacrosFile->setText(db->getPreference("DefaultMacrosFile", ""));
    m_defaultColoursFile->setText(db->getPreference("DefaultColoursFile", ""));

    qCDebug(lcDialog) << "GlobalPreferencesDialog::loadSettings() - loaded from database";
}

void GlobalPreferencesDialog::saveSettings()
{
    Database* db = Database::instance();

    // === Worlds Page ===
    db->setPreference("DefaultWorldFileDirectory", m_worldDirectory->text());

    // World list
    QStringList worlds;
    for (int i = 0; i < m_worldList->count(); ++i) {
        worlds.append(m_worldList->item(i)->text());
    }
    db->setPreference("WorldList", worlds.join("\n"));

    // === General Page ===
    db->setPreferenceInt("AutoConnectWorlds", m_autoConnectWorlds->isChecked() ? 1 : 0);
    db->setPreferenceInt("ReconnectOnLinkFailure", m_reconnectOnDisconnect->isChecked() ? 1 : 0);
    db->setPreferenceInt("OpenWorldsMaximised", m_openWorldsMaximized->isChecked() ? 1 : 0);
    db->setPreferenceInt("NotifyIfCannotConnect", m_notifyIfCannotConnect->isChecked() ? 1 : 0);
    db->setPreferenceInt("NotifyOnDisconnect", m_notifyOnDisconnect->isChecked() ? 1 : 0);

    db->setPreferenceInt("AllTypingToCommandWindow",
                         m_allTypingToCommandWindow->isChecked() ? 1 : 0);
    db->setPreferenceInt("DisableKeyboardMenuActivation",
                         m_altKeyActivatesMenu->isChecked() ? 1 : 0);
    db->setPreferenceInt("FixedFontForEditing", m_fixedFontForEditing->isChecked() ? 1 : 0);
    db->setPreferenceInt("F1macro", m_f1Macro->isChecked() ? 1 : 0);
    db->setPreferenceInt("RegexpMatchEmpty", m_regexpMatchEmpty->isChecked() ? 1 : 0);
    db->setPreferenceInt("TriggerRemoveCheck", m_triggerRemoveCheck->isChecked() ? 1 : 0);
    db->setPreferenceInt("ErrorNotificationToOutputWindow",
                         m_errorNotificationToOutput->isChecked() ? 1 : 0);

    db->setPreference("WordDelimiters", m_wordDelimiters->text());
    db->setPreference("WordDelimitersDblClick", m_wordDelimitersDblClick->text());

    db->setPreferenceInt("WindowTabsStyle", m_windowTabsStyle->currentIndex());
    db->setPreference("Locale", m_localeCode->text());

    db->setPreferenceInt("AutoExpandConfig", m_autoExpandConfig->isChecked() ? 1 : 0);
    db->setPreferenceInt("ColourGradientConfig", m_colourGradientConfig->isChecked() ? 1 : 0);
    db->setPreferenceInt("BleedBackground", m_bleedBackground->isChecked() ? 1 : 0);
    db->setPreferenceInt("SmoothScrolling", m_smoothScrolling->isChecked() ? 1 : 0);
    db->setPreferenceInt("SmootherScrolling", m_smootherScrolling->isChecked() ? 1 : 0);
    db->setPreferenceInt("ShowGridLinesInListViews",
                         m_showGridLinesInListViews->isChecked() ? 1 : 0);
    db->setPreferenceInt("FlatToolbars", m_flatToolbars->isChecked() ? 1 : 0);

    // === Closing Page ===
    db->setPreferenceInt("ConfirmBeforeClosingMushclient",
                         m_confirmBeforeClosingMushclient->isChecked() ? 1 : 0);
    db->setPreferenceInt("ConfirmBeforeClosingWorld",
                         m_confirmBeforeClosingWorld->isChecked() ? 1 : 0);
    db->setPreferenceInt("ConfirmBeforeClosingMXPdebug",
                         m_confirmBeforeClosingMXPdebug->isChecked() ? 1 : 0);
    db->setPreferenceInt("ConfirmBeforeSavingVariables",
                         m_confirmBeforeSavingVariables->isChecked() ? 1 : 0);

    // === Logging Page ===
    db->setPreferenceInt("AutoLogWorld", m_autoLogWorld->isChecked() ? 1 : 0);
    db->setPreferenceInt("AppendToLogFiles", m_appendToLogFiles->isChecked() ? 1 : 0);
    db->setPreferenceInt("ConfirmLogFileClose", m_confirmLogFileClose->isChecked() ? 1 : 0);
    db->setPreference("DefaultLogFileDirectory", m_logDirectory->text());

    // === Plugins Page ===
    db->setPreference("PluginsDirectory", m_pluginsDirectory->text());
    db->setPreference("StateFilesDirectory", m_stateFilesDirectory->text());

    // Plugin list
    QStringList plugins;
    for (int i = 0; i < m_pluginList->count(); ++i) {
        plugins.append(m_pluginList->item(i)->text());
    }
    db->setPreference("PluginList", plugins.join("\n"));

    // === Notepad Page ===
    db->setPreferenceInt("NotepadWordWrap", m_notepadWordWrap->isChecked() ? 1 : 0);
    db->setPreferenceInt("TabInsertsTabInMultiLineDialogs", m_tabInsertsTab->isChecked() ? 1 : 0);
    db->setPreference("NotepadFont", m_notepadFont.family());
    db->setPreferenceInt("NotepadFontHeight", m_notepadFont.pointSize());
    db->setPreferenceInt("NotepadBackColour", m_notepadBackColour);
    db->setPreferenceInt("NotepadTextColour", m_notepadTextColour);
    db->setPreference("NotepadQuoteString", m_notepadQuoteString->text());

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
    db->setPreferenceInt("ParenMatchFlags", parenFlags);

    // === Lua Scripts Page ===
    db->setPreference("LuaScript", m_luaScript->toPlainText());
    db->setPreferenceInt("EnablePackageLibrary", m_enablePackageLibrary->isChecked() ? 1 : 0);

    // === Timers Page ===
    db->setPreferenceInt("TimerInterval", m_timerInterval->value());

    // === Activity Page ===
    db->setPreferenceInt("OpenActivityWindow", m_openActivityWindow->isChecked() ? 1 : 0);
    db->setPreferenceInt("ActivityWindowRefreshInterval", m_activityRefreshInterval->value());
    int refreshType = 2; // Default: eRefreshBoth
    if (m_refreshOnActivity->isChecked())
        refreshType = 0; // eRefreshOnActivity
    else if (m_refreshPeriodically->isChecked())
        refreshType = 1; // eRefreshPeriodically
    else
        refreshType = 2; // eRefreshBoth
    db->setPreferenceInt("ActivityWindowRefreshType", refreshType);
    db->setPreferenceInt("ActivityButtonBarStyle", m_activityButtonBarStyle->currentIndex());

    // === Tray Icon Page ===
    db->setPreferenceInt("IconPlacement", m_iconPlacement->currentIndex());
    db->setPreferenceInt("TrayIcon", m_useCustomIcon->isChecked() ? 10 : 0);
    db->setPreference("TrayIconFileName", m_customIconFile->text());

    // === Defaults Page ===
    // Output font
    db->setPreference("DefaultOutputFont", m_defaultOutputFont.family());
    db->setPreferenceInt("DefaultOutputFontHeight", m_defaultOutputFont.pointSize());

    // Input font
    db->setPreference("DefaultInputFont", m_defaultInputFont.family());
    db->setPreferenceInt("DefaultInputFontHeight", m_defaultInputFont.pointSize());
    db->setPreferenceInt("DefaultInputFontWeight", static_cast<int>(m_defaultInputFont.weight()));
    db->setPreferenceInt("DefaultInputFontItalic", m_defaultInputFont.italic() ? 1 : 0);

    // Fixed pitch font
    db->setPreference("FixedPitchFont", m_fixedPitchFont.family());
    db->setPreferenceInt("FixedPitchFontSize", m_fixedPitchFont.pointSize());

    // Import files
    db->setPreference("DefaultAliasesFile", m_defaultAliasesFile->text());
    db->setPreference("DefaultTriggersFile", m_defaultTriggersFile->text());
    db->setPreference("DefaultTimersFile", m_defaultTimersFile->text());
    db->setPreference("DefaultMacrosFile", m_defaultMacrosFile->text());
    db->setPreference("DefaultColoursFile", m_defaultColoursFile->text());

    // Refresh GlobalOptions cache so other code sees the changes
    GlobalOptions::instance()->load();

    qCDebug(lcDialog) << "GlobalPreferencesDialog::saveSettings() - saved to database";
}

void GlobalPreferencesDialog::applySettings()
{
    saveSettings();
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
