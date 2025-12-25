#include "variable_edit_dialog.h"
#include "../../automation/variable.h"
#include "../../world/world_document.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// Constructor for adding new variable
VariableEditDialog::VariableEditDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_variableName(), m_isEditMode(false)
{
    setWindowTitle("Add Variable - " + doc->m_mush_name);
    resize(500, 200);
    setupUi();
}

// Constructor for editing existing variable
VariableEditDialog::VariableEditDialog(WorldDocument* doc, const QString& variableName,
                                       QWidget* parent)
    : QDialog(parent), m_doc(doc), m_variableName(variableName), m_isEditMode(true)
{
    setWindowTitle("Edit Variable - " + doc->m_mush_name);
    resize(500, 200);
    setupUi();
    loadVariableData();
}

void VariableEditDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create form layout
    QFormLayout* formLayout = new QFormLayout();

    // Variable name
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Variable name (required)");
    m_nameEdit->setMaxLength(1000);
    formLayout->addRow("&Variable name:", m_nameEdit);

    // Variable contents with Edit button
    QHBoxLayout* contentsLayout = new QHBoxLayout();
    m_contentsEdit = new QLineEdit(this);
    m_contentsEdit->setPlaceholderText("Variable value");
    contentsLayout->addWidget(m_contentsEdit);

    m_editContentsButton = new QPushButton("&Edit...", this);
    m_editContentsButton->setToolTip("Open multi-line editor for variable contents");
    connect(m_editContentsButton, &QPushButton::clicked, this, &VariableEditDialog::onEditContents);
    contentsLayout->addWidget(m_editContentsButton);

    formLayout->addRow("&Contents:", contentsLayout);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // Button box
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &VariableEditDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &VariableEditDialog::onCancel);
    mainLayout->addWidget(buttonBox);

    // Set focus
    if (m_isEditMode) {
        // When editing, focus on contents and select all
        m_contentsEdit->setFocus();
        m_contentsEdit->selectAll();
    } else {
        // When adding, focus on name field
        m_nameEdit->setFocus();
    }
}

void VariableEditDialog::loadVariableData()
{
    if (!m_isEditMode || m_variableName.isEmpty()) {
        return;
    }

    // Get variable value
    QString value = m_doc->getVariable(m_variableName);

    // Load data into form
    m_nameEdit->setText(m_variableName);
    m_contentsEdit->setText(value);

    // Disable name editing when editing existing variable
    m_nameEdit->setEnabled(false);
}

bool VariableEditDialog::checkLabel(const QString& name)
{
    // Variable name must start with a letter and consist of letters, numbers, or underscore
    if (name.isEmpty()) {
        return true; // Invalid
    }

    // First character must be a letter
    if (!name[0].isLetter()) {
        return true; // Invalid
    }

    // Rest must be letters, numbers, or underscore
    for (int i = 1; i < name.length(); ++i) {
        QChar c = name[i];
        if (!c.isLetterOrNumber() && c != '_') {
            return true; // Invalid
        }
    }

    return false; // Valid
}

bool VariableEditDialog::validateForm()
{
    // Get trimmed name
    QString name = m_nameEdit->text().trimmed();

    // Name is required
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
                             "Variable name is required.\n\nPlease enter a variable name.");
        m_nameEdit->setFocus();
        return false;
    }

    // Check name length (1-1000 characters)
    if (name.length() > 1000) {
        QMessageBox::warning(this, "Validation Error",
                             "Variable name is too long.\n\nMaximum length is 1000 characters.");
        m_nameEdit->setFocus();
        return false;
    }

    // Validate name format
    if (checkLabel(name)) {
        QMessageBox::warning(this, "Validation Error",
                             "The variable name must start with a letter and consist of letters, "
                             "numbers or the underscore character.");
        m_nameEdit->setFocus();
        return false;
    }

    // Check for duplicate names (only when adding new variable)
    if (!m_isEditMode) {
        QString existingValue = m_doc->getVariable(name);
        if (!existingValue.isNull()) {
            QMessageBox::warning(this, "Validation Error",
                                 "This variable name is already in the list of variables.");
            m_nameEdit->setFocus();
            return false;
        }
    }

    return true;
}

bool VariableEditDialog::saveVariable()
{
    // Get trimmed values
    QString name = m_nameEdit->text().trimmed();
    QString contents = m_contentsEdit->text();

    // Set the variable in the document
    qint32 result = m_doc->setVariable(name, contents);

    if (result != 0) {
        QMessageBox::warning(this, "Error", "Failed to save variable.");
        return false;
    }

    return true;
}

void VariableEditDialog::onOk()
{
    if (!validateForm()) {
        return;
    }

    if (saveVariable()) {
        accept();
    }
}

void VariableEditDialog::onCancel()
{
    reject();
}

void VariableEditDialog::onEditContents()
{
    // Get current contents
    QString currentText = m_contentsEdit->text();

    // Determine dialog title
    QString title;
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        title = "Edit variable";
    } else {
        title = QString("Edit variable '%1'").arg(name);
    }

    // Show multi-line input dialog
    bool ok;
    QString text =
        QInputDialog::getMultiLineText(this, title, "Variable contents:", currentText, &ok);

    if (ok) {
        m_contentsEdit->setText(text);
    }
}
