#include "storage/database.h"

#include "logging.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

// Initialize static instance
Database* Database::s_instance = nullptr;

Database::Database(QObject* parent) : QObject(parent)
{
}

Database::~Database()
{
    close();
}

Database* Database::instance()
{
    if (!s_instance) {
        s_instance = new Database();
    }
    return s_instance;
}

bool Database::open()
{
    // If already open, return success
    if (m_db.isOpen()) {
        return true;
    }

    // Determine database path using original MUSHclient approach:
    // 1. Try working directory first
    // 2. Fall back to application directory
    //
    // This matches: MUSHclient.cpp

    QString filename = "mushclient_prefs.sqlite";

    // Try working directory first
    m_dbPath = QDir::currentPath() + "/" + filename;
    qCDebug(lcStorage) << "Trying working directory:" << m_dbPath;

    if (!QFile::exists(m_dbPath)) {
        // Fall back to application directory
        m_dbPath = QCoreApplication::applicationDirPath() + "/" + filename;
        qCDebug(lcStorage) << "Trying application directory:" << m_dbPath;
    }

    qCDebug(lcStorage) << "Database path:" << m_dbPath;

    // Check if database file exists
    bool databaseExists = QFile::exists(m_dbPath);

    // Create/open SQLite database connection
    m_db = QSqlDatabase::addDatabase("QSQLITE", "preferences");
    m_db.setDatabaseName(m_dbPath);

    // Open the database
    if (!m_db.open()) {
        QString errMsg = QString("Failed to open database: %1").arg(lastError());
        qWarning() << errMsg;
        emit error(errMsg);
        return false;
    }

    qCDebug(lcStorage) << "Database opened successfully";

    // If database was just created, create schema
    if (!databaseExists) {
        qCDebug(lcStorage) << "Creating database schema...";
        if (!createSchema()) {
            QString errMsg = QString("Failed to create database schema: %1").arg(lastError());
            qWarning() << errMsg;
            emit error(errMsg);
            close();
            return false;
        }
        qCDebug(lcStorage) << "Database schema created successfully";
    } else {
        // Existing database - check if migration is needed
        if (!migrateSchema()) {
            QString errMsg = QString("Failed to migrate database schema: %1").arg(lastError());
            qWarning() << errMsg;
            emit error(errMsg);
            // Don't close - try to continue with potentially outdated schema
        }
    }

    return true;
}

void Database::close()
{
    if (m_db.isOpen()) {
        qCDebug(lcStorage) << "Closing database";
        m_db.close();
    }

    // Remove the connection
    if (QSqlDatabase::contains("preferences")) {
        QSqlDatabase::removeDatabase("preferences");
    }
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

QString Database::databasePath() const
{
    return m_dbPath;
}

bool Database::createSchema()
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot create schema: database not open";
        return false;
    }

    // Match original MUSHclient schema

    // Create control table (database metadata and version tracking)
    QString createControl = R"(
        CREATE TABLE IF NOT EXISTS control (
            name VARCHAR(10) NOT NULL PRIMARY KEY,
            value INT NOT NULL
        )
    )";

    if (!execute(createControl)) {
        qWarning() << "Failed to create control table:" << lastError();
        return false;
    }

    // Set initial database version if not already set
    int dbVersion = getDatabaseVersion();
    if (dbVersion == 0) {
        setDatabaseVersion(CURRENT_DB_VERSION);
        qCDebug(lcStorage) << "Set initial database version to" << CURRENT_DB_VERSION;
    }

    // Create prefs table (global preferences - key/value pairs)
    // Matches original: all values stored as TEXT
    QString createPrefs = R"(
        CREATE TABLE IF NOT EXISTS prefs (
            name VARCHAR(50) NOT NULL PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";

    if (!execute(createPrefs)) {
        qWarning() << "Failed to create prefs table:" << lastError();
        return false;
    }

    // Create worlds table (world window geometry - key/value pairs)
    // Original stores: "WorldName:wp.left" -> "100", etc.
    QString createWorlds = R"(
        CREATE TABLE IF NOT EXISTS worlds (
            name VARCHAR(50) NOT NULL PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";

    if (!execute(createWorlds)) {
        qWarning() << "Failed to create worlds table:" << lastError();
        return false;
    }

    // Create recent_files table (NEW - cross-platform MRU replacement)
    // Original MUSHclient uses Windows Registry for MRU
    // We use a table for cross-platform support
    QString createRecentFiles = R"(
        CREATE TABLE IF NOT EXISTS recent_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            timestamp INTEGER NOT NULL,
            file_size INTEGER DEFAULT 0,
            world_name TEXT DEFAULT ''
        )
    )";

    if (!execute(createRecentFiles)) {
        qWarning() << "Failed to create recent_files table:" << lastError();
        return false;
    }

    // Create index on timestamp for efficient sorting
    QString createRecentFilesIndex = R"(
        CREATE INDEX IF NOT EXISTS idx_recent_files_timestamp
        ON recent_files(timestamp DESC)
    )";

    if (!execute(createRecentFilesIndex)) {
        qWarning() << "Failed to create recent_files index:" << lastError();
        return false;
    }

    qCDebug(lcStorage) << "All database tables created successfully";
    return true;
}

