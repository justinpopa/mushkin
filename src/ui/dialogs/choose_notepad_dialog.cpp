#include "choose_notepad_dialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

ChooseNotepadDialog::ChooseNotepadDialog(const QString& title, const QStringList& notepadNames,
                                         QWidget* parent)
    : QDialog(parent), m_title(title), m_notepadNames(notepadNames)
{
    setWindowTitle(m_title);
    setModal(true);
    setupUi();
}

void ChooseNotepadDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Instruction label
    QLabel* instructionLabel = new QLabel("Select a notepad:", this);
    instructionLabel->setWordWrap(true);
    mainLayout->addWidget(instructionLabel);

    // List widget with notepad names
    m_listWidget = new QListWidget(this);
    m_listWidget->addItems(m_notepadNames);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set first item as default selection if available
    if (!m_notepadNames.isEmpty()) {
        m_listWidget->setCurrentRow(0);
    }

    // Double-click to accept
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
            &ChooseNotepadDialog::onItemDoubleClicked);

    mainLayout->addWidget(m_listWidget);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus on list widget
    m_listWidget->setFocus();

    // Set minimum size
    setMinimumSize(300, 400);
    adjustSize();
}

QString ChooseNotepadDialog::selectedNotepad() const
{
    QListWidgetItem* item = m_listWidget->currentItem();
    return item ? item->text() : QString();
}

void ChooseNotepadDialog::onItemDoubleClicked()
{
    accept();
}
