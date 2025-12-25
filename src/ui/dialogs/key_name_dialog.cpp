#include "key_name_dialog.h"
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

KeyNameDialog::KeyNameDialog(QWidget* parent)
    : QDialog(parent), m_keyCode(0), m_modifiers(Qt::NoModifier)
{
    setWindowTitle("Press a Key");
    setupUi();
}

void KeyNameDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Instruction label
    m_instructionLabel = new QLabel("Press a key...", this);
    mainLayout->addWidget(m_instructionLabel);

    // Key name display (read-only)
    m_keyNameEdit = new QLineEdit(this);
    m_keyNameEdit->setReadOnly(true);
    m_keyNameEdit->setPlaceholderText("No key pressed");
    m_keyNameEdit->setMinimumWidth(300);
    mainLayout->addWidget(m_keyNameEdit);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setMinimumWidth(300);
}

void KeyNameDialog::keyPressEvent(QKeyEvent* event)
{
    // Ignore modifier-only key presses
    if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control ||
        event->key() == Qt::Key_Alt || event->key() == Qt::Key_Meta) {
        QDialog::keyPressEvent(event);
        return;
    }

    // Capture key information
    m_keyCode = event->key();
    m_modifiers = event->modifiers();

    // Create a QKeySequence from the key event and convert to string
    int combined = m_keyCode;
    if (m_modifiers & Qt::ShiftModifier)
        combined |= Qt::SHIFT;
    if (m_modifiers & Qt::ControlModifier)
        combined |= Qt::CTRL;
    if (m_modifiers & Qt::AltModifier)
        combined |= Qt::ALT;
    if (m_modifiers & Qt::MetaModifier)
        combined |= Qt::META;

    QKeySequence keySequence(combined);
    m_keyName = keySequence.toString(QKeySequence::NativeText);

    // Display the key name
    m_keyNameEdit->setText(m_keyName);

    // Don't propagate Escape or Enter to avoid closing the dialog unintentionally
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter) {
        event->accept();
        return;
    }

    QDialog::keyPressEvent(event);
}

QString KeyNameDialog::keyName() const
{
    return m_keyName;
}

int KeyNameDialog::keyCode() const
{
    return m_keyCode;
}

Qt::KeyboardModifiers KeyNameDialog::modifiers() const
{
    return m_modifiers;
}
