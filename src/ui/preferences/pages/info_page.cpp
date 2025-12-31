#include "info_page.h"
#include "world/world_document.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

InfoPage::InfoPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void InfoPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // World identification section
    QGroupBox* idGroup = new QGroupBox(tr("World Identification"), this);
    QFormLayout* idLayout = new QFormLayout(idGroup);
    idLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_worldNameEdit = new QLineEdit(this);
    m_worldNameEdit->setPlaceholderText(tr("Enter world name"));
    connect(m_worldNameEdit, &QLineEdit::textChanged, this, &InfoPage::markChanged);
    idLayout->addRow(tr("World name:"), m_worldNameEdit);

    m_worldIdLabel = new QLabel(this);
    m_worldIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_worldIdLabel->setStyleSheet("color: gray;");
    idLayout->addRow(tr("World ID:"), m_worldIdLabel);

    m_filePathLabel = new QLabel(this);
    m_filePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_filePathLabel->setWordWrap(true);
    m_filePathLabel->setStyleSheet("color: gray;");
    idLayout->addRow(tr("File path:"), m_filePathLabel);

    mainLayout->addWidget(idGroup);

    // Notes section
    QGroupBox* notesGroup = new QGroupBox(tr("Notes"), this);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setPlaceholderText(tr("Enter notes about this world..."));
    m_notesEdit->setAcceptRichText(false);
    connect(m_notesEdit, &QTextEdit::textChanged, this, &InfoPage::markChanged);
    notesLayout->addWidget(m_notesEdit);

    mainLayout->addWidget(notesGroup, 1);
}

void InfoPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_worldNameEdit->blockSignals(true);
    m_notesEdit->blockSignals(true);

    m_worldNameEdit->setText(m_doc->worldName());
    m_worldIdLabel->setText(m_doc->m_strWorldID);
    m_filePathLabel->setText(m_doc->m_strWorldFilePath.isEmpty()
                             ? tr("(not saved)")
                             : m_doc->m_strWorldFilePath);
    m_notesEdit->setPlainText(m_doc->m_notes);

    // Unblock signals
    m_worldNameEdit->blockSignals(false);
    m_notesEdit->blockSignals(false);

    m_hasChanges = false;
}

void InfoPage::saveSettings()
{
    if (!m_doc)
        return;

    m_doc->setWorldName(m_worldNameEdit->text());
    m_doc->m_notes = m_notesEdit->toPlainText();

    m_doc->setModified(true);

    m_hasChanges = false;
}

bool InfoPage::hasChanges() const
{
    return m_hasChanges;
}

void InfoPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
