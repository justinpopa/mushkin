#include "storage/database.h"

#include "../utils/app_paths.h"
#include "logging.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <array>
#include <span>

namespace {
StorageError makeError(StorageErrorType type, const QString& message)
{
    return StorageError{type, message};
}
} // namespace

Database::Database(QObject* parent) : QObject(parent)
{
}

Database::~Database()
{
    close();
}

Database& Database::instance()
{
    static Database instance;
    return instance;
}

std::expected<void, StorageError> Database::open()
{
    if (m_db.isOpen()) {
        return {};
    }

    const QString filename = "mushclient_prefs.sqlite";

    m_dbPath = QDir::currentPath() + "/" + filename;
    qCDebug(lcStorage) << "Trying working directory:" << m_dbPath;

    if (!QFile::exists(m_dbPath)) {
        m_dbPath = AppPaths::getAppDirectory() + "/" + filename;
        qCDebug(lcStorage) << "Trying application directory:" << m_dbPath;
    }

    qCDebug(lcStorage) << "Database path:" << m_dbPath;

    const bool databaseExists = QFile::exists(m_dbPath);

    if (QSqlDatabase::contains("preferences")) {
        m_db = QSqlDatabase::database("preferences");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "preferences");
    }
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        const QString errMsg = QString("Failed to open database: %1").arg(lastError());
        qWarning() << errMsg;
        emit error(errMsg);
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, errMsg));
    }

    qCDebug(lcStorage) << "Database opened successfully";

    if (!databaseExists) {
        qCDebug(lcStorage) << "Creating database schema...";
        const auto schemaResult = createSchema();
        if (!schemaResult.has_value()) {
            const QString errMsg =
                QString("Failed to create database schema: %1").arg(schemaResult.error().message);
            qWarning() << errMsg;
            emit error(errMsg);
            close();
            return std::unexpected(makeError(StorageErrorType::SchemaError, errMsg));
        }
        qCDebug(lcStorage) << "Database schema created successfully";
    } else {
        const auto migrationResult = migrateSchema();
        if (!migrationResult.has_value()) {
            const QString errMsg = QString("Failed to migrate database schema: %1")
                                       .arg(migrationResult.error().message);
            qWarning() << errMsg;
            emit error(errMsg);
            // Preserve previous behavior: keep DB open and continue.
        }
    }

    return {};
}

