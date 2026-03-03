#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include <array>
#include <cstdio>

namespace {

constexpr std::array<int, 8> k_font_sizes{8, 9, 10, 11, 12, 14, 16, 20};
constexpr const char* k_sample_text = "The quick brown fox AaBbCcXxYyZz 0123456789";
constexpr const char* k_font_family = "Courier New";

QString platform_name()
{
#if defined(Q_OS_MACOS)
    return QStringLiteral("macOS");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Linux");
#elif defined(Q_OS_WIN)
    return QStringLiteral("Windows");
#else
    return QStringLiteral("Unknown");
#endif
}

struct FontSample {
    QString label;
    QFont font;
};

std::array<FontSample, 3> make_samples(int point_size, double screen_dpi_y)
{
    QFont qt_point_font{QString::fromUtf8(k_font_family)};
    qt_point_font.setPointSize(point_size);

    QFont hack_font{QString::fromUtf8(k_font_family)};
    hack_font.setPixelSize(point_size * 96 / 72);

    QFont dynamic_font{QString::fromUtf8(k_font_family)};
    dynamic_font.setPixelSize(static_cast<int>(point_size * screen_dpi_y / 72.0));

    return {FontSample{QStringLiteral("Qt pointSize"), qt_point_font},
            FontSample{QStringLiteral("96/72 hack"), hack_font},
            FontSample{QStringLiteral("Dynamic DPI"), dynamic_font}};
}

QLabel* make_label(const QString& text, const QFont& font)
{
    auto* label = new QLabel{text};
    label->setFont(font);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

QLabel* make_plain_label(const QString& text)
{
    auto* label = new QLabel{text};
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app{argc, argv};

    QScreen* screen = QGuiApplication::primaryScreen();
    const double logical_dpi_x = screen ? screen->logicalDotsPerInchX() : 96.0;
    const double logical_dpi_y = screen ? screen->logicalDotsPerInchY() : 96.0;
    const double physical_dpi = screen ? screen->physicalDotsPerInch() : 96.0;
    const double device_pixel_ratio = screen ? screen->devicePixelRatio() : 1.0;

    // Print to stdout for CI log comparison
    std::printf("=== Mushkin Font Diagnostic ===\n");
    std::printf("Platform:          %s\n", platform_name().toUtf8().constData());
    std::printf("Logical DPI X:     %.2f\n", logical_dpi_x);
    std::printf("Logical DPI Y:     %.2f\n", logical_dpi_y);
    std::printf("Physical DPI:      %.2f\n", physical_dpi);
    std::printf("Device Pixel Ratio:%.2f\n", device_pixel_ratio);
    std::printf("\n%-4s  %-14s  %8s  %8s  %8s\n", "pt", "method", "pixelSz", "height", "ascent");
    std::printf("%-4s  %-14s  %8s  %8s  %8s\n", "----", "--------------", "--------", "--------",
                "--------");

    for (int pt : k_font_sizes) {
        auto samples = make_samples(pt, logical_dpi_y);
        for (auto& [label, font] : samples) {
            QFontInfo info{font};
            QFontMetrics metrics{font};
            std::printf("%-4d  %-14s  %8d  %8d  %8d\n", pt, label.toUtf8().constData(),
                        info.pixelSize(), metrics.height(), metrics.ascent());
        }
    }
    std::fflush(stdout);

    // Build UI
    auto* root_widget = new QWidget;
    root_widget->setWindowTitle(QStringLiteral("Mushkin Font Diagnostic"));

    auto* root_layout = new QVBoxLayout{root_widget};
    root_layout->setContentsMargins(12, 12, 12, 12);
    root_layout->setSpacing(8);

    // System info header
    const QString sys_info = QString::fromUtf8("Platform: %1  |  Logical DPI: %2 x %3"
                                               "  |  Physical DPI: %4  |  Device Pixel Ratio: %5")
                                 .arg(platform_name())
                                 .arg(logical_dpi_x, 0, 'f', 2)
                                 .arg(logical_dpi_y, 0, 'f', 2)
                                 .arg(physical_dpi, 0, 'f', 2)
                                 .arg(device_pixel_ratio, 0, 'f', 2);

    auto* sys_label = new QLabel{sys_info};
    {
        QFont bold_font = sys_label->font();
        bold_font.setBold(true);
        sys_label->setFont(bold_font);
    }
    sys_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root_layout->addWidget(sys_label);

    // Divider label
    root_layout->addWidget(make_plain_label(QString("Font family: Courier New  |  Sample: \"%1\"")
                                                .arg(QString::fromUtf8(k_sample_text))));

    // Scroll area wrapping the grid
    auto* scroll_area = new QScrollArea;
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);

    auto* grid_container = new QWidget;
    auto* grid = new QGridLayout{grid_container};
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setVerticalSpacing(6);
    grid->setHorizontalSpacing(16);

    // Column headers
    int col = 0;
    for (auto* header_text : {"pt", "Qt pointSize", "96/72 hack", "Dynamic DPI"}) {
        auto* h = new QLabel{QString::fromUtf8(header_text)};
        QFont hf = h->font();
        hf.setBold(true);
        h->setFont(hf);
        grid->addWidget(h, 0, col++);
    }

    // One row per point size; two sub-rows: sample text + metrics
    int grid_row = 1;
    for (int pt : k_font_sizes) {
        auto samples = make_samples(pt, logical_dpi_y);

        // Row A: sample text
        grid->addWidget(make_plain_label(QString::number(pt) + QStringLiteral(" pt")), grid_row, 0,
                        Qt::AlignTop);
        int sample_col = 1;
        for (auto& [label, font] : samples) {
            grid->addWidget(make_label(QString::fromUtf8(k_sample_text), font), grid_row,
                            sample_col++, Qt::AlignTop);
        }
        ++grid_row;

        // Row B: metrics (smaller grey label under each sample)
        sample_col = 1;
        for (auto& [label, font] : samples) {
            QFontInfo info{font};
            QFontMetrics metrics{font};
            const QString metrics_text = QString::fromUtf8("pixelSz=%1  height=%2  ascent=%3")
                                             .arg(info.pixelSize())
                                             .arg(metrics.height())
                                             .arg(metrics.ascent());

            auto* m_label = make_plain_label(metrics_text);
            QFont small_font = m_label->font();
            small_font.setPointSize(9);
            m_label->setFont(small_font);
            QPalette pal = m_label->palette();
            pal.setColor(QPalette::WindowText, QColor{0x88, 0x88, 0x88});
            m_label->setPalette(pal);
            grid->addWidget(m_label, grid_row, sample_col++, Qt::AlignTop);
        }
        ++grid_row;

        // Thin separator between point sizes (empty row with a bit of spacing)
        grid->setRowMinimumHeight(grid_row, 4);
        ++grid_row;
    }

    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);

    scroll_area->setWidget(grid_container);
    root_layout->addWidget(scroll_area, 1);

    root_widget->resize(1100, 700);
    root_widget->show();

    return QApplication::exec();
}
