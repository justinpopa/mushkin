/**
 * test_main_gui.cpp - Shared main() for GUI-dependent GoogleTest tests
 *
 * Provides QApplication + GoogleTest initialization.
 * Use this for tests that create widgets, progress dialogs,
 * or anything requiring a full GUI event loop.
 */

#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    QApplication app(argc, argv);
    return RUN_ALL_TESTS();
}
