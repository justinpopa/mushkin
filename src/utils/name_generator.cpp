#include "name_generator.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QRandomGenerator>
#include <QTextStream>
#include <QUuid>
#include <QVector>

/**
 * generateCharacterName - Generate a random fantasy name using Markov chains
 *
 * Uses a 2nd-order Markov chain trained on a corpus of fantasy names.
 * Reads names from embedded resource file and generates new names that
 * have similar phonetic patterns.
 */
QString generateCharacterName()
{
    // Read training corpus from embedded resource
    QFile file(":/data/resources/names.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open names.txt resource";
        return QString();
    }

    QTextStream in(&file);
    QStringList trainingNames;

    // Load all names from the corpus
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            trainingNames.append(line.toLower());
        }
    }
    file.close();

    if (trainingNames.isEmpty()) {
        qWarning() << "Name generation failed - empty corpus";
        return QString();
    }

    // Build 2nd-order Markov chain (trigrams)
    // Map from 2-character prefix to list of possible next characters
    QMap<QString, QVector<QChar>> chain;
    QVector<QString> startBigrams; // Starting bigrams for name generation

    for (const QString& name : trainingNames) {
        // Add start markers
        QString augmented = "^^" + name + "$";

        // Build trigrams
        for (int i = 0; i < augmented.length() - 2; ++i) {
            QString prefix = augmented.mid(i, 2);
            QChar next = augmented[i + 2];

            chain[prefix].append(next);

            // Collect starting bigrams (after ^^)
            if (prefix == "^^") {
                startBigrams.append(QString(next));
            }
        }
    }

    if (startBigrams.isEmpty()) {
        qWarning() << "Name generation failed - no start bigrams";
        return QString();
    }

    // Generate a new name
    QRandomGenerator* rng = QRandomGenerator::global();

    // Pick a random starting character
    QString current = "^" + startBigrams[rng->bounded(startBigrams.size())];
    QString result;

    const int maxLength = 12; // Maximum name length
    const int minLength = 4;  // Minimum name length

    for (int i = 0; i < maxLength; ++i) {
        // Look up possible next characters
        if (!chain.contains(current)) {
            break;
        }

        const QVector<QChar>& possibleNext = chain[current];
        if (possibleNext.isEmpty()) {
            break;
        }

        // Pick a random next character
        QChar next = possibleNext[rng->bounded(possibleNext.size())];

        // Check for end marker
        if (next == '$') {
            // Only accept if name is long enough
            if (result.length() >= minLength) {
                break;
            }
            // Otherwise continue with a different character
            continue;
        }

        // Append character and update current bigram
        result.append(next);
        current = current.mid(1) + next;
    }

    // Capitalize first letter
    if (!result.isEmpty()) {
        result[0] = result[0].toUpper();
    }

    // If we got an empty or too-short name, fall back to picking a random training name
    if (result.length() < minLength) {
        result = trainingNames[rng->bounded(trainingNames.size())];
        if (!result.isEmpty()) {
            result[0] = result[0].toUpper();
        }
    }

    return result;
}

/**
 * generateUniqueID - Generate a cryptographically unique identifier
 *
 * Creates a UUID and hashes it with SHA-1 to produce a 24-character
 * hex string suitable for use as plugin IDs or other unique identifiers.
 *
 * Original MUSHclient: GUID (no dashes) + SHA-1 + first 24 hex chars
 */
QString generateUniqueID()
{
    // Generate a UUID
    QUuid uuid = QUuid::createUuid();
    QString uuidStr = uuid.toString(QUuid::WithoutBraces);

    // Remove dashes to match original behavior
    uuidStr.remove('-');

    // Hash it with SHA-1
    QByteArray uuidBytes = uuidStr.toUtf8();
    QByteArray hash = QCryptographicHash::hash(uuidBytes, QCryptographicHash::Sha1);

    // Convert to hex string and take first 24 characters
    // (Original PLUGIN_UNIQUE_ID_LENGTH = 24)
    QString hexHash = hash.toHex().left(24);

    return hexHash;
}

/**
 * createGUID - Create a GUID in standard format
 *
 * Returns a UUID/GUID string with dashes in the format:
 * XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
 * (matching original MUSHclient CreateGUID function)
 */
QString createGUID()
{
    QUuid uuid = QUuid::createUuid();
    // Use WithoutBraces to get format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    return uuid.toString(QUuid::WithoutBraces).toUpper();
}
