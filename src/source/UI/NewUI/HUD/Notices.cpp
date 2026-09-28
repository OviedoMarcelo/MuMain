#include "stdafx.h"
#include "Core/Text/TextLineWrap.h"
#include "UI/NewUI/HUD/Notices.h"

#include "UI/Legacy/UIControls.h"        // g_pRenderText, RT3_WRITE_CENTER
#include "App/Platform/Windows/Winmain.h"    // g_hFontBold
#include "Render/Textures/ZzzOpenglUtil.h" // EnableAlphaTest
#include "UI/NewUI/NewUISystem.h"        // g_pNewUISystem
#include "Engine/AI/ZzzAI.h"             // FPS_ANIMATION_FACTOR
#include "Engine/Object/ZzzInterface.h"  // CutText

namespace
{
    constexpr int MAX_NOTICE = 6;
    constexpr int NOTICE_LIFETIME = 300;
    constexpr int NOTICE_TEXT_MAX = 256;
    constexpr int NOTICE_BLINK_PERIOD = 20; // reference frames per on/off cycle (0.8 s)

    struct Notice
    {
        wchar_t Text[NOTICE_TEXT_MAX];
        int     LifeTime;
        BYTE    Color;
    };

    int    s_count = 0;
    // float: as an int, subtracting FPS_ANIMATION_FACTOR (< 1 above 25 FPS) truncated to a whole
    // frame, so notices expired faster the higher the frame rate.
    float  s_time = NOTICE_LIFETIME;
    float  s_blinkPhase = 0.f;
    Notice s_notices[MAX_NOTICE];

    // Shift the buffer up by one when it is full so the newest line fits.
    void Scroll()
    {
        if (s_count > MAX_NOTICE - 1)
        {
            s_count = MAX_NOTICE - 1;
            for (int i = 1; i < MAX_NOTICE; i++)
            {
                s_notices[i - 1].Color = s_notices[i].Color;
                wcscpy(s_notices[i - 1].Text, s_notices[i].Text);
            }
        }
    }
}

namespace UI::Notices
{
    void CopyText(wchar_t (&destination)[NOTICE_TEXT_MAX], const wchar_t* source)
    {
        wcsncpy_s(destination, source, _TRUNCATE);
    }

    void Clear()
    {
        memset(s_notices, 0, sizeof(s_notices));
    }

    void Create(const wchar_t* text, int color)
    {
        g_pRenderText->SetFont(g_hFontBold);
        const SIZE size = g_pRenderText->MeasureText(text, lstrlen(text));

        Scroll();
        s_notices[s_count].Color = color;
        if (size.cx < NOTICE_TEXT_MAX)
        {
            CopyText(s_notices[s_count++].Text, text);
        }
        else
        {
            wchar_t topText[NOTICE_TEXT_MAX] = { 0 };
            wchar_t bottomText[NOTICE_TEXT_MAX] = { 0 };
            CutText(text, topText, bottomText, NOTICE_TEXT_MAX);
            CopyText(s_notices[s_count++].Text, topText);
            Scroll();
            s_notices[s_count].Color = color;
            CopyText(s_notices[s_count++].Text, bottomText);
        }
        s_time = NOTICE_LIFETIME;
    }

    void Move()
    {
        s_time -= FPS_ANIMATION_FACTOR;
        if (s_time <= 0)
        {
            s_time = NOTICE_LIFETIME;
            Create(L"", 0);
        }
    }

    void Render()
    {
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
        if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == true)
            return;
#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM

        EnableAlphaTest();

        g_pRenderText->SetFont(g_hFontBold);

        for (int i = 0; i < MAX_NOTICE; i++)
        {
            Notice* n = &s_notices[i];
            if (n->Color == 0)
            {
                g_pRenderText->SetBgColor(0, 0, 0, 128);
                if ((int)s_blinkPhase < NOTICE_BLINK_PERIOD / 2)
                {
                    g_pRenderText->SetTextColor(255, 200, 80, 128);
                }
                else
                {
                    g_pRenderText->SetTextColor(255, 200, 80, 255);
                }
            }
            else
            {
                g_pRenderText->SetTextColor(100, 255, 200, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 128);
            }

            g_pRenderText->RenderText(320, 300 + i * 13, n->Text, 0, 0, RT3_WRITE_CENTER);
        }

        // Wrap so the float keeps its precision over long sessions.
        s_blinkPhase = std::fmod(s_blinkPhase + FPS_ANIMATION_FACTOR, static_cast<float>(NOTICE_BLINK_PERIOD));
    }
}
