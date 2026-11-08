// NewUIPartyListWindow.cpp: implementation of the CNewUIPartyInfo class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/Chat/Chat.h"

#include "UI/NewUI/Party/NewUIPartyListWindow.h"
#include "UI/NewUI/NewUISystem.h"

#include "Engine/Object/ZzzInventory.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Skills/SkillManager.h"
#include "Engine/Object/ZzzInterface.h"
#include "Camera/CameraProjection.h"
#include "Data/GameConfig/GameConfig.h"

#include <algorithm>

using namespace SEASON3B;

CNewUIPartyListWindow::CNewUIPartyListWindow()
{
    m_pNewUIMng = NULL;
    m_Pos.x = m_Pos.y = 0;
    m_bActive = false;
    m_iVal = 24;
    m_iLimitUserIDHeight[0] = 48;
    m_iLimitUserIDHeight[1] = 58;
    m_iSelectedCharacter = -1;
    m_bMovedByPlayer = false;
    m_bPressed = false;
    m_bDragging = false;
    m_PressPos.x = m_PressPos.y = 0;
    m_DragOffset.x = m_DragOffset.y = 0;

    for (int i = 0; i < MAX_PARTYS; i++)
    {
        m_iPartyListBGColor[i] = PARTY_LIST_BGCOLOR_DEFAULT;
        m_bPartyMemberoutofSight[i] = false;
    }
}

CNewUIPartyListWindow::~CNewUIPartyListWindow()
{
    Release();
}

bool CNewUIPartyListWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_PARTY_INFO_WINDOW, this);

    SetPos(x, y);

    const int savedX = GameConfig::GetInstance().GetPartyListX();
    const int savedY = GameConfig::GetInstance().GetPartyListY();
    if (savedX >= 0 && savedY >= 0)
    {
        m_bMovedByPlayer = true;
        SetPos((std::min)(savedX, REFERENCE_WIDTH - PARTY_LIST_WINDOW_WIDTH),
            (std::min)(savedY, REFERENCE_HEIGHT - PARTY_LIST_WINDOW_HEIGHT * MAX_PARTYS));
    }

    LoadImages();

    // Exit Party Button Initialize
    for (int i = 0; i < MAX_PARTYS; i++)
    {
        int iVal = i * m_iVal;
        m_BtnPartyExit[i].ChangeButtonImgState(true, IMAGE_PARTY_LIST_EXIT);
        m_BtnPartyExit[i].ChangeButtonInfo(m_Pos.x + 63, m_Pos.y + 3 + iVal, 11, 11);
    }

    Show(true);

    return true;
}

void CNewUIPartyListWindow::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUIPartyListWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;

    for (int i = 0; i < MAX_PARTYS; i++)
    {
        int iVal = i * m_iVal;
        m_BtnPartyExit[i].ChangeButtonInfo(m_Pos.x + 63, m_Pos.y + 3 + iVal, 11, 11);
    }
}

void CNewUIPartyListWindow::SetPos(int x)
{
    // A list which the player moved stays where it was put.
    if (m_bMovedByPlayer)
        return;

    SetPos(x - (PARTY_LIST_WINDOW_WIDTH + 2), m_Pos.y);
}

int CNewUIPartyListWindow::GetListHeight() const
{
    return (PARTY_LIST_WINDOW_HEIGHT * PartyNumber) + (4 * (PartyNumber - 1));
}

