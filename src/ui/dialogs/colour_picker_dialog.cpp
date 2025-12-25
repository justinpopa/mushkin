#include "colour_picker_dialog.h"
#include "../../world/world_document.h"
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

// ANSI color names for tooltips
static const char* ANSI_NORMAL_NAMES[] = {"Black", "Red",     "Green", "Yellow",
                                          "Blue",  "Magenta", "Cyan",  "White"};

static const char* ANSI_BOLD_NAMES[] = {"Bold Black", "Bold Red",     "Bold Green", "Bold Yellow",
                                        "Bold Blue",  "Bold Magenta", "Bold Cyan",  "Bold White"};

ColourPickerDialog::ColourPickerDialog(WorldDocument* doc, QRgb initialColor, QWidget* parent)
    : QDialog(parent), m_doc(doc), m_selectedColor(initialColor)
{
    setWindowTitle("Pick a Colour");
    setModal(true);
    resize(500, 400);
    setupUi();
    updateColorSwatch();
}

void ColourPickerDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ========================================================================
    // CURRENT COLOR SWATCH (at top)
    // ========================================================================
    QGroupBox* swatchGroup = new QGroupBox("Selected Colour", this);
    QVBoxLayout* swatchLayout = new QVBoxLayout(swatchGroup);

    // Large color swatch
    m_colorSwatch = new QLabel(this);
    m_colorSwatch->setMinimumSize(100, 60);
    m_colorSwatch->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_colorSwatch->setLineWidth(2);
    swatchLayout->addWidget(m_colorSwatch);

    // Color info label (RGB values, color name if available)
    m_colorInfoLabel = new QLabel(this);
    m_colorInfoLabel->setWordWrap(true);
    swatchLayout->addWidget(m_colorInfoLabel);

    mainLayout->addWidget(swatchGroup);

    // ========================================================================
    // ANSI COLORS SECTION
    // ========================================================================
    QGroupBox* ansiGroup = new QGroupBox("Standard ANSI Colours", this);
    QGridLayout* ansiLayout = new QGridLayout(ansiGroup);

    // Create ANSI color buttons (8 normal + 8 bold = 16 colors in 2 rows)
    for (int i = 0; i < 16; i++) {
        QRgb color;
        QString tooltip;

        if (i < 8) {
            // Normal colors (first row)
            color = m_doc ? m_doc->m_normalcolour[i] : qRgb(0, 0, 0);
            tooltip = QString("%1 (Normal %2)").arg(ANSI_NORMAL_NAMES[i]).arg(i + 1);
        } else {
            // Bold colors (second row)
            int boldIndex = i - 8;
            color = m_doc ? m_doc->m_boldcolour[boldIndex] : qRgb(255, 255, 255);
            tooltip = QString("%1 (Bold %2)").arg(ANSI_BOLD_NAMES[boldIndex]).arg(boldIndex + 1);
        }

        QPushButton* btn = createColorButton(color, tooltip);
        connect(btn, &QPushButton::clicked, this, &ColourPickerDialog::onAnsiColorClicked);

        // Store in vector for later reference
        m_ansiButtons.append(btn);

        // Layout: 8 columns, 2 rows (normal colors on top, bold below)
        int row = i / 8;
        int col = i % 8;
        ansiLayout->addWidget(btn, row, col);
    }

    mainLayout->addWidget(ansiGroup);

    // ========================================================================
    // CUSTOM COLORS SECTION
    // ========================================================================
    QGroupBox* customGroup = new QGroupBox("Custom Colours", this);
    QGridLayout* customLayout = new QGridLayout(customGroup);

    // Create 16 custom color buttons (2 rows x 8 columns)
    for (int i = 0; i < MAX_CUSTOM; i++) {
        QRgb color = m_doc ? m_doc->m_customtext[i] : qRgb(255, 255, 255);

        // Check if there's a custom name for this color
        QString tooltip = QString("Custom %1").arg(i + 1);
        if (m_doc && !m_doc->m_strCustomColourName[i].isEmpty()) {
            tooltip = QString("%1 (Custom %2)").arg(m_doc->m_strCustomColourName[i]).arg(i + 1);
        }

        QPushButton* btn = createColorButton(color, tooltip);
        connect(btn, &QPushButton::clicked, this, &ColourPickerDialog::onCustomColorClicked);

        // Store in vector
        m_customButtons.append(btn);

        // Layout: 8 columns, 2 rows
        int row = i / 8;
        int col = i % 8;
        customLayout->addWidget(btn, row, col);
    }

    mainLayout->addWidget(customGroup);

    // ========================================================================
    // PICK COLOR BUTTON
    // ========================================================================
    m_pickColorButton = new QPushButton("&Pick Custom Colour...", this);
    m_pickColorButton->setToolTip("Open color picker to select any color");
    connect(m_pickColorButton, &QPushButton::clicked, this,
            &ColourPickerDialog::onPickColorClicked);
    mainLayout->addWidget(m_pickColorButton);

    // ========================================================================
    // DIALOG BUTTONS (OK/Cancel)
    // ========================================================================
    QDialogButtonBox* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ColourPickerDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ColourPickerDialog::onCancel);
    mainLayout->addWidget(buttonBox);
}

