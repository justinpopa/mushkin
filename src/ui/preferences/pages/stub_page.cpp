#include "stub_page.h"
#include <QLabel>
#include <QVBoxLayout>

StubPage::StubPage(WorldDocument* doc, const QString& name, const QString& description,
                   QWidget* parent)
    : PreferencesPageBase(doc, parent), m_name(name), m_description(description)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    auto* nameLabel = new QLabel(QString("<h2>%1</h2>").arg(name), this);
    layout->addWidget(nameLabel);

    auto* descLabel = new QLabel(description, this);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    auto* comingSoon = new QLabel("<i>This page is under construction.</i>", this);
    comingSoon->setStyleSheet("color: gray; margin-top: 20px;");
    layout->addWidget(comingSoon);

    layout->addStretch();
}