bool Database::migrateSchema()
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot migrate schema: database not open";
        return false;
    }

    // Ensure control table exists (might be missing in very old databases)
    QString createControl = R"(
        CREATE TABLE IF NOT EXISTS control (
            name VARCHAR(10) NOT NULL PRIMARY KEY,
            value INT NOT NULL
        )
    )";
    if (!execute(createControl)) {
        qWarning() << "Failed to ensure control table exists";
        return false;
    }

    int currentVersion = getDatabaseVersion();
    qCDebug(lcStorage) << "Database version:" << currentVersion
                       << "Current version:" << CURRENT_DB_VERSION;

    if (currentVersion >= CURRENT_DB_VERSION) {
        qCDebug(lcStorage) << "No migration needed";
        return true;
    }

    qCDebug(lcStorage) << "Migrating database from version" << currentVersion << "to"
                       << CURRENT_DB_VERSION;

    // Migration from version 0/1 to version 2:
    // Ensure recent_files table has the correct schema
    if (currentVersion < 2) {
        qCDebug(lcStorage) << "Applying migration to version 2...";

        // Check if recent_files table exists
        QSqlQuery query(m_db);
        query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='recent_files'");

        if (query.next()) {
            // Table exists - add missing columns if needed
            if (!columnExists("recent_files", "file_size")) {
                qCDebug(lcStorage) << "Adding file_size column to recent_files";
                if (!execute("ALTER TABLE recent_files ADD COLUMN file_size INTEGER DEFAULT 0")) {
                    qWarning() << "Failed to add file_size column";
                    return false;
                }
            }

            if (!columnExists("recent_files", "world_name")) {
                qCDebug(lcStorage) << "Adding world_name column to recent_files";
                if (!execute("ALTER TABLE recent_files ADD COLUMN world_name TEXT DEFAULT ''")) {
                    qWarning() << "Failed to add world_name column";
                    return false;
                }
            }

            if (!columnExists("recent_files", "timestamp")) {
                qCDebug(lcStorage) << "Adding timestamp column to recent_files";
                if (!execute("ALTER TABLE recent_files ADD COLUMN timestamp INTEGER DEFAULT 0")) {
                    qWarning() << "Failed to add timestamp column";
                    return false;
                }
            }
        } else {
            // Table doesn't exist - create it fresh
            QString createRecentFiles = R"(
                CREATE TABLE IF NOT EXISTS recent_files (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    path TEXT NOT NULL UNIQUE,
                    timestamp INTEGER NOT NULL,
                    file_size INTEGER DEFAULT 0,
                    world_name TEXT DEFAULT ''
                )
            )";
            if (!execute(createRecentFiles)) {
                qWarning() << "Failed to create recent_files table";
                return false;
            }

            // Create index
            QString createIndex = R"(
                CREATE INDEX IF NOT EXISTS idx_recent_files_timestamp
                ON recent_files(timestamp DESC)
            )";
            if (!execute(createIndex)) {
                qWarning() << "Failed to create recent_files index";
                return false;
            }
        }

        // Ensure prefs and worlds tables exist too
        if (!execute("CREATE TABLE IF NOT EXISTS prefs (name VARCHAR(50) NOT NULL PRIMARY KEY, "
                     "value TEXT NOT NULL)")) {
            qWarning() << "Failed to ensure prefs table exists";
            return false;
        }
        if (!execute("CREATE TABLE IF NOT EXISTS worlds (name VARCHAR(50) NOT NULL PRIMARY KEY, "
                     "value TEXT NOT NULL)")) {
            qWarning() << "Failed to ensure worlds table exists";
            return false;
        }
    }

    // Update version to current
    if (!setDatabaseVersion(CURRENT_DB_VERSION)) {
        qWarning() << "Failed to update database version";
        return false;
    }

    qCDebug(lcStorage) << "Migration complete - now at version" << CURRENT_DB_VERSION;
    return true;
}

