#include "send_to_all_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

SendToAllDialog::SendToAllDialog(QWidget* parent)
    : QDialog(parent), m_sendText(nullptr), m_worldList(nullptr), m_echo(nullptr),
      m_buttonBox(nullptr)
{
    setWindowTitle("Send to All");
    setupUi();
}

SendToAllDialog::~SendToAllDialog()
{
    // Qt handles cleanup of child widgets
}

void SendToAllDialog::setupUi()
{
    resize(450, 400);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Text to send
    QFormLayout* textLayout = new QFormLayout();
    m_sendText = new QLineEdit(this);
    m_sendText->setObjectName("IDC_SEND_TEXT");
    m_sendText->setPlaceholderText("Enter command or text to send...");
    textLayout->addRow("Text to send:", m_sendText);
    mainLayout->addLayout(textLayout);

    mainLayout->addSpacing(8);

    // Worlds list
    QLabel* worldsLabel = new QLabel("Worlds:", this);
    mainLayout->addWidget(worldsLabel);

    m_worldList = new QListWidget(this);
    m_worldList->setObjectName("IDC_WORLD_LIST");
    m_worldList->setSelectionMode(QAbstractItemView::NoSelection);
    m_worldList->setMinimumHeight(200);
    mainLayout->addWidget(m_worldList, 1);

    mainLayout->addSpacing(8);

    // Echo checkbox
    m_echo = new QCheckBox("Echo to output", this);
    m_echo->setObjectName("IDC_ECHO");
    m_echo->setChecked(true);
    mainLayout->addWidget(m_echo);

    mainLayout->addSpacing(8);

    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SendToAllDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SendToAllDialog::onRejected);

    // Set focus to text input
    m_sendText->setFocus();
}

QString SendToAllDialog::sendText() const
{
    return m_sendText ? m_sendText->text() : QString();
}

QStringList SendToAllDialog::selectedWorlds() const
{
    QStringList selected;
    if (m_worldList) {
        for (int i = 0; i < m_worldList->count(); ++i) {
            QListWidgetItem* item = m_worldList->item(i);
            if (item && item->checkState() == Qt::Checked) {
                selected.append(item->text());
            }
        }
    }
    return selected;
}

bool SendToAllDialog::echo() const
{
    return m_echo ? m_echo->isChecked() : true;
}

void SendToAllDialog::setWorlds(const QStringList& worlds)
{
    if (!m_worldList) {
        return;
    }

    m_worldList->clear();

    for (const QString& world : worlds) {
        QListWidgetItem* item = new QListWidgetItem(world, m_worldList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_worldList->addItem(item);
    }
}

void SendToAllDialog::onAccepted()
{
    accept();
}

void SendToAllDialog::onRejected()
{
    reject();
}
