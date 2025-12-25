#include "missing_entry_points_dialog.h"
#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

MissingEntryPointsDialog::MissingEntryPointsDialog(const QString& errorMessage, QWidget* parent)
    : QDialog(parent), m_errorMessage(errorMessage)
{
    setWindowTitle("Missing Entry Points");
    setModal(true);
    setMinimumSize(400, 250);
    resize(500, 300);
    setupUi();
}

void MissingEntryPointsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Warning icon and label layout
    QHBoxLayout* headerLayout = new QHBoxLayout();

    // Warning icon
    QLabel* iconLabel = new QLabel(this);
    QStyle::StandardPixmap icon = QStyle::SP_MessageBoxWarning;
    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
    iconLabel->setPixmap(style()->standardIcon(icon).pixmap(iconSize, iconSize));
    headerLayout->addWidget(iconLabel);

    // Warning text
    QLabel* warningLabel = new QLabel("The following script entry points are missing:", this);
    headerLayout->addWidget(warningLabel, 1);

    mainLayout->addLayout(headerLayout);

    // Add spacing
    mainLayout->addSpacing(10);

    // Error message text edit (read-only, monospace)
    m_errorTextEdit = new QTextEdit(this);
    m_errorTextEdit->setPlainText(m_errorMessage);
    m_errorTextEdit->setReadOnly(true);

    // Set monospace font
    QFont monoFont("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    m_errorTextEdit->setFont(monoFont);

    mainLayout->addWidget(m_errorTextEdit);

    // Add spacing
    mainLayout->addSpacing(10);

    // Dialog button (OK only)
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}
