#ifndef DATABASE_H
#define DATABASE_H

#include <QDateTime>
#include <QObject>
#include <QRect>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

// Database version (for schema migrations)
// Increment this when adding migrations
// Version history:
//   1 - Initial schema (control, prefs, worlds, recent_files)
//   2 - Ensure recent_files has file_size and world_name columns
#define CURRENT_DB_VERSION 2

/**
 * Database - Global preferences database manager
 *
 * Manages the SQLite database for global application preferences.
 * This is SEPARATE from world files (.mcl) which store per-world settings.
 *
 * Schema matches original MUSHclient for backward compatibility:
 * - control table: database version tracking and UI state
 * - prefs table: global preferences (key-value pairs)
 * - worlds table: world window geometry (key-value pairs)
 * - recent_files table: recent files list (NEW - cross-platform MRU)
 *
 * Database location (matches original MUSHclient):
 * 1. Working directory: ./mushclient_prefs.sqlite
 * 2. Application directory (fallback): <app-dir>/mushclient_prefs.sqlite
 *
 * This approach allows:
 * - Drop-in compatibility with original MUSHclient
 * - Portable installations (database next to world files)
 * - Multiple instances with different preferences
 *
 * Source: MUSHclient.cpp CMUSHclientApp::InitInstance()
 */
class Database : public QObject {
    Q_OBJECT

  public:
    /**
     * Get singleton instance
     */
    static Database* instance();

    /**
     * Destructor
     */
    ~Database() override;

    /**
     * Open or create the preferences database
     *
     * @return true if successful, false on error
     */
    bool open();

    /**
     * Close the database
     */
    void close();

    /**
     * Check if database is open
     */
    bool isOpen() const;

    /**
     * Get the database file path
     */
    QString databasePath() const;

    /**
     * Execute a SQL query (for internal use or testing)
     *
     * @param sql SQL statement to execute
     * @return true if successful, false on error
     */
    bool execute(const QString& sql);

    // Recent Files Operations

    /**
     * Add a world file to the recent files list
     *
     * @param path Absolute path to world file
     * @return true if successful
     */
    bool addRecentFile(const QString& path);

    /**
     * Get the recent files list
     *
     * @param maxCount Maximum number of files to return (default 10)
     * @return List of recent file paths, most recent first
     */
    QStringList getRecentFiles(int maxCount = 10);

    /**
     * Clear the recent files list
     */
    bool clearRecentFiles();

    /**
     * Remove a specific file from recent files
     *
     * @param path Path to remove
     */
    bool removeRecentFile(const QString& path);

    // Control Table Operations (database metadata)

    /**
     * Get database version
     *
     * @return Database version number, or 0 if not set
     */
    int getDatabaseVersion();

    /**
     * Set database version
     *
     * @param version Version number to set
     * @return true if successful
     */
    bool setDatabaseVersion(int version);

    /**
     * Get integer value from control table
     *
     * @param name Key name
     * @param defaultValue Default if not found
     * @return Value from database or default
     */
    int getControlInt(const QString& name, int defaultValue = 0);

    /**
     * Set integer value in control table
     *
     * @param name Key name
     * @param value Value to set
     * @return true if successful
     */
    bool setControlInt(const QString& name, int value);

    // Prefs Table Operations (global preferences)

    /**
     * Get string preference
     *
     * @param name Preference name
     * @param defaultValue Default if not found
     * @return Preference value or default
     */
    QString getPreference(const QString& name, const QString& defaultValue = QString());

    /**
     * Set string preference
     *
     * @param name Preference name
     * @param value Value to set
     * @return true if successful
     */
    bool setPreference(const QString& name, const QString& value);

    /**
     * Get integer preference
     *
     * @param name Preference name
     * @param defaultValue Default if not found
     * @return Preference value or default
     */
    int getPreferenceInt(const QString& name, int defaultValue = 0);

    /**
     * Set integer preference
     *
     * @param name Preference name
     * @param value Value to set
     * @return true if successful
     */
    bool setPreferenceInt(const QString& name, int value);

    // Worlds Table Operations (per-world window geometry)

    /**
     * Save window geometry for a world
     * Stores in worlds table with keys: "{worldname}:wp.left", etc.
     *
     * @param worldName World name (key prefix)
     * @param geometry Window geometry (x, y, width, height)
     * @return true if successful
     */
    bool saveWindowGeometry(const QString& worldName, const QRect& geometry);

    /**
     * Load window geometry for a world
     * Reads from worlds table with keys: "{worldname}:wp.left", etc.
     *
     * @param worldName World name (key prefix)
     * @param geometry Output parameter for geometry
     * @return true if geometry found, false if not
     */
    bool loadWindowGeometry(const QString& worldName, QRect& geometry);

  signals:
    /**
     * Emitted when a database error occurs
     */
    void error(const QString& message);

  private:
    /**
     * Private constructor for singleton
     */
    explicit Database(QObject* parent = nullptr);

    /**
     * Create database schema (tables)
     *
     * @return true if successful
     */
    bool createSchema();

    /**
     * Migrate database schema to current version
     *
     * Called when opening an existing database that has an older schema version.
     * Applies migrations sequentially from the current version to CURRENT_DB_VERSION.
     *
     * @return true if successful (or no migration needed)
     */
    bool migrateSchema();

    /**
     * Check if a column exists in a table
     *
     * @param tableName Table to check
     * @param columnName Column to look for
     * @return true if column exists
     */
    bool columnExists(const QString& tableName, const QString& columnName);

    /**
     * Get last error message
     */
    QString lastError() const;

    // Singleton instance
    static Database* s_instance;

    // Database connection
    QSqlDatabase m_db;

    // Database file path
    QString m_dbPath;

    // Delete copy constructor and assignment operator
    Q_DISABLE_COPY(Database)
};

#endif // DATABASE_H