void Database::close()
{
    const QString connectionName = m_db.connectionName();

    if (m_db.isOpen()) {
        qCDebug(lcStorage) << "Closing database";
        m_db.close();
    }

    m_db = QSqlDatabase();

    if (!connectionName.isEmpty() && QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
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

std::expected<void, StorageError> Database::createSchema()
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot create schema: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    const QString createControl = R"(
        CREATE TABLE IF NOT EXISTS control (
            name VARCHAR(10) NOT NULL PRIMARY KEY,
            value INT NOT NULL
        )
    )";
    if (const auto res = execute(createControl); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const int dbVersion = getDatabaseVersion();
    if (dbVersion == 0) {
        if (const auto res = setDatabaseVersion(CURRENT_DB_VERSION); !res.has_value()) {
            return std::unexpected(res.error());
        }
        qCDebug(lcStorage) << "Set initial database version to" << CURRENT_DB_VERSION;
    }

    const QString createPrefs = R"(
        CREATE TABLE IF NOT EXISTS prefs (
            name VARCHAR(50) NOT NULL PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";
    if (const auto res = execute(createPrefs); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const QString createWorlds = R"(
        CREATE TABLE IF NOT EXISTS worlds (
            name VARCHAR(50) NOT NULL PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";
    if (const auto res = execute(createWorlds); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const QString createRecentFiles = R"(
        CREATE TABLE IF NOT EXISTS recent_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            timestamp INTEGER NOT NULL,
            file_size INTEGER DEFAULT 0,
            world_name TEXT DEFAULT ''
        )
    )";
    if (const auto res = execute(createRecentFiles); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const QString createRecentFilesIndex = R"(
        CREATE INDEX IF NOT EXISTS idx_recent_files_timestamp
        ON recent_files(timestamp DESC)
    )";
    if (const auto res = execute(createRecentFilesIndex); !res.has_value()) {
        return std::unexpected(res.error());
    }

    qCDebug(lcStorage) << "All database tables created successfully";
    return {};
}

std::expected<void, StorageError> Database::migrateSchema()
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot migrate schema: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    const QString createControl = R"(
        CREATE TABLE IF NOT EXISTS control (
            name VARCHAR(10) NOT NULL PRIMARY KEY,
            value INT NOT NULL
        )
    )";
    if (const auto res = execute(createControl); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const int currentVersion = getDatabaseVersion();
    qCDebug(lcStorage) << "Database version:" << currentVersion
                       << "Current version:" << CURRENT_DB_VERSION;

    if (currentVersion >= CURRENT_DB_VERSION) {
        qCDebug(lcStorage) << "No migration needed";
        return {};
    }

    qCDebug(lcStorage) << "Migrating database from version" << currentVersion << "to"
                       << CURRENT_DB_VERSION;

    if (currentVersion < 2) {
        qCDebug(lcStorage) << "Applying migration to version 2...";

        QSqlQuery query(m_db);
        if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND "
                        "name='recent_files'")) {
            const QString msg =
                QString("Failed to inspect sqlite_master: %1").arg(query.lastError().text());
            qWarning() << msg;
            return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
        }

        if (query.next()) {
            const auto fileSizeExists = columnExists("recent_files", "file_size");
            if (!fileSizeExists.has_value()) {
                return std::unexpected(fileSizeExists.error());
            }
            if (!fileSizeExists.value()) {
                qCDebug(lcStorage) << "Adding file_size column to recent_files";
                if (const auto res =
                        execute("ALTER TABLE recent_files ADD COLUMN file_size INTEGER DEFAULT 0");
                    !res.has_value()) {
                    return std::unexpected(res.error());
                }
            }

            const auto worldNameExists = columnExists("recent_files", "world_name");
            if (!worldNameExists.has_value()) {
                return std::unexpected(worldNameExists.error());
            }
            if (!worldNameExists.value()) {
                qCDebug(lcStorage) << "Adding world_name column to recent_files";
                if (const auto res =
                        execute("ALTER TABLE recent_files ADD COLUMN world_name TEXT DEFAULT ''");
                    !res.has_value()) {
                    return std::unexpected(res.error());
                }
            }

            const auto timestampExists = columnExists("recent_files", "timestamp");
            if (!timestampExists.has_value()) {
                return std::unexpected(timestampExists.error());
            }
            if (!timestampExists.value()) {
                qCDebug(lcStorage) << "Adding timestamp column to recent_files";
                if (const auto res =
                        execute("ALTER TABLE recent_files ADD COLUMN timestamp INTEGER DEFAULT 0");
                    !res.has_value()) {
                    return std::unexpected(res.error());
                }
            }
        } else {
            const QString createRecentFiles = R"(
                CREATE TABLE IF NOT EXISTS recent_files (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    path TEXT NOT NULL UNIQUE,
                    timestamp INTEGER NOT NULL,
                    file_size INTEGER DEFAULT 0,
                    world_name TEXT DEFAULT ''
                )
            )";
            if (const auto res = execute(createRecentFiles); !res.has_value()) {
                return std::unexpected(res.error());
            }

            const QString createIndex = R"(
                CREATE INDEX IF NOT EXISTS idx_recent_files_timestamp
                ON recent_files(timestamp DESC)
            )";
            if (const auto res = execute(createIndex); !res.has_value()) {
                return std::unexpected(res.error());
            }
        }

        if (const auto res =
                execute("CREATE TABLE IF NOT EXISTS prefs (name VARCHAR(50) NOT NULL PRIMARY KEY, "
                        "value TEXT NOT NULL)");
            !res.has_value()) {
            return std::unexpected(res.error());
        }

        if (const auto res =
                execute("CREATE TABLE IF NOT EXISTS worlds (name VARCHAR(50) NOT NULL PRIMARY KEY, "
                        "value TEXT NOT NULL)");
            !res.has_value()) {
            return std::unexpected(res.error());
        }
    }

    if (const auto res = setDatabaseVersion(CURRENT_DB_VERSION); !res.has_value()) {
        return std::unexpected(res.error());
    }

    qCDebug(lcStorage) << "Migration complete - now at version" << CURRENT_DB_VERSION;
    return {};
}

