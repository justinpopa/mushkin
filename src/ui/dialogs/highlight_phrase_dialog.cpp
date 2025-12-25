#include "highlight_phrase_dialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

HighlightPhraseDialog::HighlightPhraseDialog(QWidget* parent)
    : QDialog(parent), m_textEdit(nullptr), m_matchWholeWordCheck(nullptr),
      m_matchCaseCheck(nullptr), m_textColorButton(nullptr), m_backgroundColorButton(nullptr),
      m_textColorPreview(nullptr), m_backgroundColorPreview(nullptr), m_buttonBox(nullptr),
      m_textColor(Qt::black), m_backgroundColor(Qt::yellow)
{
    setupUi();
}

HighlightPhraseDialog::~HighlightPhraseDialog()
{
    // Qt handles cleanup of child widgets
}

void HighlightPhraseDialog::setupUi()
{
    setWindowTitle("Highlight Phrase");
    setModal(true);
    resize(450, 300);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Text input section
    QFormLayout* textLayout = new QFormLayout();

    m_textEdit = new QLineEdit(this);
    m_textEdit->setMaxLength(255);
    m_textEdit->setPlaceholderText("Enter text to highlight...");
    textLayout->addRow("Text to highlight:", m_textEdit);

    mainLayout->addLayout(textLayout);

    // Options group
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    m_matchWholeWordCheck = new QCheckBox("Match whole word only", this);
    m_matchWholeWordCheck->setToolTip("Only highlight if the text appears as a complete word");
    optionsLayout->addWidget(m_matchWholeWordCheck);

    m_matchCaseCheck = new QCheckBox("Match case", this);
    m_matchCaseCheck->setToolTip("Make the search case-sensitive");
    optionsLayout->addWidget(m_matchCaseCheck);

    mainLayout->addWidget(optionsGroup);

    // Color selection group
    QGroupBox* colorGroup = new QGroupBox("Colors", this);
    QFormLayout* colorLayout = new QFormLayout(colorGroup);

    // Text color row
    QHBoxLayout* textColorLayout = new QHBoxLayout();

    m_textColorButton = new QPushButton("Choose...", this);
    m_textColorButton->setToolTip("Select text color");
    connect(m_textColorButton, &QPushButton::clicked, this,
            &HighlightPhraseDialog::onTextColorButtonClicked);
    textColorLayout->addWidget(m_textColorButton);

    m_textColorPreview = new QLabel(this);
    m_textColorPreview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_textColorPreview->setLineWidth(1);
    m_textColorPreview->setMinimumSize(60, 30);
    m_textColorPreview->setMaximumSize(60, 30);
    updateColorPreview(m_textColorPreview, m_textColor);
    textColorLayout->addWidget(m_textColorPreview);

    textColorLayout->addStretch();

    colorLayout->addRow("Text color:", textColorLayout);

    // Background color row
    QHBoxLayout* backgroundColorLayout = new QHBoxLayout();

    m_backgroundColorButton = new QPushButton("Choose...", this);
    m_backgroundColorButton->setToolTip("Select background color");
    connect(m_backgroundColorButton, &QPushButton::clicked, this,
            &HighlightPhraseDialog::onBackgroundColorButtonClicked);
    backgroundColorLayout->addWidget(m_backgroundColorButton);

    m_backgroundColorPreview = new QLabel(this);
    m_backgroundColorPreview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_backgroundColorPreview->setLineWidth(1);
    m_backgroundColorPreview->setMinimumSize(60, 30);
    m_backgroundColorPreview->setMaximumSize(60, 30);
    updateColorPreview(m_backgroundColorPreview, m_backgroundColor);
    backgroundColorLayout->addWidget(m_backgroundColorPreview);

    backgroundColorLayout->addStretch();

    colorLayout->addRow("Background color:", backgroundColorLayout);

    mainLayout->addWidget(colorGroup);

    // Add stretch to push everything to the top
    mainLayout->addStretch();

    // Dialog buttons (OK/Cancel)
    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_buttonBox);

    // Set initial focus to text edit
    m_textEdit->setFocus();
}

void HighlightPhraseDialog::updateColorPreview(QLabel* preview, const QColor& color)
{
    if (!preview)
        return;

    QString styleSheet = QString("QLabel { background-color: %1; }").arg(color.name());
    preview->setStyleSheet(styleSheet);
}

void HighlightPhraseDialog::onTextColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_textColor, this, "Choose Text Color");

    if (color.isValid()) {
        m_textColor = color;
        updateColorPreview(m_textColorPreview, m_textColor);
    }
}

void HighlightPhraseDialog::onBackgroundColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_backgroundColor, this, "Choose Background Color");

    if (color.isValid()) {
        m_backgroundColor = color;
        updateColorPreview(m_backgroundColorPreview, m_backgroundColor);
    }
}

QString HighlightPhraseDialog::text() const
{
    return m_textEdit ? m_textEdit->text() : QString();
}

bool HighlightPhraseDialog::matchWholeWord() const
{
    return m_matchWholeWordCheck ? m_matchWholeWordCheck->isChecked() : false;
}

bool HighlightPhraseDialog::matchCase() const
{
    return m_matchCaseCheck ? m_matchCaseCheck->isChecked() : false;
}

QColor HighlightPhraseDialog::textColor() const
{
    return m_textColor;
}

QColor HighlightPhraseDialog::backgroundColor() const
{
    return m_backgroundColor;
}
