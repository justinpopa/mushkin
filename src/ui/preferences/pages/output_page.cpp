#include "output_page.h"
#include "world/world_document.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

OutputPage::OutputPage(WorldDocument* doc, QWidget* parent)
    : PreferencesPageBase(doc, parent)
{
    setupUi();
}

void OutputPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Font section
    QGroupBox* fontGroup = new QGroupBox(tr("Font"), this);
    QHBoxLayout* fontLayout = new QHBoxLayout(fontGroup);

    m_outputFontLabel = new QLabel(tr("Courier New, 10pt"), this);
    fontLayout->addWidget(m_outputFontLabel);

    m_outputFontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_outputFontButton, &QPushButton::clicked, this,
            &OutputPage::onOutputFontButtonClicked);
    fontLayout->addWidget(m_outputFontButton);
    fontLayout->addStretch();

    mainLayout->addWidget(fontGroup);

    // Display options section
    QGroupBox* displayGroup = new QGroupBox(tr("Display Options"), this);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    // Word wrap checkbox (m_wrap - enable word-wrap at spaces)
    m_wordWrapCheck = new QCheckBox(tr("Word wrap at spaces"), this);
    m_wordWrapCheck->setToolTip(
        tr("When enabled, lines wrap at the last space before the wrap column.\n"
           "When disabled, lines wrap exactly at the column boundary."));
    connect(m_wordWrapCheck, &QCheckBox::toggled, this, &OutputPage::markChanged);
    displayLayout->addWidget(m_wordWrapCheck);

    // Wrap column (m_nWrapColumn - the column width)
    QHBoxLayout* wrapLayout = new QHBoxLayout();
    wrapLayout->addWidget(new QLabel(tr("Wrap at column:"), this));
    m_wrapColumnSpin = new QSpinBox(this);
    m_wrapColumnSpin->setRange(40, 500);
    m_wrapColumnSpin->setValue(80);
    m_wrapColumnSpin->setToolTip(tr("Column at which lines are wrapped"));
    connect(m_wrapColumnSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &OutputPage::markChanged);
    wrapLayout->addWidget(m_wrapColumnSpin);
    wrapLayout->addStretch();
    displayLayout->addLayout(wrapLayout);

    // Text styles
    m_showBoldCheck = new QCheckBox(tr("Show bold text"), this);
    connect(m_showBoldCheck, &QCheckBox::toggled, this, &OutputPage::markChanged);
    displayLayout->addWidget(m_showBoldCheck);

    m_showItalicCheck = new QCheckBox(tr("Show italic text"), this);
    connect(m_showItalicCheck, &QCheckBox::toggled, this, &OutputPage::markChanged);
    displayLayout->addWidget(m_showItalicCheck);

    m_showUnderlineCheck = new QCheckBox(tr("Show underlined text"), this);
    connect(m_showUnderlineCheck, &QCheckBox::toggled, this, &OutputPage::markChanged);
    displayLayout->addWidget(m_showUnderlineCheck);

    mainLayout->addWidget(displayGroup);

    // ANSI color palette section
    QGroupBox* colorGroup = new QGroupBox(tr("ANSI Color Palette"), this);
    QGridLayout* colorGrid = new QGridLayout(colorGroup);

    const char* colorNames[16] = {"Black",       "Red",          "Green",         "Yellow",
                                  "Blue",        "Magenta",      "Cyan",          "White",
                                  "Bright Black","Bright Red",   "Bright Green",  "Bright Yellow",
                                  "Bright Blue", "Bright Magenta","Bright Cyan",  "Bright White"};

    // Create 16 color buttons in 2 columns (8 rows x 2 cols)
    for (int i = 0; i < 16; i++) {
        QHBoxLayout* colorRow = new QHBoxLayout();

        QLabel* label = new QLabel(colorNames[i], this);
        label->setMinimumWidth(100);
        colorRow->addWidget(label);

        m_colorButtons[i] = new QPushButton(this);
        m_colorButtons[i]->setFixedSize(70, 24);
        m_colorButtons[i]->setProperty("colorIndex", i);
        connect(m_colorButtons[i], &QPushButton::clicked, this,
                &OutputPage::onColorButtonClicked);
        colorRow->addWidget(m_colorButtons[i]);
        colorRow->addStretch();

        // Place in grid: 2 columns, 8 rows
        int row = i % 8;
        int col = i / 8;
        colorGrid->addLayout(colorRow, row, col);
    }

    mainLayout->addWidget(colorGroup);

    // Activity section
    QGroupBox* activityGroup = new QGroupBox(tr("Activity Notification"), this);
    QVBoxLayout* activityLayout = new QVBoxLayout(activityGroup);

    m_flashIconCheck = new QCheckBox(tr("Flash taskbar icon when new output arrives"), this);
    connect(m_flashIconCheck, &QCheckBox::toggled, this, &OutputPage::markChanged);
    activityLayout->addWidget(m_flashIconCheck);

    mainLayout->addWidget(activityGroup);

    mainLayout->addStretch();
}

