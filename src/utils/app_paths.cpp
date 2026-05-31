#include "app_paths.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

namespace AppPaths {

namespace {
double s_appStartTime = 0.0;
}

void recordAppStartTime()
{
    s_appStartTime = static_cast<double>(QDateTime::currentDateTime().toSecsSinceEpoch());
}

double appStartTime()
{
    return s_appStartTime;
}

QString getAppDirectory()
{
    QString path = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MAC
    // If inside a .app bundle, go up to where the .app lives
    // Bundle structure: /path/to/mushkin.app/Contents/MacOS/mushkin
    // We want: /path/to/
    if (path.contains(".app/Contents/MacOS")) {
        QDir dir(path);
        dir.cdUp(); // MacOS -> Contents
        dir.cdUp(); // Contents -> mushkin.app
        dir.cdUp(); // mushkin.app -> containing folder
        return dir.absolutePath();
    }
#endif

    return path;
}

QString getExecutableDirectory()
{
    return QCoreApplication::applicationDirPath();
}

} // namespace AppPaths
