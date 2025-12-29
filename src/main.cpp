#include "storage/database.h"
#include "storage/global_options.h"
#include "ui/lua_dialog_registration.h"
#include "ui/main_window.h"
#include "utils/logging.h"
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <cstdlib>

// Static Qt Plugin Imports (Windows only)
#ifdef QT_STATIC_BUILD
#    include <QtPlugin>

// Import platform plugin (REQUIRED for Windows static builds)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

#endif // QT_STATIC_BUILD

// Load fonts from application directory
// Supports .ttf, .otf, and .ttc font files
// Note: .fon (Windows bitmap fonts) are NOT supported by Qt on macOS/Linux
static void loadLocalFonts()
{
    QString appDir = QCoreApplication::applicationDirPath();

    // Font file extensions that Qt supports
    QStringList fontExtensions = {"*.ttf", "*.otf", "*.ttc"};
#ifdef Q_OS_WIN
    // Windows also supports some additional formats
    fontExtensions << "*.fon";
#endif

    // Directories to search for fonts
    QStringList fontDirs = {
        appDir,            // Application directory
        appDir + "/fonts", // fonts subdirectory
    };

    int fontsLoaded = 0;
    for (const QString& dir : fontDirs) {
        QDir fontDir(dir);
        if (!fontDir.exists())
            continue;

        QDirIterator it(dir, fontExtensions, QDir::Files);
        while (it.hasNext()) {
            QString fontPath = it.next();
            int fontId = QFontDatabase::addApplicationFont(fontPath);
            if (fontId != -1) {
                fontsLoaded++;
            } else {
                qWarning() << "Failed to load font:" << fontPath;
            }
        }
    }

    (void)fontsLoaded; // Suppress unused variable warning
}

// Custom message handler to filter out harmless Qt warnings
static QtMessageHandler defaultMessageHandler = nullptr;

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Filter out QTextCursor position warnings - these are harmless and come from
    // Qt's internal undo/redo system when cursor positions become invalid after text changes
    if (type == QtWarningMsg && msg.contains("QTextCursor::setPosition")) {
        return; // Suppress this warning
    }

    // Pass other messages to the default handler
    if (defaultMessageHandler) {
        defaultMessageHandler(type, context, msg);
    }
}

int main(int argc, char* argv[])
{
    // Install custom message handler before creating QApplication
    defaultMessageHandler = qInstallMessageHandler(customMessageHandler);

    QApplication app(argc, argv);

    // Load local fonts from application directory (must be after QApplication is created)
    loadLocalFonts();

    // Set application info (org/app names kept as 'Gammon'/'MUSHclient' for settings compatibility)
    QApplication::setApplicationName("MUSHclient");
    QApplication::setApplicationVersion("5.0.0");
    QApplication::setOrganizationName("Gammon");
    QApplication::setOrganizationDomain("gammon.com.au");

    // Set up LUA_PATH and LUA_CPATH environment variables for Lua module loading
    // This is critical for llthreads2 and other libraries that create fresh Lua states,
    // as they don't inherit our custom package.path settings from script_engine.cpp
    QString appDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    QString luaPathSep = ";";
    QString libExt = "dll";
#else
    QString luaPathSep = ";";
    QString libExt = "so";
#endif

    // Build LUA_PATH for Lua modules (issue #4)
    // Use only relative paths for portability - no system paths
    // MUSHclient also includes absolute exe_dir paths (the ! notation in luaconf.h),
    // but we intentionally omit those to avoid the portability issues they cause
    QStringList luaPaths = {
        "./?.lua",
        "./lua/?.lua",
        "./lua/?/init.lua",
    };
    QString newLuaPath = luaPaths.join(luaPathSep);
    qputenv("LUA_PATH", newLuaPath.toLocal8Bit());

    // Build LUA_CPATH for compiled C modules (.so/.dll)
    // Include app bundle paths for bundled modules (LuaSocket, etc.)
    // Include relative paths for user modules
    // No system paths (issue #4)
    QStringList luaCPaths = {
        // App bundle paths (for bundled C modules like LuaSocket)
        appDir + "/lib/?." + libExt,
        appDir + "/lib/?/core." + libExt,
        appDir + "/lua/?." + libExt,
        appDir + "/lua/?/core." + libExt,
        // Relative paths (for user C modules)
        "./lib/?." + libExt,
        "./lib/?/core." + libExt,
        "./lua/?." + libExt,
        "./lua/?/core." + libExt,
        "./?." + libExt,
    };
    QString newLuaCPath = luaCPaths.join(luaPathSep);
    qputenv("LUA_CPATH", newLuaCPath.toLocal8Bit());

    // Open preferences database
    Database* db = Database::instance();
    if (!db->open()) {
        qWarning() << "Failed to open preferences database";
    }

    // Load global options from database
    GlobalOptions::instance()->load();

    // Register Lua dialog callbacks (connects ui module dialogs to world module)
    LuaDialogRegistration::registerDialogCallbacks();

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    // Handle command line arguments (world files to open)
    QStringList worldFiles;
    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        if (arg.endsWith(".mcl", Qt::CaseInsensitive)) {
            worldFiles.append(arg);
        }
    }

    // Queue world files to open after event loop starts
    if (!worldFiles.isEmpty()) {
        mainWindow.queueWorldFiles(worldFiles);
    }

    return app.exec();
}
