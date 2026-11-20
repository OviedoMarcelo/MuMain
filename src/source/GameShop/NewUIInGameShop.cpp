// NewUIInGameShop.cpp: implementation of the NewUIInGameShop class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#ifdef PBG_ADD_INGAMESHOP_UI_ITEMSHOP
#include "App/Platform/Windows/iexplorer.h"
#include "UI/NewUI/NewUISystem.h"
#include "NewUIInGameShop.h"
#include "MsgBoxIGSBuyPackageItem.h"
#include "MsgBoxIGSBuySelectItem.h"
#include "MsgBoxIGSCommon.h"
#include "MsgBoxIGSStorageItemInfo.h"
#include "MsgBoxIGSGiftStorageItemInfo.h"
#include "World/MapInfra/MapManager.h"
#include "Audio/DSPlaySound.h"
#include "Camera/CameraProjection.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/Quests/QuestTrackerShapes.h"
#include "CyberShopStyle.h"

#include <cmath>

using namespace SEASON3B;

CNewUIInGameShop::CNewUIInGameShop()
{
    Init();
}

CNewUIInGameShop::~CNewUIInGameShop()
{
    Release();
}

void CNewUIInGameShop::Init()
{
    m_ItemAngle = false;
    m_bLoadBanner = false;
    m_bBannerLink = false;
    m_iStorageTotalItemCnt = 0;
    m_iStorageCurrentPageItemCnt = 0;
    m_iStorageTotalPage = 0;
    m_iStorageCurrentPage = 0;
    m_iSelectedStorageItemIndex = 0;
    m_iStorageCurrentPageReceiveItemCnt = 0;
    m_bRequestCurrentPage = false;
    m_iZoneTabWidth = IMAGE_IGS_ZONE_BTN_WIDTH;
}

void CNewUIInGameShop::Release()
{
    ReleaseBanner();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }

    ClearAllStorageItem();
}

bool CNewUIInGameShop::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == NULL)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_INGAMESHOP, this);

    SetPos(x, y);
    SetBtnInfo();
    Show(false);	//visible()을 flase로

    return true;
}

void CNewUIInGameShop::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

namespace
{
namespace Shapes = UI::Quests::TrackerShapes;

using namespace GameShop::CyberStyle;

constexpr float HeaderLineY = 28.0f;

// The item cards, in a grid of 3 x 3 around the buy buttons and the 3D items.
constexpr int CardLeft = 101;
constexpr int CardTop = 35;
constexpr int CardWidth = 112;
constexpr int CardHeight = 118;
constexpr float CardCorner = 6.0f;
// The same grid as the buy buttons and the 3D items (IGS_NUM_ITEMS_WIDTH, IMAGE_IGS_VIEWDETAIL_BTN_DISTANCE_X/Y).
constexpr int CardsPerRow = 3;
constexpr int CardDistanceX = 122;
constexpr int CardDistanceY = 121;
constexpr float EmptySlotRadius = 12.0f;

// The column at the right: the character and the Cyber Coins, the link to the website and the storage.
constexpr int AccountLeft = 484;
constexpr int AccountTop = 34;
constexpr int AccountWidth = 148;
constexpr int AccountHeight = 86;
constexpr int PromoTop = 128;
constexpr int PromoHeight = 64;
constexpr int StorageTop = 224;
constexpr int StorageHeight = 140;

constexpr const wchar_t* WebsiteUrl = L"https://mucyber.tech";
constexpr const wchar_t* WebsiteName = L"mucyber.tech";

bool IsMouseIn(int x, int y, int width, int height)
{
    return SEASON3B::CheckMouseIn(x, y, width, height);
}

// A tab or a category: gold when it's selected, lighter when the mouse is over it.
void DrawSelectable(float x, float y, float width, float height, bool isSelected, bool isHovered)
{
    const unsigned int fill = isSelected ? ActiveFill : (isHovered ? TabHoverFill : TabFill);
    Shapes::DrawPanel(x, y, width, height, SmallCorner, fill, isSelected ? ActiveBorder : TabBorder);
}

// An arrow head which points to the left or to the right.
void DrawSideChevron(float centerX, float centerY, float size, bool pointsLeft, unsigned int color)
{
    const float tipX = pointsLeft ? centerX - size / 2 : centerX + size / 2;
    const float endX = pointsLeft ? centerX + size / 2 : centerX - size / 2;
    RenderColorLineARGB(endX, centerY - size, tipX, centerY, 1.5f, color);
    RenderColorLineARGB(tipX, centerY, endX, centerY + size, 1.5f, color);
}

// The refresh icon: a circle which is open at the top right, with an arrow head at its end.
void DrawRefreshIcon(float centerX, float centerY, float radius, unsigned int color)
{
    constexpr int Segments = 10;
    constexpr float StartAngle = 0.35f;
    constexpr float EndAngle = 5.6f;
    float lastX = centerX + radius * cosf(StartAngle);
    float lastY = centerY - radius * sinf(StartAngle);
    for (int segment = 1; segment <= Segments; ++segment)
    {
        const float angle = StartAngle + (EndAngle - StartAngle) * segment / Segments;
        const float x = centerX + radius * cosf(angle);
        const float y = centerY - radius * sinf(angle);
        RenderColorLineARGB(lastX, lastY, x, y, 1.5f, color);
        lastX = x;
        lastY = y;
    }

    const float tipX = centerX + radius * cosf(StartAngle);
    const float tipY = centerY - radius * sinf(StartAngle);
    RenderColorLineARGB(tipX, tipY, tipX - 3.0f, tipY - 1.0f, 1.5f, color);
    RenderColorLineARGB(tipX, tipY, tipX + 1.0f, tipY - 3.5f, 1.5f, color);
}

void DrawCross(float x, float y, float size, unsigned int color)
{
    RenderColorLineARGB(x, y, x + size, y + size, 2.0f, color);
    RenderColorLineARGB(x + size, y, x, y + size, 2.0f, color);
}

int CardX(int index)
{
    return CardLeft + (index % CardsPerRow) * CardDistanceX;
}

int CardY(int index)
{
    return CardTop + (index / CardsPerRow) * CardDistanceY;
}
} // namespace

bool CNewUIInGameShop::Render()
{
    EnableAlphaTest();
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    RenderFrame();
    RenderZoneTabs();
    RenderCategories();
    RenderItemCards();
    RenderAccountPanel();
    RenderStorage();
    RenderPagers();
    RenderTexts();
    EndRenderColor();

    RenderListBox();
    RenderDisplayItems();

    g_pRenderText->SetFont(g_hFont);
    DisableAlphaBlend();
    return true;
}

