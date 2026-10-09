//////////////////////////////////////////////////////////////////////
// NewUIMainFrameWindow.cpp: implementation of the CNewUIMainFrameWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <algorithm>
#include "I18N/All.h"

#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"	// self
#include "UI/NewUI/Options/NewUIOptionWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Widgets/UIBaseDef.h"
#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzInventory.h"

#include "GameLogic/Items/CSItemOption.h"
#include "GameLogic/Events/CSChaosCastle.h"
#include "World/MapInfra/MapManager.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Skills/SkillManager.h"
#include "UI/NewUI/HUD/Skills/SkillTooltip.h"
#include "UI/NewUI/HUD/MainFrameGauges.h"
#include "UI/NewUI/HUD/MainFrameLayout.h"
#include "UI/Scaling/UITransform.h"
#include "Core/Time/CTimCheck.h"
#include "GameLogic/Social/MonkSystem.h"
#include "GameLogic/Items/ItemCategories.h"

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
#include "GameShop/InGameShopSystem.h"
#endif //PBG_ADD_INGAMESHOP_UI_MAINFRAME

namespace
{
    namespace Layout = UI::MainFrame::Layout;
    namespace Gauges = UI::MainFrame::Gauges;

    // Source art sizes, in texels.
    constexpr Gauges::ArtSize kFrameArt{935.0f, 110.0f};
    constexpr Gauges::ArtSize kOrbArt{90.0f, 90.0f};
    constexpr Gauges::ArtSize kGaugeArt{122.0f, 16.0f};
    constexpr Gauges::ArtSize kButtonArt{27.0f, 28.0f};
    constexpr Gauges::ArtSize kSkillBoxArt{32.0f, 38.0f};
    // The experience art is a 6x4 strip padded to an 8x4 texture.
    constexpr float kExperienceArtU = 6.0f / 8.0f;
    constexpr float kExperienceArtV = 1.0f;

    constexpr float kLowLifeWarningRatio = 0.2f;
    // RenderNumber digits are 16 units tall before their 0.3 scale offset.
    constexpr float kNumberDigitHeight = 16.0f;
    constexpr float kNumberScaleOffset = 0.3f;
    constexpr float kGaugeNumberScale = 0.9f;
    constexpr float kPercentNumberScale = 0.8f;
    constexpr float kPercentMax = 100.0f;
    constexpr float kTooltipGap = 14.0f;
    constexpr float kExperienceTooltipLeftOffset = 40.0f;

    // White veil over the part of the experience bar gained since the last update.
    constexpr DWORD kExperienceGainColor = 0x66FFFFFFu;
    constexpr DWORD kMasterExperienceGainColor = 0x99FFFFFFu;

    // Additive tint laid over a button while hovered, active or blinking an alert.
    constexpr unsigned char kButtonHighlightLevel = 90;

    // The character button blinks while a quest waits for the player.
    constexpr int kQuestAlertTimerId = 5;
    constexpr int kQuestAlertBlinkMs = 500;
    // The friend button blinks on for half of every period, counted in rendered frames.
    constexpr int kFriendBlinkPeriod = 24;
    constexpr int kFriendBlinkOnFrames = 12;

    Layout::MenuButton MenuButtonSlot(int btnType)
    {
        switch (btnType)
        {
        case SEASON3B::MAINFRAME_BTN_CHAINFO:
            return Layout::MenuButton::Character;
        case SEASON3B::MAINFRAME_BTN_MYINVEN:
            return Layout::MenuButton::Inventory;
        case SEASON3B::MAINFRAME_BTN_QUEST:
            return Layout::MenuButton::Quest;
        case SEASON3B::MAINFRAME_BTN_FRIEND:
            return Layout::MenuButton::Friend;
        case SEASON3B::MAINFRAME_BTN_WINDOW:
            return Layout::MenuButton::Menu;
        default:
            return Layout::MenuButton::Shop;
        }
    }

    // 3D item models and their stack count inside the Q/W/E/R slots.
    constexpr float kItemIconSize = 20.0f;
    constexpr float kItemCountLift = 6.0f;

    // Skill icons are 20x28 cells in their sprite sheets, whatever size they are drawn at.
    constexpr float kSkillIconCellWidth = 20.0f;
    constexpr float kSkillIconCellHeight = 28.0f;
    // Margin between a bar slot's edge and the skill icon inside it.
    constexpr float kSkillIconInset = 1.4f;
    // The hotkey digit overlaps the icon's bottom-right corner by this much.
    constexpr float kSkillHotKeyNumberLift = 8.0f;

    // The skill list pops up above the bar as a grid of boxes fanning out from this x;
    // it stays put so the widest list still fits on a 4:3 screen.
    constexpr float kSkillListOriginX = 385.0f;
    constexpr float kSkillBoxWidth = 32.0f;
    constexpr float kSkillBoxHeight = 38.0f;

    float SkillListTop()
    {
        return Layout::SkillListBottom() - kSkillBoxHeight;
    }

    // Pet commands get their own row above the skill list, starting one box to its left.
    float PetCommandRowX()
    {
        return kSkillListOriginX - kSkillBoxWidth;
    }

    float PetCommandRowTop()
    {
        return SkillListTop() - kSkillBoxHeight;
    }

    // Largest icon with the sprite's aspect ratio that fits inside a bar slot.
    Layout::Rect SkillIconRect(const Layout::Rect& slot)
    {
        const float height = slot.height - kSkillIconInset * 2.0f;
        const float width = std::min(slot.width - kSkillIconInset * 2.0f,
                                     height * kSkillIconCellWidth / kSkillIconCellHeight);
        return {slot.CenterX() - width * 0.5f, slot.CenterY() - height * 0.5f, width, height};
    }

    struct Resource
    {
        DWORD current;
        DWORD max;
    };

    Resource HeroLife()
    {
        const DWORD max = gCharacterManager.IsMasterLevel(Hero->Class) ? Master_Level_Data.wMaxLife
                                                                       : CharacterAttribute->LifeMax;
        return {static_cast<DWORD>(std::min<int>(std::max<int>(0, CharacterAttribute->Life), max)), max};
    }

    Resource HeroMana()
    {
        const DWORD max = gCharacterManager.IsMasterLevel(Hero->Class) ? Master_Level_Data.wMaxMana
                                                                       : CharacterAttribute->ManaMax;
        return {static_cast<DWORD>(std::min<int>(std::max<int>(0, CharacterAttribute->Mana), max)), max};
    }

    Resource HeroShield()
    {
        const DWORD max = gCharacterManager.IsMasterLevel(Hero->Class)
                              ? std::max<int>(1, Master_Level_Data.wMaxShield)
                              : std::max<int>(1, CharacterAttribute->ShieldMax);
        return {static_cast<DWORD>(std::min<int>(max, CharacterAttribute->Shield)), max};
    }

    Resource HeroSkillMana()
    {
        const DWORD max = gCharacterManager.IsMasterLevel(Hero->Class)
                              ? std::max<int>(1, Master_Level_Data.wMaxBP)
                              : std::max<int>(1, CharacterAttribute->SkillManaMax);
        return {static_cast<DWORD>(std::min<int>(max, CharacterAttribute->SkillMana)), max};
    }

    bool IsMouseIn(const Layout::Rect& rect)
    {
        return rect.Contains(static_cast<float>(MouseX), static_cast<float>(MouseY));
    }

    float NumberHeight(float scale)
    {
        return kNumberDigitHeight * (scale - kNumberScaleOffset);
    }

    void RenderCenteredNumber(const Layout::Rect& rect, int value, float scale = 1.0f)
    {
        SEASON3B::RenderNumber(rect.CenterX(), rect.CenterY() - NumberHeight(scale) * 0.5f, value, scale);
    }

    void RenderResourceTooltip(const Layout::Rect& rect, const wchar_t* format, const Resource& value)
    {
        if (!IsMouseIn(rect))
            return;

        wchar_t text[256];
        mu_swprintf(text, format, value.current, value.max);
        RenderTipText(static_cast<int>(rect.x), static_cast<int>(rect.y - kTooltipGap), text);
    }

    Layout::Rect ItemIconRect(int hotKey)
    {
        const Layout::Rect slot = Layout::ItemHotKeySlot(hotKey);
        return {slot.CenterX() - kItemIconSize * 0.5f, slot.CenterY() - kItemIconSize * 0.5f, kItemIconSize,
                kItemIconSize};
    }

    // The experience bar shows progress through the current tenth of the level;
    // index is which tenth (0-9) and progress how far through it (0..1).
    struct ExperienceSegment
    {
        int index;
        double progress;
    };

    constexpr double kExperienceSegments = 10.0;
    constexpr int kLastExperienceSegment = 9;

    ExperienceSegment SegmentForRatio(double ratio)
    {
        const double clampedRatio = std::clamp(ratio, 0.0, 1.0);
        if (clampedRatio >= 1.0)
            return {kLastExperienceSegment, 1.0};

        const double scaled = clampedRatio * kExperienceSegments;
        const int index = std::clamp(static_cast<int>(scaled), 0, kLastExperienceSegment);
        const double progress = std::clamp(scaled - static_cast<double>(static_cast<long long>(scaled)), 0.0, 1.0);
        return {index, progress};
    }

    double ExperienceRatio(__int64 experience, __int64 lowerBound, __int64 upperBound)
    {
        const double needed = static_cast<double>(upperBound - lowerBound);
        if (needed <= 0.0)
            return 0.0;

        const double clamped = std::clamp(static_cast<double>(experience), static_cast<double>(lowerBound),
                                          static_cast<double>(upperBound));
        return std::clamp((clamped - static_cast<double>(lowerBound)) / needed, 0.0, 1.0);
    }

    // Experience needed to reach the given level; levels above 255 add a steeper term.
    constexpr __int64 kExperienceCurveOffset = 9;
    constexpr __int64 kExperienceCurveFactor = 10;
    constexpr __int64 kExperienceSteepLevel = 255;
    constexpr __int64 kExperienceSteepFactor = 1000;

    __int64 LevelBaseExperience(__int64 level)
    {
        if (level <= 0)
            return 0;

        __int64 experience = (kExperienceCurveOffset + level) * level * level * kExperienceCurveFactor;
        if (level > kExperienceSteepLevel)
        {
            const __int64 overLevel = level - kExperienceSteepLevel;
            experience += (kExperienceCurveOffset + overLevel) * overLevel * overLevel * kExperienceSteepFactor;
        }
        return experience;
    }

