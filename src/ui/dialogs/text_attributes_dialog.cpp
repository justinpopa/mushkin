#include "text_attributes_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

TextAttributesDialog::TextAttributesDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Text Attributes");
    setModal(true);
    setMinimumWidth(350);
    setupUi();
}

void TextAttributesDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Character information group
    QGroupBox* charGroup = new QGroupBox("Character", this);
    QFormLayout* charLayout = new QFormLayout(charGroup);

    m_letterLabel = new QLabel(this);
    charLayout->addRow("Letter:", m_letterLabel);

    mainLayout->addWidget(charGroup);

    // Color information group
    QGroupBox* colorGroup = new QGroupBox("Colors", this);
    QFormLayout* colorLayout = new QFormLayout(colorGroup);

    // Text color with swatch
    QHBoxLayout* textColorLayout = new QHBoxLayout();
    m_textColourLabel = new QLabel(this);
    m_textColourSwatch = new QLabel(this);
    m_textColourSwatch->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_textColourSwatch->setFixedSize(40, 20);
    textColorLayout->addWidget(m_textColourLabel);
    textColorLayout->addWidget(m_textColourSwatch);
    textColorLayout->addStretch();
    colorLayout->addRow("Text Colour:", textColorLayout);

    // Background color with swatch
    QHBoxLayout* backColorLayout = new QHBoxLayout();
    m_backColourLabel = new QLabel(this);
    m_backColourSwatch = new QLabel(this);
    m_backColourSwatch->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_backColourSwatch->setFixedSize(40, 20);
    backColorLayout->addWidget(m_backColourLabel);
    backColorLayout->addWidget(m_backColourSwatch);
    backColorLayout->addStretch();
    colorLayout->addRow("Background Colour:", backColorLayout);

    // RGB values
    m_textColourRGBLabel = new QLabel(this);
    colorLayout->addRow("Text RGB:", m_textColourRGBLabel);

    m_backgroundColourRGBLabel = new QLabel(this);
    colorLayout->addRow("Background RGB:", m_backgroundColourRGBLabel);

    // Custom color info
    m_customColourLabel = new QLabel(this);
    colorLayout->addRow("Custom Colour:", m_customColourLabel);

    mainLayout->addWidget(colorGroup);

    // Attributes group
    QGroupBox* attrGroup = new QGroupBox("Attributes", this);
    QVBoxLayout* attrLayout = new QVBoxLayout(attrGroup);

    m_boldCheckBox = new QCheckBox("Bold", this);
    m_boldCheckBox->setEnabled(false);
    attrLayout->addWidget(m_boldCheckBox);

    m_italicCheckBox = new QCheckBox("Italic", this);
    m_italicCheckBox->setEnabled(false);
    attrLayout->addWidget(m_italicCheckBox);

    m_inverseCheckBox = new QCheckBox("Inverse", this);
    m_inverseCheckBox->setEnabled(false);
    attrLayout->addWidget(m_inverseCheckBox);

    mainLayout->addWidget(attrGroup);

    // Modified information
    QGroupBox* modifiedGroup = new QGroupBox("Status", this);
    QFormLayout* modifiedLayout = new QFormLayout(modifiedGroup);

    m_modifiedLabel = new QLabel(this);
    modifiedLayout->addRow("Modified:", m_modifiedLabel);

    mainLayout->addWidget(modifiedGroup);

    // Add spacing
    mainLayout->addSpacing(10);

    // Line Info button
    m_lineInfoButton = new QPushButton("Line Info...", this);
    m_lineInfoButton->setToolTip("View detailed line information");
    connect(m_lineInfoButton, &QPushButton::clicked, this, &TextAttributesDialog::onLineInfo);
    mainLayout->addWidget(m_lineInfoButton);

    // Add spacing
    mainLayout->addSpacing(10);

    // Dialog button (Close only)
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Add stretch at the end to keep everything compact at top
    mainLayout->addStretch();
}

void TextAttributesDialog::setTextColour(const QString& colourName)
{
    m_textColourLabel->setText(colourName);
}

void TextAttributesDialog::setBackColour(const QString& colourName)
{
    m_backColourLabel->setText(colourName);
}

void TextAttributesDialog::setTextColourRGB(const QString& rgb)
{
    m_textColourRGBLabel->setText(rgb);
}

void TextAttributesDialog::setBackgroundColourRGB(const QString& rgb)
{
    m_backgroundColourRGBLabel->setText(rgb);
}

void TextAttributesDialog::setCustomColour(const QString& customInfo)
{
    m_customColourLabel->setText(customInfo);
}

void TextAttributesDialog::setLetter(const QString& letter)
{
    m_letterLabel->setText(letter);
}

void TextAttributesDialog::setBold(bool bold)
{
    m_boldCheckBox->setChecked(bold);
}

void TextAttributesDialog::setInverse(bool inverse)
{
    m_inverseCheckBox->setChecked(inverse);
}

void TextAttributesDialog::setItalic(bool italic)
{
    m_italicCheckBox->setChecked(italic);
}

void TextAttributesDialog::setModified(const QString& modifiedInfo)
{
    m_modifiedLabel->setText(modifiedInfo);
}

void TextAttributesDialog::setTextColour(const QColor& color)
{
    m_textColor = color;
    setTextColour(color.name());
    updateColorSwatch(m_textColourSwatch, color);
}

void TextAttributesDialog::setBackColour(const QColor& color)
{
    m_backColor = color;
    setBackColour(color.name());
    updateColorSwatch(m_backColourSwatch, color);
}

void TextAttributesDialog::updateColorSwatch(QLabel* swatch, const QColor& color)
{
    if (!swatch || !color.isValid()) {
        return;
    }

    QString styleSheet = QString("background-color: %1; border: 1px solid #999;").arg(color.name());
    swatch->setStyleSheet(styleSheet);
}

void TextAttributesDialog::onLineInfo()
{
    // TODO: Implement line info functionality
    // This will show detailed information about the entire line
    // containing the character being inspected.
    // For now, this is a placeholder that can be implemented when
    // the line info system is available.

    // Expected behavior:
    // 1. Open a new dialog showing line-level information
    // 2. Display line number, line length, timestamp, etc.
    // 3. Show all styles/attributes present on the line
}
