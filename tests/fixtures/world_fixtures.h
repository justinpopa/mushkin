/**
 * world_fixtures.h - Shared GoogleTest fixtures for WorldDocument-based tests
 *
 * Fixture hierarchy:
 *   ::testing::Test
 *   └── WorldDocumentTest       — doc = make_unique<WorldDocument>()
 *       └── LuaWorldTest        — + L = doc->m_ScriptEngine->L + Lua helpers
 *           └── ConnectedWorldTest — + world config, currentLine, style
 */

#pragma once

#include "../../src/text/line.h"
#include "../../src/text/style.h"
#include "../../src/world/script_engine.h"
#include "../../src/world/world_document.h"
#include <gtest/gtest.h>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

/// Base fixture: creates a WorldDocument.
class WorldDocumentTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        doc = std::make_unique<WorldDocument>();
    }

    std::unique_ptr<WorldDocument> doc;
};

/// Fixture with Lua state access and common Lua helpers.
class LuaWorldTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        ASSERT_NE(doc->m_ScriptEngine, nullptr) << "ScriptEngine should exist";
        ASSERT_NE(doc->m_ScriptEngine->L, nullptr) << "Lua state should exist";
        L = doc->m_ScriptEngine->L;
    }

    void executeLua(const char* code)
    {
        ASSERT_EQ(luaL_loadstring(L, code), 0) << "Lua code should compile: " << code;
        ASSERT_EQ(lua_pcall(L, 0, 0, 0), 0) << "Lua code should execute: " << code;
    }

    QString getGlobalString(const char* name)
    {
        lua_getglobal(L, name);
        const char* str = lua_tostring(L, -1);
        QString result = str ? QString::fromUtf8(str) : QString();
        lua_pop(L, 1);
        return result;
    }

    double getGlobalNumber(const char* name)
    {
        lua_getglobal(L, name);
        double result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    int getGlobalInt(const char* name)
    {
        lua_getglobal(L, name);
        int result = lua_tointeger(L, -1);
        lua_pop(L, 1);
        return result;
    }

    bool getGlobalBool(const char* name)
    {
        lua_getglobal(L, name);
        bool result = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return result;
    }

    bool isGlobalNil(const char* name)
    {
        lua_getglobal(L, name);
        bool result = lua_isnil(L, -1);
        lua_pop(L, 1);
        return result;
    }

    bool functionExists(const char* tableName, const char* funcName)
    {
        lua_getglobal(L, tableName);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_getfield(L, -1, funcName);
        bool exists = lua_isfunction(L, -1);
        lua_pop(L, 2);
        return exists;
    }

    lua_State* L = nullptr;
};

/// Fixture with full connected world state (line, style, connection phase).
/// Use for tests that call note(), ColourNote(), or depend on current line state.
class ConnectedWorldTest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();

        doc->m_mush_name = "Test World";
        doc->m_server = "test.mud.com";
        doc->m_port = 4000;
        doc->m_connectionManager->m_iConnectPhase = eConnectConnectedToMud;
        doc->m_display.utf8 = true;

        // Create initial line (needed for note() to work)
        doc->m_currentLine =
            std::make_unique<Line>(1, 80, 0, qRgb(192, 192, 192), qRgb(0, 0, 0), true);
        auto initialStyle = std::make_unique<Style>();
        initialStyle->iLength = 0;
        initialStyle->iFlags = COLOUR_RGB;
        initialStyle->iForeColour = qRgb(192, 192, 192);
        initialStyle->iBackColour = qRgb(0, 0, 0);
        initialStyle->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(initialStyle));

        // Set current style
        doc->m_iFlags = COLOUR_RGB;
        doc->m_iForeColour = qRgb(192, 192, 192);
        doc->m_iBackColour = qRgb(0, 0, 0);
    }
};
