#include "unified_preferences_dialog.h"
#include "preferences_page_base.h"
#include "pages/stub_page.h"
#include "pages/triggers_page.h"
#include "pages/aliases_page.h"
#include "pages/timers_page.h"
#include "world/world_document.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

UnifiedPreferencesDialog::UnifiedPreferencesDialog(WorldDocument* doc, Page initialPage,
                                                     QWidget* parent)
    : QDialog(parent), m_doc(doc), m_currentPage(initialPage), m_hasChanges(false)
{
    setWindowTitle(tr("World Configuration - %1").arg(doc->worldName()));
    setMinimumSize(900, 600);
    resize(1000, 700);

    setupUi();
    setupTree();
    setupPages();
    connectSignals();

    // Navigate to initial page
    setCurrentPage(initialPage);
}

UnifiedPreferencesDialog::~UnifiedPreferencesDialog() = default;

void UnifiedPreferencesDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Create splitter for tree and content
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: Tree widget
    m_tree = new QTreeWidget(splitter);
    m_tree->setHeaderHidden(true);
    m_tree->setMinimumWidth(180);
    m_tree->setMaximumWidth(250);
    m_tree->setIndentation(20);
    m_tree->setAnimated(true);
    m_tree->setExpandsOnDoubleClick(true);

    // Right side: Content area
    auto* contentWidget = new QWidget(splitter);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 0, 0, 0);
    contentLayout->setSpacing(10);

    // Page header
    auto* headerWidget = new QWidget(contentWidget);
    auto* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 10);
    headerLayout->setSpacing(2);

    m_pageTitle = new QLabel(headerWidget);
    m_pageTitle->setStyleSheet("font-size: 16px; font-weight: bold;");
    headerLayout->addWidget(m_pageTitle);

    m_pageDescription = new QLabel(headerWidget);
    m_pageDescription->setStyleSheet("color: gray;");
    m_pageDescription->setWordWrap(true);
    headerLayout->addWidget(m_pageDescription);

    contentLayout->addWidget(headerWidget);

    // Stacked widget for pages
    m_stack = new QStackedWidget(contentWidget);
    contentLayout->addWidget(m_stack, 1);

    // Set splitter sizes
    splitter->addWidget(m_tree);
    splitter->addWidget(contentWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 700});

    mainLayout->addWidget(splitter, 1);

    // Button box
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    mainLayout->addWidget(m_buttonBox);
}

void UnifiedPreferencesDialog::setupTree()
{
    // General group
    auto* generalGroup = new QTreeWidgetItem(m_tree, {tr("General")});
    generalGroup->setFlags(generalGroup->flags() & ~Qt::ItemIsSelectable);
    generalGroup->setExpanded(true);
    addPageItem(generalGroup, Page::Connection, tr("Connection"));
    addPageItem(generalGroup, Page::Logging, tr("Logging"));
    addPageItem(generalGroup, Page::Info, tr("Info"));

    // Appearance group
    auto* appearanceGroup = new QTreeWidgetItem(m_tree, {tr("Appearance")});
    appearanceGroup->setFlags(appearanceGroup->flags() & ~Qt::ItemIsSelectable);
    appearanceGroup->setExpanded(true);
    addPageItem(appearanceGroup, Page::Output, tr("Output"));
    addPageItem(appearanceGroup, Page::Colors, tr("Colors"));
    addPageItem(appearanceGroup, Page::MXP, tr("MXP / Pueblo"));

    // Automation group
    auto* automationGroup = new QTreeWidgetItem(m_tree, {tr("Automation")});
    automationGroup->setFlags(automationGroup->flags() & ~Qt::ItemIsSelectable);
    automationGroup->setExpanded(true);
    addPageItem(automationGroup, Page::Triggers, tr("Triggers"));
    addPageItem(automationGroup, Page::Aliases, tr("Aliases"));
    addPageItem(automationGroup, Page::Timers, tr("Timers"));
    addPageItem(automationGroup, Page::Macros, tr("Macros"));

    // Input group
    auto* inputGroup = new QTreeWidgetItem(m_tree, {tr("Input")});
    inputGroup->setFlags(inputGroup->flags() & ~Qt::ItemIsSelectable);
    inputGroup->setExpanded(true);
    addPageItem(inputGroup, Page::Commands, tr("Commands"));
    addPageItem(inputGroup, Page::Keypad, tr("Keypad"));
    addPageItem(inputGroup, Page::PasteSend, tr("Paste / Send"));

    // Scripting group
    auto* scriptingGroup = new QTreeWidgetItem(m_tree, {tr("Scripting")});
    scriptingGroup->setFlags(scriptingGroup->flags() & ~Qt::ItemIsSelectable);
    scriptingGroup->setExpanded(true);
    addPageItem(scriptingGroup, Page::Scripting, tr("Script File"));
    addPageItem(scriptingGroup, Page::Variables, tr("Variables"));
}

