#include "map_move_dialog.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

MapMoveDialog::MapMoveDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Special Move");
    setupUi();
}

void MapMoveDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for action and reverse inputs
    QFormLayout* formLayout = new QFormLayout();

    m_actionEdit = new QLineEdit(this);
    m_actionEdit->setPlaceholderText("e.g., climb tree");
    formLayout->addRow("Action:", m_actionEdit);

    m_reverseEdit = new QLineEdit(this);
    m_reverseEdit->setPlaceholderText("e.g., climb down");
    formLayout->addRow("Reverse:", m_reverseEdit);

    mainLayout->addLayout(formLayout);

    // Send to MUD checkbox
    m_sendToMudCheckBox = new QCheckBox("Send to MUD", this);
    m_sendToMudCheckBox->setChecked(true);
    mainLayout->addWidget(m_sendToMudCheckBox);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false); // Initially disabled until action is entered

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Connect text change signal to enable/disable OK button
    connect(m_actionEdit, &QLineEdit::textChanged, this, &MapMoveDialog::onActionTextChanged);

    // Set minimum width
    setMinimumWidth(350);

    // Set focus to the action edit
    m_actionEdit->setFocus();
}

QString MapMoveDialog::action() const
{
    return m_actionEdit->text();
}

void MapMoveDialog::setAction(const QString& action)
{
    m_actionEdit->setText(action);
}

QString MapMoveDialog::reverse() const
{
    return m_reverseEdit->text();
}

void MapMoveDialog::setReverse(const QString& reverse)
{
    m_reverseEdit->setText(reverse);
}

bool MapMoveDialog::sendToMud() const
{
    return m_sendToMudCheckBox->isChecked();
}

void MapMoveDialog::setSendToMud(bool send)
{
    m_sendToMudCheckBox->setChecked(send);
}

void MapMoveDialog::onActionTextChanged(const QString& text)
{
    // Enable OK button only when action is non-empty
    m_okButton->setEnabled(!text.trimmed().isEmpty());
}
