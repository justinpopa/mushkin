#include "map_dialog.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

MapDialog::MapDialog(WorldDocument* doc, QWidget* parent) : QDialog(parent), m_doc(doc)
{
    setWindowTitle("Mapper Configuration");
    setMinimumSize(500, 450);
    setupUi();
    loadSettings();
    updateDirectionDisplays();
}

void MapDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Enable mapper checkbox
    m_enableMapperCheck = new QCheckBox("&Enable mapper", this);
    m_enableMapperCheck->setToolTip("Enable the automapper feature");
    mainLayout->addWidget(m_enableMapperCheck);

    // Remove reverse directions checkbox
    m_removeMapReversesCheck = new QCheckBox("&Remove reverse directions automatically", this);
    m_removeMapReversesCheck->setToolTip("Automatically remove reverse direction when mapping");
    mainLayout->addWidget(m_removeMapReversesCheck);

    // Failure detection group
    QGroupBox* failureGroup = new QGroupBox("Failure Detection", this);
    QVBoxLayout* failureLayout = new QVBoxLayout(failureGroup);

    // Failure pattern input
    QHBoxLayout* patternLayout = new QHBoxLayout();
    QLabel* patternLabel = new QLabel("&Failure pattern:", this);
    m_failurePatternEdit = new QLineEdit(this);
    m_failurePatternEdit->setToolTip("Text pattern indicating movement failure");
    patternLabel->setBuddy(m_failurePatternEdit);
    patternLayout->addWidget(patternLabel);
    patternLayout->addWidget(m_failurePatternEdit, 1);
    failureLayout->addLayout(patternLayout);

    // Regex checkbox and convert button
    QHBoxLayout* regexLayout = new QHBoxLayout();
    m_failureRegexpCheck = new QCheckBox("&Regular expression", this);
    m_failureRegexpCheck->setToolTip("Treat failure pattern as a regular expression");
    regexLayout->addWidget(m_failureRegexpCheck);

    m_convertToRegexpButton = new QPushButton("Convert to &Regexp", this);
    m_convertToRegexpButton->setToolTip(
        "Convert failure pattern to regular expression (placeholder)");
    regexLayout->addWidget(m_convertToRegexpButton);
    regexLayout->addStretch();

    failureLayout->addLayout(regexLayout);
    mainLayout->addWidget(failureGroup);

    // Directions display group
    QGroupBox* directionsGroup = new QGroupBox("Mapper Directions", this);
    QVBoxLayout* directionsLayout = new QVBoxLayout(directionsGroup);

    // Forwards directions
    QLabel* forwardsLabel = new QLabel("F&orwards:", this);
    m_forwardsDisplay = new QPlainTextEdit(this);
    m_forwardsDisplay->setReadOnly(true);
    m_forwardsDisplay->setMaximumHeight(80);
    m_forwardsDisplay->setToolTip("Forward directions recorded by the mapper");
    forwardsLabel->setBuddy(m_forwardsDisplay);
    directionsLayout->addWidget(forwardsLabel);
    directionsLayout->addWidget(m_forwardsDisplay);

    // Backwards directions
    QLabel* backwardsLabel = new QLabel("&Backwards:", this);
    m_backwardsDisplay = new QPlainTextEdit(this);
    m_backwardsDisplay->setReadOnly(true);
    m_backwardsDisplay->setMaximumHeight(80);
    m_backwardsDisplay->setToolTip("Reverse directions recorded by the mapper");
    backwardsLabel->setBuddy(m_backwardsDisplay);
    directionsLayout->addWidget(backwardsLabel);
    directionsLayout->addWidget(m_backwardsDisplay);

    mainLayout->addWidget(directionsGroup);

    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();

    m_removeAllButton = new QPushButton("Remove &All", this);
    m_removeAllButton->setToolTip("Remove all mapper data");
    actionLayout->addWidget(m_removeAllButton);

    m_removeLastButton = new QPushButton("Remove &Last", this);
    m_removeLastButton->setToolTip("Remove the last mapper entry");
    actionLayout->addWidget(m_removeLastButton);

    m_specialMoveButton = new QPushButton("&Special Move...", this);
    m_specialMoveButton->setToolTip("Add a special move to the mapper (placeholder)");
    actionLayout->addWidget(m_specialMoveButton);

    m_editButton = new QPushButton("&Edit...", this);
    m_editButton->setToolTip("Edit mapper data (placeholder)");
    actionLayout->addWidget(m_editButton);

    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MapDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MapDialog::onRejected);
    connect(m_removeAllButton, &QPushButton::clicked, this, &MapDialog::onRemoveAllClicked);
    connect(m_removeLastButton, &QPushButton::clicked, this, &MapDialog::onRemoveLastClicked);
    connect(m_specialMoveButton, &QPushButton::clicked, this, &MapDialog::onSpecialMoveClicked);
    connect(m_editButton, &QPushButton::clicked, this, &MapDialog::onEditClicked);
    connect(m_convertToRegexpButton, &QPushButton::clicked, this,
            &MapDialog::onConvertToRegexpClicked);
}

void MapDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Load mapper settings from WorldDocument
    // Note: m_bEnable is not currently in WorldDocument - this is a placeholder
    // In the original MFC code, this would be m_bEnable
    m_enableMapperCheck->setChecked(false); // TODO: Add m_bEnable to WorldDocument

    // Note: m_bRemoveMapReverses is not currently in WorldDocument - this is a placeholder
    m_removeMapReversesCheck->setChecked(false); // TODO: Add m_bRemoveMapReverses to WorldDocument

    // Failure detection settings
    m_failurePatternEdit->setText(m_doc->m_strMappingFailure);
    m_failureRegexpCheck->setChecked(m_doc->m_bMapFailureRegexp != 0);
}

void MapDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Save mapper settings to WorldDocument
    // TODO: Add m_bEnable to WorldDocument
    // m_doc->m_bEnable = m_enableMapperCheck->isChecked() ? 1 : 0;

    // TODO: Add m_bRemoveMapReverses to WorldDocument
    // m_doc->m_bRemoveMapReverses = m_removeMapReversesCheck->isChecked() ? 1 : 0;

    // Failure detection settings
    m_doc->m_strMappingFailure = m_failurePatternEdit->text();
    m_doc->m_bMapFailureRegexp = m_failureRegexpCheck->isChecked() ? 1 : 0;

    // Pack the flags back into m_iFlags1
    m_doc->packFlags();

    // Mark document as modified
    m_doc->setModified(true);
}

void MapDialog::updateDirectionDisplays()
{
    if (!m_doc)
        return;

    // Update forwards and backwards direction displays
    // These come from m_strSpecialForwards and m_strSpecialBackwards
    m_forwardsDisplay->setPlainText(m_doc->m_strSpecialForwards);
    m_backwardsDisplay->setPlainText(m_doc->m_strSpecialBackwards);
}

void MapDialog::onAccepted()
{
    saveSettings();
    accept();
}

void MapDialog::onRejected()
{
    reject();
}

void MapDialog::onRemoveAllClicked()
{
    if (!m_doc)
        return;

    // Confirm before removing all mapper data
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Remove All", "Are you sure you want to remove all mapper data?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Clear all mapper data
        m_doc->m_strSpecialForwards.clear();
        m_doc->m_strSpecialBackwards.clear();

        // TODO: Clear m_strMapList when implemented
        // m_doc->m_strMapList->clear();

        updateDirectionDisplays();

        QMessageBox::information(this, "Mapper", "All mapper data has been removed.");
    }
}

void MapDialog::onRemoveLastClicked()
{
    if (!m_doc)
        return;

    // TODO: Implement removing last mapper entry
    // This would require access to m_strMapList to remove the last entry
    QMessageBox::information(this, "Remove Last",
                             "This feature will remove the last mapper entry.\n\n"
                             "Implementation requires m_strMapList to be available.");
}

void MapDialog::onSpecialMoveClicked()
{
    // TODO: Implement special move dialog
    // This would open a dialog to add custom movement commands
    QMessageBox::information(this, "Special Move",
                             "This feature will allow you to add special move commands.\n\n"
                             "Implementation pending.");
}

void MapDialog::onEditClicked()
{
    // TODO: Implement mapper editor dialog
    // This would open a more advanced mapper editing interface
    QMessageBox::information(this, "Edit Mapper",
                             "This feature will open the mapper editor.\n\n"
                             "Implementation pending.");
}

void MapDialog::onConvertToRegexpClicked()
{
    // TODO: Implement conversion of text pattern to regex
    // This would help users convert simple text patterns to regex format
    QString currentPattern = m_failurePatternEdit->text();

    if (currentPattern.isEmpty()) {
        QMessageBox::information(this, "Convert to Regexp",
                                 "Please enter a failure pattern first.");
        return;
    }

    QMessageBox::information(
        this, "Convert to Regexp",
        "This feature will convert the failure pattern to a regular expression.\n\n"
        "Implementation pending.");
}