void CNewUIInGameShop::RenderFrame()
{
    const float left = static_cast<float>(m_Pos.x);
    const float top = static_cast<float>(m_Pos.y);
    const float right = left + IMAGE_IGS_BACK_WIDTH;
    const float bottom = top + IMAGE_IGS_BACK_HEIGHT;

    Shapes::DrawPanel(left, top, IMAGE_IGS_BACK_WIDTH, IMAGE_IGS_BACK_HEIGHT, WindowCorner, WindowFill, WindowBorder);

    // The faint grid of the website.
    for (float x = left + GridStep; x < right; x += GridStep)
    {
        RenderColorLineARGB(x, top + 1, x, bottom - 1, 1.0f, GridLine);
    }
    for (float y = top + GridStep; y < bottom; y += GridStep)
    {
        RenderColorLineARGB(left + 1, y, right - 1, y, 1.0f, GridLine);
    }

    // The line under the title and the tabs, with a diamond at each end.
    const float lineLeft = left + 10;
    const float lineRight = right - 10;
    RenderColorLineARGB(lineLeft, top + HeaderLineY, lineRight, top + HeaderLineY, 1.0f, HeaderLine);
    Shapes::DrawDiamond(lineLeft, top + HeaderLineY, 3.0f, WindowBorder, WindowFill, WindowFill);
    Shapes::DrawDiamond(lineRight, top + HeaderLineY, 3.0f, WindowBorder, WindowFill, WindowFill);

    // The close button.
    const bool isCloseHovered = IsMouseIn(m_Pos.x + IMAGE_IGS_EXIT_BTN_POS_X, m_Pos.y + IMAGE_IGS_EXIT_BTN_POS_Y,
                                          IMAGE_IGS_EXIT_BTN_WIDTH, IMAGE_IGS_EXIT_BTN_HEIGHT);
    DrawCross(left + IMAGE_IGS_EXIT_BTN_POS_X + 5, top + IMAGE_IGS_EXIT_BTN_POS_Y + 5, 10.0f,
              isCloseHovered ? IconHoverColor : CloseColor);
}

void CNewUIInGameShop::RenderZoneTabs()
{
    const int zoneCount = g_InGameShopSystem->GetSizeZones();
    const int selected = m_ZoneButton.GetCurButtonIndex();
    auto name = g_InGameShopSystem->GetZoneName().begin();
    for (int i = 0; i < zoneCount; ++i, ++name)
    {
        const POINT pos = m_ZoneButton.GetPos(i);
        const int width = m_iZoneTabWidth;
        const bool isSelected = i == selected;
        const bool isHovered = IsMouseIn(pos.x, pos.y + 4, width, IMAGE_IGS_ZONE_BTN_HEIGHT - 4);
        DrawSelectable(static_cast<float>(pos.x), static_cast<float>(pos.y + 5), static_cast<float>(width),
                       static_cast<float>(IMAGE_IGS_ZONE_BTN_HEIGHT - 5), isSelected, isHovered);
        if (isSelected)
        {
            const float lineY = static_cast<float>(pos.y + IMAGE_IGS_ZONE_BTN_HEIGHT - 1);
            RenderColorLineARGB(static_cast<float>(pos.x + 4), lineY, static_cast<float>(pos.x + width - 4), lineY,
                                2.0f, Ember);
        }

        EndRenderColor();
        RenderTextAt(pos.x, pos.y + 9, name->c_str(), width, isSelected ? GoldLightText : MutedText, isSelected,
                     RT3_SORT_CENTER);
    }
}

void CNewUIInGameShop::RenderCategories()
{
    const int categoryCount = g_InGameShopSystem->GetSizeCategoriesAsSelectedZone();
    const int selected = m_CategoryButton.GetCurButtonIndex();
    auto name = g_InGameShopSystem->GetCategoryName().begin();
    for (int i = 0; i < categoryCount; ++i, ++name)
    {
        const POINT pos = m_CategoryButton.GetPos(i);
        const bool isSelected = i == selected;
        const bool isHovered = IsMouseIn(pos.x, pos.y, IMAGE_IGS_CATEGORY_BTN_WIDTH, IMAGE_IGS_CATEGORY_BTN_HEIGHT);
        const float x = static_cast<float>(pos.x);
        const float y = static_cast<float>(pos.y);
        DrawSelectable(x, y, IMAGE_IGS_CATEGORY_BTN_WIDTH, IMAGE_IGS_CATEGORY_BTN_HEIGHT, isSelected, isHovered);

        const float markerY = y + IMAGE_IGS_CATEGORY_BTN_HEIGHT / 2.0f;
        if (isSelected)
        {
            Shapes::DrawOpenQuestMarker(x + 9, markerY, 4.5f);
        }
        else
        {
            Shapes::DrawFoldedQuestMarker(x + 9, markerY, 3.5f);
        }

        EndRenderColor();
        RenderTextAt(pos.x + 17, pos.y + 8, name->c_str(), IMAGE_IGS_CATEGORY_BTN_WIDTH - 19,
                     isSelected ? GoldLightText : MutedText, isSelected, RT3_SORT_LEFT);
    }
}

void CNewUIInGameShop::RenderItemCards()
{
    const int packageCount = g_InGameShopSystem->GetSizePackageAsDisplayPackage();
    for (int i = 0; i < INGAMESHOP_DISPLAY_ITEMLIST_SIZE; ++i)
    {
        const int x = m_Pos.x + CardX(i);
        const int y = m_Pos.y + CardY(i);
        const bool hasPackage = i < packageCount;
        const bool isHovered = hasPackage && IsMouseIn(x, y, CardWidth, CardHeight);
        Shapes::DrawPanel(static_cast<float>(x), static_cast<float>(y), CardWidth, CardHeight, CardCorner, CardFill,
                          isHovered ? CardHoverBorder : CardBorder);

        if (!hasPackage)
        {
            Shapes::DrawDiamond(x + CardWidth / 2.0f, y + CardHeight / 2.0f, EmptySlotRadius, EmptySlotMark, CardFill,
                                CardFill);
            continue;
        }

        const POINT& buttonPos = m_ViewDetailButton[i].GetPos();
        const bool isButtonHovered =
            IsMouseIn(buttonPos.x, buttonPos.y, IMAGE_IGS_VIEWDETAIL_BTN_WIDTH, IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT);
        DrawActionButton(static_cast<float>(buttonPos.x - 6), static_cast<float>(buttonPos.y + 3),
                         IMAGE_IGS_VIEWDETAIL_BTN_WIDTH + 12, IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT - 6, isButtonHovered);

        EndRenderColor();
        RenderTextAt(buttonPos.x - 6, buttonPos.y + 8, I18N::Game::Buy1124, IMAGE_IGS_VIEWDETAIL_BTN_WIDTH + 12,
                     GoldLightText, true, RT3_SORT_CENTER);
    }
}

