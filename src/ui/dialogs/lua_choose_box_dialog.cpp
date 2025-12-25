#include "lua_choose_box_dialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

LuaChooseBoxDialog::LuaChooseBoxDialog(const QString& title, const QString& message,
                                       const QStringList& choices, int defaultIndex,
                                       QWidget* parent)
    : QDialog(parent), m_title(title), m_message(message), m_choices(choices),
      m_defaultIndex(defaultIndex)
{
    setWindowTitle(m_title);
    setModal(true);
    setupUi();
}

void LuaChooseBoxDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Message label
    m_messageLabel = new QLabel(m_message, this);
    m_messageLabel->setWordWrap(true);
    mainLayout->addWidget(m_messageLabel);

    // Combo box with choices
    m_choiceCombo = new QComboBox(this);
    m_choiceCombo->addItems(m_choices);

    // Set default selection if valid
    if (m_defaultIndex >= 0 && m_defaultIndex < m_choices.size()) {
        m_choiceCombo->setCurrentIndex(m_defaultIndex);
    }

    mainLayout->addWidget(m_choiceCombo);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on combo box
    m_choiceCombo->setFocus();

    // Set minimum width
    setMinimumWidth(300);
    adjustSize();
}

int LuaChooseBoxDialog::selectedIndex() const
{
    return m_choiceCombo->currentIndex();
}

QString LuaChooseBoxDialog::selectedText() const
{
    return m_choiceCombo->currentText();
}
