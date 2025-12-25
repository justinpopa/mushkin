#include "import_xml_dialog.h"

#include "../../world/world_document.h"
#include "../../world/xml_serialization.h"

#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ImportXmlDialog::ImportXmlDialog(WorldDocument* doc, QWidget* parent) : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Import from XML");
    setMinimumSize(350, 400);
    setupUi();
}

int ImportXmlDialog::importFlags() const
{
    int flags = 0;
    if (m_general->isChecked())
        flags |= XML_GENERAL;
    if (m_triggers->isChecked())
        flags |= XML_TRIGGERS;
    if (m_aliases->isChecked())
        flags |= XML_ALIASES;
    if (m_timers->isChecked())
        flags |= XML_TIMERS;
    if (m_macros->isChecked())
        flags |= XML_MACROS;
    if (m_variables->isChecked())
        flags |= XML_VARIABLES;
    if (m_colours->isChecked())
        flags |= XML_COLOURS;
    if (m_keypad->isChecked())
        flags |= XML_KEYPAD;
    if (m_printing->isChecked())
        flags |= XML_PRINTING;
    return flags;
}

void ImportXmlDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Import selection group
    QGroupBox* selectionGroup = new QGroupBox("Select items to import", this);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);

    m_general = new QCheckBox("&General settings", this);
    m_general->setChecked(true);
    m_general->setToolTip("Import general world settings");
    selectionLayout->addWidget(m_general);

    m_triggers = new QCheckBox("&Triggers", this);
    m_triggers->setChecked(true);
    m_triggers->setToolTip("Import trigger definitions");
    selectionLayout->addWidget(m_triggers);

    m_aliases = new QCheckBox("&Aliases", this);
    m_aliases->setChecked(true);
    m_aliases->setToolTip("Import alias definitions");
    selectionLayout->addWidget(m_aliases);

    m_timers = new QCheckBox("Ti&mers", this);
    m_timers->setChecked(true);
    m_timers->setToolTip("Import timer definitions");
    selectionLayout->addWidget(m_timers);

    m_macros = new QCheckBox("&Macros", this);
    m_macros->setChecked(true);
    m_macros->setToolTip("Import macro definitions");
    selectionLayout->addWidget(m_macros);

    m_variables = new QCheckBox("&Variables", this);
    m_variables->setChecked(true);
    m_variables->setToolTip("Import variable values");
    selectionLayout->addWidget(m_variables);

    m_colours = new QCheckBox("&Colours", this);
    m_colours->setChecked(true);
    m_colours->setToolTip("Import color settings");
    selectionLayout->addWidget(m_colours);

    m_keypad = new QCheckBox("&Keypad settings", this);
    m_keypad->setChecked(true);
    m_keypad->setToolTip("Import keypad configuration");
    selectionLayout->addWidget(m_keypad);

    m_printing = new QCheckBox("&Printing settings", this);
    m_printing->setChecked(true);
    m_printing->setToolTip("Import printing configuration");
    selectionLayout->addWidget(m_printing);

    mainLayout->addWidget(selectionGroup);

    // Select All/None buttons
    QHBoxLayout* selectButtonsLayout = new QHBoxLayout();
    selectButtonsLayout->addStretch();

    QPushButton* selectAllButton = new QPushButton("Select &All", this);
    selectButtonsLayout->addWidget(selectAllButton);

    QPushButton* selectNoneButton = new QPushButton("Select &None", this);
    selectButtonsLayout->addWidget(selectNoneButton);

    selectButtonsLayout->addStretch();

    mainLayout->addLayout(selectButtonsLayout);

    // Add some spacing
    mainLayout->addSpacing(20);

    // Import buttons
    QVBoxLayout* importButtonsLayout = new QVBoxLayout();

    QPushButton* importFileButton = new QPushButton("Import from &File...", this);
    importFileButton->setDefault(true);
    importButtonsLayout->addWidget(importFileButton);

    QPushButton* importClipboardButton = new QPushButton("Import from Clip&board", this);
    importButtonsLayout->addWidget(importClipboardButton);

    mainLayout->addLayout(importButtonsLayout);

    mainLayout->addStretch();

    // Connect signals
    connect(selectAllButton, &QPushButton::clicked, this, &ImportXmlDialog::onSelectAll);
    connect(selectNoneButton, &QPushButton::clicked, this, &ImportXmlDialog::onSelectNone);
    connect(importFileButton, &QPushButton::clicked, this, &ImportXmlDialog::onImportFromFile);
    connect(importClipboardButton, &QPushButton::clicked, this,
            &ImportXmlDialog::onImportFromClipboard);
}

void ImportXmlDialog::onSelectAll()
{
    m_general->setChecked(true);
    m_triggers->setChecked(true);
    m_aliases->setChecked(true);
    m_timers->setChecked(true);
    m_macros->setChecked(true);
    m_variables->setChecked(true);
    m_colours->setChecked(true);
    m_keypad->setChecked(true);
    m_printing->setChecked(true);
}

void ImportXmlDialog::onSelectNone()
{
    m_general->setChecked(false);
    m_triggers->setChecked(false);
    m_aliases->setChecked(false);
    m_timers->setChecked(false);
    m_macros->setChecked(false);
    m_variables->setChecked(false);
    m_colours->setChecked(false);
    m_keypad->setChecked(false);
    m_printing->setChecked(false);
}

void ImportXmlDialog::onImportFromFile()
{
    if (!m_doc) {
        QMessageBox::warning(this, "Error", "No world document available for import.");
        return;
    }

    // Show file dialog
    QString filename = QFileDialog::getOpenFileName(this, "Import XML File", QString(),
                                                    "XML Files (*.xml);;All Files (*)");

    if (filename.isEmpty()) {
        return; // User cancelled
    }

    // Read file contents
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error",
                             QString("Could not open file: %1").arg(file.errorString()));
        return;
    }

    QString xmlContent = QString::fromUtf8(file.readAll());
    file.close();

    if (xmlContent.isEmpty()) {
        QMessageBox::warning(this, "Error", "File is empty.");
        return;
    }

    // Import the XML with selected flags
    int flags = importFlags();
    int count = XmlSerialization::ImportXML(m_doc, xmlContent, flags);

    if (count < 0) {
        QMessageBox::warning(this, "Import Failed",
                             "Failed to parse XML. Please check the file format.");
        return;
    }

    QMessageBox::information(this, "Import Complete",
                             QString("Successfully imported %1 item(s).").arg(count));
    accept();
}

void ImportXmlDialog::onImportFromClipboard()
{
    if (!m_doc) {
        QMessageBox::warning(this, "Error", "No world document available for import.");
        return;
    }

    // Get clipboard content
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString xmlContent = clipboard->text();

    if (xmlContent.isEmpty()) {
        QMessageBox::warning(this, "Error", "Clipboard is empty.");
        return;
    }

    // Import the XML with selected flags
    int flags = importFlags();
    int count = XmlSerialization::ImportXML(m_doc, xmlContent, flags);

    if (count < 0) {
        QMessageBox::warning(this, "Import Failed",
                             "Failed to parse XML from clipboard. Please check the format.");
        return;
    }

    QMessageBox::information(
        this, "Import Complete",
        QString("Successfully imported %1 item(s) from clipboard.").arg(count));
    accept();
}