bool Database::columnExists(const QString& tableName, const QString& columnName)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("PRAGMA table_info(" + tableName + ")");

    if (!query.exec()) {
        qWarning() << "Failed to get table info for" << tableName;
        return false;
    }

    while (query.next()) {
        // Column 1 is the column name
        if (query.value(1).toString().compare(columnName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

bool Database::execute(const QString& sql)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot execute query: database not open";
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        qWarning() << "Query failed:" << sql;
        qWarning() << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

QString Database::lastError() const
{
    if (!m_db.isValid()) {
        return "Database connection is invalid";
    }

    QSqlError err = m_db.lastError();
    if (err.isValid()) {
        return err.text();
    }

    return QString();
}

// Recent Files Operations

bool Database::addRecentFile(const QString& path)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot add recent file: database not open";
        return false;
    }

    if (path.isEmpty()) {
        qWarning() << "Cannot add recent file: path is empty";
        return false;
    }

    // Get current timestamp (seconds since epoch)
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();

    // Get file size
    QFileInfo fileInfo(path);
    qint64 fileSize = fileInfo.size();

    // Try to update existing entry first
    QSqlQuery query(m_db);
    query.prepare("UPDATE recent_files SET timestamp = ?, file_size = ? WHERE path = ?");
    query.addBindValue(timestamp);
    query.addBindValue(fileSize);
    query.addBindValue(path);

    if (!query.exec()) {
        qWarning() << "Failed to update recent file:" << query.lastError().text();
        return false;
    }

    // If no rows were updated, insert new entry
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO recent_files (path, timestamp, file_size) VALUES (?, ?, ?)");
        query.addBindValue(path);
        query.addBindValue(timestamp);
        query.addBindValue(fileSize);

        if (!query.exec()) {
            qWarning() << "Failed to insert recent file:" << query.lastError().text();
            return false;
        }
    }

    qCDebug(lcStorage) << "Added recent file:" << path;
    return true;
}

QStringList Database::getRecentFiles(int maxCount)
{
    QStringList files;

    if (!m_db.isOpen()) {
        qWarning() << "Cannot get recent files: database not open";
        return files;
    }

    // Query recent files, ordered by timestamp (most recent first)
    QSqlQuery query(m_db);
    query.prepare("SELECT path FROM recent_files ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(maxCount);

    if (!query.exec()) {
        qWarning() << "Failed to get recent files:" << query.lastError().text();
        return files;
    }

    // Collect results
    while (query.next()) {
        QString path = query.value(0).toString();

        // Only include files that still exist
        if (QFile::exists(path)) {
            files.append(path);
        } else {
            qCDebug(lcStorage) << "Skipping non-existent recent file:" << path;
        }
    }

    qCDebug(lcStorage) << "Retrieved" << files.size() << "recent files";
    return files;
}

bool Database::clearRecentFiles()
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot clear recent files: database not open";
        return false;
    }

    if (!execute("DELETE FROM recent_files")) {
        qWarning() << "Failed to clear recent files:" << lastError();
        return false;
    }

    qCDebug(lcStorage) << "Cleared all recent files";
    return true;
}

bool Database::removeRecentFile(const QString& path)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot remove recent file: database not open";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM recent_files WHERE path = ?");
    query.addBindValue(path);

    if (!query.exec()) {
        qWarning() << "Failed to remove recent file:" << query.lastError().text();
        return false;
    }

    qCDebug(lcStorage) << "Removed recent file:" << path;
    return true;
}

// Control Table Operations

int Database::getDatabaseVersion()
{
    return getControlInt("database_version", 0);
}

bool Database::setDatabaseVersion(int version)
{
    return setControlInt("database_version", version);
}

int Database::getControlInt(const QString& name, int defaultValue)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot get control value: database not open";
        return defaultValue;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM control WHERE name = ?");
    query.addBindValue(name);

    if (!query.exec()) {
        qWarning() << "Failed to query control table:" << query.lastError().text();
        return defaultValue;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return defaultValue;
}

bool Database::setControlInt(const QString& name, int value)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot set control value: database not open";
        return false;
    }

    // Try UPDATE first
    QSqlQuery query(m_db);
    query.prepare("UPDATE control SET value = ? WHERE name = ?");
    query.addBindValue(value);
    query.addBindValue(name);

    if (!query.exec()) {
        qWarning() << "Failed to update control table:" << query.lastError().text();
        return false;
    }

    // If no rows updated, INSERT
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO control (name, value) VALUES (?, ?)");
        query.addBindValue(name);
        query.addBindValue(value);

        if (!query.exec()) {
            qWarning() << "Failed to insert into control table:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

// Prefs Table Operations

QString Database::getPreference(const QString& name, const QString& defaultValue)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot get preference: database not open";
        return defaultValue;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM prefs WHERE name = ?");
    query.addBindValue(name);

    if (!query.exec()) {
        qWarning() << "Failed to query prefs table:" << query.lastError().text();
        return defaultValue;
    }

    if (query.next()) {
        return query.value(0).toString();
    }

    return defaultValue;
}