void CNewUIInGameShop::RenderAccountPanel()
{
    const int left = m_Pos.x + AccountLeft;
    const int top = m_Pos.y + AccountTop;
    Shapes::DrawPanel(static_cast<float>(left), static_cast<float>(top), AccountWidth, AccountHeight, SmallCorner,
                      PanelFill, PanelBorder);
    RenderColorLineARGB(static_cast<float>(left + 12), static_cast<float>(top + 22),
                        static_cast<float>(left + AccountWidth - 12), static_cast<float>(top + 22), 1.0f, Separator);
    DrawCoin(static_cast<float>(left + 16), static_cast<float>(top + 66), 6.0f);

    const int refreshX = m_Pos.x + IMAGE_IGS_REFRESH_BTN_POS_X;
    const int refreshY = m_Pos.y + IMAGE_IGS_ICON_BTN_POS_Y;
    const bool isRefreshHovered = IsMouseIn(refreshX, refreshY, IMAGE_IGS_ICON_BTN_WIDTH, IMAGE_IGS_ICON_BTN_HEIGHT);
    Shapes::DrawPanel(static_cast<float>(refreshX), static_cast<float>(refreshY), IMAGE_IGS_ICON_BTN_WIDTH,
                      IMAGE_IGS_ICON_BTN_HEIGHT, SmallCorner, IconFill, isRefreshHovered ? ActiveBorder : IconBorder);
    DrawRefreshIcon(refreshX + IMAGE_IGS_ICON_BTN_WIDTH / 2.0f, refreshY + IMAGE_IGS_ICON_BTN_HEIGHT / 2.0f, 5.5f,
                    isRefreshHovered ? IconHoverColor : IconColor);

    // Where to get Cyber Coins, in the place of Webzen's banner. A click opens the website.
    const int promoTop = m_Pos.y + PromoTop;
    const bool isPromoHovered = IsMouseIn(left, promoTop, AccountWidth, PromoHeight);
    Shapes::DrawPanel(static_cast<float>(left), static_cast<float>(promoTop), AccountWidth, PromoHeight, SmallCorner,
                      isPromoHovered ? TabHoverFill : TabFill, isPromoHovered ? ActiveBorder : TabBorder);
    DrawCoin(static_cast<float>(left + AccountWidth / 2), static_cast<float>(promoTop + 12), 4.0f);
}

void CNewUIInGameShop::RenderStorage()
{
    const int selected = m_ListBoxTabButton.GetCurButtonIndex();
    for (int i = 0; i < IGS_TOTAL_LISTBOX; ++i)
    {
        const POINT pos = m_ListBoxTabButton.GetPos(i);
        const bool isHovered = IsMouseIn(pos.x, pos.y, IMAGE_IGS_TAB_BTN_WIDTH, IMAGE_IGS_TAB_BTN_HEIGHT);
        DrawSelectable(static_cast<float>(pos.x), static_cast<float>(pos.y), IMAGE_IGS_TAB_BTN_WIDTH,
                       IMAGE_IGS_TAB_BTN_HEIGHT, i == selected, isHovered);
    }

    const int left = m_Pos.x + AccountLeft;
    const int top = m_Pos.y + StorageTop;
    Shapes::DrawPanel(static_cast<float>(left), static_cast<float>(top), AccountWidth, StorageHeight, SmallCorner,
                      ListFill, ListBorder);
    RenderColorLineARGB(static_cast<float>(left + 6), static_cast<float>(top + 20),
                        static_cast<float>(left + AccountWidth - 6), static_cast<float>(top + 20), 1.0f, Separator);

    const POINT& usePos = m_UseButton.GetPos();
    DrawActionButton(static_cast<float>(usePos.x), static_cast<float>(usePos.y + 2), IMAGE_IGS_VIEWDETAIL_BTN_WIDTH,
                     IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT - 4,
                     IsMouseIn(usePos.x, usePos.y, IMAGE_IGS_VIEWDETAIL_BTN_WIDTH, IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT));
}

void CNewUIInGameShop::RenderPagers()
{
    CNewUIButton* buttons[] = {&m_PrevButton, &m_NextButton, &m_StoragePrevButton, &m_StorageNextButton};
    for (int i = 0; i < 4; ++i)
    {
        const POINT& pos = buttons[i]->GetPos();
        const POINT& size = buttons[i]->GetSize();
        const bool isHovered = IsMouseIn(pos.x, pos.y, size.x, size.y);
        DrawSideChevron(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f, 5.0f, i % 2 == 0,
                        isHovered ? IconHoverColor : IconColor);
    }
}