// Holding the left button on the list and moving the mouse drags it. A click without moving still
// selects the member, as before. The new place is kept in config.ini.
bool CNewUIPartyListWindow::UpdateDrag()
{
    constexpr int DragThreshold = 3;

    if (m_bPressed)
    {
        if (!MouseLButton)
        {
            m_bPressed = false;
            if (m_bDragging)
            {
                m_bDragging = false;
                GameConfig::GetInstance().SetPartyListPosition(m_Pos.x, m_Pos.y);
                GameConfig::GetInstance().Save();
                return true;
            }

            return false;
        }

        if (!m_bDragging && (std::abs(MouseX - m_PressPos.x) > DragThreshold || std::abs(MouseY - m_PressPos.y) > DragThreshold))
        {
            m_bDragging = true;
            m_bMovedByPlayer = true;
        }

        if (m_bDragging)
        {
            const int x = std::clamp<int>(MouseX - m_DragOffset.x, 0, REFERENCE_WIDTH - PARTY_LIST_WINDOW_WIDTH);
            const int y = std::clamp<int>(MouseY - m_DragOffset.y, 0, (std::max)(0, REFERENCE_HEIGHT - GetListHeight()));
            SetPos(x, y);
            return true;
        }

        return false;
    }

    if (MouseLButtonPush && CheckMouseIn(m_Pos.x, m_Pos.y, PARTY_LIST_WINDOW_WIDTH, GetListHeight()))
    {
        m_bPressed = true;
        m_PressPos.x = MouseX;
        m_PressPos.y = MouseY;
        m_DragOffset.x = MouseX - m_Pos.x;
        m_DragOffset.y = MouseY - m_Pos.y;
    }

    return false;
}

int CNewUIPartyListWindow::GetSelectedCharacter()
{
    if (m_iSelectedCharacter == -1)
        return -1;

    return Party[m_iSelectedCharacter].index;
}

void CNewUIPartyListWindow::SetListBGColor()
{
    for (int i = 0; i < PartyNumber; i++)
    {
        m_iPartyListBGColor[i] = PARTY_LIST_BGCOLOR_DEFAULT;

        if (Party[i].index == -1)
        {
            m_iPartyListBGColor[i] = PARTY_LIST_BGCOLOR_RED;
        }

        if (Party[i].index > -1)
        {
            m_iPartyListBGColor[i] = PARTY_LIST_BGCOLOR_GREEN;
        }
    }
}

bool CNewUIPartyListWindow::BtnProcess()
{
    m_iSelectedCharacter = -1;

    for (int i = 0; i < PartyNumber; i++)
    {
        int iVal = i * m_iVal;

        if (!wcscmp(Party[0].Name, Hero->ID) || !wcscmp(Party[i].Name, Hero->ID))
        {
            if (m_BtnPartyExit[i].UpdateMouseEvent())
            {
                g_pPartyInfoWindow->LeaveParty(i);
                return true;
            }
        }

        if (CheckMouseIn(m_Pos.x, m_Pos.y + iVal, PARTY_LIST_WINDOW_WIDTH, PARTY_LIST_WINDOW_HEIGHT))
        {
            m_iSelectedCharacter = i;

            if (SelectedCharacter == -1) {
                CHARACTER* c = &CharactersClient[Party[i].index];
                if (c && c != Hero) {
                    UI::Chat::CreateChat(c->ID, L"", c);
                }
            }

            if (SelectCharacterInPartyList(&Party[i]))
            {
                return true;
            }
        }
    }

    return false;
}

bool CNewUIPartyListWindow::UpdateMouseEvent()
{
    if (!m_bActive)
    {
        m_bPressed = m_bDragging = false;
        return true;
    }

    if (UpdateDrag())
        return false;

    if (true == BtnProcess())
        return false;

    if (PartyNumber > 0)
    {
        if (CheckMouseIn(m_Pos.x, m_Pos.y, PARTY_LIST_WINDOW_WIDTH, GetListHeight()))
        {
            return false;
        }
    }

    return true;
}

bool CNewUIPartyListWindow::UpdateKeyEvent()
{
    return true;
}

bool CNewUIPartyListWindow::Update()
{
    if (PartyNumber <= 0)
    {
        m_bActive = false;
        return true;
    }

    m_bActive = true;

    for (int i = 0; i < PartyNumber; i++)
    {
        Party[i].index = -2;
    }

    return true;
}

