#pragma once

// blend_modes.h
// Per-channel blend functions for MiniWindow::BlendImage
// Ported from MUSHclient blending.h macros.
// Convention: A = blend layer (source image), B = base layer (destination)
// All functions take uint8_t and return uint8_t.

#include <algorithm>
#include <cmath>
#include <cstdint>

// ─── Per-channel blend functions (modes 1–35) ────────────────────────────────

[[nodiscard]] inline uint8_t Blend_Normal(uint8_t a, uint8_t /*b*/) noexcept
{
    return a;
}

[[nodiscard]] inline uint8_t Blend_Average(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>((a + b) / 2);
}

// Mode 3: Interpolate — cosine-table blend (rare).
// Approximated here with the same formula as Average; callers pass the
// precomputed cos_table values when needed.
[[nodiscard]] inline uint8_t Blend_Interpolate(uint8_t cos_a, uint8_t cos_b) noexcept
{
    return static_cast<uint8_t>((static_cast<int>(cos_a) + cos_b) > 255 ? 255 : cos_a + cos_b);
}

[[nodiscard]] inline uint8_t Blend_Darken(uint8_t a, uint8_t b) noexcept
{
    return (b > a) ? a : b;
}

[[nodiscard]] inline uint8_t Blend_Multiply(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>((static_cast<unsigned>(a) * b) / 255u);
}

[[nodiscard]] inline uint8_t Blend_ColorBurn(uint8_t a, uint8_t b) noexcept
{
    if (a == 0)
        return 0;
    int v = 255 - static_cast<int>((static_cast<unsigned long>(255 - b) << 8u) / a);
    return static_cast<uint8_t>(v < 0 ? 0 : v);
}

[[nodiscard]] inline uint8_t Blend_LinearBurn(uint8_t a, uint8_t b) noexcept
{
    // same as Subtract
    return (static_cast<int>(a) + b < 255) ? uint8_t{0} : static_cast<uint8_t>(a + b - 255);
}

[[nodiscard]] inline uint8_t Blend_InverseColorBurn(uint8_t a, uint8_t b) noexcept
{
    return Blend_ColorBurn(b, a);
}

[[nodiscard]] inline uint8_t Blend_Subtract(uint8_t a, uint8_t b) noexcept
{
    return Blend_LinearBurn(a, b);
}

[[nodiscard]] inline uint8_t Blend_Lighten(uint8_t a, uint8_t b) noexcept
{
    return (b > a) ? b : a;
}

[[nodiscard]] inline uint8_t Blend_Screen(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(255 - (static_cast<unsigned>(255 - a) * (255 - b) / 255u));
}

[[nodiscard]] inline uint8_t Blend_ColorDodge(uint8_t a, uint8_t b) noexcept
{
    if (a == 255)
        return 255;
    unsigned v = (static_cast<unsigned long>(b) << 8u) / (255u - a);
    return static_cast<uint8_t>(v > 255u ? 255u : v);
}

[[nodiscard]] inline uint8_t Blend_LinearDodge(uint8_t a, uint8_t b) noexcept
{
    return (static_cast<int>(a) + b > 255) ? uint8_t{255} : static_cast<uint8_t>(a + b);
}

[[nodiscard]] inline uint8_t Blend_InverseColorDodge(uint8_t a, uint8_t b) noexcept
{
    return Blend_ColorDodge(b, a);
}

[[nodiscard]] inline uint8_t Blend_Add(uint8_t a, uint8_t b) noexcept
{
    return Blend_LinearDodge(a, b);
}

[[nodiscard]] inline uint8_t Blend_Overlay(uint8_t a, uint8_t b) noexcept
{
    if (b < 128)
        return static_cast<uint8_t>(2u * a * b / 255u);
    return static_cast<uint8_t>(255u - 2u * (255u - a) * (255u - b) / 255u);
}

