// MsgBoxIGSDeleteItemConfirm.cpp: implementation of the CMsgBoxIGSDeleteItemConfirm class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM

#include "MsgBoxIGSDeleteItemConfirm.h"
#include "CyberShopStyle.h"


#include "Audio/DSPlaySound.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CMsgBoxIGSDeleteItemConfirm::CMsgBoxIGSDeleteItemConfirm()
{
    m_iMiddleCount = 4;

    m_iStorageSeq = 0;			// ������ ����
    m_iStorageItemSeq = 0;		// ������ ��ǰ ����
    m_szItemType = '\0';		// ��ǰ���� (C : ĳ��, P : ��ǰ)

    for (int i = 0; i < UIMAX_TEXT_LINE; i++)
    {
        m_szDescription[i][0] = '\0';
    }

    m_iDesciptionLine = 0;
}

CMsgBoxIGSDeleteItemConfirm::~CMsgBoxIGSDeleteItemConfirm()
{
    Release();
}

//--------------------------------------------
// Create
bool CMsgBoxIGSDeleteItemConfirm::Create(float fPriority)
{
    SetAddCallbackFunc();

    CNewUIMessageBoxBase::Create((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2),
        (IMAGE_IGS_WINDOW_HEIGHT / 2) - (IMAGE_IGS_FRAME_HEIGHT / 2),
        IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_FRAME_HEIGHT, fPriority);

    SetButtonInfo();

    SetMsgBackOpacity();

    return true;
}

//--------------------------------------------
// Initialize
void CMsgBoxIGSDeleteItemConfirm::Initialize(int iStorageSeq, int iStorageItemSeq, wchar_t szItemType)
{
    m_iStorageSeq = iStorageSeq;
    m_iStorageItemSeq = iStorageItemSeq;
    m_szItemType = szItemType;

    m_iDesciptionLine = ::DivideStringByPixel(&m_szDescription[0][0], UIMAX_TEXT_LINE, MAX_TEXT_LENGTH, I18N::Game::ThisWillDeleteTheSelectedItem, IGS_TEXT_DIVIDE_WIDTH, false, '#');
}

//--------------------------------------------
// Release
void CMsgBoxIGSDeleteItemConfirm::Release()
{
    CNewUIMessageBoxBase::Release();
}

//--------------------------------------------
// Update
bool CMsgBoxIGSDeleteItemConfirm::Update()
{
    m_BtnDelete.Update();
    m_BtnCancel.Update();

    return true;
}

//--------------------------------------------
// Render
bool CMsgBoxIGSDeleteItemConfirm::Render()
{
    EnableAlphaTest();

    RenderMsgBackColor(true);

    RenderFrame();
    RenderTexts();
    RenderButtons();
    DisableAlphaBlend();
    return true;
}

//--------------------------------------------
// LButtonUp
CALLBACK_RESULT CMsgBoxIGSDeleteItemConfirm::LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSDeleteItemConfirm*>(pOwner);

    if (pOwnMsgBox)
    {
        if (pOwnMsgBox->m_BtnDelete.IsMouseIn() == true)
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

//--------------------------------------------
// OKButtonDown
CALLBACK_RESULT CMsgBoxIGSDeleteItemConfirm::OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSDeleteItemConfirm*>(pOwner);

    SocketClient->ToGameServer()->SendCashShopDeleteStorageItemRequest(pOwnMsgBox->m_iStorageSeq, pOwnMsgBox->m_iStorageItemSeq, pOwnMsgBox->m_szItemType);

    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

    return CALLBACK_BREAK;
}

//--------------------------------------------
// CancelButtonDown
CALLBACK_RESULT CMsgBoxIGSDeleteItemConfirm::CancelButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

    return CALLBACK_BREAK;
}

//--------------------------------------------
// SetAddCallbackFunc
void CMsgBoxIGSDeleteItemConfirm::SetAddCallbackFunc()
{
    AddCallbackFunc(CMsgBoxIGSDeleteItemConfirm::LButtonUp, MSGBOX_EVENT_MOUSE_LBUTTON_UP);
    AddCallbackFunc(CMsgBoxIGSDeleteItemConfirm::OKButtonDown, MSGBOX_EVENT_USER_COMMON_OK);
    AddCallbackFunc(CMsgBoxIGSDeleteItemConfirm::CancelButtonDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
}

//--------------------------------------------
// SetButtonInfo
void CMsgBoxIGSDeleteItemConfirm::SetButtonInfo()
{
    // Ȯ�� ��ư
    m_BtnDelete.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_DEL_POS_X, GetPos().y + IGS_BTN_POS_Y,
        IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT);
    m_BtnDelete.SetText(I18N::Game::Delete);

    // ��� ��ư
    m_BtnCancel.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_CANCEL_POS_X, GetPos().y + IGS_BTN_POS_Y,
        IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT);
    m_BtnCancel.SetText(I18N::Game::Cancel);
}

//--------------------------------------------
// RenderFrame
void CMsgBoxIGSDeleteItemConfirm::RenderFrame()
{
    namespace Style = GameShop::CyberStyle;
    const float x = static_cast<float>(GetPos().x);
    const float y = static_cast<float>(GetPos().y);
    Style::DrawMessageBox(x, y, IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_FRAME_HEIGHT, I18N::Game::DeleteItem);
}

//--------------------------------------------
// RenderTexts
void CMsgBoxIGSDeleteItemConfirm::RenderTexts()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(241, 233, 238, 255);
    g_pRenderText->SetFont(g_hFontBold);

    g_pRenderText->SetFont(g_hFont);

    // Decription
    for (int i = 0; i < m_iDesciptionLine; ++i)
    {
        g_pRenderText->RenderText(GetPos().x + IGS_TEXT_DESCRIPTION_POS_X, GetPos().y + IGS_TEXT_DESCRIPTION_POS_Y + (i * IGS_TEXT_DESCRIPTION_INTERVAL),
            m_szDescription[i], IGS_TEXT_DESCRIPTION_WIDTH, 0, RT3_SORT_LEFT);
    }
}

//--------------------------------------------
// RenderButtons
void CMsgBoxIGSDeleteItemConfirm::RenderButtons()
{
    GameShop::CyberStyle::RenderMessageBoxButton(m_BtnDelete, true);
    GameShop::CyberStyle::RenderMessageBoxButton(m_BtnCancel, false);
}

////////////////////////////////////////////////////////////////////
// LayOut
////////////////////////////////////////////////////////////////////
bool CMsgBoxIGSDeleteItemConfirmLayout::SetLayout()
{
    CMsgBoxIGSDeleteItemConfirm* pMsgBox = GetMsgBox();
    if (pMsgBox == nullptr)
        return false;

    if (false == pMsgBox->Create())
        return false;

    return true;
}

#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