std::expected<bool, StorageError> Database::columnExists(const QString& tableName,
                                                         const QString& columnName)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot inspect schema: database not open";
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    QSqlQuery query(m_db);
    query.prepare("PRAGMA table_info(" + tableName + ")");

    if (!query.exec()) {
        const QString msg =
            QString("Failed to get table info for %1: %2").arg(tableName, query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    while (query.next()) {
        if (query.value(1).toString().compare(columnName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

std::expected<void, StorageError> Database::execute(const QString& sql)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot execute query: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        const QString msg =
            QString("Query failed: %1 | Error: %2").arg(sql, query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    return {};
}

QString Database::lastError() const
{
    if (!m_db.isValid()) {
        return "Database connection is invalid";
    }

    const QSqlError err = m_db.lastError();
    if (err.isValid()) {
        return err.text();
    }

    return QString();
}

std::expected<void, StorageError> Database::addRecentFile(const QString& path)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot add recent file: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    if (path.isEmpty()) {
        const QString msg = "Cannot add recent file: path is empty";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::InvalidArgument, msg));
    }

    const qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    QFileInfo fileInfo(path);
    const qint64 fileSize = fileInfo.size();

    QSqlQuery query(m_db);
    query.prepare("UPDATE recent_files SET timestamp = ?, file_size = ? WHERE path = ?");
    query.addBindValue(timestamp);
    query.addBindValue(fileSize);
    query.addBindValue(path);

    if (!query.exec()) {
        const QString msg =
            QString("Failed to update recent file: %1").arg(query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO recent_files (path, timestamp, file_size) VALUES (?, ?, ?)");
        query.addBindValue(path);
        query.addBindValue(timestamp);
        query.addBindValue(fileSize);

        if (!query.exec()) {
            const QString msg =
                QString("Failed to insert recent file: %1").arg(query.lastError().text());
            qWarning() << msg;
            return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
        }
    }

    qCDebug(lcStorage) << "Added recent file:" << path;
    return {};
}

QStringList Database::getRecentFiles(int maxCount)
{
    QStringList files;

    if (!m_db.isOpen()) {
        qWarning() << "Cannot get recent files: database not open";
        return files;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT path FROM recent_files ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(maxCount);

    if (!query.exec()) {
        qWarning() << "Failed to get recent files:" << query.lastError().text();
        return files;
    }

    while (query.next()) {
        const QString path = query.value(0).toString();
        if (QFile::exists(path)) {
            files.append(path);
        } else {
            qCDebug(lcStorage) << "Skipping non-existent recent file:" << path;
        }
    }

    qCDebug(lcStorage) << "Retrieved" << files.size() << "recent files";
    return files;
}

std::expected<void, StorageError> Database::clearRecentFiles()
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot clear recent files: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    if (const auto res = execute("DELETE FROM recent_files"); !res.has_value()) {
        return std::unexpected(res.error());
    }

    qCDebug(lcStorage) << "Cleared all recent files";
    return {};
}

std::expected<void, StorageError> Database::removeRecentFile(const QString& path)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot remove recent file: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM recent_files WHERE path = ?");
    query.addBindValue(path);

    if (!query.exec()) {
        const QString msg =
            QString("Failed to remove recent file: %1").arg(query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    qCDebug(lcStorage) << "Removed recent file:" << path;
    return {};
}

int Database::getDatabaseVersion()
{
    return getControlInt("database_version", 0);
}

std::expected<void, StorageError> Database::setDatabaseVersion(int version)
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

std::expected<void, StorageError> Database::setControlInt(const QString& name, int value)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot set control value: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE control SET value = ? WHERE name = ?");
    query.addBindValue(value);
    query.addBindValue(name);

    if (!query.exec()) {
        const QString msg =
            QString("Failed to update control table: %1").arg(query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO control (name, value) VALUES (?, ?)");
        query.addBindValue(name);
        query.addBindValue(value);

        if (!query.exec()) {
            const QString msg =
                QString("Failed to insert into control table: %1").arg(query.lastError().text());
            qWarning() << msg;
            return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
        }
    }

    return {};
}

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

std::expected<void, StorageError> Database::setPreference(const QString& name, const QString& value)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot set preference: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    const QString safeValue = value.isNull() ? QString("") : value;

    QSqlQuery query(m_db);
    query.prepare("UPDATE prefs SET value = ? WHERE name = ?");
    query.addBindValue(safeValue);
    query.addBindValue(name);

    if (!query.exec()) {
        const QString msg =
            QString("Failed to update prefs table: %1").arg(query.lastError().text());
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
    }

    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO prefs (name, value) VALUES (?, ?)");
        query.addBindValue(name);
        query.addBindValue(safeValue);

        if (!query.exec()) {
            const QString msg =
                QString("Failed to insert into prefs table: %1").arg(query.lastError().text());
            qWarning() << msg;
            return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
        }
    }

    return {};
}

int Database::getPreferenceInt(const QString& name, int defaultValue)
{
    const QString value = getPreference(name, QString::number(defaultValue));
    return value.toInt();
}

std::expected<void, StorageError> Database::setPreferenceInt(const QString& name, int value)
{
    return setPreference(name, QString::number(value));
}

std::expected<void, StorageError> Database::saveWindowGeometry(const QString& worldName,
                                                               const QRect& geometry)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot save window geometry: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    if (worldName.isEmpty()) {
        const QString msg = "Cannot save window geometry: world name is empty";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::InvalidArgument, msg));
    }

    const QString prefix = worldName + ":wp.";

    struct FieldValue {
        QString key;
        int value;
    };

    const std::array<FieldValue, 4> fields = {{
        {prefix + "left", geometry.x()},
        {prefix + "top", geometry.y()},
        {prefix + "width", geometry.width()},
        {prefix + "height", geometry.height()},
    }};

    QSqlQuery query(m_db);

    const auto saveValues =
        [&](std::span<const FieldValue> items) -> std::expected<void, StorageError> {
        for (const auto& item : items) {
            query.prepare("UPDATE worlds SET value = ? WHERE name = ?");
            query.addBindValue(QString::number(item.value));
            query.addBindValue(item.key);

            if (!query.exec()) {
                const QString msg = QString("Failed to update worlds entry '%1': %2")
                                        .arg(item.key, query.lastError().text());
                return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
            }

            if (query.numRowsAffected() == 0) {
                query.prepare("INSERT INTO worlds (name, value) VALUES (?, ?)");
                query.addBindValue(item.key);
                query.addBindValue(QString::number(item.value));

                if (!query.exec()) {
                    const QString msg = QString("Failed to insert worlds entry '%1': %2")
                                            .arg(item.key, query.lastError().text());
                    return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
                }
            }
        }
        return {};
    };

    if (const auto res = saveValues(std::span<const FieldValue>(fields)); !res.has_value()) {
        return std::unexpected(res.error());
    }

    qCDebug(lcStorage) << "Saved window geometry for" << worldName << ":" << geometry;
    return {};
}