[[nodiscard]] inline uint8_t Blend_SoftLight(uint8_t a, uint8_t b) noexcept
{
    // From blending.h:
    // ((A * B) >> 8) + ((B * (255 - (((255-B)*(255-A)) >> 8) - ((A*B)>>8))) >> 8)
    unsigned ab = (static_cast<unsigned>(a) * b) >> 8u;
    unsigned inv = (static_cast<unsigned>(255u - b) * (255u - a)) >> 8u;
    unsigned inner = 255u - inv - ab;
    unsigned result = ab + ((static_cast<unsigned>(b) * inner) >> 8u);
    return static_cast<uint8_t>(result > 255u ? 255u : result);
}

[[nodiscard]] inline uint8_t Blend_HardLight(uint8_t a, uint8_t b) noexcept
{
    return Blend_Overlay(b, a);
}

[[nodiscard]] inline uint8_t Blend_VividLight(uint8_t a, uint8_t b) noexcept
{
    if (a < 128)
        return Blend_ColorBurn(static_cast<uint8_t>(2 * a), b);
    return Blend_ColorDodge(static_cast<uint8_t>(2 * (a - 128)), b);
}

[[nodiscard]] inline uint8_t Blend_LinearLight(uint8_t a, uint8_t b) noexcept
{
    if (a < 128)
        return Blend_LinearBurn(static_cast<uint8_t>(2 * a), b);
    return Blend_LinearDodge(static_cast<uint8_t>(2 * (a - 128)), b);
}

[[nodiscard]] inline uint8_t Blend_PinLight(uint8_t a, uint8_t b) noexcept
{
    if (a < 128)
        return Blend_Darken(static_cast<uint8_t>(2 * a), b);
    return Blend_Lighten(static_cast<uint8_t>(2 * (a - 128)), b);
}

[[nodiscard]] inline uint8_t Blend_HardMix(uint8_t a, uint8_t b) noexcept
{
    return (a < 255 - b) ? uint8_t{0} : uint8_t{255};
}

[[nodiscard]] inline uint8_t Blend_Difference(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(std::abs(static_cast<int>(a) - b));
}

[[nodiscard]] inline uint8_t Blend_Exclusion(uint8_t a, uint8_t b) noexcept
{
    int v = static_cast<int>(a) + b - 2 * static_cast<int>(a) * b / 255;
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

[[nodiscard]] inline uint8_t Blend_Reflect(uint8_t a, uint8_t b) noexcept
{
    if (b == 255)
        return 255;
    unsigned v = static_cast<unsigned>(a) * a / (255u - b);
    return static_cast<uint8_t>(v > 255u ? 255u : v);
}

[[nodiscard]] inline uint8_t Blend_Glow(uint8_t a, uint8_t b) noexcept
{
    return Blend_Reflect(b, a);
}

[[nodiscard]] inline uint8_t Blend_Freeze(uint8_t a, uint8_t b) noexcept
{
    if (a == 0)
        return 0;
    int v = 255 - static_cast<int>(255u - b) * (255u - b) / a;
    return static_cast<uint8_t>(v < 0 ? 0 : v);
}

[[nodiscard]] inline uint8_t Blend_Heat(uint8_t a, uint8_t b) noexcept
{
    return Blend_Freeze(b, a);
}

[[nodiscard]] inline uint8_t Blend_Negation(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(255 - std::abs(255 - static_cast<int>(a) - b));
}

[[nodiscard]] inline uint8_t Blend_Phoenix(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(std::min(a, b) - std::max(a, b) + 255);
}

[[nodiscard]] inline uint8_t Blend_Stamp(uint8_t a, uint8_t b) noexcept
{
    int v = static_cast<int>(b) + 2 * a - 256;
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

[[nodiscard]] inline uint8_t Blend_Xor(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(a ^ b);
}

[[nodiscard]] inline uint8_t Blend_And(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(a & b);
}

[[nodiscard]] inline uint8_t Blend_Or(uint8_t a, uint8_t b) noexcept
{
    return static_cast<uint8_t>(a | b);
}

// ─── Opacity helper ──────────────────────────────────────────────────────────

// Apply opacity to blend result X over base B.
[[nodiscard]] inline uint8_t Simple_Opacity(uint8_t base, uint8_t blended, double opacity) noexcept
{
    return static_cast<uint8_t>(opacity * blended + (1.0 - opacity) * base + 0.5);
}
