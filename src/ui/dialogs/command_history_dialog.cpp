#include "command_history_dialog.h"
#include "../../storage/global_options.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QTextStream>
#include <QVBoxLayout>

CommandHistoryDialog::CommandHistoryDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_commandList(nullptr)
    , m_filterEdit(nullptr)
    , m_sendButton(nullptr)
    , m_editButton(nullptr)
    , m_deleteButton(nullptr)
    , m_clearButton(nullptr)
    , m_saveButton(nullptr)
    , m_buttonBox(nullptr)
{
    setupUi();
    setupConnections();
    populateList();
    updateButtons();
}

CommandHistoryDialog::~CommandHistoryDialog()
{
}

void CommandHistoryDialog::setupUi()
{
    setWindowTitle("Command History");
    resize(600, 500);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Filter section
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QLabel* filterLabel = new QLabel("Filter:", this);
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Search commands...");
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(m_filterEdit);
    mainLayout->addLayout(filterLayout);

    // Command list
    m_commandList = new QListWidget(this);
    m_commandList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commandList->setAlternatingRowColors(true);
    mainLayout->addWidget(m_commandList);

    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setToolTip("Send command to MUD (Enter)");
    m_editButton = new QPushButton("Edit", this);
    m_editButton->setToolTip("Edit command before sending (F2)");
    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setToolTip("Remove command from history (Delete)");
    m_clearButton = new QPushButton("Clear All", this);
    m_clearButton->setToolTip("Empty entire history");
    m_saveButton = new QPushButton("Save to File...", this);
    m_saveButton->setToolTip("Export history to text file");

    actionLayout->addWidget(m_sendButton);
    actionLayout->addWidget(m_editButton);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addWidget(m_clearButton);
    actionLayout->addWidget(m_saveButton);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mainLayout->addWidget(m_buttonBox);

    // Keyboard shortcuts
    QShortcut* sendShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(sendShortcut, &QShortcut::activated, this, &CommandHistoryDialog::sendCommand);

    QShortcut* deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(deleteShortcut, &QShortcut::activated, this, &CommandHistoryDialog::deleteCommand);

    QShortcut* editShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(editShortcut, &QShortcut::activated, this, &CommandHistoryDialog::editCommand);
}

void CommandHistoryDialog::setupConnections()
{
    // Button connections
    connect(m_sendButton, &QPushButton::clicked, this, &CommandHistoryDialog::sendCommand);
    connect(m_editButton, &QPushButton::clicked, this, &CommandHistoryDialog::editCommand);
    connect(m_deleteButton, &QPushButton::clicked, this, &CommandHistoryDialog::deleteCommand);
    connect(m_clearButton, &QPushButton::clicked, this, &CommandHistoryDialog::clearAll);
    connect(m_saveButton, &QPushButton::clicked, this, &CommandHistoryDialog::saveToFile);

    // Filter connections
    connect(m_filterEdit, &QLineEdit::textChanged, this, &CommandHistoryDialog::filterChanged);

    // List connections
    connect(m_commandList, &QListWidget::itemDoubleClicked, this, &CommandHistoryDialog::commandDoubleClicked);
    connect(m_commandList, &QListWidget::itemSelectionChanged, this, &CommandHistoryDialog::selectionChanged);

    // Dialog buttons
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CommandHistoryDialog::populateList()
{
    if (!m_doc)
        return;

    m_commandList->clear();

    QString filter = m_filterEdit->text().toLower();

    // Add commands from history
    for (const QString& cmd : m_doc->m_commandHistory) {
        // Apply filter
        if (!filter.isEmpty() && !cmd.toLower().contains(filter))
            continue;

        m_commandList->addItem(cmd);
    }

    // Update window title with count
    int totalCount = m_doc->m_commandHistory.count();
    int displayedCount = m_commandList->count();

    if (filter.isEmpty()) {
        setWindowTitle(QString("Command History (%1 commands)").arg(totalCount));
    } else {
        setWindowTitle(QString("Command History (%1 of %2 commands)").arg(displayedCount).arg(totalCount));
    }
}

void CommandHistoryDialog::updateButtons()
{
    bool hasSelection = m_commandList->currentItem() != nullptr;
    bool hasCommands = m_commandList->count() > 0;

    m_sendButton->setEnabled(hasSelection);
    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    m_clearButton->setEnabled(hasCommands);
    m_saveButton->setEnabled(hasCommands);
}

void CommandHistoryDialog::sendCommand()
{
    QListWidgetItem* item = m_commandList->currentItem();
    if (!item || !m_doc)
        return;

    QString command = item->text();

    // Send to MUD
    m_doc->sendToMud(command);

    // Add to history (will move to end)
    m_doc->addToCommandHistory(command);

    // Refresh list
    populateList();
    updateButtons();
}

void CommandHistoryDialog::editCommand()
{
    QListWidgetItem* item = m_commandList->currentItem();
    if (!item || !m_doc)
        return;

    QString command = item->text();

    // Show input dialog
    bool ok;
    QString edited = QInputDialog::getText(
        this,
        "Edit Command",
        "Command:",
        QLineEdit::Normal,
        command,
        &ok
    );

    if (ok && !edited.isEmpty()) {
        // Find the actual index in the full history list
        int row = m_commandList->row(item);

        // If we're filtering, we need to find the real index
        if (!m_filterEdit->text().isEmpty()) {
            // Find the command in the original history
            int realIndex = m_doc->m_commandHistory.indexOf(command);
            if (realIndex >= 0) {
                m_doc->m_commandHistory[realIndex] = edited;
            }
        } else {
            // No filter, row index matches
            if (row >= 0 && row < m_doc->m_commandHistory.count()) {
                m_doc->m_commandHistory[row] = edited;
            }
        }

        // Refresh list
        populateList();
    }
}

void CommandHistoryDialog::deleteCommand()
{
    QListWidgetItem* item = m_commandList->currentItem();
    if (!item || !m_doc)
        return;

    QString command = item->text();

    // Remove from history (find and remove by value)
    int index = m_doc->m_commandHistory.indexOf(command);
    if (index >= 0) {
        m_doc->m_commandHistory.removeAt(index);
    }

    // Reset history position
    m_doc->m_historyPosition = m_doc->m_commandHistory.count();

    // Refresh list
    populateList();
    updateButtons();
}

void CommandHistoryDialog::clearAll()
{
    if (!m_doc)
        return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear History",
        "Are you sure you want to clear all command history?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_doc->m_commandHistory.clear();
        m_doc->m_historyPosition = 0;
        m_doc->m_iHistoryStatus = eAtBottom;

        // Refresh list
        populateList();
        updateButtons();
    }
}

void CommandHistoryDialog::filterChanged(const QString& filter)
{
    Q_UNUSED(filter);
    populateList();
    updateButtons();
}

void CommandHistoryDialog::saveToFile()
{
    if (!m_doc)
        return;

    // Use configured log directory (matches original MUSHclient behavior)
    QString logDir = GlobalOptions::instance()->defaultLogFileDirectory();
    QString filename = QFileDialog::getSaveFileName(
        this, "Save Command History", logDir + "/command_history.txt",
        "Text Files (*.txt);;All Files (*)");

    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open file for writing");
        return;
    }

    QTextStream out(&file);
    for (const QString& cmd : m_doc->m_commandHistory) {
        out << cmd << "\n";
    }

    file.close();
    QMessageBox::information(this, "Saved", "Command history saved to " + filename);
}

void CommandHistoryDialog::commandDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        sendCommand();
    }
}

void CommandHistoryDialog::selectionChanged()
{
    updateButtons();
}
