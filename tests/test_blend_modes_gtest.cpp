/**
 * test_blend_modes_gtest.cpp
 * Behavioral parity tests for blend_modes.h per-channel blend functions.
 *
 * Reference: MUSHclient blending.h (original source).
 * Key invariant: Blend_ColorBurn uses << 8 (multiply by 256), not * 255.
 */

#include "../src/world/blend_modes.h"
#include <cstdint>
#include <gtest/gtest.h>

// ─── Blend_ColorBurn parity (H30) ────────────────────────────────────────────
//
// Original formula (blending.h:33):
//   255 - (((unsigned long)(255 - B)) << 8) / A
//
// A == 0 => 0 (special case)
// Otherwise clamp to [0, 255].

TEST(BlendColorBurnTest, ZeroBlendReturnsZero)
{
    // A == 0 => 0 (special case in original)
    EXPECT_EQ(Blend_ColorBurn(0, 0), 0u);
    EXPECT_EQ(Blend_ColorBurn(0, 128), 0u);
    EXPECT_EQ(Blend_ColorBurn(0, 255), 0u);
}

TEST(BlendColorBurnTest, FullBlendWithMidBase)
{
    // A=255, B=128: 255 - ((127 << 8) / 255) = 255 - 127 = 128
    // (127 * 256) / 255 = 32512 / 255 = 127 (integer division)
    uint8_t result = Blend_ColorBurn(255, 128);
    EXPECT_EQ(result, 128u);
}

TEST(BlendColorBurnTest, DarkBlendBrightBase)
{
    // A=128, B=255: 255 - ((0 << 8) / 128) = 255 - 0 = 255
    EXPECT_EQ(Blend_ColorBurn(128, 255), 255u);
}

TEST(BlendColorBurnTest, DarkBlendDarkBase)
{
    // A=64, B=0: 255 - ((255 << 8) / 64) = 255 - 1020 => clamped to 0
    EXPECT_EQ(Blend_ColorBurn(64, 0), 0u);
}

TEST(BlendColorBurnTest, MidBlendMidBase)
{
    // A=128, B=128: 255 - ((127 << 8) / 128) = 255 - 254 = 1
    // (127 * 256) / 128 = 32512 / 128 = 254 (integer division)
    EXPECT_EQ(Blend_ColorBurn(128, 128), 1u);
}

TEST(BlendColorBurnTest, FullBlendFullBase)
{
    // A=255, B=255: 255 - ((0 << 8) / 255) = 255 - 0 = 255
    EXPECT_EQ(Blend_ColorBurn(255, 255), 255u);
}

// Verify the << 8 vs * 255 difference matters for a specific value.
// A=100, B=100:
//   Original (<<8): 255 - ((155 * 256) / 100) = 255 - (39680/100) = 255 - 396 = clamped 0
//   Wrong (*255):   255 - ((155 * 255) / 100) = 255 - (39525/100) = 255 - 395 = clamped 0
// A=200, B=100:
//   Original (<<8): 255 - ((155 * 256) / 200) = 255 - (39680/200) = 255 - 198 = 57
//   Wrong (*255):   255 - ((155 * 255) / 200) = 255 - (39525/200) = 255 - 197 = 58
TEST(BlendColorBurnTest, ShiftVsMultiplyDifference)
{
    // A=200, B=100 exposes the << 8 vs *255 difference.
    // With << 8: (155 * 256) / 200 = 39680 / 200 = 198  => 255 - 198 = 57
    // With *255: (155 * 255) / 200 = 39525 / 200 = 197  => 255 - 197 = 58
    EXPECT_EQ(Blend_ColorBurn(200, 100), 57u);
}

// ─── InverseColorBurn delegation ─────────────────────────────────────────────

TEST(BlendColorBurnTest, InverseColorBurnSwapsArguments)
{
    // Blend_InverseColorBurn(A, B) == Blend_ColorBurn(B, A)
    for (uint8_t a : {uint8_t{0}, uint8_t{64}, uint8_t{128}, uint8_t{200}, uint8_t{255}}) {
        for (uint8_t b : {uint8_t{0}, uint8_t{64}, uint8_t{128}, uint8_t{200}, uint8_t{255}}) {
            EXPECT_EQ(Blend_InverseColorBurn(a, b), Blend_ColorBurn(b, a))
                << "a=" << int(a) << " b=" << int(b);
        }
    }
}
