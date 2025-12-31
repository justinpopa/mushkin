#include "keypad_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>

KeypadPage::KeypadPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void KeypadPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Enable checkbox
    m_enableCheck = new QCheckBox(tr("Enable numeric keypad commands"), this);
    connect(m_enableCheck, &QCheckBox::toggled, this, &KeypadPage::markChanged);
    mainLayout->addWidget(m_enableCheck);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("Configure commands sent when numeric keypad keys are pressed. "
           "Use with Num Lock off for directional movement."),
        this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(helpLabel);

    // Keypad layout in a scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollWidget = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);

    // Standard keypad keys layout
    // Keys: 0-9, /, *, -, +, Enter, .
    // With modifiers: None, Shift, Ctrl
    const char* keyNames[] = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "/", "*", "-", "+", "Enter", ".",
        "Shift+0", "Shift+1", "Shift+2", "Shift+3", "Shift+4",
        "Shift+5", "Shift+6", "Shift+7", "Shift+8", "Shift+9",
        "Ctrl+0", "Ctrl+1", "Ctrl+2", "Ctrl+3"
    };

    // Create groups for organization
    QGroupBox* basicGroup = new QGroupBox(tr("Basic Keys (Num Lock Off)"), scrollWidget);
    QGridLayout* basicLayout = new QGridLayout(basicGroup);

    // Numpad visual layout: 7 8 9 / * -
    //                       4 5 6 + (spans 2 rows)
    //                       1 2 3 Enter (spans 2 rows)
    //                       0 (spans 2) .
    // Create a grid that represents this

    // Row 0: 7, 8, 9
    for (int i = 7; i <= 9; i++) {
        QLabel* lbl = new QLabel(keyNames[i], basicGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        basicLayout->addWidget(lbl, 0, (i - 7) * 2);
        m_keypadEdits[i] = new QLineEdit(basicGroup);
        connect(m_keypadEdits[i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        basicLayout->addWidget(m_keypadEdits[i], 0, (i - 7) * 2 + 1);
    }

    // Row 1: 4, 5, 6
    for (int i = 4; i <= 6; i++) {
        QLabel* lbl = new QLabel(keyNames[i], basicGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        basicLayout->addWidget(lbl, 1, (i - 4) * 2);
        m_keypadEdits[i] = new QLineEdit(basicGroup);
        connect(m_keypadEdits[i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        basicLayout->addWidget(m_keypadEdits[i], 1, (i - 4) * 2 + 1);
    }

    // Row 2: 1, 2, 3
    for (int i = 1; i <= 3; i++) {
        QLabel* lbl = new QLabel(keyNames[i], basicGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        basicLayout->addWidget(lbl, 2, (i - 1) * 2);
        m_keypadEdits[i] = new QLineEdit(basicGroup);
        connect(m_keypadEdits[i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        basicLayout->addWidget(m_keypadEdits[i], 2, (i - 1) * 2 + 1);
    }

    // Row 3: 0, .
    {
        QLabel* lbl = new QLabel(keyNames[0], basicGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        basicLayout->addWidget(lbl, 3, 0);
        m_keypadEdits[0] = new QLineEdit(basicGroup);
        connect(m_keypadEdits[0], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        basicLayout->addWidget(m_keypadEdits[0], 3, 1, 1, 3);

        QLabel* dotLbl = new QLabel(keyNames[15], basicGroup);
        dotLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        basicLayout->addWidget(dotLbl, 3, 4);
        m_keypadEdits[15] = new QLineEdit(basicGroup);
        connect(m_keypadEdits[15], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        basicLayout->addWidget(m_keypadEdits[15], 3, 5);
    }

    scrollLayout->addWidget(basicGroup);

    // Operator keys group
    QGroupBox* opGroup = new QGroupBox(tr("Operator Keys"), scrollWidget);
    QGridLayout* opLayout = new QGridLayout(opGroup);

    // /, *, -, +, Enter (indices 10-14)
    const char* opNames[] = {"/", "*", "-", "+", "Enter"};
    for (int i = 0; i < 5; i++) {
        QLabel* lbl = new QLabel(opNames[i], opGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        opLayout->addWidget(lbl, i / 3, (i % 3) * 2);
        m_keypadEdits[10 + i] = new QLineEdit(opGroup);
        connect(m_keypadEdits[10 + i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        opLayout->addWidget(m_keypadEdits[10 + i], i / 3, (i % 3) * 2 + 1);
    }

    scrollLayout->addWidget(opGroup);

    // Shift keys group
    QGroupBox* shiftGroup = new QGroupBox(tr("Shift + Keypad"), scrollWidget);
    QGridLayout* shiftLayout = new QGridLayout(shiftGroup);

    for (int i = 0; i < 10; i++) {
        QString label = QString("Shift+%1").arg(i);
        QLabel* lbl = new QLabel(label, shiftGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        shiftLayout->addWidget(lbl, i / 5, (i % 5) * 2);
        m_keypadEdits[16 + i] = new QLineEdit(shiftGroup);
        connect(m_keypadEdits[16 + i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        shiftLayout->addWidget(m_keypadEdits[16 + i], i / 5, (i % 5) * 2 + 1);
    }

    scrollLayout->addWidget(shiftGroup);

    // Ctrl keys group
    QGroupBox* ctrlGroup = new QGroupBox(tr("Ctrl + Keypad"), scrollWidget);
    QGridLayout* ctrlLayout = new QGridLayout(ctrlGroup);

    for (int i = 0; i < 4; i++) {
        QString label = QString("Ctrl+%1").arg(i);
        QLabel* lbl = new QLabel(label, ctrlGroup);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ctrlLayout->addWidget(lbl, 0, i * 2);
        m_keypadEdits[26 + i] = new QLineEdit(ctrlGroup);
        connect(m_keypadEdits[26 + i], &QLineEdit::textChanged, this, &KeypadPage::markChanged);
        ctrlLayout->addWidget(m_keypadEdits[26 + i], 0, i * 2 + 1);
    }

    scrollLayout->addWidget(ctrlGroup);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea, 1);
}

void KeypadPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_enableCheck->blockSignals(true);
    for (int i = 0; i < KEYPAD_MAX_ITEMS; i++) {
        if (m_keypadEdits[i])
            m_keypadEdits[i]->blockSignals(true);
    }

    m_enableCheck->setChecked(m_doc->m_keypad_enable != 0);

    for (int i = 0; i < KEYPAD_MAX_ITEMS; i++) {
        if (m_keypadEdits[i])
            m_keypadEdits[i]->setText(m_doc->m_keypad[i]);
    }

    // Unblock signals
    m_enableCheck->blockSignals(false);
    for (int i = 0; i < KEYPAD_MAX_ITEMS; i++) {
        if (m_keypadEdits[i])
            m_keypadEdits[i]->blockSignals(false);
    }

    m_hasChanges = false;
}

void KeypadPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->m_keypad_enable = m_enableCheck->isChecked() ? 1 : 0;

    for (int i = 0; i < KEYPAD_MAX_ITEMS; i++) {
        if (m_keypadEdits[i])
            m_doc->m_keypad[i] = m_keypadEdits[i]->text();
    }

    m_doc->setModified(true);
    m_hasChanges = false;
}

bool KeypadPage::hasChanges() const
{
    return m_hasChanges;
}

void KeypadPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