    // Master levels continue the normal curve from level 400; the result is
    // rebased onto the experience a character has when master level 0 starts.
    constexpr __int64 kMasterLevelOffset = 400;
    constexpr __int64 kMasterExperienceBase = 3892250000;
    constexpr __int64 kMasterExperienceDivisor = 2;

    __int64 MasterLevelBaseExperience(__int64 masterLevel)
    {
        const __int64 totalLevel = masterLevel + kMasterLevelOffset;
        const __int64 overLevel = totalLevel - kExperienceSteepLevel;
        const __int64 total = (kExperienceCurveOffset + totalLevel) * totalLevel * totalLevel * kExperienceCurveFactor
                              + (kExperienceCurveOffset + overLevel) * overLevel * overLevel * kExperienceSteepFactor;
        return (total - kMasterExperienceBase) / kMasterExperienceDivisor;
    }
}

SEASON3B::CNewUIMainFrameWindow::CNewUIMainFrameWindow()
{
    m_bExpEffect = false;
    m_dwExpEffectTime = 0;
    m_dwPreExp = 0;
    m_dwGetExp = 0;
    m_bButtonBlink = false;
    std::fill(std::begin(m_bButtonActive), std::end(m_bButtonActive), false);
}

SEASON3B::CNewUIMainFrameWindow::~CNewUIMainFrameWindow()
{
    Release();
}

void SEASON3B::CNewUIMainFrameWindow::LoadImages()
{
    LoadBitmap(L"Interface\\MenuS8_Main.tga", IMAGE_FRAME, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_black.tga", IMAGE_ORB_EMPTY, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_red.tga", IMAGE_ORB_LIFE, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_green.tga", IMAGE_ORB_POISON, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_blue.tga", IMAGE_ORB_MANA, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_AG.jpg", IMAGE_GAUGE_AG, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_SD.jpg", IMAGE_GAUGE_SD, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\newui_exbar.jpg", IMAGE_GAUGE_EXBAR, GL_LINEAR);
    LoadBitmap(L"Interface\\Exbar_Master.jpg", IMAGE_MASTER_GAUGE_BAR, GL_LINEAR);
    LoadBitmap(L"Interface\\MenuS8_shop.tga", IMAGE_MENU_BTN_CSHOP, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_character.tga", IMAGE_MENU_BTN_CHAINFO, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_inventory.tga", IMAGE_MENU_BTN_MYINVEN, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_quest.tga", IMAGE_MENU_BTN_QUEST, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_friend.tga", IMAGE_MENU_BTN_FRIEND, GL_LINEAR, GL_CLAMP_TO_EDGE);
    LoadBitmap(L"Interface\\MenuS8_btmenu.tga", IMAGE_MENU_BTN_WINDOW, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

void SEASON3B::CNewUIMainFrameWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_FRAME);
    DeleteBitmap(IMAGE_ORB_EMPTY);
    DeleteBitmap(IMAGE_ORB_LIFE);
    DeleteBitmap(IMAGE_ORB_POISON);
    DeleteBitmap(IMAGE_ORB_MANA);
    DeleteBitmap(IMAGE_GAUGE_AG);
    DeleteBitmap(IMAGE_GAUGE_SD);
    DeleteBitmap(IMAGE_GAUGE_EXBAR);
    DeleteBitmap(IMAGE_MASTER_GAUGE_BAR);
    DeleteBitmap(IMAGE_MENU_BTN_CSHOP);
    DeleteBitmap(IMAGE_MENU_BTN_CHAINFO);
    DeleteBitmap(IMAGE_MENU_BTN_MYINVEN);
    DeleteBitmap(IMAGE_MENU_BTN_QUEST);
    DeleteBitmap(IMAGE_MENU_BTN_FRIEND);
    DeleteBitmap(IMAGE_MENU_BTN_WINDOW);
}

bool SEASON3B::CNewUIMainFrameWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
    if (NULL == pNewUIMng || NULL == pNewUI3DRenderMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MAINFRAME, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    m_pNewUI3DRenderMng->Add3DRenderObj(this, ITEMHOTKEYNUMBER_CAMERA_Z_ORDER);

    LoadImages();

    SetButtonInfo();

    Show(true);

    return true;
}

namespace
{
    // The button only handles clicks and its tooltip; RenderMenuButton draws the art
    // scaled into the socket, which CNewUIButton's 1:1 blit cannot do.
    void PlaceMenuButton(SEASON3B::CNewUIButton& button, Layout::MenuButton slot, const wchar_t* const* tooltip)
    {
        const Layout::Rect rect = Layout::MenuButtonRect(slot);
        button.ChangeButtonInfo(static_cast<int>(std::lround(rect.x)), static_cast<int>(std::lround(rect.y)),
                                static_cast<int>(std::lround(rect.width)), static_cast<int>(std::lround(rect.height)));
        button.ChangeToolTipText(tooltip, true);
    }
}

void SEASON3B::CNewUIMainFrameWindow::SetButtonInfo()
{
    PlaceMenuButton(m_BtnCShop, Layout::MenuButton::Shop, &I18N::Game::MUItemShopX);
    PlaceMenuButton(m_BtnChaInfo, Layout::MenuButton::Character, &I18N::Game::CharacterC);
    PlaceMenuButton(m_BtnMyInven, Layout::MenuButton::Inventory, &I18N::Game::InventoryIV);
    PlaceMenuButton(m_BtnQuest, Layout::MenuButton::Quest, &I18N::Game::Quest);
    PlaceMenuButton(m_BtnFriend, Layout::MenuButton::Friend, &I18N::Game::FriendF);
    PlaceMenuButton(m_BtnWindow, Layout::MenuButton::Menu, &I18N::Game::MenuU);
}

void SEASON3B::CNewUIMainFrameWindow::Release()
{
    UnloadImages();

    if (m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->Remove3DRenderObj(this);
        m_pNewUI3DRenderMng = NULL;
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

bool SEASON3B::CNewUIMainFrameWindow::Render()
{
    EnableAlphaTest();

    const auto transform = UI::Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight);
    UI::Scaling::ScopedActiveTransform layout(transform, true);

    // The orbs sit behind the frame, which has see-through holes for them.
    RenderOrbs();
    const Layout::Rect frame = Layout::Frame();
    RenderImageStretch(IMAGE_FRAME, frame.x, frame.y, frame.width, frame.height, 0.0f, 0.0f, kFrameArt.width,
                       kFrameArt.height);

    m_pNewUI3DRenderMng->RenderUI2DEffect(ITEMHOTKEYNUMBER_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);
    g_pSkillList->RenderCurrentSkillAndHotSkillList();
    RenderLifeMana();
    RenderGuageSD();
    RenderGuageAG();
    RenderExperience();
    RenderButtons();

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIMainFrameWindow::Render3D()
{
    const auto transform = UI::Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight);
    UI::Scaling::ScopedActiveTransform layout(transform);
    m_ItemHotKey.RenderItems();
}

void SEASON3B::CNewUIMainFrameWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
    g_pMainFrame->RenderHotKeyItemCount();
}

bool SEASON3B::CNewUIMainFrameWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIMainFrameWindow::RenderOrbs()
{
    const Resource life = HeroLife();
    const Resource mana = HeroMana();
    const int lifeImage = g_isCharacterBuff((&Hero->Object), eDeBuff_Poison) ? IMAGE_ORB_POISON : IMAGE_ORB_LIFE;

    Gauges::RenderOrb(IMAGE_ORB_EMPTY, lifeImage, Layout::LifeOrb(), kOrbArt,
                      Gauges::FillRatio(life.current, life.max));
    Gauges::RenderOrb(IMAGE_ORB_EMPTY, IMAGE_ORB_MANA, Layout::ManaOrb(), kOrbArt,
                      Gauges::FillRatio(mana.current, mana.max));
}

void SEASON3B::CNewUIMainFrameWindow::RenderLifeMana()
{
    const Resource life = HeroLife();
    const Resource mana = HeroMana();

    if (life.current > 0 && Gauges::FillRatio(life.current, life.max) < kLowLifeWarningRatio)
    {
        PlayBuffer(SOUND_HEART);
    }

    RenderCenteredNumber(Layout::LifeOrb(), static_cast<int>(life.current));
    RenderCenteredNumber(Layout::ManaOrb(), static_cast<int>(mana.current));

    RenderResourceTooltip(Layout::LifeOrb(), I18N::Game::LifeDD, life);
    RenderResourceTooltip(Layout::ManaOrb(), I18N::Game::ManaDD359, mana);
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageAG()
{
    const Resource skillMana = HeroSkillMana();
    const Layout::Rect gauge = Layout::SkillManaGauge();

    Gauges::RenderHorizontal(IMAGE_GAUGE_AG, gauge, kGaugeArt, Gauges::FillRatio(skillMana.current, skillMana.max));
    RenderCenteredNumber(gauge, static_cast<int>(skillMana.current), kGaugeNumberScale);
    RenderResourceTooltip(gauge, I18N::Game::AGDD, skillMana);
}

void SEASON3B::CNewUIMainFrameWindow::RenderGuageSD()
{
    const Resource shield = HeroShield();
    const Layout::Rect gauge = Layout::ShieldGauge();

    Gauges::RenderHorizontal(IMAGE_GAUGE_SD, gauge, kGaugeArt, Gauges::FillRatio(shield.current, shield.max));
    RenderCenteredNumber(gauge, static_cast<int>(shield.current), kGaugeNumberScale);
    RenderResourceTooltip(gauge, I18N::Game::SDDD, shield);
}

void SEASON3B::CNewUIMainFrameWindow::RenderExperience()
{
    __int64 experience = 0;
    __int64 nextExperience = 0;
    __int64 lowerBound = 0;
    __int64 previousExperience = 0;
    int image = IMAGE_GAUGE_EXBAR;
    DWORD gainColor = kExperienceGainColor;

    if (gCharacterManager.IsMasterExperienceActive(CharacterAttribute->Class, CharacterAttribute->Level) == true)
    {
        experience = (__int64)Master_Level_Data.lMasterLevel_Experince;
        nextExperience = (__int64)Master_Level_Data.lNext_MasterLevel_Experince;
        lowerBound = MasterLevelBaseExperience((__int64)Master_Level_Data.nMLevel);
        previousExperience = m_loPreExp;
        image = IMAGE_MASTER_GAUGE_BAR;
        gainColor = kMasterExperienceGainColor;
    }
    else
    {
        experience = CharacterAttribute->Experience;
        nextExperience = CharacterAttribute->NextExperience;
        lowerBound = LevelBaseExperience((__int64)CharacterAttribute->Level - 1);
        previousExperience = m_dwPreExp;
    }

    const __int64 upperBound = std::max(nextExperience, lowerBound);
    const double ratio = ExperienceRatio(experience, lowerBound, upperBound);
    const ExperienceSegment current = SegmentForRatio(ratio);

    // While the gain effect runs, the part earned since the last update is
    // highlighted; a gain that crossed into a new tenth highlights the whole fill.
    double gainStart = 0.0;
    if (m_bExpEffect && previousExperience >= lowerBound)
    {
        const ExperienceSegment previous =
            SegmentForRatio(ExperienceRatio(previousExperience, lowerBound, upperBound));
        if (current.index <= previous.index)
            gainStart = previous.progress;
    }
    RenderExperienceFill(image, gainColor, gainStart, current.progress);

    RenderCenteredNumber(Layout::ExperiencePercent(), static_cast<int>(ratio * kPercentMax), kPercentNumberScale);

    const Layout::Rect bar = Layout::ExperienceBar();
    if (IsMouseIn(bar))
    {
        wchar_t strTipText[256];
        mu_swprintf(strTipText, I18N::Game::EXPI64dI64d, experience, nextExperience);
        RenderTipText(static_cast<int>(bar.CenterX() - kExperienceTooltipLeftOffset),
                      static_cast<int>(bar.y - kTooltipGap), strTipText);
    }
}

void SEASON3B::CNewUIMainFrameWindow::RenderExperienceFill(int iImage, DWORD dwGainColor, double fGainStart,
                                                           double fProgress)
{
    const Layout::Rect bar = Layout::ExperienceBar();
    const float filledWidth = static_cast<float>(fProgress) * bar.width;
    if (filledWidth <= 0.0f)
        return;

    RenderBitmap(iImage, bar.x, bar.y, filledWidth, bar.height, 0.f, 0.f, kExperienceArtU, kExperienceArtV);

    if (!m_bExpEffect)
        return;

    const float gainX = bar.x + static_cast<float>(fGainStart) * bar.width;
    const float gainWidth = std::max(0.0f, bar.x + filledWidth - gainX);
    RenderColorQuadARGB(gainX, bar.y, gainWidth, bar.height, dwGainColor);
}

void SEASON3B::CNewUIMainFrameWindow::RenderHotKeyItemCount()
{
    m_ItemHotKey.RenderItemCount();
}

void SEASON3B::CNewUIMainFrameWindow::RenderButtons()
{
#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
    RenderMenuButton(m_BtnCShop, IMAGE_MENU_BTN_CSHOP, MAINFRAME_BTN_PARTCHARGE, false);
#endif //defined PBG_ADD_INGAMESHOP_UI_MAINFRAME
    RenderMenuButton(m_BtnChaInfo, IMAGE_MENU_BTN_CHAINFO, MAINFRAME_BTN_CHAINFO, IsCharInfoAlertOn());
    RenderMenuButton(m_BtnMyInven, IMAGE_MENU_BTN_MYINVEN, MAINFRAME_BTN_MYINVEN, false);
    RenderMenuButton(m_BtnQuest, IMAGE_MENU_BTN_QUEST, MAINFRAME_BTN_QUEST, false);
    RenderMenuButton(m_BtnFriend, IMAGE_MENU_BTN_FRIEND, MAINFRAME_BTN_FRIEND, IsFriendAlertOn());
    RenderMenuButton(m_BtnWindow, IMAGE_MENU_BTN_WINDOW, MAINFRAME_BTN_WINDOW, false);
}

void SEASON3B::CNewUIMainFrameWindow::RenderMenuButton(CNewUIButton& button, int iImage, int iBtnType, bool bAlert)
{
    const Layout::Rect rect = Layout::MenuButtonRect(MenuButtonSlot(iBtnType));
    RenderImageStretch(iImage, rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, kButtonArt.width,
                       kButtonArt.height);

    if (m_bButtonActive[iBtnType] || bAlert || IsMouseIn(rect))
    {
        EnableAlphaBlend();
        RenderImageStretch(iImage, rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, kButtonArt.width,
                           kButtonArt.height,
                           RGBA(kButtonHighlightLevel, kButtonHighlightLevel, kButtonHighlightLevel, 255));
        EnableAlphaTest();
    }

    // No art is registered on the button, so this only draws its tooltip.
    button.Render();
}

bool SEASON3B::CNewUIMainFrameWindow::IsCharInfoAlertOn()
{
    if (g_QuestMng.IsQuestIndexByEtcListEmpty())
        return false;

    if (g_Time.GetTimeCheck(kQuestAlertTimerId, kQuestAlertBlinkMs))
        m_bButtonBlink = !m_bButtonBlink;

    const bool questWindowOpen = g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_QUEST_PROGRESS_ETC)
                                 || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER);
    return m_bButtonBlink && !questWindowOpen;
}