void CNewUIInGameShop::RenderTexts()
{
    EndRenderColor();

    wchar_t szText[256] = { 0, };
    wchar_t szValue[256] = { 0, };

    RenderTextAt(m_Pos.x + 8, m_Pos.y + 10, I18N::Game::CyberShopTitle, IMAGE_IGS_ZONE_BTN_POS_X - 12, GoldText, true,
                 RT3_SORT_CENTER);

    // The name and the price of each item.
    for (int i = 0; i < g_InGameShopSystem->GetSizePackageAsDisplayPackage(); i++)
    {
        CShopPackage* pPackage = g_InGameShopSystem->GetDisplayPackage(i);
        const int x = m_Pos.x + CardX(i);
        const int y = m_Pos.y + CardY(i);
        RenderTextAt(x + 4, y + 6, pPackage->PackageProductName, CardWidth - 8, WhiteText, true, RT3_SORT_CENTER);

        ConvertGold(pPackage->Price, szValue);
        mu_swprintf(szText, L"%ls %ls", szValue, pPackage->PricUnitName);
        g_pRenderText->SetFont(g_hFont);
        const int priceWidth = MeasureText(szText) + 10;
        const int priceX = x + (CardWidth - priceWidth) / 2;
        DrawCoin(static_cast<float>(priceX + 3), static_cast<float>(y + 82), 3.5f);
        EndRenderColor();
        RenderTextAt(priceX + 10, y + 78, szText, priceWidth, GoldText, false, RT3_SORT_LEFT);
    }

    // The character and the Cyber Coins. MU Cyber has one currency, which the server sends as the "credit card
    // cash"; the prepaid cash and the Goblin Points don't exist, so they aren't shown.
    const int panelX = m_Pos.x + AccountLeft;
    const int panelY = m_Pos.y + AccountTop;
    RenderTextAt(panelX, panelY + 7, Hero->ID, AccountWidth, GoldText, true, RT3_SORT_CENTER);
    RenderTextAt(panelX + 10, panelY + 32, I18N::Game::CyberShopBalance, AccountWidth - 20, MutedText, false,
                 RT3_SORT_LEFT);
    ConvertGold(g_InGameShopSystem->GetCashCreditCard(), szValue);
    RenderTextAt(panelX + 28, panelY + 61, szValue, AccountWidth - 60, GoldLightText, true, RT3_SORT_LEFT);

    const int promoY = m_Pos.y + PromoTop;
    RenderTextAt(panelX, promoY + 23, I18N::Game::CyberShopGetCoins, AccountWidth, MutedText, false, RT3_SORT_CENTER);
    RenderTextAt(panelX, promoY + 38, WebsiteName, AccountWidth, EmberText, true, RT3_SORT_CENTER);

    // The storage: its tabs, the titles of the list, the use button and the pages.
    const int selectedTab = m_ListBoxTabButton.GetCurButtonIndex();
    const wchar_t* tabNames[IGS_TOTAL_LISTBOX] = {I18N::Game::Storage, I18N::Game::GiftInventory};
    for (int i = 0; i < IGS_TOTAL_LISTBOX; ++i)
    {
        const POINT pos = m_ListBoxTabButton.GetPos(i);
        RenderTextAt(pos.x, pos.y + 5, tabNames[i], IMAGE_IGS_TAB_BTN_WIDTH,
                     i == selectedTab ? GoldLightText : MutedText, i == selectedTab, RT3_SORT_CENTER);
    }

    const int storageY = m_Pos.y + StorageTop;
    RenderTextAt(panelX + 8, storageY + 6, I18N::Game::ItemName, 96, MutedText, false, RT3_SORT_LEFT);
    RenderTextAt(panelX + AccountWidth - 58, storageY + 6, I18N::Game::Duration, 50, MutedText, false,
                 RT3_SORT_RIGHT);

    const POINT& usePos = m_UseButton.GetPos();
    RenderTextAt(usePos.x, usePos.y + 8, I18N::Game::Use, IMAGE_IGS_VIEWDETAIL_BTN_WIDTH, GoldLightText, true,
                 RT3_SORT_CENTER);

    mu_swprintf(szText, L"%d / %d", g_InGameShopSystem->GetSelectPage(), g_InGameShopSystem->GetTotalPages());
    const POINT& prevPos = m_PrevButton.GetPos();
    const POINT& nextPos = m_NextButton.GetPos();
    RenderTextAt(prevPos.x + IMAGE_IGS_PAGE_BTN_WIDTH, prevPos.y + 6, szText,
                 nextPos.x - prevPos.x - IMAGE_IGS_PAGE_BTN_WIDTH, WhiteText, true, RT3_SORT_CENTER);

    mu_swprintf(szText, L"%d / %d", m_iStorageCurrentPage, m_iStorageTotalPage);
    const POINT& storagePrevPos = m_StoragePrevButton.GetPos();
    const POINT& storageNextPos = m_StorageNextButton.GetPos();
    RenderTextAt(storagePrevPos.x + IMGAE_IGS_STORAGE_PAGE_BTN_WIDTH, storagePrevPos.y + 6, szText,
                 storageNextPos.x - storagePrevPos.x - IMGAE_IGS_STORAGE_PAGE_BTN_WIDTH, WhiteText, true,
                 RT3_SORT_CENTER);

    g_pRenderText->SetFont(g_hFont);
}

void CNewUIInGameShop::RenderListBox()
{
    m_StorageItemListBox.Render();
}

bool CNewUIInGameShop::IsInGameShopRect(float _x, float _y)
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP))
        return false;

    RECT _TempRT;

    _TempRT.top = 0;
    _TempRT.bottom = IMAGE_IGS_BACK_HEIGHT;
    _TempRT.left = 0;
    _TempRT.right = IMAGE_IGS_BACK_WIDTH;

    if (_x >= _TempRT.left && _x < _TempRT.right && _y < _TempRT.bottom && _y >= _TempRT.top)
        return true;
    else
        return false;

    return false;
}

void CNewUIInGameShop::SetConvertInvenCoord(WORD _ItemType, float _Width, float _Height)
{
    ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[_ItemType];
    float _TempWidth = pItemAttr->Width * 20.0f;
    float _TempHeight = pItemAttr->Height * 20.0f;
    float _fCoodX = 0, _fCoodY = 0;

    if (_ItemType == ITEM_WING_OF_STORM)
    {
        _fCoodY = 5.0f;
    }

    else if (pItemAttr->Height >= 4)
    {
        _fCoodY = -10.0f;
    }

    m_fRePos.x = (_Width / 2) - (_TempWidth / 2) + _fCoodX;
    m_fRePos.y = (_Height / 2) - (_TempHeight / 2) + _fCoodY;
    m_fReSize.x = _TempWidth;
    m_fReSize.y = _TempHeight;
}
void CNewUIInGameShop::SetRateScale(int _ItemType)
{
    const float _fRate_Value = 0.703f;
    ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[_ItemType];

    if (_ItemType == ITEM_WING_OF_STORM)
    {
        m_fRate_Scale = _fRate_Value * 0.7f;
    }
    else if (_ItemType == ITEM_DIVINE_STAFF_OF_ARCHANGEL)
    {
        m_fRate_Scale = _fRate_Value * 0.7f;
    }
    else if (_ItemType >= ITEM_HELPER + 117 && _ItemType <= ITEM_HELPER + 120)
    {
        m_fRate_Scale = _fRate_Value * 1.6f;
    }
    else if (pItemAttr->Height >= 4)
    {
        m_fRate_Scale = _fRate_Value * 0.7f;
    }
    else
    {
        m_fRate_Scale = _fRate_Value;
    }
}

// DXP-07d increment 3's shadow-compare diagnostic validated RenderDisplayItems()'s proj/view closed
// form and post-pop restore across multiple soaks; DXP-08a deleted the diagnostic and the FFP
// matrix-stack calls it was validating (see RenderDisplayItems()'s own comments below). Its per-item
// loop only calls RenderItem3D(), which carries no GL model transform. The restore mirror runs
// BEFORE BeginBitmap() is called again (unlike CNewUI3DCamera::Render(), where it runs after) — a
// genuine save/restore of whatever context was active at entry, not a BeginBitmap()-delegated
// restore, hence its own pre-panel snapshot below.
static float s_PreShopProj[16];
static float s_PreShopView[16];

void CNewUIInGameShop::RenderDisplayItems()
{
    EndBitmap();

    mu::GetRenderer().SetMatrixMode(GL_PROJECTION);
    mu::GetRenderer().PushMatrix();
    mu::GetRenderer().LoadIdentity();
    SetRenderViewport(0, 0, WindowWidth, WindowHeight);
    gluPerspective2(2.0f, (float)(WindowWidth) / (float)(WindowHeight), RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);
    mu::GetRenderer().SetMatrixMode(GL_MODELVIEW);
    mu::GetRenderer().PushMatrix();
    mu::GetRenderer().LoadIdentity();
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    EnableDepthTest();
    EnableDepthMask();

    mu::GetRenderer().ClearDepthBuffer();

    for (int i = 0; i < g_InGameShopSystem->GetSizePackageAsDisplayPackage(); i++)
    {
        int iPosX = IGS_ITEMRENDER_POS_X_STANDAD + (IMAGE_IGS_VIEWDETAIL_BTN_DISTANCE_X * (i % IGS_NUM_ITEMS_WIDTH));
        int iPosY = IGS_ITEMRENDER_POS_Y_STANDAD + (IMAGE_IGS_VIEWDETAIL_BTN_DISTANCE_Y * (i / IGS_NUM_ITEMS_HEIGHT));
        RenderItem3D(iPosX, iPosY, IGS_ITEMRENDER_POS_WIDTH, IGS_ITEMRENDER_POS_HEIGHT, g_InGameShopSystem->GetPackageItemCode(i), 0, 0, 0, true);
    }

    UpdateMousePositionn();

    mu::GetRenderer().SetMatrixMode(GL_MODELVIEW);
    mu::GetRenderer().PopMatrix();
    mu::GetRenderer().SetMatrixMode(GL_PROJECTION);
    mu::GetRenderer().PopMatrix();

    BeginBitmap();
}

