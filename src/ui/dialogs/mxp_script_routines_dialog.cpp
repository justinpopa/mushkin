#include "mxp_script_routines_dialog.h"
#include "../../world/world_document.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

MxpScriptRoutinesDialog::MxpScriptRoutinesDialog(WorldDocument* doc, QWidget* parent)
    : QDialog(parent), m_doc(doc)
{
    setWindowTitle("MXP Script Routines");
    setMinimumWidth(400);
    setupUi();
    loadSettings();
}

void MxpScriptRoutinesDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for main controls
    QFormLayout* formLayout = new QFormLayout();

    // On MXP Start
    m_onMxpStart = new QLineEdit(this);
    m_onMxpStart->setPlaceholderText("Script routine name");
    m_onMxpStart->setToolTip("Script to call when MXP starts");
    formLayout->addRow("On MXP &Start:", m_onMxpStart);

    // On MXP Stop
    m_onMxpStop = new QLineEdit(this);
    m_onMxpStop->setPlaceholderText("Script routine name");
    m_onMxpStop->setToolTip("Script to call when MXP stops");
    formLayout->addRow("On MXP S&top:", m_onMxpStop);

    // On MXP Open Tag
    m_onMxpOpenTag = new QLineEdit(this);
    m_onMxpOpenTag->setPlaceholderText("Script routine name");
    m_onMxpOpenTag->setToolTip("Script to call on open tag");
    formLayout->addRow("On MXP &Open Tag:", m_onMxpOpenTag);

    // On MXP Close Tag
    m_onMxpCloseTag = new QLineEdit(this);
    m_onMxpCloseTag->setPlaceholderText("Script routine name");
    m_onMxpCloseTag->setToolTip("Script to call on close tag");
    formLayout->addRow("On MXP &Close Tag:", m_onMxpCloseTag);

    // On MXP Set Variable
    m_onMxpSetVariable = new QLineEdit(this);
    m_onMxpSetVariable->setPlaceholderText("Script routine name");
    m_onMxpSetVariable->setToolTip("Script to call when variable is set");
    formLayout->addRow("On MXP Set &Variable:", m_onMxpSetVariable);

    // On MXP Error
    m_onMxpError = new QLineEdit(this);
    m_onMxpError->setPlaceholderText("Script routine name");
    m_onMxpError->setToolTip("Script to call on MXP error");
    formLayout->addRow("On MXP &Error:", m_onMxpError);

    mainLayout->addLayout(formLayout);

    // Dialog buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &MxpScriptRoutinesDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MxpScriptRoutinesDialog::onRejected);
}

void MxpScriptRoutinesDialog::loadSettings()
{
    if (!m_doc)
        return;

    // Load MXP script routine names
    m_onMxpStart->setText(m_doc->m_strOnMXP_Start);
    m_onMxpStop->setText(m_doc->m_strOnMXP_Stop);
    m_onMxpOpenTag->setText(m_doc->m_strOnMXP_OpenTag);
    m_onMxpCloseTag->setText(m_doc->m_strOnMXP_CloseTag);
    m_onMxpSetVariable->setText(m_doc->m_strOnMXP_SetVariable);
    m_onMxpError->setText(m_doc->m_strOnMXP_Error);
}

void MxpScriptRoutinesDialog::saveSettings()
{
    if (!m_doc)
        return;

    // Save MXP script routine names
    m_doc->m_strOnMXP_Start = m_onMxpStart->text();
    m_doc->m_strOnMXP_Stop = m_onMxpStop->text();
    m_doc->m_strOnMXP_OpenTag = m_onMxpOpenTag->text();
    m_doc->m_strOnMXP_CloseTag = m_onMxpCloseTag->text();
    m_doc->m_strOnMXP_SetVariable = m_onMxpSetVariable->text();
    m_doc->m_strOnMXP_Error = m_onMxpError->text();

    // Mark document as modified
    m_doc->setModified(true);
}

void MxpScriptRoutinesDialog::onAccepted()
{
    saveSettings();
    accept();
}

void MxpScriptRoutinesDialog::onRejected()
{
    reject();
}
