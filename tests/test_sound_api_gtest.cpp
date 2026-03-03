/**
 * test_sound_api_gtest.cpp - Sound API Tests
 *
 * Verifies:
 * 1. world.PlaySound() function exists and is callable
 * 2. world.StopSound() function exists and is callable
 * 3. world.Sound() function exists and is callable
 * 4. world.GetSoundStatus() function exists and is callable
 * 5. Functions return correct error codes
 */

#include "../src/world/lua_api/lua_common.h"
#include "../src/world/script_engine.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"

// Test fixture for Sound API tests
class SoundApiTest : public LuaWorldTest {
  protected:
    void SetUp() override
    {
        LuaWorldTest::SetUp();
    }
};

/**
 * Test: world.PlaySound function exists
 */
TEST_F(SoundApiTest, PlaySoundExists)
{
    executeLua("result = (type(world.PlaySound) == 'function')");
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "world.PlaySound should be a function";
    lua_pop(L, 1);
}

/**
 * Test: world.StopSound function exists
 */
TEST_F(SoundApiTest, StopSoundExists)
{
    executeLua("result = (type(world.StopSound) == 'function')");
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "world.StopSound should be a function";
    lua_pop(L, 1);
}

/**
 * Test: world.Sound function exists
 */
TEST_F(SoundApiTest, SoundExists)
{
    executeLua("result = (type(world.Sound) == 'function')");
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "world.Sound should be a function";
    lua_pop(L, 1);
}

/**
 * Test: world.GetSoundStatus function exists
 */
TEST_F(SoundApiTest, GetSoundStatusExists)
{
    executeLua("result = (type(world.GetSoundStatus) == 'function')");
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "world.GetSoundStatus should be a function";
    lua_pop(L, 1);
}

/**
 * Test: world.Sound() returns error code for nonexistent file
 */
TEST_F(SoundApiTest, SoundReturnsErrorCode)
{
    executeLua("result = world.Sound('nonexistent.wav')");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eCannotPlaySound)
        << "Sound() should return eCannotPlaySound for nonexistent file";
}

/**
 * Test: world.GetSoundStatus() returns correct codes for invalid buffers
 */
TEST_F(SoundApiTest, GetSoundStatusInvalidBuffer)
{
    // Buffer out of range (too high)
    executeLua("result = world.GetSoundStatus(999)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, -1) << "GetSoundStatus should return -1 for buffer out of range";

    // Buffer out of range (negative)
    executeLua("result = world.GetSoundStatus(-1)");
    result = getGlobalInt("result");
    EXPECT_EQ(result, -1) << "GetSoundStatus should return -1 for negative buffer";
}

/**
 * Test: world.GetSoundStatus() returns -2 for free buffer
 */
TEST_F(SoundApiTest, GetSoundStatusFreeBuffer)
{
    // Buffer 1 should be free initially (no sound loaded)
    executeLua("result = world.GetSoundStatus(1)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, -2) << "GetSoundStatus should return -2 for free buffer";
}

/**
 * Test: world.StopSound(0) stops all buffers
 */
TEST_F(SoundApiTest, StopSoundAllBuffers)
{
    executeLua("result = world.StopSound(0)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eOK) << "StopSound(0) should return eOK";
}

/**
 * Test: world.PlaySound() accepts all parameters
 */
TEST_F(SoundApiTest, PlaySoundWithParameters)
{
    // This will fail (file doesn't exist) but should accept parameters
    // Original MUSHclient returns eFileNotFound for missing sound files
    executeLua("result = world.PlaySound(1, 'test.wav', false, 0, 0)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eFileNotFound)
        << "PlaySound should return eFileNotFound for nonexistent file";
}