bool SEASON3B::CNewUIMainFrameWindow::IsFriendAlertOn()
{
    const int iBlinkTemp = g_pFriendMenu->GetBlinkTemp();
    const bool bIsAlertTime = (iBlinkTemp % kFriendBlinkPeriod < kFriendBlinkOnFrames);
    bool bAlert = g_pFriendMenu->IsNewChatAlert() && bIsAlertTime;

    if (g_pFriendMenu->IsNewMailAlert())
    {
        if (bIsAlertTime)
        {
            bAlert = true;

            if (iBlinkTemp % kFriendBlinkPeriod == kFriendBlinkOnFrames - 1)
            {
                g_pFriendMenu->IncreaseLetterBlink();
            }
        }
    }
    else if (g_pLetterList->CheckNoReadLetter())
    {
        bAlert = true;
    }

    g_pFriendMenu->IncreaseBlinkTemp();
    return bAlert;
}

bool SEASON3B::CNewUIMainFrameWindow::UpdateMouseEvent()
{
    if (g_pNewUIHotKey->IsStateGameOver())
        return true;

    const auto transform = UI::Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight);
    UI::Scaling::ScopedActiveTransform layout(transform, true);
    return !BtnProcess();
}

namespace
{
    constexpr int kFriendMinimumLevel = 6;

    void ToggleFriendWindow()
    {
        if (gMapManager.InChaosCastle() == true)
            return;

        if (CharacterAttribute->Level < kFriendMinimumLevel)
        {
            if (g_pSystemLogBox->CheckChatRedundancy(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction) == FALSE)
            {
                g_pSystemLogBox->AddText(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction, SEASON3B::TYPE_SYSTEM_MESSAGE);
            }
            return;
        }

        g_pNewUISystem->Toggle(SEASON3B::INTERFACE_FRIEND);
    }
}

bool SEASON3B::CNewUIMainFrameWindow::BtnProcess()
{
    if (g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory() == true)
    {
        if (m_BtnMyInven.UpdateMouseEvent() == true)
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }
    else if (g_pNewUIHotKey->CanUpdateKeyEvent() == true)
    {
        if (m_BtnMyInven.UpdateMouseEvent() == true)
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        else if (m_BtnChaInfo.UpdateMouseEvent() == true)
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_CHARACTER);

            PlayBuffer(SOUND_CLICK01);

            if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHARACTER))
                g_QuestMng.SendQuestIndexByEtcSelection();

            return true;
        }
        else if (m_BtnFriend.UpdateMouseEvent() == true)
        {
            ToggleFriendWindow();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        else if (m_BtnQuest.UpdateMouseEvent() == true)
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MYQUEST);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        else if (m_BtnWindow.UpdateMouseEvent() == true)
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_WINDOW_MENU);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

#ifdef PBG_ADD_INGAMESHOP_UI_MAINFRAME
        else if (m_BtnCShop.UpdateMouseEvent() == true)
        {
            if (g_pInGameShop->IsInGameShopOpen() == false)
                return false;

#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
            if (g_InGameShopSystem->IsScriptDownload() == true)
            {
                if (g_InGameShopSystem->ScriptDownload() == false)
                    return false;
            }

            if (g_InGameShopSystem->IsBannerDownload() == true)
            {
                g_InGameShopSystem->BannerDownload();
            }
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD

            if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == false)
            {
                if (g_InGameShopSystem->GetIsRequestShopOpenning() == false)
                {
                    SocketClient->ToGameServer()->SendCashShopOpenState(0);
                    g_InGameShopSystem->SetIsRequestShopOpenning(true);

#ifdef KJH_MOD_SHOP_SCRIPT_DOWNLOAD
                    g_pMainFrame->SetBtnState(MAINFRAME_BTN_PARTCHARGE, true);
#endif // KJH_MOD_SHOP_SCRIPT_DOWNLOAD
                }
            }
            else
            {
                SocketClient->ToGameServer()->SendCashShopOpenState(1);
                g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);
            }

            return true;
        }
#endif //PBG_ADD_INGAMESHOP_UI_MAINFRAME
    }

    return false;
}

bool SEASON3B::CNewUIMainFrameWindow::UpdateKeyEvent()
{
    if (m_ItemHotKey.UpdateKeyEvent() == false)
    {
        return false;
    }
    return true;
}

bool SEASON3B::CNewUIMainFrameWindow::Update()
{
    if (m_bExpEffect == true)
    {
        if (timeGetTime() - m_dwExpEffectTime > 2000)
        {
            m_bExpEffect = false;
            m_dwExpEffectTime = 0;
            m_dwGetExp = 0;
        }
    }

    return true;
}

float SEASON3B::CNewUIMainFrameWindow::GetLayerDepth()
{
    return 10.6f;
}

float SEASON3B::CNewUIMainFrameWindow::GetKeyEventOrder()
{
    return 2.9f;
}

