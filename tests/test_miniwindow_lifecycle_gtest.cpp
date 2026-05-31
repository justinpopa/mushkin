/**
 * test_miniwindow_lifecycle_gtest.cpp
 *
 * Behavioral-parity regression tests for WindowCreate lifecycle semantics.
 *
 * Covered deviations (f011):
 *   M43/LOW  — When KEEP_HOTSPOTS is NOT set, DeleteAllHotspots() semantics:
 *              mouseOverHotspot, mouseDownHotspot, and callbackPlugin must also
 *              be cleared (original: miniwindow.cpp:1833-1844).
 *   L37/M43  — creatingPlugin is always erased before conditionally setting;
 *              callbackPlugin is never written in WindowCreate (original:
 *              methods_miniwindows.cpp:130-132).
 *   M43 MEDIUM — callbackPlugin must NOT be overwritten when KEEP_HOTSPOTS is
 *               set (hotspot ownership is preserved).
 */

#include "../src/world/miniwindow.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"

class MiniWindowLifecycleTest : public LuaWorldTest {
  protected:
    // Creates "win" via Lua and returns a pointer to the MiniWindow.
    MiniWindow* createWindow(const char* name = "win", int flags = 0)
    {
        QString code = QString("world.WindowCreate('%1', 0, 0, 100, 100, 0, %2, 0xFF000000)")
                           .arg(name)
                           .arg(flags);
        executeLua(code.toUtf8().constData());
        auto it = doc->m_MiniWindowMap.find(name);
        if (it == doc->m_MiniWindowMap.end())
            return nullptr;
        return it->second.get();
    }
};

// ---------------------------------------------------------------------------
// L37 / M43 MEDIUM: creatingPlugin always erased first
// ---------------------------------------------------------------------------

// Outside plugin context: creatingPlugin must be empty after WindowCreate.
TEST_F(MiniWindowLifecycleTest, CreatingPluginClearedOutsidePluginContext)
{
    MiniWindow* win = createWindow();
    ASSERT_NE(win, nullptr);
    EXPECT_TRUE(win->getCreatingPlugin().isEmpty())
        << "creatingPlugin must be empty when WindowCreate is called outside a plugin";
}

// If a window previously had a creatingPlugin (e.g. created from a plugin),
// a subsequent WindowCreate from non-plugin context must erase it.
TEST_F(MiniWindowLifecycleTest, CreatingPluginResetOnSubsequentWindowCreate)
{
    MiniWindow* win = createWindow();
    ASSERT_NE(win, nullptr);

    // Simulate a prior plugin value manually.
    win->setCreatingPlugin("plugin_a");

    // Re-create the same window from non-plugin context.
    executeLua("world.WindowCreate('win', 0, 0, 50, 50, 0, 0, 0xFF000000)");

    EXPECT_TRUE(win->getCreatingPlugin().isEmpty())
        << "creatingPlugin must be cleared when WindowCreate is called outside a plugin";
}

// ---------------------------------------------------------------------------
// M43 MEDIUM: callbackPlugin never set by WindowCreate
// ---------------------------------------------------------------------------

// WindowCreate must not set callbackPlugin regardless of plugin context.
TEST_F(MiniWindowLifecycleTest, CallbackPluginNotSetByWindowCreate)
{
    MiniWindow* win = createWindow();
    ASSERT_NE(win, nullptr);
    EXPECT_TRUE(win->callbackPlugin.isEmpty())
        << "callbackPlugin must not be written by WindowCreate (only AddHotspot sets it)";
}

// If callbackPlugin was set (e.g. via hotspot), a plain WindowCreate without
// KEEP_HOTSPOTS must clear it (DeleteAllHotspots semantics).
TEST_F(MiniWindowLifecycleTest, CallbackPluginClearedWhenHotspotsNotKept)
{
    MiniWindow* win = createWindow();
    ASSERT_NE(win, nullptr);

    win->callbackPlugin = "plugin_x";

    // Re-create without KEEP_HOTSPOTS.
    executeLua("world.WindowCreate('win', 0, 0, 50, 50, 0, 0, 0xFF000000)");

    EXPECT_TRUE(win->callbackPlugin.isEmpty())
        << "callbackPlugin must be cleared when KEEP_HOTSPOTS is not set";
}

// With KEEP_HOTSPOTS set, callbackPlugin must be preserved.
TEST_F(MiniWindowLifecycleTest, CallbackPluginPreservedWhenKeepHotspotsSet)
{
    // MINIWINDOW_KEEP_HOTSPOTS == 0x10 (16)
    MiniWindow* win = createWindow("win", 0);
    ASSERT_NE(win, nullptr);

    win->callbackPlugin = "plugin_y";

    // Re-create with KEEP_HOTSPOTS flag.
    executeLua("world.WindowCreate('win', 0, 0, 50, 50, 0, 16, 0xFF000000)");

    EXPECT_EQ(win->callbackPlugin, "plugin_y")
        << "callbackPlugin must be preserved when KEEP_HOTSPOTS is set";
}

// ---------------------------------------------------------------------------
// M43 LOW: mouseOverHotspot / mouseDownHotspot cleared with hotspots
// ---------------------------------------------------------------------------

TEST_F(MiniWindowLifecycleTest, MouseHotspotsAreResetWhenHotspotsCleared)
{
    MiniWindow* win = createWindow();
    ASSERT_NE(win, nullptr);

    // Simulate stale mouse state.
    win->mouseOverHotspot = "hotspot_1";
    win->mouseDownHotspot = "hotspot_1";

    // Re-create without KEEP_HOTSPOTS.
    executeLua("world.WindowCreate('win', 0, 0, 50, 50, 0, 0, 0xFF000000)");

    EXPECT_TRUE(win->mouseOverHotspot.isEmpty())
        << "mouseOverHotspot must be cleared by WindowCreate when KEEP_HOTSPOTS not set";
    EXPECT_TRUE(win->mouseDownHotspot.isEmpty())
        << "mouseDownHotspot must be cleared by WindowCreate when KEEP_HOTSPOTS not set";
}

// With KEEP_HOTSPOTS, mouse-over/down state must be preserved.
TEST_F(MiniWindowLifecycleTest, MouseHotspotsPreservedWhenKeepHotspotsSet)
{
    MiniWindow* win = createWindow("win", 0);
    ASSERT_NE(win, nullptr);

    win->mouseOverHotspot = "hotspot_2";
    win->mouseDownHotspot = "hotspot_2";

    // Re-create with KEEP_HOTSPOTS (flag 16).
    executeLua("world.WindowCreate('win', 0, 0, 50, 50, 0, 16, 0xFF000000)");

    EXPECT_EQ(win->mouseOverHotspot, "hotspot_2")
        << "mouseOverHotspot must be preserved when KEEP_HOTSPOTS is set";
    EXPECT_EQ(win->mouseDownHotspot, "hotspot_2")
        << "mouseDownHotspot must be preserved when KEEP_HOTSPOTS is set";
}
