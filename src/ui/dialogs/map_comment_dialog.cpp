#include "map_comment_dialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

MapCommentDialog::MapCommentDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Map Comment");
    setMinimumSize(350, 200);
    setupUi();
}

void MapCommentDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Comment label
    QLabel* label = new QLabel("Comment:", this);
    mainLayout->addWidget(label);

    // Comment text edit
    m_commentEdit = new QPlainTextEdit(this);
    mainLayout->addWidget(m_commentEdit);

    // OK/Cancel buttons
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Set focus to the text edit
    m_commentEdit->setFocus();
}

QString MapCommentDialog::comment() const
{
    return m_commentEdit->toPlainText();
}

void MapCommentDialog::setComment(const QString& comment)
{
    m_commentEdit->setPlainText(comment);
}
