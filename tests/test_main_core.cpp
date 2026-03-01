/**
 * test_main_core.cpp - Shared main() for non-GUI GoogleTest tests
 *
 * Provides QCoreApplication + GoogleTest initialization.
 * Link against the test_main_core OBJECT library instead of
 * writing a custom main() in every test file.
 */

#include <QCoreApplication>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