void SEASON3B::CNewUIMainFrameWindow::SetItemHotKey(int iHotKey, int iItemType, int iItemLevel)
{
    m_ItemHotKey.SetHotKey(iHotKey, iItemType, iItemLevel);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKey(int iHotKey)
{
    return m_ItemHotKey.GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetItemHotKeyLevel(int iHotKey)
{
    return m_ItemHotKey.GetHotKeyLevel(iHotKey);
}

void SEASON3B::CNewUIMainFrameWindow::UseHotKeyItemRButton()
{
    const auto transform = UI::Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight);
    UI::Scaling::ScopedActiveTransform layout(transform, true);
    m_ItemHotKey.UseItemRButton();
}

void SEASON3B::CNewUIMainFrameWindow::UpdateItemHotKey()
{
    m_ItemHotKey.UpdateKeyEvent();
}

void SEASON3B::CNewUIMainFrameWindow::ResetSkillHotKey()
{
    g_pSkillList->Reset();
}

void SEASON3B::CNewUIMainFrameWindow::SetSkillHotKey(int iHotKey, int iSkillType)
{
    g_pSkillList->SetHotKey(iHotKey, iSkillType);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKey(int iHotKey)
{
    return g_pSkillList->GetHotKey(iHotKey);
}

int SEASON3B::CNewUIMainFrameWindow::GetSkillHotKeyIndex(int iSkillType)
{
    return g_pSkillList->GetSkillIndex(iSkillType);
}

SEASON3B::CNewUIItemHotKey::CNewUIItemHotKey()
{
    for (int i = 0; i < HOTKEY_COUNT; ++i)
    {
        m_iHotKeyItemType[i] = -1;
        m_iHotKeyItemLevel[i] = 0;
    }
}

SEASON3B::CNewUIItemHotKey::~CNewUIItemHotKey()
{
}

bool SEASON3B::CNewUIItemHotKey::UpdateKeyEvent()
{
    int iIndex = -1;

    if (SEASON3B::IsPress('Q') == true)
    {
        iIndex = GetHotKeyItemIndex(HOTKEY_Q);
    }
    else if (SEASON3B::IsPress('W') == true)
    {
        iIndex = GetHotKeyItemIndex(HOTKEY_W);
    }
    else if (SEASON3B::IsPress('E') == true)
    {
        iIndex = GetHotKeyItemIndex(HOTKEY_E);
    }
    else if (SEASON3B::IsPress('R') == true)
    {
        iIndex = GetHotKeyItemIndex(HOTKEY_R);
    }

    if (iIndex != -1)
    {
        ITEM* pItem = NULL;
        pItem = g_pMyInventory->FindItem(iIndex);
        if (GameLogic::Items::IsElixir(pItem))
        {
            std::list<eBuffState> secretPotionbufflist;
            secretPotionbufflist.push_back(eBuff_SecretPotion1);
            secretPotionbufflist.push_back(eBuff_SecretPotion2);
            secretPotionbufflist.push_back(eBuff_SecretPotion3);
            secretPotionbufflist.push_back(eBuff_SecretPotion4);
            secretPotionbufflist.push_back(eBuff_SecretPotion5);

            if (g_isCharacterBufflist((&Hero->Object), secretPotionbufflist) != eBuffNone) {
                SEASON3B::CreateOkMessageBox(I18N::Game::YouCannotUseThisItemWhileThePotionEffectsRemainActive, RGBA(255, 30, 0, 255));
            }
            else {
                SendRequestUse(iIndex, 0);
            }
        }
        else

        {
            SendRequestUse(iIndex, 0);
        }
        return false;
    }

    return true;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemIndex(int iType, bool bItemCount)
{
    int iStartItemType = 0, iEndItemType = 0;
    int i, j;

    switch (iType)
    {
    case HOTKEY_Q:
        if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
        {
            if (GameLogic::Items::IsManaPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_MANA_POTION; iEndItemType = ITEM_SMALL_MANA_POTION;
            }
            else
            {
                iStartItemType = ITEM_LARGE_HEALING_POTION; iEndItemType = ITEM_APPLE;
            }
        }
        break;
    case HOTKEY_W:
        if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
        {
            if (GameLogic::Items::IsHealingPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_HEALING_POTION; iEndItemType = ITEM_APPLE;
            }
            else
            {
                iStartItemType = ITEM_LARGE_MANA_POTION; iEndItemType = ITEM_SMALL_MANA_POTION;
            }
        }
        break;
    case HOTKEY_E:
        if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
        {
            if (GameLogic::Items::IsHealingPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_HEALING_POTION; iEndItemType = ITEM_APPLE;
            }
            else if (GameLogic::Items::IsManaPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_MANA_POTION; iEndItemType = ITEM_SMALL_MANA_POTION;
            }
            else
            {
                iStartItemType = ITEM_ANTIDOTE; iEndItemType = ITEM_ANTIDOTE;
            }
        }
        break;
    case HOTKEY_R:
        if (GetHotKeyCommonItem(iType, iStartItemType, iEndItemType) == false)
        {
            if (GameLogic::Items::IsHealingPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_HEALING_POTION; iEndItemType = ITEM_APPLE;
            }
            else if (GameLogic::Items::IsManaPotionType(m_iHotKeyItemType[iType]))
            {
                iStartItemType = ITEM_LARGE_MANA_POTION; iEndItemType = ITEM_SMALL_MANA_POTION;
            }
            else
            {
                iStartItemType = ITEM_LARGE_SHIELD_POTION; iEndItemType = ITEM_SMALL_SHIELD_POTION;
            }
        }
        break;
    }

    int iItemCount = 0;
    ITEM* pItem = NULL;

    int iNumberofItems = g_pMyInventory->GetInventoryCtrl()->GetNumberOfItems();
    for (i = iStartItemType; i >= iEndItemType; --i)
    {
        if (bItemCount)
        {
            for (j = 0; j < iNumberofItems; ++j)
            {
                pItem = g_pMyInventory->GetInventoryCtrl()->GetItem(j);
                if (pItem == NULL)
                {
                    continue;
                }

                if (
                    (pItem->Type == i && pItem->Level == m_iHotKeyItemLevel[iType])
                    || (pItem->Type == i && GameLogic::Items::IsHealingPotion(pItem))
                    )
                {
                    if (pItem->Type == ITEM_ALE
                        || pItem->Type == ITEM_TOWN_PORTAL_SCROLL
                        || pItem->Type == ITEM_REMEDY_OF_LOVE
                        )
                    {
                        iItemCount++;
                    }
                    else
                    {
                        iItemCount += pItem->Durability;
                    }
                }
            }
        }
        else
        {
            int iIndex = -1;
            if (GameLogic::Items::IsHealingPotionType(i))
            {
                iIndex = g_pMyInventory->FindItemReverseIndex(i);
            }
            else
            {
                iIndex = g_pMyInventory->FindItemReverseIndex(i, m_iHotKeyItemLevel[iType]);
            }

            if (-1 != iIndex)
            {
                pItem = g_pMyInventory->FindItem(iIndex);
                if ((pItem->Type != ITEM_SIEGE_POTION
                    && pItem->Type != ITEM_TOWN_PORTAL_SCROLL
                    && pItem->Type != ITEM_REMEDY_OF_LOVE)
                    || pItem->Level == m_iHotKeyItemLevel[iType]
                    )
                {
                    return iIndex;
                }
            }
        }
    }

    if (bItemCount == true)
    {
        return iItemCount;
    }

    return -1;
}

bool SEASON3B::CNewUIItemHotKey::GetHotKeyCommonItem(IN int iHotKey, OUT int& iStart, OUT int& iEnd)
{
    switch (m_iHotKeyItemType[iHotKey])
    {
    case ITEM_SIEGE_POTION:
    case ITEM_ANTIDOTE:
    case ITEM_ALE:
    case ITEM_TOWN_PORTAL_SCROLL:
    case ITEM_REMEDY_OF_LOVE:
    case ITEM_JACK_OLANTERN_BLESSINGS:
    case ITEM_JACK_OLANTERN_WRATH:
    case ITEM_JACK_OLANTERN_CRY:
    case ITEM_JACK_OLANTERN_FOOD:
    case ITEM_JACK_OLANTERN_DRINK:
    case ITEM_ELITE_HEALING_POTION:
    case ITEM_ELITE_MANA_POTION:
    case ITEM_ELIXIR_OF_STRENGTH:
    case ITEM_ELIXIR_OF_AGILITY:
    case ITEM_ELIXIR_OF_HEALTH:
    case ITEM_ELIXIR_OF_ENERGY:
    case ITEM_ELIXIR_OF_CONTROL:
    case ITEM_MEDIUM_ELITE_HEALING_POTION:
    case ITEM_CHERRY_BLOSSOM_WINE:
    case ITEM_CHERRY_BLOSSOM_RICE_CAKE:
    case ITEM_CHERRY_BLOSSOM_FLOWER_PETAL:
    case ITEM_ELITE_SD_POTION:
        if (m_iHotKeyItemType[iHotKey] != ITEM_REMEDY_OF_LOVE || m_iHotKeyItemLevel[iHotKey] == 0)
        {
            iStart = iEnd = m_iHotKeyItemType[iHotKey];
            return true;
        }
        break;
    default:
        if (m_iHotKeyItemType[iHotKey] >= ITEM_SMALL_SHIELD_POTION && m_iHotKeyItemType[iHotKey] <= ITEM_LARGE_SHIELD_POTION)
        {
            iStart = ITEM_LARGE_SHIELD_POTION; iEnd = ITEM_SMALL_SHIELD_POTION;
            return true;
        }
        else if (GameLogic::Items::IsComplexPotionType(m_iHotKeyItemType[iHotKey]))
        {
            iStart = ITEM_LARGE_COMPLEX_POTION; iEnd = ITEM_SMALL_COMPLEX_POTION;
            return true;
        }
        break;
    }
    return false;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyItemCount(int iType)
{
    return 0;
}

void SEASON3B::CNewUIItemHotKey::SetHotKey(int iHotKey, int iItemType, int iItemLevel)
{
    if (iHotKey != -1 && CNewUIMyInventory::CanRegisterItemHotKey(iItemType) == true
        )
    {
        m_iHotKeyItemType[iHotKey] = iItemType;
        m_iHotKeyItemLevel[iHotKey] = iItemLevel;
    }
    else
    {
        m_iHotKeyItemType[iHotKey] = -1;
        m_iHotKeyItemLevel[iHotKey] = 0;
    }
}

int SEASON3B::CNewUIItemHotKey::GetHotKey(int iHotKey)
{
    if (iHotKey != -1)
    {
        return m_iHotKeyItemType[iHotKey];
    }

    return -1;
}

int SEASON3B::CNewUIItemHotKey::GetHotKeyLevel(int iHotKey)
{
    if (iHotKey != -1)
    {
        return m_iHotKeyItemLevel[iHotKey];
    }

    return 0;
}

void SEASON3B::CNewUIItemHotKey::RenderItems()
{
    for (int i = 0; i < HOTKEY_COUNT; ++i)
    {
        int iIndex = GetHotKeyItemIndex(i);
        if (iIndex != -1)
        {
            ITEM* pItem = g_pMyInventory->FindItem(iIndex);
            if (pItem)
            {
                const Layout::Rect icon = ItemIconRect(i);
                RenderItem3D(icon.x, icon.y, icon.width, icon.height, pItem->Type, pItem->Level, 0, 0);
            }
        }
    }
}

void SEASON3B::CNewUIItemHotKey::RenderItemCount()
{
    for (int i = 0; i < HOTKEY_COUNT; ++i)
    {
        int iCount = GetHotKeyItemIndex(i, true);
        if (iCount > 0)
        {
            const Layout::Rect icon = ItemIconRect(i);
            SEASON3B::RenderNumber(icon.x + icon.width, icon.y + icon.height - kItemCountLift, iCount);
        }
    }
}

void SEASON3B::CNewUIItemHotKey::UseItemRButton()
{
    for (int i = 0; i < HOTKEY_COUNT; ++i)
    {
        if (IsMouseIn(Layout::ItemHotKeySlot(i)))
        {
            if (MouseRButtonPush)
            {
                MouseRButtonPush = false;
                int iIndex = GetHotKeyItemIndex(i);
                if (iIndex != -1)
                {
                    SendRequestUse(iIndex, 0);
                    break;
                }
            }
        }
    }
}

SEASON3B::CNewUISkillList::CNewUISkillList()
{
    m_pNewUIMng = NULL;
    Reset();
}

SEASON3B::CNewUISkillList::~CNewUISkillList()
{
    Release();
}

bool SEASON3B::CNewUISkillList::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_SKILL_LIST, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;

    LoadImages();

    Show(true);

    return true;
}

void SEASON3B::CNewUISkillList::Release()
{
    if (m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->DeleteUI2DEffectObject(UI2DEffectCallback);
    }

    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void SEASON3B::CNewUISkillList::Reset()
{
    m_bSkillList = false;
    m_bHotKeySkillListUp = false;

    m_bRenderSkillInfo = false;
    m_iRenderSkillInfoType = 0;
    m_iRenderSkillInfoPosX = 0;
    m_iRenderSkillInfoPosY = 0;

    for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
    {
        m_iHotKeySkillType[i] = -1;
    }

    m_EventState = EVENT_NONE;
}

void SEASON3B::CNewUISkillList::LoadImages()
{
    LoadBitmap(L"Interface\\newui_skill.jpg", IMAGE_SKILL1, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_skill2.jpg", IMAGE_SKILL2, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_command.jpg", IMAGE_COMMAND, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_skillbox.jpg", IMAGE_SKILLBOX, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_skillbox2.jpg", IMAGE_SKILLBOX_USE, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_non_skill.jpg", IMAGE_NON_SKILL1, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_non_skill2.jpg", IMAGE_NON_SKILL2, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_non_command.jpg", IMAGE_NON_COMMAND, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_skill3.jpg", IMAGE_SKILL3, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_non_skill3.jpg", IMAGE_NON_SKILL3, GL_LINEAR);
}

void SEASON3B::CNewUISkillList::UnloadImages()
{
    DeleteBitmap(IMAGE_SKILL1);
    DeleteBitmap(IMAGE_SKILL2);
    DeleteBitmap(IMAGE_COMMAND);
    DeleteBitmap(IMAGE_SKILLBOX);
    DeleteBitmap(IMAGE_SKILLBOX_USE);
    DeleteBitmap(IMAGE_NON_SKILL1);
    DeleteBitmap(IMAGE_NON_SKILL2);
    DeleteBitmap(IMAGE_NON_COMMAND);
    DeleteBitmap(IMAGE_SKILL3);
    DeleteBitmap(IMAGE_NON_SKILL3);
}

bool SEASON3B::CNewUISkillList::UpdateMouseEvent()
{
#ifdef MOD_SKILLLIST_UPDATEMOUSE_BLOCK
    if (GFxProcess::GetInstancePtr()->GetUISelect() == 1)
    {
        return true;
    }
#endif //MOD_SKILLLIST_UPDATEMOUSE_BLOCK

    if (g_isCharacterBuff((&Hero->Object), eBuff_DuelWatch))
    {
        m_bSkillList = false;
        return true;
    }

    BYTE bySkillNumber = CharacterAttribute->SkillNumber;
    BYTE bySkillMasterNumber = CharacterAttribute->SkillMasterNumber;

    float x, y, width, height;

    m_bRenderSkillInfo = false;

    if (bySkillNumber <= 0)
    {
        return true;
    }

    const Layout::Rect currentSkillSlot = Layout::CurrentSkillSlot();
    x = currentSkillSlot.x; y = currentSkillSlot.y; width = currentSkillSlot.width; height = currentSkillSlot.height;
    if (SEASON3B::CheckMouseIn(x, y, width, height))
    {
        MouseOnWindow = true;
    }

    if (m_EventState == EVENT_NONE && MouseLButtonPush == false
        && SEASON3B::CheckMouseIn(x, y, width, height) == true)
    {
        m_EventState = EVENT_BTN_HOVER_CURRENTSKILL;
        return true;
    }
    if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && MouseLButtonPush == false
        && SEASON3B::CheckMouseIn(x, y, width, height) == false)
    {
        m_EventState = EVENT_NONE;
        return true;
    }
    if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL && (MouseLButtonPush == true || MouseLButtonDBClick == true)
        && SEASON3B::CheckMouseIn(x, y, width, height) == true)
    {
        m_EventState = EVENT_BTN_DOWN_CURRENTSKILL;
        return false;
    }
    if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
    {
        if (MouseLButtonPush == false && MouseLButtonDBClick == false)
        {
            if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
            {
                m_bSkillList = !m_bSkillList;
                PlayBuffer(SOUND_CLICK01);
                m_EventState = EVENT_NONE;
                return false;
            }
            m_EventState = EVENT_NONE;
            return true;
        }
    }

    if (m_EventState == EVENT_BTN_HOVER_CURRENTSKILL)
    {
        m_bRenderSkillInfo = true;
        m_iRenderSkillInfoType = Hero->CurrentSkill;
        m_iRenderSkillInfoPosX = x - 5;
        m_iRenderSkillInfoPosY = y;

        return false;
    }
    else if (m_EventState == EVENT_BTN_DOWN_CURRENTSKILL)
    {
        return false;
    }

    const Layout::Rect hotKeyStrip = Layout::SkillHotKeyStrip();
    x = hotKeyStrip.x; y = hotKeyStrip.y; width = hotKeyStrip.width; height = hotKeyStrip.height;
    if (SEASON3B::CheckMouseIn(x, y, width, height))
    {
        MouseOnWindow = true;
    }

    if (m_EventState == EVENT_NONE && MouseLButtonPush == false
        && SEASON3B::CheckMouseIn(x, y, width, height) == true)
    {
        m_EventState = EVENT_BTN_HOVER_SKILLHOTKEY;
        return true;
    }
    if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == false
        && SEASON3B::CheckMouseIn(x, y, width, height) == false)
    {
        m_EventState = EVENT_NONE;
        return true;
    }
    if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY && MouseLButtonPush == true
        && SEASON3B::CheckMouseIn(x, y, width, height) == true)
    {
        m_EventState = EVENT_BTN_DOWN_SKILLHOTKEY;
        return false;
    }

    int iStartIndex = (m_bHotKeySkillListUp == true) ? 6 : 1;
    for (int i = 0, iIndex = iStartIndex; i < Layout::SkillHotKeySlotCount; ++i, iIndex++)
    {
        const Layout::Rect slot = Layout::SkillHotKeySlot(i);
        x = slot.x; y = slot.y; width = slot.width; height = slot.height;

        if (iIndex == 10)
        {
            iIndex = 0;
        }
        if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
        {
            if (m_iHotKeySkillType[iIndex] == -1)
            {
                if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
                {
                    m_bRenderSkillInfo = false;
                    m_iRenderSkillInfoType = -1;
                }
                if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY && MouseLButtonPush == false)
                {
                    m_EventState = EVENT_NONE;
                }
                continue;
            }

            WORD bySkillType = CharacterAttribute->Skill[m_iHotKeySkillType[iIndex]];

            if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
                continue;

            BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

            if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
            {
                continue;
            }

            if (m_EventState == EVENT_BTN_HOVER_SKILLHOTKEY)
            {
                m_bRenderSkillInfo = true;
                m_iRenderSkillInfoType = m_iHotKeySkillType[iIndex];
                m_iRenderSkillInfoPosX = x - 5;
                m_iRenderSkillInfoPosY = y;
                return true;
            }
            if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
            {
                if (MouseLButtonPush == false)
                {
                    if (m_iRenderSkillInfoType == m_iHotKeySkillType[iIndex])
                    {
                        m_EventState = EVENT_NONE;
                        m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];
                        Hero->CurrentSkill = m_iHotKeySkillType[iIndex];
                        PlayBuffer(SOUND_CLICK01);
                        return false;
                    }
                    else
                    {
                        m_EventState = EVENT_NONE;
                    }
                }
            }
        }
    }

    x = hotKeyStrip.x; y = hotKeyStrip.y; width = hotKeyStrip.width; height = hotKeyStrip.height;
    if (m_EventState == EVENT_BTN_DOWN_SKILLHOTKEY)
    {
        if (MouseLButtonPush == false && SEASON3B::CheckMouseIn(x, y, width, height) == false)
        {
            m_EventState = EVENT_NONE;
            return true;
        }
        return false;
    }

    if (m_bSkillList == false)
        return true;

    WORD bySkillType = 0;

    int iSkillCount = 0;
    bool bMouseOnSkillList = false;

    x = kSkillListOriginX; y = SkillListTop(); width = kSkillBoxWidth; height = kSkillBoxHeight;
    float fOrigX = kSkillListOriginX;

    EVENT_STATE PrevEventState = m_EventState;

    for (int i = 0; i < MAX_MAGIC; ++i)
    {
        bySkillType = CharacterAttribute->Skill[i];

        if (bySkillType == 0 || (bySkillType >= AT_SKILL_STUN && bySkillType <= AT_SKILL_REMOVAL_BUFF))
            continue;

        BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;

        if (bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
        {
            continue;
        }

        if (iSkillCount == 18)
        {
            y -= height;
        }

        if (iSkillCount < 14)
        {
            int iRemainder = iSkillCount % 2;
            int iQuotient = iSkillCount / 2;

            if (iRemainder == 0)
            {
                x = fOrigX + iQuotient * width;
            }
            else
            {
                x = fOrigX - (iQuotient + 1) * width;
            }
        }
        else if (iSkillCount >= 14 && iSkillCount < 18)
        {
            x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
        }
        else
        {
            x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
        }

        iSkillCount++;

        if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
        {
            bMouseOnSkillList = true;
            MouseOnWindow = true;
            if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
            {
                m_EventState = EVENT_BTN_HOVER_SKILLLIST;
                break;
            }
        }

        if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true
            && SEASON3B::CheckMouseIn(x, y, width, height) == true)
        {
            m_EventState = EVENT_BTN_DOWN_SKILLLIST;
            break;
        }

        if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == false
            && SEASON3B::CheckMouseIn(x, y, width, height) == true)
        {
            m_bRenderSkillInfo = true;
            m_iRenderSkillInfoType = i;
            m_iRenderSkillInfoPosX = x;
            m_iRenderSkillInfoPosY = y;
        }

        if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
            && m_iRenderSkillInfoType == i && SEASON3B::CheckMouseIn(x, y, width, height) == true)
        {
            m_EventState = EVENT_NONE;

            m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

            Hero->CurrentSkill = i;
            m_bSkillList = false;

            PlayBuffer(SOUND_CLICK01);
            return false;
        }
    }

    if (PrevEventState != m_EventState)
    {
        if (m_EventState == EVENT_NONE || m_EventState == EVENT_BTN_HOVER_SKILLLIST)
            return true;
        return false;
    }

    if (Hero->m_pPet != NULL)
    {
        x = PetCommandRowX(); y = PetCommandRowTop(); width = kSkillBoxWidth; height = kSkillBoxHeight;
        for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
        {
            if (SEASON3B::CheckMouseIn(x, y, width, height) == true)
            {
                bMouseOnSkillList = true;
                MouseOnWindow = true;

                if (m_EventState == EVENT_NONE && MouseLButtonPush == false)
                {
                    m_EventState = EVENT_BTN_HOVER_SKILLLIST;
                    return true;
                }
                if (m_EventState == EVENT_BTN_HOVER_SKILLLIST && MouseLButtonPush == true)
                {
                    m_EventState = EVENT_BTN_DOWN_SKILLLIST;
                    return false;
                }

                if (m_EventState == EVENT_BTN_HOVER_SKILLLIST)
                {
                    m_bRenderSkillInfo = true;
                    m_iRenderSkillInfoType = i;
                    m_iRenderSkillInfoPosX = x;
                    m_iRenderSkillInfoPosY = y;
                }
                if (m_EventState == EVENT_BTN_DOWN_SKILLLIST && MouseLButtonPush == false
                    && m_iRenderSkillInfoType == i)
                {
                    m_EventState = EVENT_NONE;

                    m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

                    Hero->CurrentSkill = i;
                    m_bSkillList = false;
                    PlayBuffer(SOUND_CLICK01);
                    return false;
                }
            }
            x += width;
        }
    }

    if (bMouseOnSkillList == false && m_EventState == EVENT_BTN_HOVER_SKILLLIST)
    {
        m_EventState = EVENT_NONE;
        return true;
    }
    if (bMouseOnSkillList == false && MouseLButtonPush == false
        && m_EventState == EVENT_BTN_DOWN_SKILLLIST)
    {
        m_EventState = EVENT_NONE;
        return false;
    }
    if (m_EventState == EVENT_BTN_DOWN_SKILLLIST)
    {
        if (MouseLButtonPush == false)
        {
            m_EventState = EVENT_NONE;
            return true;
        }
        return false;
    }

    return true;
}

