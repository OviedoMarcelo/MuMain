#include "UI/NewUI/Quests/ColorMath.h"

namespace UI::Quests::ColorMath
{
namespace
{
constexpr int AlphaShift = 24;
constexpr unsigned int ChannelMask = 0xFFu;
constexpr unsigned int AlphaMask = ChannelMask << AlphaShift;
constexpr int ChannelBits = 8;
constexpr int ChannelCount = 4;
constexpr float FullChannel = 255.0f;
} // namespace

unsigned int Fade(unsigned int color, float opacity)
{
    const auto alpha = static_cast<unsigned int>(((color >> AlphaShift) & ChannelMask) * opacity);
    return (color & ~AlphaMask) | (alpha << AlphaShift);
}

unsigned int WithAlpha(unsigned int color, float alpha)
{
    return (color & ~AlphaMask) | (static_cast<unsigned int>(alpha * FullChannel) << AlphaShift);
}

unsigned int Blend(unsigned int from, unsigned int to, float ratio)
{
    unsigned int result = 0;
    for (int channel = 0; channel < ChannelCount; ++channel)
    {
        const int shift = channel * ChannelBits;
        const float start = static_cast<float>((from >> shift) & ChannelMask);
        const float end = static_cast<float>((to >> shift) & ChannelMask);
        result |= static_cast<unsigned int>(start + (end - start) * ratio) << shift;
    }

    return result;
}
} // namespace UI::Quests::ColorMath