void UnifiedPreferencesDialog::addPageItem(QTreeWidgetItem* parent, Page page,
                                            const QString& label)
{
    auto* item = new QTreeWidgetItem(parent, {label});
    m_treeItems[page] = item;
    m_itemToPage[item] = page;
}

void UnifiedPreferencesDialog::setupPages()
{
    // Create stub pages for now - these will be replaced with real implementations
    auto addStubPage = [this](Page page, const QString& name, const QString& desc) {
        auto* stubPage = new StubPage(m_doc, name, desc, this);
        m_pages[page] = stubPage;
        m_stack->addWidget(stubPage);
    };

    // General pages
    addStubPage(Page::Connection, tr("Connection"),
                tr("Configure server address, port, and connection options."));
    addStubPage(Page::Logging, tr("Logging"),
                tr("Configure log file settings and automatic logging."));
    addStubPage(Page::Info, tr("Info"),
                tr("View and edit world information and notes."));

    // Appearance pages
    addStubPage(Page::Output, tr("Output"),
                tr("Configure output window appearance, fonts, and colors."));
    addStubPage(Page::Colors, tr("Colors"),
                tr("Configure ANSI and custom color mappings."));
    addStubPage(Page::MXP, tr("MXP / Pueblo"),
                tr("Configure MXP and Pueblo protocol settings."));

    // Automation pages - using real implementations
    auto* triggersPage = new TriggersPage(m_doc, this);
    m_pages[Page::Triggers] = triggersPage;
    m_stack->addWidget(triggersPage);

    auto* aliasesPage = new AliasesPage(m_doc, this);
    m_pages[Page::Aliases] = aliasesPage;
    m_stack->addWidget(aliasesPage);

    auto* timersPage = new TimersPage(m_doc, this);
    m_pages[Page::Timers] = timersPage;
    m_stack->addWidget(timersPage);

    addStubPage(Page::Macros, tr("Macros"),
                tr("Manage keyboard macros and accelerators."));

    // Input pages
    addStubPage(Page::Commands, tr("Commands"),
                tr("Configure command input behavior and history."));
    addStubPage(Page::Keypad, tr("Keypad"),
                tr("Configure numeric keypad for speedwalking."));
    addStubPage(Page::PasteSend, tr("Paste / Send"),
                tr("Configure paste and send file options."));

    // Scripting pages
    addStubPage(Page::Scripting, tr("Script File"),
                tr("Configure script file and scripting language."));
    addStubPage(Page::Variables, tr("Variables"),
                tr("View and manage script variables."));
}

void UnifiedPreferencesDialog::connectSignals()
{
    connect(m_tree, &QTreeWidget::itemClicked, this,
            &UnifiedPreferencesDialog::onTreeItemClicked);
    connect(m_tree, &QTreeWidget::itemActivated, this,
            &UnifiedPreferencesDialog::onTreeItemClicked);

    connect(m_buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &UnifiedPreferencesDialog::onOkClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &UnifiedPreferencesDialog::onCancelClicked);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &UnifiedPreferencesDialog::onApplyClicked);
}

void UnifiedPreferencesDialog::setCurrentPage(Page page)
{
    if (!m_pages.contains(page)) {
        return;
    }

    m_currentPage = page;

    // Update tree selection
    selectTreeItem(page);

    // Switch to page
    PreferencesPageBase* pageWidget = m_pages[page];
    m_stack->setCurrentWidget(pageWidget);

    // Update header
    m_pageTitle->setText(pageWidget->pageName());
    m_pageDescription->setText(pageWidget->pageDescription());

    // Load settings for the page
    pageWidget->loadSettings();
}

UnifiedPreferencesDialog::Page UnifiedPreferencesDialog::currentPage() const
{
    return m_currentPage;
}

void UnifiedPreferencesDialog::selectTreeItem(Page page)
{
    if (m_treeItems.contains(page)) {
        m_tree->blockSignals(true);
        m_tree->setCurrentItem(m_treeItems[page]);
        m_tree->blockSignals(false);
    }
}

void UnifiedPreferencesDialog::onTreeItemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (m_itemToPage.contains(item)) {
        setCurrentPage(m_itemToPage[item]);
    }
}

void UnifiedPreferencesDialog::onApplyClicked()
{
    saveAllPages();
    m_hasChanges = false;
    updateApplyButton();
}

void UnifiedPreferencesDialog::onOkClicked()
{
    saveAllPages();
    accept();
}

void UnifiedPreferencesDialog::onCancelClicked()
{
    if (m_hasChanges) {
        auto result = QMessageBox::question(
            this, tr("Unsaved Changes"),
            tr("You have unsaved changes. Are you sure you want to cancel?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }
    }
    reject();
}

void UnifiedPreferencesDialog::onPageSettingsChanged()
{
    m_hasChanges = true;
    updateApplyButton();
}

void UnifiedPreferencesDialog::updateApplyButton()
{
    m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(m_hasChanges);
}

void UnifiedPreferencesDialog::saveAllPages()
{
    for (auto* page : m_pages) {
        if (page->hasChanges()) {
            page->saveSettings();
        }
    }
}
