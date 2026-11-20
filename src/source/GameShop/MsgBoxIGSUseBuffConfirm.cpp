// MsgBoxIGSUseBuffConfirm.cpp: implementation of the CMsgBoxIGSUseBuffConfirm class.
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "I18N/All.h"

#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
#include "MsgBoxIGSUseBuffConfirm.h"
#include "CyberShopStyle.h"

#include "Audio/DSPlaySound.h"

CMsgBoxIGSUseBuffConfirm::CMsgBoxIGSUseBuffConfirm()
{
    m_iMiddleCount = 5;

    for (int i = 0; i < UIMAX_TEXT_LINE; i++)
    {
        m_szDescription[i][0] = '\0';
    }

    m_iStorageSeq = 0;
    m_iStorageItemSeq = 0;
    m_wItemCode = -1;
    m_szItemType = '\0';
}

CMsgBoxIGSUseBuffConfirm::~CMsgBoxIGSUseBuffConfirm()
{
    Release();
}

bool CMsgBoxIGSUseBuffConfirm::Create(float fPriority)
{
    SetAddCallbackFunc();

    CNewUIMessageBoxBase::Create((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2), (IMAGE_IGS_WINDOW_HEIGHT / 2) - (IMAGE_IGS_FRAME_HEIGHT / 2), IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_FRAME_HEIGHT, fPriority);

    SetButtonInfo();

    SetMsgBackOpacity();

    return true;
}

void CMsgBoxIGSUseBuffConfirm::Initialize(int iStorageSeq, int iStorageItemSeq, WORD wItemCode, wchar_t szItemType, wchar_t* pszItemName, wchar_t* pszBuffName)
{
    wchar_t szText[256] = { 0, };

    m_iStorageSeq = iStorageSeq;
    m_iStorageItemSeq = iStorageItemSeq;
    m_wItemCode = wItemCode;
    m_szItemType = szItemType;
    wcscpy(m_szCurrentBuffName, pszBuffName);

    mu_swprintf(szText, I18N::Game::UsingTheSItemWillNegate, pszItemName, pszBuffName, pszItemName);
    m_iDesciptionLine = ::DivideStringByPixel(&m_szDescription[0][0], UIMAX_TEXT_LINE, MAX_TEXT_LENGTH, szText, IGS_TEXT_DIVIDE_WIDTH, false, '#');
}

void CMsgBoxIGSUseBuffConfirm::Release()
{
    CNewUIMessageBoxBase::Release();
}

bool CMsgBoxIGSUseBuffConfirm::Update()
{
    m_BtnOk.Update();
    m_BtnCancel.Update();

    return true;
}

bool CMsgBoxIGSUseBuffConfirm::Render()
{
    EnableAlphaTest();
    RenderMsgBackColor(true);
    RenderFrame();
    RenderTexts();
    RenderButtons();
    DisableAlphaBlend();
    return true;
}

CALLBACK_RESULT CMsgBoxIGSUseBuffConfirm::LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSUseBuffConfirm*>(pOwner);

    if (pOwnMsgBox)
    {
        if (pOwnMsgBox->m_BtnOk.IsMouseIn() == true)
        {
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_USER_COMMON_OK);
            return CALLBACK_BREAK;
        }

        if (pOwnMsgBox->m_BtnCancel.IsMouseIn() == true)
        {
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_USER_COMMON_CANCEL);
            return CALLBACK_BREAK;
        }
    }

    return CALLBACK_CONTINUE;
}

CALLBACK_RESULT CMsgBoxIGSUseBuffConfirm::OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSUseBuffConfirm*>(pOwner);

    SocketClient->ToGameServer()->SendCashShopStorageItemConsumeRequest(pOwnMsgBox->m_iStorageSeq, pOwnMsgBox->m_iStorageItemSeq, pOwnMsgBox->m_wItemCode, pOwnMsgBox->m_szItemType);
    SocketClient->ToGameServer()->SendCashShopPointInfoRequest();

    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

    return CALLBACK_BREAK;
}

CALLBACK_RESULT CMsgBoxIGSUseBuffConfirm::CancelButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

    return CALLBACK_BREAK;
}

void CMsgBoxIGSUseBuffConfirm::SetAddCallbackFunc()
{
    AddCallbackFunc(CMsgBoxIGSUseBuffConfirm::LButtonUp, MSGBOX_EVENT_MOUSE_LBUTTON_UP);
    AddCallbackFunc(CMsgBoxIGSUseBuffConfirm::OKButtonDown, MSGBOX_EVENT_USER_COMMON_OK);
    AddCallbackFunc(CMsgBoxIGSUseBuffConfirm::CancelButtonDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
}

void CMsgBoxIGSUseBuffConfirm::SetButtonInfo()
{
    m_BtnOk.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_OK_POS_X, GetPos().y + IGS_BTN_POS_Y, IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT, CNewUIMessageBoxButton::MSGBOX_BTN_CUSTOM, true);
    m_BtnOk.MoveTextPos(0, -1);
    m_BtnOk.SetText(I18N::Game::OK);

    m_BtnCancel.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_CANCEL_POS_X, GetPos().y + IGS_BTN_POS_Y, IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT, CNewUIMessageBoxButton::MSGBOX_BTN_CUSTOM, true);
    m_BtnCancel.MoveTextPos(0, -1);
    m_BtnCancel.SetText(I18N::Game::Cancel);
}

void CMsgBoxIGSUseBuffConfirm::RenderFrame()
{
    namespace Style = GameShop::CyberStyle;
    const float x = static_cast<float>(GetPos().x);
    const float y = static_cast<float>(GetPos().y);
    Style::DrawMessageBox(x, y, IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_FRAME_HEIGHT, I18N::Game::BuffItemUseConfirmation);
}

void CMsgBoxIGSUseBuffConfirm::RenderTexts()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(241, 233, 238, 255);
    g_pRenderText->SetFont(g_hFontBold);

    g_pRenderText->SetFont(g_hFont);

    // Decription
    for (int i = 0; i < m_iDesciptionLine; ++i)
    {
        g_pRenderText->RenderText(GetPos().x + IGS_TEXT_DESCRIPTION_POS_X, GetPos().y + IGS_TEXT_DESCRIPTION_POS_Y + (i * IGS_TEXT_DESCRIPTION_INTERVAL), m_szDescription[i], IGS_TEXT_DESCRIPTION_WIDTH, 0, RT3_SORT_LEFT);
    }
}

void CMsgBoxIGSUseBuffConfirm::RenderButtons()
{
    GameShop::CyberStyle::RenderMessageBoxButton(m_BtnOk, true);
    GameShop::CyberStyle::RenderMessageBoxButton(m_BtnCancel, false);
}

bool CMsgBoxIGSUseBuffConfirmLayout::SetLayout()
{
    CMsgBoxIGSUseBuffConfirm* pMsgBox = GetMsgBox();
    if (pMsgBox == nullptr)
        return false;

    if (false == pMsgBox->Create())
        return false;

    return true;
}

#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