bool CNewUIInGameShop::BtnProcess()
{
    if (g_InGameShopSystem->IsRequestEventPackge() == true)
    {
        if (m_ZoneButton.UpdateMouseEvent() != -1)
        {
            g_InGameShopSystem->SelectZone(m_ZoneButton.GetCurButtonIndex());
            InitCategoryBtn();
            g_InGameShopSystem->SelectCategory(m_CategoryButton.GetCurButtonIndex());
            return true;
        }

        if (m_CategoryButton.UpdateMouseEvent() != -1)
        {
            g_InGameShopSystem->SelectCategory(m_CategoryButton.GetCurButtonIndex());
            return true;
        }
    }

    if (m_ListBoxTabButton.UpdateMouseEvent() != -1)
    {
        char szCode = GetCurrentStorageCode();
        m_iSelectedStorageItemIndex = 0;
        m_bRequestCurrentPage = true;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(1, szCode);
        return true;
    }

    for (int i = 0; i < g_InGameShopSystem->GetSizePackageAsDisplayPackage(); i++)
    {
        if (m_ViewDetailButton[i].UpdateMouseEvent())
        {
            CShopPackage* pPackage = g_InGameShopSystem->GetDisplayPackage(i);

            if (pPackage->PriceCount == 1)
            {
                CMsgBoxIGSBuyPackageItem* pMsgBox = NULL;
                CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxBuyPackageItemLayout), &pMsgBox);
                pMsgBox->Initialize(pPackage);
            }
            else if (pPackage->PriceCount > 1)
            {
                CMsgBoxIGSBuySelectItem* pMsgBox = NULL;
                CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSBuySelectItemLayout), &pMsgBox);
                pMsgBox->Initialize(pPackage);
            }

            return true;
        }
    }

    if (m_CashRefreshButton.UpdateMouseEvent() == true)
    {
        SocketClient->ToGameServer()->SendCashShopPointInfoRequest();

        return true;
    }

    if (m_UseButton.UpdateMouseEvent() == true)
    {
        if (m_StorageItemListBox.GetLineNum() <= 0)
        {
            CMsgBoxIGSCommon* pMsgBox = NULL;
            CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
            pMsgBox->Initialize(I18N::Game::Error, I18N::Game::ThereIsNoUsableItem);
            return true;
        }

        int iStorageIndex = m_ListBoxTabButton.GetCurButtonIndex();

        IGS_StorageItem* pSelectItem = m_StorageItemListBox.GetSelectedText();

        if (iStorageIndex == IGS_SAFEKEEPING_LISTBOX)					// 보관함
        {
            CMsgBoxIGSStorageItemInfo* pMsgBox = NULL;
            CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSStorageItemInfoLayout), &pMsgBox);
            pMsgBox->Initialize(pSelectItem->m_iStorageSeq, pSelectItem->m_iStorageItemSeq, pSelectItem->m_wItemCode, pSelectItem->m_szType,
                pSelectItem->m_szName, pSelectItem->m_szNum, pSelectItem->m_szPeriod);
        }
        else if (iStorageIndex == IGS_PRESENTBOX_LISTBOX)				// 선물 보관함
        {
            CMsgBoxIGSGiftStorageItemInfo* pMsgBox = NULL;
            CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSGiftStorageItemInfoLayout), &pMsgBox);
            pMsgBox->Initialize(pSelectItem->m_iStorageSeq, pSelectItem->m_iStorageItemSeq, pSelectItem->m_wItemCode,
                pSelectItem->m_szType, pSelectItem->m_szSendUserName, pSelectItem->m_szMessage,
                pSelectItem->m_szName, pSelectItem->m_szNum, pSelectItem->m_szPeriod);
        }
        return true;
    }

    // Prev Button
    if (m_PrevButton.UpdateMouseEvent())
    {
        g_InGameShopSystem->PrePage();
        return true;
    }

    // Next Button
    if (m_NextButton.UpdateMouseEvent())
    {
        g_InGameShopSystem->NextPage();
        return true;
    }

    // Storage Prev Button
    if (m_StoragePrevButton.UpdateMouseEvent())
    {
        StoragePrevPage();
        return true;
    }

    // Next Button
    if (m_StorageNextButton.UpdateMouseEvent())
    {
        StorageNextPage();
        return true;
    }

    if (m_CloseButton.UpdateMouseEvent() == true)
    {
        if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == true)
        {
            SocketClient->ToGameServer()->SendCashShopOpenState(1);
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);

            return true;
        }
        return false;
    }

    return false;
}