bool CNewUIPartyListWindow::Render()
{
    if (!m_bActive)
        return true;

    EnableAlphaTest();

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    for (int i = 0; i < PartyNumber; i++)
    {
        int iVal = i * m_iVal;

        RenderColorQuadARGB(float(m_Pos.x + 2), float(m_Pos.y + 2 + iVal), PARTY_LIST_WINDOW_WIDTH - 3,
            PARTY_LIST_WINDOW_HEIGHT - 6, 0xE6000000u);
        EnableAlphaTest();

        if (Party[i].index == -1)
        {
            RenderColorQuadARGB(m_Pos.x + 2, m_Pos.y + 2 + iVal, PARTY_LIST_WINDOW_WIDTH - 3,
                PARTY_LIST_WINDOW_HEIGHT - 6, 0x804D0000u);
            EnableAlphaTest();
        }
        else
        {
            if (Party[i].index >= 0 && Party[i].index < MAX_CHARACTERS_CLIENT)
            {
                CHARACTER* pChar = &CharactersClient[Party[i].index];
                OBJECT* pObj = &pChar->Object;

                if (g_isCharacterBuff(pObj, eBuff_Defense) == true)
                {
                    RenderColorQuadARGB(m_Pos.x + 2, m_Pos.y + 2 + iVal, PARTY_LIST_WINDOW_WIDTH - 3,
                        PARTY_LIST_WINDOW_HEIGHT - 6, 0x3333FF33u);
                    EnableAlphaTest();
                }
            }
            if (m_iSelectedCharacter != -1 && m_iSelectedCharacter == i)
            {
                RenderColorQuadARGB(m_Pos.x + 2, m_Pos.y + 2 + iVal, PARTY_LIST_WINDOW_WIDTH - 3,
                    PARTY_LIST_WINDOW_HEIGHT - 6, 0xB3666666u);
                EnableAlphaTest();
            }
        }

        RenderImage(IMAGE_PARTY_LIST_BACK, m_Pos.x, m_Pos.y + iVal, PARTY_LIST_WINDOW_WIDTH, PARTY_LIST_WINDOW_HEIGHT);

        if (i == 0)
        {
            if (Party[i].index == -1)
            {
                g_pRenderText->SetTextColor(RGBA(128, 75, 11, 255));
            }
            else
            {
                g_pRenderText->SetTextColor(RGBA(255, 148, 22, 255));
            }

            RenderImage(IMAGE_PARTY_LIST_FLAG, m_Pos.x + 53, m_Pos.y + 3, 9, 10);
            g_pRenderText->RenderText(m_Pos.x + 4, m_Pos.y + 4 + iVal, Party[i].Name, m_iLimitUserIDHeight[0], 0, RT3_SORT_LEFT);
        }
        else
        {
            if (Party[i].index == -1)
            {
                g_pRenderText->SetTextColor(RGBA(128, 128, 128, 255));
            }
            else
            {
                g_pRenderText->SetTextColor(RGBA(255, 255, 255, 255));
            }
            g_pRenderText->RenderText(m_Pos.x + 4, m_Pos.y + 4 + iVal, Party[i].Name, m_iLimitUserIDHeight[1], 0, RT3_SORT_LEFT);
        }

        int iStepHP = std::min<int>(10, Party[i].stepHP);
        float fLife = ((float)iStepHP / (float)10) * (float)PARTY_LIST_HP_BAR_WIDTH;
        RenderImage(IMAGE_PARTY_LIST_HPBAR, m_Pos.x + 4, m_Pos.y + 16 + iVal, fLife, 3);

        if (!wcscmp(Party[0].Name, Hero->ID) || !wcscmp(Party[i].Name, Hero->ID))
        {
            m_BtnPartyExit[i].Render();
        }
    }

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIPartyListWindow::RenderPartyHPOnHead()
{
    if (PartyNumber <= 0)
        return;

    float   Width = 38.f;
    wchar_t    Text[100];

    for (int j = 0; j < PartyNumber; ++j)
    {
        PARTY_t* p = &Party[j];

        if (p->index <= -1) continue;

        CHARACTER* c = &CharactersClient[p->index];
        OBJECT* o = &c->Object;
        vec3_t      Position;
        int         ScreenX, ScreenY;

        Vector(o->Position[0], o->Position[1], o->Position[2] + o->BoundingBoxMax[2] + 100.f, Position);

        BeginOpengl();
        CameraProjection::WorldToScreen(g_Camera, Position, &ScreenX, &ScreenY);
        EndOpengl();

        ScreenX -= (int)(Width / 2);

        if ((MouseX >= ScreenX && MouseX < ScreenX + Width && MouseY >= ScreenY - 2 && MouseY < ScreenY + 6))
        {
            mu_swprintf(Text, L"HP : %d0%%", p->stepHP);
            g_pRenderText->SetTextColor(255, 230, 210, 255);
            g_pRenderText->RenderText(ScreenX, ScreenY - 6, Text);
        }

        EnableAlphaTest();
        RenderColorQuadARGB((float)(ScreenX + 1), (float)(ScreenY + 1), Width + 4.f, 5.f, 0x80000000u);

        EnableAlphaBlend();
        RenderColorQuadARGB((float)ScreenX, (float)ScreenY, Width + 4.f, 5.f, 0xFF330000u);
        RenderColorQuadARGB((float)(ScreenX + 2), (float)(ScreenY + 2), Width, 1.f, 0xFF320A00u);

        int stepHP = std::min<int>(10, p->stepHP);

        for (int k = 0; k < stepHP; ++k)
        {
            RenderColorQuadARGB((float)(ScreenX + 2 + (k * 4)), (float)(ScreenY + 2), 3.f, 2.f,
                0xFFFA0A00u);
        }
        DisableAlphaBlend();
    }
    DisableAlphaBlend();
}

float CNewUIPartyListWindow::GetLayerDepth()
{
    return 5.4f;
}

void CNewUIPartyListWindow::OpenningProcess()
{
}

void CNewUIPartyListWindow::ClosingProcess()
{
}

bool CNewUIPartyListWindow::SelectCharacterInPartyList(PARTY_t* pMember)
{
    auto HeroClass = gCharacterManager.GetBaseClass(Hero->Class);

    if (HeroClass == CLASS_ELF
        || HeroClass == CLASS_WIZARD
        || HeroClass == CLASS_SUMMONER
        )
    {
        auto Skill = CharacterAttribute->Skill[Hero->CurrentSkill];

        if (Skill == AT_SKILL_HEALING
            || Skill == AT_SKILL_HEALING_STR
            || Skill == AT_SKILL_DEFENSE
            || Skill == AT_SKILL_DEFENSE_STR
            || Skill == AT_SKILL_DEFENSE_MASTERY
            || Skill == AT_SKILL_ATTACK
            || Skill == AT_SKILL_ATTACK_STR
            || Skill == AT_SKILL_ATTACK_MASTERY
            || Skill == AT_SKILL_TELEPORT_ALLY
            || Skill == AT_SKILL_SOUL_BARRIER
            || Skill == AT_SKILL_SOUL_BARRIER_STR
            || Skill == AT_SKILL_SOUL_BARRIER_PROFICIENCY
            || Skill == AT_SKILL_ALICE_THORNS
            || Skill == AT_SKILL_RECOVER
            )
        {
            SelectedCharacter = pMember->index;
            return true;
        }
    }

    return false;
}

void CNewUIPartyListWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_party_flag.tga", IMAGE_PARTY_LIST_FLAG, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_x.tga", IMAGE_PARTY_LIST_EXIT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_back.tga", IMAGE_PARTY_LIST_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_hpbar.jpg", IMAGE_PARTY_LIST_HPBAR, GL_LINEAR);
}

void CNewUIPartyListWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_PARTY_LIST_FLAG);
    DeleteBitmap(IMAGE_PARTY_LIST_EXIT);
    DeleteBitmap(IMAGE_PARTY_LIST_BACK);
    DeleteBitmap(IMAGE_PARTY_LIST_HPBAR);
}