bool SEASON3B::CNewUISkillList::UpdateKeyEvent()
{
    for (int i = 0; i < 9; ++i)
    {
        if (SEASON3B::IsPress('1' + i))
        {
            UseHotKey(i + 1);
        }
    }

    if (SEASON3B::IsPress('0'))
    {
        UseHotKey(0);
    }

    if (m_EventState == EVENT_BTN_HOVER_SKILLLIST)
    {
        if (SEASON3B::IsRepeat(VK_CONTROL))
        {
            for (int i = 0; i < 9; ++i)
            {
                if (SEASON3B::IsPress('1' + i))
                {
                    SetHotKey(i + 1, m_iRenderSkillInfoType);

                    return false;
                }
            }

            if (SEASON3B::IsPress('0'))
            {
                SetHotKey(0, m_iRenderSkillInfoType);

                return false;
            }
        }
    }

    if (SEASON3B::IsRepeat(VK_SHIFT))
    {
        for (int i = 0; i < 4; ++i)
        {
            if (SEASON3B::IsPress('1' + i))
            {
                Hero->CurrentSkill = AT_PET_COMMAND_DEFAULT + i;
                return false;
            }
        }
    }

    return true;
}

bool SEASON3B::CNewUISkillList::IsArrayUp(BYTE bySkill)
{
    for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
    {
        if (m_iHotKeySkillType[i] == bySkill)
        {
            if (i == 0 || i > 5)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

bool SEASON3B::CNewUISkillList::IsArrayIn(BYTE bySkill)
{
    for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
    {
        if (m_iHotKeySkillType[i] == bySkill)
        {
            return true;
        }
    }

    return false;
}

void SEASON3B::CNewUISkillList::SetHotKey(int iHotKey, int iSkillType)
{
    for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
    {
        if (m_iHotKeySkillType[i] == iSkillType)
        {
            m_iHotKeySkillType[i] = -1;
            break;
        }
    }

    m_iHotKeySkillType[iHotKey] = iSkillType;
}

int SEASON3B::CNewUISkillList::GetHotKey(int iHotKey)
{
    return m_iHotKeySkillType[iHotKey];
}

int SEASON3B::CNewUISkillList::GetSkillIndex(int iSkillType)
{
    // special handling for skills with different skill id for the trigger
    if (iSkillType == AT_SKILL_NOVA_BEGIN)
    {
        iSkillType = AT_SKILL_NOVA;
    }

    int iReturn = -1;
    for (int i = 0; i < MAX_MAGIC; ++i)
    {
        if (CharacterAttribute->Skill[i] == iSkillType)
        {
            iReturn = i;
            break;
        }
    }

    return iReturn;
}

void SEASON3B::CNewUISkillList::UseHotKey(int iHotKey)
{
    if (m_iHotKeySkillType[iHotKey] != -1)
    {
        if (m_iHotKeySkillType[iHotKey] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iHotKey] < AT_PET_COMMAND_END)
        {
            if (Hero->m_pPet == NULL)
            {
                return;
            }
        }

        auto wHotKeySkill = CharacterAttribute->Skill[m_iHotKeySkillType[iHotKey]];

        if (wHotKeySkill == 0)
        {
            return;
        }

        m_wHeroPriorSkill = CharacterAttribute->Skill[Hero->CurrentSkill];

        Hero->CurrentSkill = m_iHotKeySkillType[iHotKey];

        auto bySkill = CharacterAttribute->Skill[Hero->CurrentSkill];

        if (
            g_pOption->IsAutoAttack() == true
            && gMapManager.WorldActive != WD_6STADIUM
            && gMapManager.InChaosCastle() == false
            && (bySkill == AT_SKILL_TELEPORT || bySkill == AT_SKILL_TELEPORT_ALLY))
        {
            SelectedCharacter = -1;
            Attacking = -1;
        }
    }
}

bool SEASON3B::CNewUISkillList::Update()
{
    if (IsArrayIn(Hero->CurrentSkill) == true)
    {
        if (IsArrayUp(Hero->CurrentSkill) == true)
        {
            m_bHotKeySkillListUp = true;
        }
        else
        {
            m_bHotKeySkillListUp = false;
        }
    }

    if (Hero->m_pPet == NULL)
    {
        if (Hero->CurrentSkill >= AT_PET_COMMAND_DEFAULT && Hero->CurrentSkill < AT_PET_COMMAND_END)
        {
            Hero->CurrentSkill = 0;
        }
    }

    return true;
}

void SEASON3B::CNewUISkillList::RenderCurrentSkillAndHotSkillList()
{
    int i;

    BYTE bySkillNumber = CharacterAttribute->SkillNumber;

    if (bySkillNumber > 0)
    {
        int iStartSkillIndex = 1;
        if (m_bHotKeySkillListUp)
        {
            iStartSkillIndex = 6;
        }

        for (i = 0; i < Layout::SkillHotKeySlotCount; ++i)
        {
            const Layout::Rect slot = Layout::SkillHotKeySlot(i);

            int iIndex = iStartSkillIndex + i;
            if (iIndex == 10)
            {
                iIndex = 0;
            }

            if (m_iHotKeySkillType[iIndex] == -1)
            {
                continue;
            }

            if (m_iHotKeySkillType[iIndex] >= AT_PET_COMMAND_DEFAULT && m_iHotKeySkillType[iIndex] < AT_PET_COMMAND_END)
            {
                if (Hero->m_pPet == NULL)
                {
                    continue;
                }
            }

            if (Hero->CurrentSkill == m_iHotKeySkillType[iIndex])
            {
                SEASON3B::RenderImageStretch(IMAGE_SKILLBOX_USE, slot.x, slot.y, slot.width, slot.height, 0.0f, 0.0f,
                                             kSkillBoxArt.width, kSkillBoxArt.height);
            }
            const Layout::Rect icon = SkillIconRect(slot);
            RenderSkillIcon(m_iHotKeySkillType[iIndex], icon.x, icon.y, icon.width, icon.height);
        }

        const Layout::Rect icon = SkillIconRect(Layout::CurrentSkillSlot());
        RenderSkillIcon(Hero->CurrentSkill, icon.x, icon.y, icon.width, icon.height);
    }
}

bool SEASON3B::CNewUISkillList::Render()
{
    int i;
    float x, y, width, height;

    BYTE bySkillNumber = CharacterAttribute->SkillNumber;

    if (bySkillNumber > 0)
    {
        if (m_bSkillList == true)
        {
            x = kSkillListOriginX; y = SkillListTop(); width = kSkillBoxWidth; height = kSkillBoxHeight;
            float fOrigX = kSkillListOriginX;
            int iSkillType = 0;
            int iSkillCount = 0;

            for (i = 0; i < MAX_MAGIC; ++i)
            {
                iSkillType = CharacterAttribute->Skill[i];

                if (iSkillType != 0 && (iSkillType < AT_SKILL_STUN || iSkillType > AT_SKILL_REMOVAL_BUFF))
                {
                    BYTE bySkillUseType = SkillAttribute[iSkillType].SkillUseType;

                    if (bySkillUseType == SKILL_USE_TYPE_MASTER || bySkillUseType == SKILL_USE_TYPE_MASTERLEVEL)
                    {
                        continue;
                    }

                    if (iSkillCount == 18)
                    {
                        y -= height;
                    }

                    if (iSkillCount < 14)
                    {
                        int iRemainder = iSkillCount % 2;
                        int iQuotient = iSkillCount / 2;

                        if (iRemainder == 0)
                        {
                            x = fOrigX + iQuotient * width;
                        }
                        else
                        {
                            x = fOrigX - (iQuotient + 1) * width;
                        }
                    }
                    else if (iSkillCount >= 14 && iSkillCount < 18)
                    {
                        x = fOrigX - (8 * width) - ((iSkillCount - 14) * width);
                    }
                    else
                    {
                        x = fOrigX - (12 * width) + ((iSkillCount - 17) * width);
                    }

                    iSkillCount++;

                    if (i == Hero->CurrentSkill)
                    {
                        SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
                    }
                    else
                    {
                        SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
                    }

                    RenderSkillIcon(i, x + 6, y + 6, 20, 28);
                }
            }
            RenderPetSkill();
        }
    }

    // Do NOT reset m_bRenderSkillInfo here. UpdateMouseEvent() runs once per fixed 50Hz tick and is
    // the sole authority on hover state (it re-derives true/false fresh every tick), but Render()
    // runs at full render rate (~120fps+, decoupled from the tick since the fixed-timestep refactor
    // -- see SceneManager.cpp's UpdateSceneState()). Clearing the flag here was a one-shot
    // consume-and-reset that only rendered the tooltip on the first render frame after each tick,
    // then skipped it for the remaining ~1-2 render frames until the next tick fired -- a rapid
    // strobe, reported as the tooltip "rapidly blinking" while hovering. Safe to just read it every
    // frame instead: CNewUI3DCamera::Render() already drains m_deque2DEffects fully every single
    // call (NewUI3DRenderMng.cpp), so re-queuing every render frame while still hovering does not
    // accumulate or leak.
    if (m_bRenderSkillInfo == true && m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);
    }

    return true;
}

void SEASON3B::CNewUISkillList::RenderSkillInfo()
{
    UI::Skills::Tooltip::Render(m_iRenderSkillInfoPosX + 15, m_iRenderSkillInfoPosY - 10, m_iRenderSkillInfoType);
}

float SEASON3B::CNewUISkillList::GetLayerDepth()
{
    return 5.2f;
}

WORD SEASON3B::CNewUISkillList::GetHeroPriorSkill()
{
    return m_wHeroPriorSkill;
}

void SEASON3B::CNewUISkillList::SetHeroPriorSkill(BYTE bySkill)
{
    m_wHeroPriorSkill = bySkill;
}

void SEASON3B::CNewUISkillList::RenderPetSkill()
{
    if (Hero->m_pPet == NULL)
    {
        return;
    }

    float x, y, width, height;

    x = PetCommandRowX(); y = PetCommandRowTop(); width = kSkillBoxWidth; height = kSkillBoxHeight;
    for (int i = AT_PET_COMMAND_DEFAULT; i < AT_PET_COMMAND_END; ++i)
    {
        if (i == Hero->CurrentSkill)
        {
            SEASON3B::RenderImage(IMAGE_SKILLBOX_USE, x, y, width, height);
        }
        else
        {
            SEASON3B::RenderImage(IMAGE_SKILLBOX, x, y, width, height);
        }

        RenderSkillIcon(i, x + 6, y + 6, 20, 28);
        x += width;
    }
}

void SEASON3B::CNewUISkillList::RenderSkillIcon(int iIndex, float x, float y, float width, float height)
{
    auto bySkillType = CharacterAttribute->Skill[iIndex];

    if (bySkillType == 0)
    {
        return;
    }

    if (iIndex >= AT_PET_COMMAND_DEFAULT)
    {
        bySkillType = (ActionSkillType)iIndex;
    }

    bool bCantSkill = false;

    BYTE bySkillUseType = SkillAttribute[bySkillType].SkillUseType;
    int Skill_Icon = SkillAttribute[bySkillType].Magic_Icon;

    if (!gSkillManager.AreSkillAttributeRequirementsMet(bySkillType))
    {
        bCantSkill = true;
    }

    if (IsCanBCSkill(bySkillType) == false)
    {
        bCantSkill = true;
    }
    if (g_isCharacterBuff((&Hero->Object), eBuff_AddSkill) && bySkillUseType == SKILL_USE_TYPE_BRAND)
    {
        bCantSkill = true;
    }
    auto isSittingOnPet = GameLogic::Items::IsHornMountModel(Hero->Helper.Type);
    if (bySkillType == AT_SKILL_IMPALE && !isSittingOnPet)
    {
        bCantSkill = true;
    }

    if (bySkillType == AT_SKILL_IMPALE && isSittingOnPet)
    {
        int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
        int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;
        if ((iTypeL < ITEM_SPEAR || iTypeL >= ITEM_BOW) && (iTypeR < ITEM_SPEAR || iTypeR >= ITEM_BOW))
        {
            bCantSkill = true;
        }
    }

    if (isSittingOnPet
        && ((bySkillType >= AT_SKILL_BLOCKING && bySkillType <= AT_SKILL_SLASH)
            || bySkillType == AT_SKILL_FALLING_SLASH_STR
            || bySkillType == AT_SKILL_LUNGE_STR
            || bySkillType == AT_SKILL_CYCLONE_STR
            || bySkillType == AT_SKILL_CYCLONE_STR_MG
            || bySkillType == AT_SKILL_SLASH_STR
            ))
    {
        bCantSkill = true;
    }

    if ((bySkillType == AT_SKILL_POWER_SLASH || bySkillType == AT_SKILL_POWER_SLASH_STR)
        && isSittingOnPet)
    {
        bCantSkill = true;
    }

    if (bySkillType == AT_SKILL_PARTY_TELEPORT && PartyNumber <= 0)
    {
        bCantSkill = true;
    }

    if (bySkillType == AT_SKILL_PARTY_TELEPORT && (IsDoppelGanger1() || IsDoppelGanger2() || IsDoppelGanger3() || IsDoppelGanger4()))
    {
        bCantSkill = true;
    }

    if (bySkillType == AT_SKILL_EARTHSHAKE || bySkillType == AT_SKILL_EARTHSHAKE_STR || bySkillType == AT_SKILL_EARTHSHAKE_MASTERY)
    {
        BYTE byDarkHorseLife = 0;
        byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
        if (byDarkHorseLife == 0 || Hero->Helper.Type != MODEL_DARK_HORSE_ITEM)
        {
            bCantSkill = true;
        }
    }
#ifdef PJH_FIX_SPRIT
    /*박종훈*/
    if (bySkillType >= AT_PET_COMMAND_DEFAULT && bySkillType < AT_PET_COMMAND_END)
    {
        int iCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;
        PET_INFO PetInfo;
        giPetManager::GetPetInfo(PetInfo, 421 - PET_TYPE_DARK_SPIRIT);
        int RequireCharisma = (185 + (PetInfo.m_wLevel * 15));
        if (RequireCharisma > iCharisma)
        {
            bCantSkill = true;
        }
    }
#endif //PJH_FIX_SPRIT
    if ((bySkillType == AT_SKILL_INFINITY_ARROW)
        || (bySkillType == AT_SKILL_INFINITY_ARROW_STR)
        || (bySkillType == AT_SKILL_EXPANSION_OF_WIZARDRY)
        || (bySkillType == AT_SKILL_EXPANSION_OF_WIZARDRY_STR)
        || (bySkillType == AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY)
        )
    {
        if ((g_isCharacterBuff((&Hero->Object), eBuff_InfinityArrow)) || (g_isCharacterBuff((&Hero->Object), eBuff_SwellOfMagicPower)))
        {
            bCantSkill = true;
        }
    }

    if (bySkillType == AT_SKILL_FIRE_SLASH || bySkillType == AT_SKILL_FIRE_SLASH_STR)
    {
        WORD Strength;
        const WORD wRequireStrength = 596;
        Strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
        if (Strength < wRequireStrength)
        {
            bCantSkill = true;
        }
        int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
        int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

        if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
        {
            bCantSkill = true;
        }
    }

    switch (bySkillType)
    {
        //case AT_SKILL_PIERCING:
    case AT_SKILL_ICE_ARROW:
    case AT_SKILL_ICE_ARROW_STR:
    {
        WORD  Dexterity;
        const WORD wRequireDexterity = 646;
        Dexterity = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
        if (Dexterity < wRequireDexterity)
        {
            bCantSkill = true;
        }
    }break;
    }

    if (bySkillType == AT_SKILL_TWISTING_SLASH
        || bySkillType == AT_SKILL_TWISTING_SLASH_STR
        || bySkillType == AT_SKILL_TWISTING_SLASH_STR_MG
        || bySkillType == AT_SKILL_TWISTING_SLASH_MASTERY
        || bySkillType == AT_SKILL_RAGEFUL_BLOW
        || bySkillType == AT_SKILL_RAGEFUL_BLOW_STR
        || bySkillType == AT_SKILL_RAGEFUL_BLOW_MASTERY
        || bySkillType == AT_SKILL_DEATHSTAB
        || bySkillType == AT_SKILL_DEATHSTAB_STR
        )
    {
        int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
        int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

        if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
        {
            bCantSkill = true;
        }
    }

    if (gMapManager.InChaosCastle() == true)
    {
        if (bySkillType == AT_SKILL_EARTHSHAKE
            || bySkillType == AT_SKILL_EARTHSHAKE_STR
            || bySkillType == AT_SKILL_EARTHSHAKE_MASTERY
            || bySkillType == AT_SKILL_RIDER
            || (static_cast<int>(bySkillType) >= static_cast<int>(AT_PET_COMMAND_DEFAULT) && static_cast<int>(bySkillType) <= static_cast<int>(AT_PET_COMMAND_TARGET))
            )
        {
            bCantSkill = true;
        }
    }
    else
    {
        if (bySkillType == AT_SKILL_EARTHSHAKE
            || bySkillType == AT_SKILL_EARTHSHAKE_STR
            || bySkillType == AT_SKILL_EARTHSHAKE_MASTERY)
        {
            BYTE byDarkHorseLife = 0;
            byDarkHorseLife = CharacterMachine->Equipment[EQUIPMENT_HELPER].Durability;
            if (byDarkHorseLife == 0)
            {
                bCantSkill = true;
            }
        }
    }

    if (!g_CMonkSystem.IsSwordformGlovesUseSkill(bySkillType))
    {
        bCantSkill = true;
    }
    if (g_CMonkSystem.IsRideNotUseSkill(bySkillType, Hero->Helper.Type))
    {
        bCantSkill = true;
    }

    ITEM* pLeftRing = &CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
    ITEM* pRightRing = &CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];

    if (g_CMonkSystem.IsChangeringNotUseSkill(pLeftRing->Type, pRightRing->Type, pLeftRing->Level, pRightRing->Level)
        && (gCharacterManager.GetBaseClass(Hero->Class) == CLASS_RAGEFIGHTER))
    {
        bCantSkill = true;
    }

    float fU, fV;
    int iKindofSkill = 0;

    if (!g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(bySkillType))
    {
        bCantSkill = true;
    }

    if (static_cast<int>(bySkillType) >= static_cast<int>(AT_PET_COMMAND_DEFAULT) && static_cast<int>(bySkillType) <= static_cast<int>(AT_PET_COMMAND_END))
    {
        fU = ((static_cast<int>(bySkillType) - AT_PET_COMMAND_DEFAULT) % 8) * kSkillIconCellWidth / 256.f;
        fV = ((static_cast<int>(bySkillType) - AT_PET_COMMAND_DEFAULT) / 8) * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_COMMAND;
    }
    else if (bySkillType == AT_SKILL_PLASMA_STORM_FENRIR)
    {
        fU = 4 * kSkillIconCellWidth / 256.f;
        fV = 0.f;
        iKindofSkill = KOS_COMMAND;
    }
    else if ((bySkillType >= AT_SKILL_ALICE_DRAINLIFE && bySkillType <= AT_SKILL_ALICE_THORNS))
    {
        fU = ((bySkillType - AT_SKILL_ALICE_DRAINLIFE) % 8) * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType >= AT_SKILL_ALICE_SLEEP && bySkillType <= AT_SKILL_ALICE_BLIND)
    {
        fU = ((bySkillType - AT_SKILL_ALICE_SLEEP + 4) % 8) * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_ALICE_BERSERKER)
    {
        fU = 10 * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType >= AT_SKILL_ALICE_WEAKNESS && bySkillType <= AT_SKILL_ALICE_ENERVATION)
    {
        fU = (bySkillType - AT_SKILL_ALICE_WEAKNESS + 8) * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType >= AT_SKILL_SUMMON_EXPLOSION && bySkillType <= AT_SKILL_SUMMON_REQUIEM)
    {
        fU = ((bySkillType - AT_SKILL_SUMMON_EXPLOSION + 6) % 8) * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_SUMMON_POLLUTION)
    {
        fU = 11 * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_STRIKE_OF_DESTRUCTION)
    {
        fU = 7 * kSkillIconCellWidth / 256.f;
        fV = 2 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_CHAOTIC_DISEIER)
    {
        fU = 3 * kSkillIconCellWidth / 256.f;
        fV = 8 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_RECOVER)
    {
        fU = 9 * kSkillIconCellWidth / 256.f;
        fV = 2 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_MULTI_SHOT)
    {
        if (gCharacterManager.GetEquipedBowType_Skill() == BOWTYPE_NONE)
        {
            bCantSkill = true;
        }

        fU = 0 * kSkillIconCellWidth / 256.f;
        fV = 8 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_FLAME_STRIKE)
    {
        int iTypeL = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type;
        int iTypeR = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type;

        if (!(iTypeR != -1 && (iTypeR < ITEM_STAFF || iTypeR >= ITEM_STAFF + MAX_ITEM_INDEX) && (iTypeL < ITEM_STAFF || iTypeL >= ITEM_STAFF + MAX_ITEM_INDEX)))
        {
            bCantSkill = true;
        }

        fU = 1 * kSkillIconCellWidth / 256.f;
        fV = 8 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_GIGANTIC_STORM)
    {
        fU = 2 * kSkillIconCellWidth / 256.f;
        fV = 8 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_LIGHTNING_SHOCK)
    {
        fU = 2 * kSkillIconCellWidth / 256.f;
        fV = 3 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType == AT_SKILL_EXPANSION_OF_WIZARDRY)
    {
        fU = 8 * kSkillIconCellWidth / 256.f;
        fV = 2 * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillUseType == 4)
    {
        fU = (kSkillIconCellWidth / 256.f) * (Skill_Icon % 12);
        fV = (kSkillIconCellHeight / 256.f) * ((Skill_Icon / 12) + 4);
        iKindofSkill = KOS_SKILL2;
    }
    else if (bySkillType >= AT_SKILL_KILLING_BLOW)
    {
        fU = ((bySkillType - AT_SKILL_KILLING_BLOW) % 12) * kSkillIconCellWidth / 256.f;
        fV = ((bySkillType - AT_SKILL_KILLING_BLOW) / 12) * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL3;
    }
    else if (bySkillType >= AT_SKILL_SPIRAL_SLASH)
    {
        fU = ((bySkillType - AT_SKILL_SPIRAL_SLASH) % 8) * kSkillIconCellWidth / 256.f;
        fV = ((bySkillType - AT_SKILL_SPIRAL_SLASH) / 8) * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL2;
    }
    else
    {
        fU = ((bySkillType - 1) % 8) * kSkillIconCellWidth / 256.f;
        fV = ((bySkillType - 1) / 8) * kSkillIconCellHeight / 256.f;
        iKindofSkill = KOS_SKILL1;
    }
    int iSkillIndex = 0;
    switch (iKindofSkill)
    {
    case KOS_COMMAND:
    {
        iSkillIndex = IMAGE_COMMAND;
    }break;
    case KOS_SKILL1:
    {
        iSkillIndex = IMAGE_SKILL1;
    }break;
    case KOS_SKILL2:
    {
        iSkillIndex = IMAGE_SKILL2;
    }break;
    case KOS_SKILL3:
    {
        iSkillIndex = IMAGE_SKILL3;
    }break;
    }

    if (bySkillType >= AT_SKILL_MASTER_BEGIN)
    {
        if (bCantSkill)
        {
            RenderImage(BITMAP_INTERFACE_MASTER_BEGIN + 3, x, y, width, height, (20.f / 512.f) * (Skill_Icon % 25), ((28.f / 512.f) * ((Skill_Icon / 25))), 20.f / 512.f, 28.f / 512.f);
        }
        else
        {
            RenderImage(BITMAP_INTERFACE_MASTER_BEGIN + 2, x, y, width, height, (20.f / 512.f)* (Skill_Icon % 25), ((28.f / 512.f)* ((Skill_Icon / 25))), 20.f / 512.f, 28.f / 512.f);
        }
    }
    else
    {
        if (bCantSkill == true)
        {
            iSkillIndex += 6;
        }

        if (iSkillIndex != 0)
        {
            RenderBitmap(iSkillIndex, x, y, width, height, fU, fV, kSkillIconCellWidth / 256.f, kSkillIconCellHeight / 256.f);
        }
    }

    int iHotKey = -1;
    for (int i = 0; i < SKILLHOTKEY_COUNT; ++i)
    {
        if (m_iHotKeySkillType[i] == iIndex)
        {
            iHotKey = i;
            break;
        }
    }

    if (iHotKey != -1)
    {
        SEASON3B::RenderNumber(x + width, y + height - kSkillHotKeyNumberLift, iHotKey);
    }

    if ((bySkillType == AT_SKILL_CHAIN_DRIVE
        || bySkillType == AT_SKILL_CHAIN_DRIVE_STR
        || bySkillType == AT_SKILL_DRAGON_KICK
        || bySkillType == AT_SKILL_DRAGON_ROAR
        || bySkillType == AT_SKILL_DRAGON_ROAR_STR) && (bCantSkill))
        return;

    if ((bySkillType != AT_SKILL_INFINITY_ARROW)
        && (bySkillType != AT_SKILL_INFINITY_ARROW_STR)
        && (bySkillType != AT_SKILL_EXPANSION_OF_WIZARDRY)
        && (bySkillType != AT_SKILL_EXPANSION_OF_WIZARDRY_STR)
        && (bySkillType != AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY)
        )
    {
        RenderSkillDelay(iIndex, x, y, width, height);
    }
}