void OutputPage::loadSettings()
{
    if (!m_doc)
        return;

    // Block signals while loading
    m_wordWrapCheck->blockSignals(true);
    m_wrapColumnSpin->blockSignals(true);
    m_showBoldCheck->blockSignals(true);
    m_showItalicCheck->blockSignals(true);
    m_showUnderlineCheck->blockSignals(true);
    m_flashIconCheck->blockSignals(true);

    // Load font
    m_outputFont.setFamily(m_doc->m_font_name);
    m_outputFont.setPointSize(qAbs(m_doc->m_font_height));
    m_outputFont.setWeight(m_doc->m_font_weight >= 700 ? QFont::Bold : QFont::Normal);
    m_outputFontLabel->setText(
        QString("%1, %2pt").arg(m_outputFont.family()).arg(m_outputFont.pointSize()));

    // Load wrap settings
    m_wordWrapCheck->setChecked(m_doc->m_wrap != 0);
    m_wrapColumnSpin->setValue(m_doc->m_nWrapColumn);

    // Load text style options
    m_showBoldCheck->setChecked(m_doc->m_bShowBold != 0);
    m_showItalicCheck->setChecked(m_doc->m_bShowItalic != 0);
    m_showUnderlineCheck->setChecked(m_doc->m_bShowUnderline != 0);

    // Load ANSI colors from document
    for (int i = 0; i < 8; i++) {
        m_ansiColors[i] = m_doc->m_normalcolour[i];
        m_ansiColors[i + 8] = m_doc->m_boldcolour[i];
        updateColorButton(i);
        updateColorButton(i + 8);
    }

    // Load activity settings
    m_flashIconCheck->setChecked(m_doc->m_bFlashIcon != 0);

    // Unblock signals
    m_wordWrapCheck->blockSignals(false);
    m_wrapColumnSpin->blockSignals(false);
    m_showBoldCheck->blockSignals(false);
    m_showItalicCheck->blockSignals(false);
    m_showUnderlineCheck->blockSignals(false);
    m_flashIconCheck->blockSignals(false);

    m_hasChanges = false;
}

void OutputPage::saveSettings()
{
    if (!m_doc)
        return;

    // Save font settings
    m_doc->m_font_name = m_outputFont.family();
    m_doc->m_font_height = m_outputFont.pointSize();
    m_doc->m_font_weight = m_outputFont.weight();

    // Save wrap settings
    m_doc->m_wrap = m_wordWrapCheck->isChecked() ? 1 : 0;
    m_doc->m_nWrapColumn = m_wrapColumnSpin->value();

    // Save text style options
    m_doc->m_bShowBold = m_showBoldCheck->isChecked() ? 1 : 0;
    m_doc->m_bShowItalic = m_showItalicCheck->isChecked() ? 1 : 0;
    m_doc->m_bShowUnderline = m_showUnderlineCheck->isChecked() ? 1 : 0;

    // Save ANSI colors
    for (int i = 0; i < 8; i++) {
        m_doc->m_normalcolour[i] = m_ansiColors[i];
        m_doc->m_boldcolour[i] = m_ansiColors[i + 8];
    }

    // Save activity settings
    m_doc->m_bFlashIcon = m_flashIconCheck->isChecked() ? 1 : 0;

    m_doc->setModified(true);
    emit m_doc->outputSettingsChanged();

    m_hasChanges = false;
}

bool OutputPage::hasChanges() const
{
    return m_hasChanges;
}

void OutputPage::onOutputFontButtonClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_outputFont, this, tr("Choose Output Font"));

    if (ok) {
        m_outputFont = font;
        m_outputFontLabel->setText(
            QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
        markChanged();
    }
}

void OutputPage::onColorButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    int index = btn->property("colorIndex").toInt();

    QColor color = QColorDialog::getColor(QColor(m_ansiColors[index]), this,
                                          tr("Choose color for %1").arg(index < 8 ? "normal" : "bright"));

    if (color.isValid()) {
        m_ansiColors[index] = color.rgb();
        updateColorButton(index);
        markChanged();
    }
}

void OutputPage::updateColorButton(int index)
{
    if (index < 0 || index >= 16)
        return;

    QColor color(m_ansiColors[index]);
    QString style = QString("background-color: %1; color: %2;")
                        .arg(color.name())
                        .arg(color.lightness() > 128 ? "black" : "white");

    m_colorButtons[index]->setStyleSheet(style);
    m_colorButtons[index]->setText(color.name());
}

void OutputPage::markChanged()
{
    m_hasChanges = true;
    emit settingsChanged();
}