bool Database::setPreference(const QString& name, const QString& value)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot set preference: database not open";
        return false;
    }

    // Ensure we never try to save a null QString (convert to empty string)
    QString safeValue = value.isNull() ? QString("") : value;

    // Try UPDATE first
    QSqlQuery query(m_db);
    query.prepare("UPDATE prefs SET value = ? WHERE name = ?");
    query.addBindValue(safeValue);
    query.addBindValue(name);

    if (!query.exec()) {
        qWarning() << "Failed to update prefs table:" << query.lastError().text();
        return false;
    }

    // If no rows updated, INSERT
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO prefs (name, value) VALUES (?, ?)");
        query.addBindValue(name);
        query.addBindValue(safeValue);

        if (!query.exec()) {
            qWarning() << "Failed to insert into prefs table:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

int Database::getPreferenceInt(const QString& name, int defaultValue)
{
    QString value = getPreference(name, QString::number(defaultValue));
    return value.toInt();
}

bool Database::setPreferenceInt(const QString& name, int value)
{
    return setPreference(name, QString::number(value));
}

// Worlds Table Operations (per-world window geometry)

bool Database::saveWindowGeometry(const QString& worldName, const QRect& geometry)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot save window geometry: database not open";
        return false;
    }

    if (worldName.isEmpty()) {
        qWarning() << "Cannot save window geometry: world name is empty";
        return false;
    }

    // Store geometry using same keys as original MUSHclient: "{worldname}:wp.left", etc.
    // Original uses left/top/right/bottom, we use left/top/width/height for Qt compatibility
    QString prefix = worldName + ":wp.";

    QSqlQuery query(m_db);

    // Helper lambda to save a single value
    auto saveValue = [&](const QString& key, int value) -> bool {
        // Try UPDATE first
        query.prepare("UPDATE worlds SET value = ? WHERE name = ?");
        query.addBindValue(QString::number(value));
        query.addBindValue(key);

        if (!query.exec()) {
            return false;
        }

        // If no rows updated, INSERT
        if (query.numRowsAffected() == 0) {
            query.prepare("INSERT INTO worlds (name, value) VALUES (?, ?)");
            query.addBindValue(key);
            query.addBindValue(QString::number(value));

            if (!query.exec()) {
                return false;
            }
        }
        return true;
    };

    // Save all four geometry values
    if (!saveValue(prefix + "left", geometry.x()))
        return false;
    if (!saveValue(prefix + "top", geometry.y()))
        return false;
    if (!saveValue(prefix + "width", geometry.width()))
        return false;
    if (!saveValue(prefix + "height", geometry.height()))
        return false;

    qCDebug(lcStorage) << "Saved window geometry for" << worldName << ":" << geometry;
    return true;
}

bool Database::loadWindowGeometry(const QString& worldName, QRect& geometry)
{
    if (!m_db.isOpen()) {
        qWarning() << "Cannot load window geometry: database not open";
        return false;
    }

    if (worldName.isEmpty()) {
        qWarning() << "Cannot load window geometry: world name is empty";
        return false;
    }

    // Load geometry using same keys as saveWindowGeometry
    QString prefix = worldName + ":wp.";

    QSqlQuery query(m_db);

    // Helper lambda to load a single value
    auto loadValue = [&](const QString& key, int& value) -> bool {
        query.prepare("SELECT value FROM worlds WHERE name = ?");
        query.addBindValue(key);

        if (!query.exec()) {
            return false;
        }

        if (query.next()) {
            value = query.value(0).toInt();
            return true;
        }

        return false; // Key not found
    };

    int left = 0, top = 0, width = 800, height = 600; // Defaults

    // Load all four values - if any are missing, return false
    if (!loadValue(prefix + "left", left))
        return false;
    if (!loadValue(prefix + "top", top))
        return false;
    if (!loadValue(prefix + "width", width))
        return false;
    if (!loadValue(prefix + "height", height))
        return false;

    geometry = QRect(left, top, width, height);
    qCDebug(lcStorage) << "Loaded window geometry for" << worldName << ":" << geometry;
    return true;
}
