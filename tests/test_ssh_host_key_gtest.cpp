#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <gtest/gtest.h>

#include "../src/network/ssh_host_key_manager.h"

class SshHostKeyTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        QStandardPaths::setTestModeEnabled(true);
        // Clean up any previous test keys.
        const QString test_key_dir =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ssh";
        QDir(test_key_dir).removeRecursively();
    }

    void TearDown() override
    {
        // Clean up test keys.
        const QString test_key_dir =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ssh";
        QDir(test_key_dir).removeRecursively();
        QStandardPaths::setTestModeEnabled(false);
    }
};

TEST_F(SshHostKeyTest, EnsureHostKeyCreatesFile)
{
    auto result = SshHostKeyManager::ensureHostKey();
    ASSERT_TRUE(result.has_value()) << "ensureHostKey() failed";
    EXPECT_TRUE(QFile::exists(result.value()));
}

TEST_F(SshHostKeyTest, EnsureHostKeyIsIdempotent)
{
    auto result1 = SshHostKeyManager::ensureHostKey();
    ASSERT_TRUE(result1.has_value());
    auto result2 = SshHostKeyManager::ensureHostKey();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result1.value(), result2.value());
}

TEST_F(SshHostKeyTest, FingerprintIsNonEmpty)
{
    auto result = SshHostKeyManager::fingerprint();
    ASSERT_TRUE(result.has_value()) << "fingerprint() failed";
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_TRUE(result.value().startsWith("SHA256:") || result.value().contains(':'));
}
