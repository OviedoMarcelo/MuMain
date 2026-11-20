// MsgBoxIGSCommon.cpp: implementation of the CMsgBoxIGSCommon class.
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "I18N/All.h"

#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM

#include "MsgBoxIGSCommon.h"
#include "CyberShopStyle.h"

#include "Audio/DSPlaySound.h"

CMsgBoxIGSCommon::CMsgBoxIGSCommon()
{
    memset(m_szTitle, 0, sizeof(m_szTitle));
    memset(m_szText, 0, sizeof(m_szText));
    m_iMiddleCount = 0;

    m_iMsgBoxWidth = IMAGE_IGS_FRAME_WIDTH;
    m_iMsgBoxHeight = IMAGE_IGS_FRAME_HEIGHT;
}

CMsgBoxIGSCommon::~CMsgBoxIGSCommon()
{
    Release();
}

bool CMsgBoxIGSCommon::Create(float fPriority)
{
    SetAddCallbackFunc();

    CNewUIMessageBoxBase::Create((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2),
        (IMAGE_IGS_WINDOW_HEIGHT / 2) - (IMAGE_IGS_FRAME_HEIGHT / 2),
        m_iMsgBoxWidth, m_iMsgBoxHeight, fPriority);

    SetButtonInfo();

    SetMsgBackOpacity();

    return true;
}

void CMsgBoxIGSCommon::Initialize(const wchar_t* pszTitle, const wchar_t* pszText)
{
    wcscpy(m_szTitle, pszTitle);

    m_iNumTextLine = ::DivideStringByPixel(&m_szText[0][0], NUM_LINE_CMB, MAX_TEXT_LENGTH, pszText, IGS_TEXT_ITEM_INFO_WIDTH, true, '#');

    if (m_iNumTextLine > IGS_NUM_TEXT_LIMIT_RENDER_MIDDLE_LINE)
    {
        m_iMiddleCount = m_iNumTextLine - IGS_NUM_TEXT_LIMIT_RENDER_MIDDLE_LINE;
    }

    m_iMsgBoxWidth = IMAGE_IGS_FRAME_WIDTH;
    m_iMsgBoxHeight = IMAGE_IGS_FRAME_HEIGHT + (m_iMiddleCount * IMAGE_IGS_LINE_HEIGHT);

    CNewUIMessageBoxBase::SetSize(m_iMsgBoxWidth, m_iMsgBoxHeight);
    SetButtonInfo();
}

void CMsgBoxIGSCommon::Release()
{
    CNewUIMessageBoxBase::Release();
}

bool CMsgBoxIGSCommon::Update()
{
    m_BtnOk.Update();

    return true;
}

bool CMsgBoxIGSCommon::Render()
{
    EnableAlphaTest();
    RenderMsgBackColor(true);
    RenderFrame();
    RenderTexts();
    RenderButtons();
    DisableAlphaBlend();

    return true;
}

CALLBACK_RESULT CMsgBoxIGSCommon::LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSCommon*>(pOwner);

    if (pOwnMsgBox)
    {
        if (pOwnMsgBox->m_BtnOk.IsMouseIn() == true)
        {
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_USER_COMMON_OK);
            return CALLBACK_BREAK;
        }
    }

    return CALLBACK_CONTINUE;
}

CALLBACK_RESULT CMsgBoxIGSCommon::OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

    return CALLBACK_BREAK;
}

void CMsgBoxIGSCommon::SetAddCallbackFunc()
{
    AddCallbackFunc(CMsgBoxIGSCommon::LButtonUp, MSGBOX_EVENT_MOUSE_LBUTTON_UP);
    AddCallbackFunc(CMsgBoxIGSCommon::OKButtonDown, MSGBOX_EVENT_USER_COMMON_OK);
}

void CMsgBoxIGSCommon::SetButtonInfo()
{
    m_BtnOk.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + (IMAGE_IGS_FRAME_WIDTH / 2) - (IMAGE_IGS_BTN_WIDTH / 2), (GetPos().y + m_iMsgBoxHeight) - (static_cast<int>(IMAGE_IGS_BTN_HEIGHT) + IGS_BTN_POS_Y), IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT, CNewUIMessageBoxButton::MSGBOX_BTN_CUSTOM, true);
    m_BtnOk.MoveTextPos(0, -1);
    m_BtnOk.SetText(I18N::Game::OK);
}

void CMsgBoxIGSCommon::RenderFrame()
{
    namespace Style = GameShop::CyberStyle;
    const float x = static_cast<float>(GetPos().x);
    const float y = static_cast<float>(GetPos().y);
    Style::DrawMessageBox(x, y, static_cast<float>(m_iMsgBoxWidth), static_cast<float>(m_iMsgBoxHeight), m_szTitle);
}

void CMsgBoxIGSCommon::RenderTexts()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(241, 233, 238, 255);
    g_pRenderText->SetFont(g_hFontBold);

    g_pRenderText->SetFont(g_hFont);

    // Text
    int iY = IGS_TEXT_ITEM_INFO_POS_Y;
    if (m_iNumTextLine <= IGS_NUM_TEXT_LIMIT_RENDER_MIDDLE_LINE)
    {
        iY = iY + ((IGS_NUM_TEXT_LIMIT_RENDER_MIDDLE_LINE - m_iNumTextLine) * 5);
    }

    for (int j = 0; j < m_iNumTextLine; ++j)
    {
        g_pRenderText->RenderText(GetPos().x + IGS_TEXT_ITEM_INFO_POS_X, GetPos().y + iY + j * 12, m_szText[j], IGS_TEXT_ITEM_INFO_WIDTH, 0, RT3_SORT_CENTER);
    }
}

void CMsgBoxIGSCommon::RenderButtons()
{
    GameShop::CyberStyle::RenderMessageBoxButton(m_BtnOk, true);
}

bool CMsgBoxIGSCommonLayout::SetLayout()
{
    CMsgBoxIGSCommon* pMsgBox = GetMsgBox();
    if (pMsgBox == nullptr)
        return false;

    if (false == pMsgBox->Create())
        return false;

    return true;
}

#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
