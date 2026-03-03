// ssh_host_key_manager.h
// Manages Ed25519 host keys for the SSH remote access server
//
// Part of Remote Access Server feature

#ifndef SSH_HOST_KEY_MANAGER_H
#define SSH_HOST_KEY_MANAGER_H

#include <QString>
#include <expected>

enum class HostKeyError {
    DirectoryCreationFailed,
    KeyGenerationFailed,
    KeyExportFailed,
    KeyImportFailed,
    FingerprintFailed,
};

/** Manages SSH host key generation, storage, and retrieval. */
class SshHostKeyManager {
  public:
    /**
     * Ensure an Ed25519 host key exists at the standard location.
     * Generates one on first use.
     * Returns the path to the key file.
     */
    [[nodiscard]] static auto ensureHostKey() -> std::expected<QString, HostKeyError>;

    /**
     * Return the SHA256 fingerprint of the host key for UI display.
     */
    [[nodiscard]] static auto fingerprint() -> std::expected<QString, HostKeyError>;

  private:
    /** Standard key storage path: QStandardPaths::AppConfigLocation + "/ssh/host_ed25519" */
    static auto keyPath() -> QString;
};

#endif // SSH_HOST_KEY_MANAGER_H
