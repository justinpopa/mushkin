#include "colors_page.h"
#include "world/world_document.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ColorsPage::ColorsPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void ColorsPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("Custom colors can be used in triggers and other features. "
           "Each color has a text (foreground) and background component."),
        this);
    helpLabel->setWordWrap(true);
    mainLayout->addWidget(helpLabel);

    // Color table
    m_table = new QTableWidget(16, 4, this);
    m_table->setHorizontalHeaderLabels({tr("#"), tr("Name"), tr("Text"), tr("Background")});
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 40);
    m_table->setColumnWidth(2, 80);
    m_table->setColumnWidth(3, 80);

    // Populate rows
    for (int i = 0; i < 16; i++) {
        // Index column (read-only)
        QTableWidgetItem* indexItem = new QTableWidgetItem(QString::number(i + 1));
        indexItem->setFlags(indexItem->flags() & ~Qt::ItemIsEditable);
        indexItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, indexItem);

        // Name column (editable)
        QTableWidgetItem* nameItem = new QTableWidgetItem();
        m_table->setItem(i, 1, nameItem);

        // Text color button
        QPushButton* textBtn = new QPushButton(this);
        textBtn->setProperty("colorIndex", i);
        textBtn->setProperty("colorType", "text");
        connect(textBtn, &QPushButton::clicked, this, &ColorsPage::onTextColorClicked);
        m_table->setCellWidget(i, 2, textBtn);

        // Background color button
        QPushButton* backBtn = new QPushButton(this);
        backBtn->setProperty("colorIndex", i);
        backBtn->setProperty("colorType", "back");
        connect(backBtn, &QPushButton::clicked, this, &ColorsPage::onBackColorClicked);
        m_table->setCellWidget(i, 3, backBtn);
    }

    connect(m_table, &QTableWidget::cellChanged, this, &ColorsPage::onNameChanged);

    mainLayout->addWidget(m_table, 1);
}

void ColorsPage::loadSettings()
{
    if (!m_doc)
        return;

    m_table->blockSignals(true);

    for (int i = 0; i < 16; i++) {
        m_customText[i] = m_doc->m_customtext[i];
        m_customBack[i] = m_doc->m_customback[i];
        m_customNames[i] = m_doc->m_strCustomColourName[i];

        // Update name
        m_table->item(i, 1)->setText(m_customNames[i]);

        // Update color buttons
        updateColorCell(i, 2, m_customText[i]);
        updateColorCell(i, 3, m_customBack[i]);
    }

    m_table->blockSignals(false);
    m_hasChanges = false;
}

void ColorsPage::saveSettings()
{
    if (!m_doc)
        return;

    for (int i = 0; i < 16; i++) {
        m_doc->m_customtext[i] = m_customText[i];
        m_doc->m_customback[i] = m_customBack[i];
        m_doc->m_strCustomColourName[i] = m_table->item(i, 1)->text();
    }

    m_doc->setModified(true);
    m_hasChanges = false;
}

bool ColorsPage::hasChanges() const
{
    return m_hasChanges;
}

void ColorsPage::updateColorCell(int row, int col, QRgb color)
{
    QPushButton* btn = qobject_cast<QPushButton*>(m_table->cellWidget(row, col));
    if (!btn)
        return;

    QColor c(color);
    QString style = QString("background-color: %1; color: %2;")
                        .arg(c.name())
                        .arg(c.lightness() > 128 ? "black" : "white");
    btn->setStyleSheet(style);
    btn->setText(c.name());
}

void ColorsPage::onTextColorClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    int index = btn->property("colorIndex").toInt();
    QColor color = QColorDialog::getColor(QColor(m_customText[index]), this,
                                          tr("Choose text color for custom %1").arg(index + 1));

    if (color.isValid()) {
        m_customText[index] = color.rgb();
        updateColorCell(index, 2, m_customText[index]);
        markChanged();
    }
}

void ColorsPage::onBackColorClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    int index = btn->property("colorIndex").toInt();
    QColor color = QColorDialog::getColor(QColor(m_customBack[index]), this,
                                          tr("Choose background color for custom %1").arg(index + 1));

    if (color.isValid()) {
        m_customBack[index] = color.rgb();
        updateColorCell(index, 3, m_customBack[index]);
        markChanged();
    }
}

void ColorsPage::onNameChanged()
{
    markChanged();
}

void ColorsPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