void SEASON3B::CNewUISkillList::RenderSkillDelay(int iIndex, float x, float y, float width, float height)
{
    int iSkillDelay = CharacterAttribute->SkillDelay[iIndex];
    if (iSkillDelay > 0)
    {
        int iSkillType = CharacterAttribute->Skill[iIndex];

        if (iSkillType == AT_SKILL_PLASMA_STORM_FENRIR)
        {
            if (!CheckAttack())
            {
                return;
            }
        }

        int iSkillMaxDelay = SkillAttribute[iSkillType].Delay;

        auto fPersent = (float)(iSkillDelay / (float)iSkillMaxDelay);

        EnableAlphaTest();
        float fdeltaH = height * fPersent;
        RenderColorQuadARGB(x, y + height - fdeltaH, width, fdeltaH, 0x80FF8080u);
    }
}

bool SEASON3B::CNewUISkillList::IsSkillListUp()
{
    return m_bHotKeySkillListUp;
}

void SEASON3B::CNewUISkillList::ResetMouseLButton()
{
    MouseLButton = false;
    MouseLButtonPop = false;
    MouseLButtonPush = false;
}

void SEASON3B::CNewUISkillList::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
    if (pClass)
    {
        auto* pSkillList = (CNewUISkillList*)(pClass);
        pSkillList->RenderSkillInfo();
    }
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp_Wide(__int64 dwPreExp)
{
    m_loPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp_Wide(__int64 dwGetExp)
{
    m_loGetExp = dwGetExp;

    if (m_loGetExp > 0)
    {
        m_bExpEffect = true;
        m_dwExpEffectTime = timeGetTime();
    }
}

void SEASON3B::CNewUIMainFrameWindow::SetPreExp(__int64 dwPreExp)
{
    m_dwPreExp = dwPreExp;
}

void SEASON3B::CNewUIMainFrameWindow::SetGetExp(__int64 dwGetExp)
{
    m_dwGetExp = dwGetExp;

    if (m_dwGetExp > 0)
    {
        m_bExpEffect = true;
        m_dwExpEffectTime = timeGetTime();
    }
}

void SEASON3B::CNewUIMainFrameWindow::SetBtnState(int iBtnType, bool bStateDown)
{
    if (iBtnType < 0 || iBtnType >= MAINFRAME_BTN_COUNT)
        return;

    m_bButtonActive[iBtnType] = bStateDown;
}