QPushButton* ColourPickerDialog::createColorButton(QRgb color, const QString& tooltip)
{
    QPushButton* btn = new QPushButton(this);
    btn->setFixedSize(40, 30);
    btn->setToolTip(tooltip);

    // Store the color as a property so we can retrieve it when clicked
    btn->setProperty("color", QVariant::fromValue(color));

    // Set the button's background color using stylesheet
    // Color is in BGR format (0x00BBGGRR), extract components correctly
    int r = color & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = (color >> 16) & 0xFF;
    QString styleSheet =
        QString("QPushButton { background-color: rgb(%1, %2, %3); border: 2px solid "
                "#888; } QPushButton:hover { border: 2px solid #000; }")
            .arg(r)
            .arg(g)
            .arg(b);
    btn->setStyleSheet(styleSheet);

    return btn;
}

void ColourPickerDialog::updateColorSwatch()
{
    // Update the large swatch with current color
    // Color is in BGR format (0x00BBGGRR), extract components correctly
    int r = m_selectedColor & 0xFF;
    int g = (m_selectedColor >> 8) & 0xFF;
    int b = (m_selectedColor >> 16) & 0xFF;

    QString swatchStyle =
        QString("QLabel { background-color: rgb(%1, %2, %3); }").arg(r).arg(g).arg(b);
    m_colorSwatch->setStyleSheet(swatchStyle);

    // Update info label with RGB values (display as RGB for user clarity)
    QString info = QString("RGB: %1, %2, %3 (0x%4)")
                       .arg(r)
                       .arg(g)
                       .arg(b)
                       .arg(m_selectedColor & 0x00FFFFFF, 6, 16, QChar('0'));

    // Try to get color name if it's a named color
    if (m_doc) {
        // Simple check against ANSI colors
        for (int i = 0; i < 8; i++) {
            if (m_doc->m_normalcolour[i] == m_selectedColor) {
                info += QString("\n%1").arg(ANSI_NORMAL_NAMES[i]);
                break;
            }
            if (m_doc->m_boldcolour[i] == m_selectedColor) {
                info += QString("\n%1").arg(ANSI_BOLD_NAMES[i]);
                break;
            }
        }

        // Check custom colors
        for (int i = 0; i < MAX_CUSTOM; i++) {
            if (m_doc->m_customtext[i] == m_selectedColor) {
                if (!m_doc->m_strCustomColourName[i].isEmpty()) {
                    info += QString("\n%1").arg(m_doc->m_strCustomColourName[i]);
                } else {
                    info += QString("\nCustom %1").arg(i + 1);
                }
                break;
            }
        }
    }

    m_colorInfoLabel->setText(info);
}

void ColourPickerDialog::selectColor(QRgb color)
{
    m_selectedColor = color;
    updateColorSwatch();
}

void ColourPickerDialog::onAnsiColorClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    QRgb color = btn->property("color").value<QRgb>();
    selectColor(color);
}

void ColourPickerDialog::onCustomColorClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    QRgb color = btn->property("color").value<QRgb>();
    selectColor(color);
}

void ColourPickerDialog::onPickColorClicked()
{
    // Open Qt's color picker dialog
    QColor initial = QColor::fromRgb(m_selectedColor);
    QColor color = QColorDialog::getColor(initial, this, "Pick a Colour");

    if (color.isValid()) {
        selectColor(color.rgb() & 0x00FFFFFF);
    }
}

void ColourPickerDialog::onOk()
{
    accept();
}

void ColourPickerDialog::onCancel()
{
    reject();
}