std::expected<QRect, StorageError> Database::loadWindowGeometry(const QString& worldName)
{
    if (!m_db.isOpen()) {
        const QString msg = "Cannot load window geometry: database not open";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::ConnectionFailed, msg));
    }

    if (worldName.isEmpty()) {
        const QString msg = "Cannot load window geometry: world name is empty";
        qWarning() << msg;
        return std::unexpected(makeError(StorageErrorType::InvalidArgument, msg));
    }

    const QString prefix = worldName + ":wp.";

    struct FieldRef {
        QString key;
        int* value;
    };

    int left = 0;
    int top = 0;
    int width = 800;
    int height = 600;

    const std::array<FieldRef, 4> fields = {{
        {prefix + "left", &left},
        {prefix + "top", &top},
        {prefix + "width", &width},
        {prefix + "height", &height},
    }};

    QSqlQuery query(m_db);

    const auto loadValues =
        [&](std::span<const FieldRef> items) -> std::expected<void, StorageError> {
        for (const auto& item : items) {
            query.prepare("SELECT value FROM worlds WHERE name = ?");
            query.addBindValue(item.key);

            if (!query.exec()) {
                const QString msg = QString("Failed to load worlds entry '%1': %2")
                                        .arg(item.key, query.lastError().text());
                return std::unexpected(makeError(StorageErrorType::QueryFailed, msg));
            }

            if (!query.next()) {
                const QString msg = QString("Window geometry key not found: %1").arg(item.key);
                return std::unexpected(makeError(StorageErrorType::NotFound, msg));
            }

            *item.value = query.value(0).toInt();
        }
        return {};
    };

    if (const auto res = loadValues(std::span<const FieldRef>(fields)); !res.has_value()) {
        return std::unexpected(res.error());
    }

    const QRect geometry(left, top, width, height);
    qCDebug(lcStorage) << "Loaded window geometry for" << worldName << ":" << geometry;
    return geometry;
}