void CNewUIInGameShop::SetBtnInfo()
{
    m_CloseButton.ChangeButtonImgState(true, IMAGE_IGS_EXIT_BTN, false);
    m_CloseButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_EXIT_BTN_POS_X, m_Pos.y + IMAGE_IGS_EXIT_BTN_POS_Y, IMAGE_IGS_EXIT_BTN_WIDTH, IMAGE_IGS_EXIT_BTN_HEIGHT);
    m_CloseButton.ChangeToolTipText(&I18N::Game::Close388, true);
    m_ListBoxTabButton.CreateRadioGroup(IGS_TOTAL_LISTBOX, IMAGE_IGS_LEFT_TAB);
    m_ListBoxTabButton.ChangeRadioButtonInfo(true, m_Pos.x + IMAGE_IGS_TAB_BTN_POS_X, m_Pos.y + IMAGE_IGS_TAB_BTN_POS_Y, IMAGE_IGS_TAB_BTN_WIDTH, IMAGE_IGS_TAB_BTN_HEIGHT, IMAGE_IGS_TAB_BTN_DISTANCE);
    m_ListBoxTabButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 0);
    m_ListBoxTabButton.ChangeButtonState(IGS_SAFEKEEPING_LISTBOX, BITMAP_UNKNOWN, SEASON3B::BUTTON_STATE_UP, 0);
    m_ListBoxTabButton.ChangeButtonState(IGS_PRESENTBOX_LISTBOX, BITMAP_UNKNOWN, SEASON3B::BUTTON_STATE_UP, 0);
    m_ListBoxTabButton.ChangeButtonState(IGS_PRESENTBOX_LISTBOX, IMAGE_IGS_RIGHT_TAB, SEASON3B::BUTTON_STATE_DOWN, 0);

   std::wstring strText;
    std::list<std::wstring> TextList;
    strText = I18N::Game::Storage;
    TextList.push_back(strText);
    strText = I18N::Game::GiftInventory;
    TextList.push_back(strText);

    m_ListBoxTabButton.ChangeRadioText(TextList);
    m_ListBoxTabButton.ChangeFrame(IGS_SAFEKEEPING_LISTBOX);

    for (int i = 0; i < INGAMESHOP_DISPLAY_ITEMLIST_SIZE; i++)
    {
        m_ViewDetailButton[i].ChangeButtonImgState(true, IMAGE_IGS_VIEWDETAIL_BTN, true, false, true);
        m_ViewDetailButton[i].ChangeButtonInfo(IMAGE_IGS_VIEWDETAIL_BTN_POS_X + ((i % IGS_NUM_ITEMS_WIDTH) * IMAGE_IGS_VIEWDETAIL_BTN_DISTANCE_X), IMAGE_IGS_VIEWDETAIL_BTN_POS_Y + ((i / IGS_NUM_ITEMS_HEIGHT) * IMAGE_IGS_VIEWDETAIL_BTN_DISTANCE_Y), IMAGE_IGS_VIEWDETAIL_BTN_WIDTH, IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT);
        m_ViewDetailButton[i].MoveTextPos(0, -1);
        m_ViewDetailButton[i].ChangeText(&I18N::Game::Buy1124);
    }

    // Sending and recharging W Coins don't exist in MU Cyber: the Cyber Coins are bought on the website, which the
    // panel under the Cyber Coins links to.
    m_CashRefreshButton.ChangeButtonImgState(true, IMAGE_IGS_REFRESH_BTN, true);
    m_CashRefreshButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_REFRESH_BTN_POS_X, m_Pos.y + IMAGE_IGS_ICON_BTN_POS_Y, IMAGE_IGS_ICON_BTN_WIDTH, IMAGE_IGS_ICON_BTN_HEIGHT);
    m_CashRefreshButton.ChangeToolTipText(&I18N::Game::UpdateInformation);

    m_UseButton.ChangeButtonImgState(true, IMAGE_IGS_VIEWDETAIL_BTN, true, false, true);
    m_UseButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_USE_BTN_POS_X, m_Pos.y + IMAGE_IGS_USE_BTN_POS_Y, IMAGE_IGS_VIEWDETAIL_BTN_WIDTH, IMAGE_IGS_VIEWDETAIL_BTN_HEIGHT);
    m_UseButton.MoveTextPos(0, -1);
    m_UseButton.ChangeText(&I18N::Game::Use);

    m_PrevButton.ChangeButtonImgState(true, IMAGE_IGS_PAGE_LEFT, true);
    m_PrevButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_PAGE_LEFT_POS_X, m_Pos.y + IMAGE_IGS_PAGE_BUTTON_POS_Y, IMAGE_IGS_PAGE_BTN_WIDTH, IMAGE_IGS_PAGE_BTN_HEIGHT);

    // next
    m_NextButton.ChangeButtonImgState(true, IMAGE_IGS_PAGE_RIGHT, true);
    m_NextButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_PAGE_RIGHT_POS_X, m_Pos.y + IMAGE_IGS_PAGE_BUTTON_POS_Y, IMAGE_IGS_PAGE_BTN_WIDTH, IMAGE_IGS_PAGE_BTN_HEIGHT);

    // Storage Page prev
    m_StoragePrevButton.ChangeButtonImgState(true, IMAGE_IGS_STORAGE_PAGE_LEFT, true);
    m_StoragePrevButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_STORAGE_PAGE_LEFT_POS_X - 12, m_Pos.y + IMAGE_IGS_STORAGE_PAGE_BTN_POS_Y - 3, IMGAE_IGS_STORAGE_PAGE_BTN_WIDTH, IMGAE_IGS_STORAGE_PAGE_BTN_HEIGHT);

    // Storage Page next
    m_StorageNextButton.ChangeButtonImgState(true, IMAGE_IGS_STORAGE_PAGE_RIGHT, true);
    m_StorageNextButton.ChangeButtonInfo(m_Pos.x + IMAGE_IGS_STORAGE_PAGE_RIGHT_POS_X + 10, m_Pos.y + IMAGE_IGS_STORAGE_PAGE_BTN_POS_Y - 3, IMGAE_IGS_STORAGE_PAGE_BTN_WIDTH, IMGAE_IGS_STORAGE_PAGE_BTN_HEIGHT);
}

bool CNewUIInGameShop::Update()
{
    if (IsVisible() == false)
        return true;

    return true;
}

bool CNewUIInGameShop::UpdateMouseEvent()
{
    if (IsVisible() == false)
        return true;

    if (BtnProcess())
        return false;

    if (UpdateBanner())
        return false;

    if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, IMAGE_IGS_BACK_WIDTH, IMAGE_IGS_BACK_HEIGHT))
    {
        m_StorageItemListBox.DoAction();

        if (SEASON3B::IsPress(VK_RBUTTON))
        {
            MouseRButton = false;
            MouseRButtonPop = false;
            MouseRButtonPush = false;
            return false;
        }

        if (SEASON3B::IsNone(VK_LBUTTON) == false)
        {
            return false;
        }

        return false;
    }

    return true;
}

bool CNewUIInGameShop::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == true)
    {
        if (SEASON3B::IsPress(VK_ESCAPE) == true)
        {
            SocketClient->ToGameServer()->SendCashShopOpenState(1);
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_INGAMESHOP);

            return false;
        }
    }
    return true;
}

bool CNewUIInGameShop::IsInGameShopOpen()
{
    g_ConsoleDebug->Write(MCD_NORMAL, L"InGameShopStatue.Txt CallStack - CNewUIInGameShop::IsInGameShopOpen()");
    if (Hero->Movement)
        return false;

    if (!(Hero->SafeZone) && !(WD_0LORENCIA == gMapManager.WorldActive && WD_3NORIA == gMapManager.WorldActive && WD_2DEVIAS == gMapManager.WorldActive && WD_51HOME_6TH_CHAR == gMapManager.WorldActive))
    {
        CMsgBoxIGSCommon* pMsgBox = NULL;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::YouCanOnlyOpenMUItemShopInATownOrSafeZone);
        g_ConsoleDebug->Write(MCD_NORMAL, L"InGameShopStatue.Txt Return - false <%ls>", I18N::Game::YouCanOnlyOpenMUItemShopInATownOrSafeZone);
        return false;
    }

    if (g_InGameShopSystem->IsShopOpen() == false)
    {
        CMsgBoxIGSCommon* pMsgBox = NULL;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::CannotOpenMUItemShopPleaseReconnectToTheGame);
        g_ConsoleDebug->Write(MCD_NORMAL, L"InGameShopStatue.Txt Return - false <%ls>", I18N::Game::CannotOpenMUItemShopPleaseReconnectToTheGame);
        return false;
    }
    g_ConsoleDebug->Write(MCD_NORMAL, L"InGameShopStatue.Txt Return - true");
    return true;
}

