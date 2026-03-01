// ssh_host_key_manager.cpp
// Manages Ed25519 host keys for the SSH remote access server

#include "ssh_host_key_manager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <libssh/libssh.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

struct SshKeyGuard {
    ssh_key key{nullptr};
    explicit SshKeyGuard(ssh_key k) : key(k)
    {
    }
    ~SshKeyGuard()
    {
        if (key != nullptr) {
            ssh_key_free(key);
        }
    }
    SshKeyGuard(const SshKeyGuard&) = delete;
    SshKeyGuard& operator=(const SshKeyGuard&) = delete;
};

struct SshStringGuard {
    char* str{nullptr};
    explicit SshStringGuard(char* s) : str(s)
    {
    }
    ~SshStringGuard()
    {
        if (str != nullptr) {
            ssh_string_free_char(str);
        }
    }
    SshStringGuard(const SshStringGuard&) = delete;
    SshStringGuard& operator=(const SshStringGuard&) = delete;
};

struct SshHashGuard {
    unsigned char* hash{nullptr};
    SshHashGuard() = default;
    ~SshHashGuard()
    {
        if (hash != nullptr) {
            ssh_clean_pubkey_hash(&hash);
        }
    }
    SshHashGuard(const SshHashGuard&) = delete;
    SshHashGuard& operator=(const SshHashGuard&) = delete;
};

} // namespace

// ---------------------------------------------------------------------------
// SshHostKeyManager
// ---------------------------------------------------------------------------

auto SshHostKeyManager::keyPath() -> QString
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/ssh/host_ed25519");
}

auto SshHostKeyManager::ensureHostKey() -> std::expected<QString, HostKeyError>
{
    const QString path = keyPath();

    if (QFile::exists(path)) {
        return path;
    }

    // Create containing directory if needed.
    const QString dirPath = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(dirPath)) {
        return std::unexpected(HostKeyError::DirectoryCreationFailed);
    }

    // Generate a new Ed25519 key.
    ssh_key key = nullptr;
    if (ssh_pki_generate(SSH_KEYTYPE_ED25519, 0, &key) != SSH_OK) {
        return std::unexpected(HostKeyError::KeyGenerationFailed);
    }
    SshKeyGuard key_guard(key);

    // Export private key to disk.
    if (ssh_pki_export_privkey_file(key, nullptr, nullptr, nullptr, path.toUtf8().constData()) !=
        SSH_OK) {
        return std::unexpected(HostKeyError::KeyExportFailed);
    }

    return path;
}

auto SshHostKeyManager::fingerprint() -> std::expected<QString, HostKeyError>
{
    auto path_result = ensureHostKey();
    if (!path_result.has_value()) {
        return std::unexpected(path_result.error());
    }
    const QString path = path_result.value();

    // Import private key.
    ssh_key privkey = nullptr;
    if (ssh_pki_import_privkey_file(path.toUtf8().constData(), nullptr, nullptr, nullptr,
                                    &privkey) != SSH_OK) {
        return std::unexpected(HostKeyError::KeyImportFailed);
    }
    SshKeyGuard privkey_guard(privkey);

    // Extract public key from private key.
    ssh_key pubkey = nullptr;
    if (ssh_pki_export_privkey_to_pubkey(privkey, &pubkey) != SSH_OK) {
        return std::unexpected(HostKeyError::FingerprintFailed);
    }
    SshKeyGuard pubkey_guard(pubkey);

    // Compute SHA256 hash of the public key.
    unsigned char* hash = nullptr;
    std::size_t hlen = 0;
    SshHashGuard hash_guard;
    if (ssh_get_publickey_hash(pubkey, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen) != SSH_OK) {
        return std::unexpected(HostKeyError::FingerprintFailed);
    }
    hash_guard.hash = hash;

    // Format as human-readable fingerprint string.
    char* fp_str = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
    if (fp_str == nullptr) {
        return std::unexpected(HostKeyError::FingerprintFailed);
    }
    SshStringGuard fp_guard(fp_str);

    return QString::fromUtf8(fp_str);
}
