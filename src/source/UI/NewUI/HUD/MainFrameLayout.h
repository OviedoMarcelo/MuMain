#pragma once

// Geometry of the bottom HUD bar (the "S8" frame artwork, MenuS8_Main).
//
// Every element is measured in texels of that artwork and returned in the
// logical 640x480 space of the bottom HUD center transform, where the frame
// spans the full reference width and rests on the bottom edge. Rendering and
// hit testing both read from here so the art and the clickable areas stay in
// sync.
namespace UI::MainFrame::Layout
{
    struct Rect
    {
        float x;
        float y;
        float width;
        float height;

        bool Contains(float pointX, float pointY) const;
        float CenterX() const;
        float CenterY() const;
    };

    // The six round buttons, left to right.
    enum class MenuButton
    {
        Shop,
        Character,
        Inventory,
        Quest,
        Friend,
        Menu,
        Count,
    };

    inline constexpr int ItemHotKeySlotCount = 4;
    inline constexpr int SkillHotKeySlotCount = 5;

    Rect Frame();
    Rect LifeOrb();
    Rect ManaOrb();
    Rect ShieldGauge();
    Rect SkillManaGauge();
    Rect ExperienceBar();
    Rect ExperiencePercent();
    Rect ItemHotKeySlot(int index);
    Rect CurrentSkillSlot();
    Rect SkillHotKeySlot(int index);
    Rect SkillHotKeyStrip();
    Rect MenuButtonRect(MenuButton button);

    // Lowest logical y the floating skill list may use without covering the bar.
    float SkillListBottom();

    // True when the logical point lies on the visible part of the bar.
    bool ContainsPoint(float x, float y);
}