bool CNewUIInGameShop::IsInGameShop()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP))
        return true;
    else
        return false;
}

void CNewUIInGameShop::InitBanner(wchar_t* pszFileName, wchar_t* pszBannerURL)
{
    ReleaseBanner();

    if (pszFileName == NULL)
        return;

    if (pszBannerURL[0] != '#')
    {
        m_bBannerLink = true;
    }

    if (Bitmaps.Convert_Format(pszFileName) == false)
        return;

    if (LoadBitmap(pszFileName, IMAGE_IGS_BANNER, GL_LINEAR, GL_CLAMP, true, true) == true)
    {
        m_bLoadBanner = true;

        wcscpy(m_szBannerURL, pszBannerURL);
    }
}

// MU Cyber has no banners. In their place there is a panel which tells where to get Cyber Coins, and a click on it
// opens the website.
bool CNewUIInGameShop::UpdateBanner()
{
    if (SEASON3B::IsPress(VK_LBUTTON) && SEASON3B::CheckMouseIn(m_Pos.x + AccountLeft, m_Pos.y + PromoTop, AccountWidth, PromoHeight))
    {
        PlayBuffer(SOUND_CLICK01);
        leaf::OpenExplorer(WebsiteUrl);
        return true;
    }

    return false;
}

void CNewUIInGameShop::ReleaseBanner()
{
    if (m_bLoadBanner == false)
        return;

    DeleteBitmap(IMAGE_IGS_BANNER);

    m_bLoadBanner = false;
}

void CNewUIInGameShop::OpeningProcess()
{
    g_ConsoleDebug->Write(MCD_NORMAL, L"InGameShopStatue.Txt CallStack - CNewUIInGameShop::OpeningProcess()");
    PlayBuffer(SOUND_CLICK01);
    g_InGameShopSystem->Initalize();
    g_InGameShopSystem->SelectZone(0);
    InitZoneBtn();
    g_InGameShopSystem->SelectCategory(0);
    InitCategoryBtn();
    g_InGameShopSystem->SetRequestEventPackge();
}

void CNewUIInGameShop::ClosingProcess()
{
    PlayBuffer(SOUND_CLICK01);
    m_ListBoxTabButton.ChangeFrame(IGS_SAFEKEEPING_LISTBOX);
    ClearAllStorageItem();
}

void CNewUIInGameShop::InitZoneBtn()
{
    m_ZoneButton.UnRegisterRadioButton();

    if (g_InGameShopSystem->GetSizeZones() == 0)
        return;

    m_ZoneButton.UnRegisterRadioButton();
    m_ZoneButton.CreateRadioGroup(g_InGameShopSystem->GetSizeZones(), IMAGE_IGS_ZONE_BTN);

    // Up to 7 tabs fit with their full width. With more (MU Cyber has one per class plus the extras), they get
    // narrower, so that all fit between the title and the close button.
    const int zoneCount = g_InGameShopSystem->GetSizeZones();
    const int freeWidth = IMAGE_IGS_EXIT_BTN_POS_X - IMAGE_IGS_ZONE_BTN_POS_X - 4;
    const int zoneWidth = std::min<int>(IMAGE_IGS_ZONE_BTN_WIDTH, freeWidth / zoneCount - 1);
    m_iZoneTabWidth = zoneWidth;
    m_ZoneButton.ChangeRadioButtonInfo(true, m_Pos.x + IMAGE_IGS_ZONE_BTN_POS_X, m_Pos.y + IMAGE_IGS_ZONE_BTN_POS_Y, zoneWidth, IMAGE_IGS_ZONE_BTN_HEIGHT);
    m_ZoneButton.SetImageSize(IMAGE_IGS_ZONE_BTN_WIDTH, IMAGE_IGS_ZONE_BTN_HEIGHT);
    m_ZoneButton.SetFont(g_hFontBold);
    m_ZoneButton.ChangeRadioText(g_InGameShopSystem->GetZoneName());
    m_ZoneButton.ChangeFrame(0);
}

void CNewUIInGameShop::InitCategoryBtn()
{
    m_CategoryButton.UnRegisterRadioButton();

    if (g_InGameShopSystem->GetSizeCategoriesAsSelectedZone() == 0)
        return;

    m_CategoryButton.UnRegisterRadioButton();
    m_CategoryButton.CreateRadioGroup(g_InGameShopSystem->GetSizeCategoriesAsSelectedZone(), IMAGE_IGS_CATEGORY_BTN, true);
    m_CategoryButton.ChangeRadioButtonInfo(false, m_Pos.x + IMAGE_IGS_CATEGORY_BTN_POS_X, m_Pos.y + IMAGE_IGS_CATEGORY_BTN_POS_Y, IMAGE_IGS_CATEGORY_BTN_WIDTH, IMAGE_IGS_CATEGORY_BTN_HEIGHT, IMAGE_IGS_CATEGORY_BTN_DISTANCE);
    m_CategoryButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 2);
    m_CategoryButton.SetFont(g_hFontBold);
    m_CategoryButton.ChangeRadioText(g_InGameShopSystem->GetCategoryName());
    m_CategoryButton.ChangeFrame(0);
}

