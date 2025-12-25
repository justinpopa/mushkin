#include "progress_dialog.h"
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

ProgressDialog::ProgressDialog(const QString& title, QWidget* parent)
    : QDialog(parent), m_canceled(false)
{
    setWindowTitle(title);
    setMinimumWidth(300);
    setupUi();
}

void ProgressDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Status message label
    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    mainLayout->addWidget(m_messageLabel);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    // Cancel button
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setVisible(false);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        m_canceled = true;
        reject();
    });
    mainLayout->addWidget(m_cancelButton);

    setLayout(mainLayout);
}

void ProgressDialog::setProgress(int value)
{
    m_progressBar->setValue(value);
}

void ProgressDialog::setMessage(const QString& msg)
{
    m_messageLabel->setText(msg);
}

void ProgressDialog::setRange(int min, int max)
{
    m_progressBar->setRange(min, max);
}

void ProgressDialog::setCancelable(bool cancelable)
{
    m_cancelButton->setVisible(cancelable);
}

bool ProgressDialog::wasCanceled() const
{
    return m_canceled;
}
