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
    // -1 = out of range, -3 = audio engine not available (both valid in test env)
    EXPECT_TRUE(result == -1 || result == -3)
        << "GetSoundStatus should return -1 (out of range) or -3 (no audio), got " << result;

    // Buffer out of range (negative)
    executeLua("result = world.GetSoundStatus(-1)");
    result = getGlobalInt("result");
    EXPECT_TRUE(result == -1 || result == -3)
        << "GetSoundStatus should return -1 (out of range) or -3 (no audio), got " << result;
}

/**
 * Test: world.GetSoundStatus() returns -2 for free buffer
 */
TEST_F(SoundApiTest, GetSoundStatusFreeBuffer)
{
    // Buffer 1 should be free initially (no sound loaded)
    executeLua("result = world.GetSoundStatus(1)");
    int result = getGlobalInt("result");
    // -2 = buffer free (audio engine initialized), -3 = audio engine not available
    EXPECT_TRUE(result == -2 || result == -3)
        << "GetSoundStatus should return -2 (free) or -3 (no audio engine), got " << result;
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

// ---------------------------------------------------------------------------
// PlaySoundMemory parity tests (behavioral deviations H83 / M50)
// ---------------------------------------------------------------------------

/**
 * Test: world.PlaySoundMemory() buffer arg (arg 1) is required — nil raises Lua error.
 * Original lua_methods.cpp:4517 uses my_checknumber(L,1) which errors on nil.
 * Mushkin previously used luaL_optinteger(L,1,0) which silently defaulted to 0.
 */
TEST_F(SoundApiTest, PlaySoundMemoryBufferArgRequired)
{
    const char* code = R"(
        local ok = pcall(function()
            world.PlaySoundMemory(nil, "wav", false, 0, 0)
        end)
        return not ok  -- should have errored
    )";
    ASSERT_EQ(luaL_dostring(L, code), 0) << lua_tostring(L, -1);
    EXPECT_TRUE(lua_toboolean(L, -1))
        << "PlaySoundMemory(nil, ...) should raise a Lua type error (arg 1 is required)";
    lua_pop(L, 1);
}

/**
 * Test: world.PlaySoundMemory() with empty data returns eFileNotFound (not eCannotPlaySound).
 * Original methods_sounds.cpp:200-201 — mmioOpen failure → eFileNotFound.
 */
TEST_F(SoundApiTest, PlaySoundMemoryEmptyDataReturnsFileNotFound)
{
    // Pass buffer=1, data="" (empty string) — should get eFileNotFound
    executeLua("result = world.PlaySoundMemory(1, '')");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eFileNotFound)
        << "PlaySoundMemory with empty data should return eFileNotFound (30051), got " << result;
}

/**
 * Test: world.PlaySoundMemory() function exists.
 */
TEST_F(SoundApiTest, PlaySoundMemoryExists)
{
    executeLua("result = (type(world.PlaySoundMemory) == 'function')");
    lua_getglobal(L, "result");
    EXPECT_TRUE(lua_toboolean(L, -1)) << "world.PlaySoundMemory should be a function";
    lua_pop(L, 1);
// H83/M50: Out-of-range volume resets to 0 (full volume), not clamped.
// Original: methods_sounds.cpp:72-73 — if (Volume > 0 || Volume < (-100.0)) Volume = 0.0;
// Verify that out-of-range volume does NOT cause an early rejection.  We use a
// 1-character filename so the test reaches eBadParameter from the filename-length
// check — proving volume validation passed and did not short-circuit.  This works
// without initialising the audio engine.
TEST_F(SoundApiTest, OutOfRangeVolumePassesValidation)
{
    // Volume = 50 is out of range; original resets to 0 and continues.
    // A 1-char filename then triggers eBadParameter (strlen < 2), not volume rejection.
    executeLua("result = world.PlaySound(1, 'x', false, 50, 0)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eBadParameter)
        << "Out-of-range volume should reset silently; next rejection is the 1-char filename";
}

// H83/M50: Out-of-range pan resets to 0 (center), not clamped.
// Original: methods_sounds.cpp:81-82 — if (Pan > 100.0 || Pan < (-100)) Pan = 0;
TEST_F(SoundApiTest, OutOfRangePanPassesValidation)
{
    // Pan = 200 is out of range; original resets to 0 and continues.
    executeLua("result = world.PlaySound(1, 'x', false, 0, 200)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eBadParameter)
        << "Out-of-range pan should reset silently; next rejection is the 1-char filename";
}

// M48/M136: PlaySound(0, '', ...) with buffer 0 and empty filename returns eBadParameter.
// Original: methods_sounds.cpp:91 — adjust-path only for buffer >= 1; buffer 0 + empty
// filename falls through to strlen < 2 check → eBadParameter.
TEST_F(SoundApiTest, Buffer0EmptyFilenameReturnsBadParameter)
{
    executeLua("result = world.PlaySound(0, '', false, 0, 0)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eBadParameter)
        << "PlaySound(0, '', ...) should return eBadParameter (buffer 0 not valid for adjust)";
}

// M48: Single-character filename is rejected (strlen < 2 in original).
// Original: methods_sounds.cpp:117 — if (strlen(FileName) < 2) return eBadParameter;
TEST_F(SoundApiTest, SingleCharFilenameReturnsBadParameter)
{
    executeLua("result = world.PlaySound(1, 'x', false, 0, 0)");
    int result = getGlobalInt("result");
    EXPECT_EQ(result, eBadParameter)
        << "PlaySound with a 1-character filename should return eBadParameter";
}