void CNewUIInGameShop::AddStorageItem(int iStorageSeq, int iStorageItemSeq, int iStorageGroupCode, int iProductSeq, int iPriceSeq, int iCashPoint, wchar_t chItemType, wchar_t* pszUserName /* = NULL */, wchar_t* pszMessage /* = NULL */)
{
    int iValue = -1;
    wchar_t szText[MAX_TEXT_LENGTH] = { '\0', };
    IGS_StorageItem Item;

    Item.m_bIsSelected = FALSE;
    Item.m_iStorageSeq = iStorageSeq;
    Item.m_iStorageItemSeq = iStorageItemSeq;
    Item.m_iStorageGroupCode = iStorageGroupCode;
    Item.m_iProductSeq = iProductSeq;
    Item.m_iPriceSeq = iPriceSeq;
    Item.m_iCashPoint = iCashPoint;
    Item.m_iNum = 1;
    Item.m_szType = chItemType;
    Item.m_wItemCode = -1;

    if (pszUserName == NULL)
    {
        Item.m_szSendUserName[0] = '\0';
    }
    else
    {
        wcscpy(Item.m_szSendUserName, pszUserName);
    }

    if (pszMessage == NULL)
    {
        Item.m_szMessage[0] = '\0';
    }
    else
    {
        wcscpy(Item.m_szMessage, pszMessage);
    }

    if (chItemType == 'C' || chItemType == 'c')
    {
        wchar_t szValue[MAX_TEXT_LENGTH] = { '\0', };
        ConvertGold(iCashPoint, szValue);
        // Name
        mu_swprintf(Item.m_szName, I18N::Game::WCoinSCoins, szValue);

        // Num
        mu_swprintf(Item.m_szNum, I18N::Game::SWCoin, szValue);
        Item.m_iNum = iCashPoint;

        // Period
        mu_swprintf(Item.m_szPeriod, L"-");
    }
    else if (chItemType == 'P' || chItemType == 'p')
    {
        if (iPriceSeq > 0)
        {
            // Name
            if (g_InGameShopSystem->GetProductInfoFromPriceSeq(iProductSeq, iPriceSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_ITEMNAME, iValue, Item.m_szName) == false)
            {
                mu_swprintf(Item.m_szName, L"aaa");
            }

            g_InGameShopSystem->GetProductInfoFromPriceSeq(iProductSeq, iPriceSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_NUM, iValue, szText);
            if (iValue > 0)
            {
                mu_swprintf(Item.m_szNum, L"%d %ls", iValue, szText);
                Item.m_iNum = iValue;
            }
            else
            {
                mu_swprintf(Item.m_szNum, L"-");
            }

            // Period
            g_InGameShopSystem->GetProductInfoFromPriceSeq(iProductSeq, iPriceSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_USE_LIMIT_PERIOD, iValue, szText);
            if (iValue > 0)
            {
                mu_swprintf(Item.m_szPeriod, L"%d %ls", iValue, szText);
            }
            else
            {
                mu_swprintf(Item.m_szPeriod, L"-");
            }

            g_InGameShopSystem->GetProductInfoFromPriceSeq(iProductSeq, iPriceSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_ITEMCODE, iValue, szText);
            Item.m_wItemCode = iValue;
        }
        else
        {
            if (g_InGameShopSystem->GetProductInfoFromProductSeq(iProductSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_ITEMNAME, iValue, Item.m_szName) == false)
                return;

            // Num
            g_InGameShopSystem->GetProductInfoFromProductSeq(iProductSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_NUM, iValue, szText);
            if (iValue > 0)
            {
                mu_swprintf(Item.m_szNum, L"%d %ls", iValue, szText);
                Item.m_iNum = iValue;
            }
            else
            {
                mu_swprintf(Item.m_szNum, L"-");
            }

            // Period
            g_InGameShopSystem->GetProductInfoFromProductSeq(iProductSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_USE_LIMIT_PERIOD, iValue, szText);
            if (iValue > 0)
            {
                mu_swprintf(Item.m_szPeriod, L"%d %ls", iValue, szText);
            }
            else
            {
                mu_swprintf(Item.m_szPeriod, L"-");
            }

            g_InGameShopSystem->GetProductInfoFromProductSeq(iProductSeq, CInGameShopSystem::IGS_PRODUCT_ATT_TYPE_ITEMCODE, iValue, szText);
            Item.m_wItemCode = iValue;
        }
    }
    else
    {
        return;
    }

    m_iStorageCurrentPageReceiveItemCnt++;

    m_StorageItemListBox.AddText(Item);

    if (m_iStorageCurrentPageReceiveItemCnt >= m_iStorageCurrentPageItemCnt)
    {
        if (m_iSelectedStorageItemIndex > m_iStorageCurrentPageItemCnt)
        {
            m_StorageItemListBox.SLSetSelectLine(m_iStorageCurrentPageItemCnt);
        }
        else
        {
            m_StorageItemListBox.SLSetSelectLine(m_iSelectedStorageItemIndex);
        }
    }
}

void CNewUIInGameShop::ClearAllStorageItem()
{
    m_iStorageTotalItemCnt = 0;
    m_iStorageCurrentPageItemCnt = 0;
    m_iStorageTotalPage = 0;
    m_iStorageCurrentPage = 0;
    m_iStorageCurrentPageReceiveItemCnt = 0;
    m_StorageItemListBox.Clear();
}

void CNewUIInGameShop::InitStorage(int iTotalItemCnt, int iCurrentPageItemCnt, int iTotalPage, int iCurrentPage)
{
    ClearAllStorageItem();

    m_iStorageTotalItemCnt = iTotalItemCnt;
    m_iStorageCurrentPageItemCnt = iCurrentPageItemCnt;
    m_iStorageTotalPage = iTotalPage;

    if (m_iStorageTotalPage > 0)
    {
        m_iStorageCurrentPage = iCurrentPage;
    }
    else
    {
        m_iStorageCurrentPage = 0;
    }

    if (m_iSelectedStorageItemIndex == 0 || m_bRequestCurrentPage == false)
    {
        m_iSelectedStorageItemIndex = iCurrentPageItemCnt;
    }

    m_bRequestCurrentPage = false;
}

char CNewUIInGameShop::GetCurrentStorageCode()
{
    char szCode;
    switch (m_ListBoxTabButton.GetCurButtonIndex())
    {
    case IGS_SAFEKEEPING_LISTBOX:
        szCode = 'S';
        break;
    case IGS_PRESENTBOX_LISTBOX:
        szCode = 'G';
        break;
    default:
        szCode = 'Z';
        break;
    }
    return szCode;
}

void CNewUIInGameShop::StoragePrevPage()
{
    if (m_iStorageCurrentPage > 1)
    {
        char szCode = GetCurrentStorageCode();
        m_iSelectedStorageItemIndex = 0;
        m_bRequestCurrentPage = true;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage - 1, szCode);
    }
}

void CNewUIInGameShop::StorageNextPage()
{
    if (m_iStorageCurrentPage < m_iStorageTotalPage)
    {
        char szCode = GetCurrentStorageCode();
        m_iSelectedStorageItemIndex = 0;
        m_bRequestCurrentPage = true;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage + 1, szCode);
    }
}

void CNewUIInGameShop::UpdateStorageItemList()
{
    char szCode = GetCurrentStorageCode();
    int iSelectLineIndex = m_StorageItemListBox.SLGetSelectLineNum();
    m_bRequestCurrentPage = true;

    if ((m_iStorageCurrentPageItemCnt == 1) && (m_iStorageTotalPage > 1))
    {
        m_iSelectedStorageItemIndex = 1;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage - 1, szCode);
    }
    else if (iSelectLineIndex == 1)
    {
        m_iSelectedStorageItemIndex = iSelectLineIndex;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage, szCode);
    }
    else if (m_iStorageCurrentPageItemCnt < IGS_STORAGE_TOTAL_ITEM_PER_PAGE)
    {
        m_iSelectedStorageItemIndex = (iSelectLineIndex - 1);
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage, szCode);
    }
    else
    {
        m_iSelectedStorageItemIndex = iSelectLineIndex;
        SocketClient->ToGameServer()->SendCashShopStorageListRequest(m_iStorageCurrentPage, szCode);
    }
}

#endif //PBG_ADD_INGAMESHOP_UI_ITEMSHOP
