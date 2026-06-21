#include "lsd_effect.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

#include <engine/console.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

void CLsdEffect::OnConsoleInit()
{
    Console()->Register("lsd_toggle", "", CFGFLAG_CLIENT, ConToggle, this,
       "Toggle the LSD screen effect on/off");
    Console()->Register("lsd_speed", "?f", CFGFLAG_CLIENT, ConSpeed, this,
       "Set hue-cycle speed (default 1.0)");
    Console()->Register("lsd_intensity", "?f", CFGFLAG_CLIENT, ConIntensity, this,
       "Set tint overlay strength 0..1 (default 0.15)");
    Console()->Register("lsd_wobble", "?f", CFGFLAG_CLIENT, ConWobble, this,
       "Set zoom-breathing amount 0..1 (default 0.08)");
}

void CLsdEffect::OnRender()
{
    if(!m_Enabled)
    {
       m_Time = 0.0f;
       return;
    }

    m_Time += Client()->RenderFrameTime();
    RenderTintOverlay();
}

float CLsdEffect::ZoomModifier() const
{
    if(!m_Enabled)
       return 1.0f;
    return 1.0f + std::sin(m_Time * m_WobbleSpeed * 2.0f * 3.14159265f) * m_WobbleAmount;
}

void CLsdEffect::HSVtoRGB(float h, float s, float v, float &r, float &g, float &b){
    float h6 = h * 6.0f;
    int i = (int)h6;
    float f = h6 - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch(i % 6)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
}
void CLsdEffect::RenderTintOverlay()
{
    float Width = Graphics()->ScreenWidth();
    float Height = Graphics()->ScreenHeight();

    Graphics()->MapScreen(0, 0, Width, Height);
    Graphics()->TextureClear();

    static float s_LastModeSwitch = 0.0f;
    static int s_CurrentMode = 0;
    static int s_NextEffectMode = 0;
    static float s_TransitionProgress = 1.0f;

    float DeltaTime = Client()->RenderFrameTime();
    if(DeltaTime > 0.1f) DeltaTime = 0.1f;

    m_Time += DeltaTime;

    if(m_Time - s_LastModeSwitch > 8.0f && s_TransitionProgress >= 1.0f)
    {
        do {
            s_NextEffectMode = std::rand() % 3;
        } while(s_NextEffectMode == s_CurrentMode);

        s_TransitionProgress = 0.0f;
        s_LastModeSwitch = m_Time;
    }

    if(s_TransitionProgress < 1.0f)
    {

        s_TransitionProgress += DeltaTime * 0.4f;
        if(s_TransitionProgress >= 1.0f)
        {
            s_TransitionProgress = 1.0f;
            s_CurrentMode = s_NextEffectMode;
        }
    }

    float GridSize = 12.0f;
    Graphics()->QuadsBegin();

    for(float x = 0; x < Width; x += GridSize)
    {
        for(float y = 0; y < Height; y += GridSize)
        {
            float DX = (x - Width / 2.0f) / (Width / 2.0f);
            float DY = (y - Height / 2.0f) / (Height / 2.0f);

            float Radius = std::sqrt(DX * DX + DY * DY);
            float Angle = std::atan2(DY, DX);

            auto ComputeMode = [&](int Mode, float &OutHue, float &OutSat, float &OutVal, float &OutAlpha, float &OutWave) {
                float Wave = 0.0f;
                OutSat = 0.9f;
                OutVal = 1.0f;

                if(Mode == 0)
                {
                    float LogRadius = std::log(Radius + 0.1f);
                    float Spiral = std::sin(LogRadius * 5.0f - Angle + m_Time * m_Speed * 2.5f);
                    float Breathing = std::cos(Radius * 3.0f - m_Time * m_WobbleSpeed);
                    Wave = (Spiral + Breathing) * 0.5f;
                }
                else if(Mode == 1)
                {
                    float P1 = std::sin(DX * 4.0f + m_Time * m_Speed);
                    float P2 = std::sin(DY * 5.0f - m_Time * m_Speed * 1.2f);
                    float P3 = std::sin(std::sqrt((DX * 3.0f + m_Time) * (DX * 3.0f + m_Time) + (DY * 3.0f) * (DY * 3.0f)));
                    Wave = (P1 + P2 + P3) / 3.0f;
                    OutSat = 0.7f + 0.3f * std::sin(m_Time * 0.5f);
                }
                else
                {
                    float KAngle = std::abs(std::fmod(Angle, 3.14159265f / 3.0f) - (3.14159265f / 6.0f));
                    Wave = std::sin(Radius * 10.0f + KAngle * 4.0f - m_Time * m_Speed * 3.5f);
                    OutVal = 0.8f + 0.2f * std::abs(std::sin(m_Time * 2.0f));
                }

                OutWave = Wave;
                OutHue = std::fmod(std::fmod(std::abs(Wave), 1.0f) + m_Time * 0.05f, 1.0f);
                OutAlpha = m_Intensity * (0.3f + 0.7f * (Wave * 0.5f + 0.5f));
            };

            float HueCurr, SatCurr, ValCurr, AlphaCurr, WaveCurr;
            ComputeMode(s_CurrentMode, HueCurr, SatCurr, ValCurr, AlphaCurr, WaveCurr);

            float FinalR, FinalG, FinalB, FinalAlpha = AlphaCurr;
            float FinalWave = WaveCurr;

            HSVtoRGB(HueCurr, SatCurr, ValCurr, FinalR, FinalG, FinalB);

            if(s_TransitionProgress < 1.0f)
            {
                float HueNext, SatNext, ValNext, AlphaNext, WaveNext;
                ComputeMode(s_NextEffectMode, HueNext, SatNext, ValNext, AlphaNext, WaveNext);

                float NextR, NextG, NextB;
                HSVtoRGB(HueNext, SatNext, ValNext, NextR, NextG, NextB);

                float Blend = (1.0f - std::cos(s_TransitionProgress * 3.14159265f)) * 0.5f;

                FinalR = FinalR * (1.0f - Blend) + NextR * Blend;
                FinalG = FinalG * (1.0f - Blend) + NextG * Blend;
                FinalB = FinalB * (1.0f - Blend) + NextB * Blend;
                FinalAlpha = AlphaCurr * (1.0f - Blend) + AlphaNext * Blend;
                FinalWave = WaveCurr * (1.0f - Blend) + WaveNext * Blend;
            }

            Graphics()->SetColor(FinalR, FinalG, FinalB, FinalAlpha);

            float OffsetX = std::cos(Angle) * FinalWave * 4.0f;
            float OffsetY = std::sin(Angle) * FinalWave * 4.0f;

            IGraphics::CQuadItem QuadItem(x + OffsetX, y + OffsetY, GridSize, GridSize);
            Graphics()->QuadsDrawTL(&QuadItem, 1);
        }
    }

    Graphics()->QuadsEnd();
}
void CLsdEffect::ConToggle(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    pSelf->m_Enabled = !pSelf->m_Enabled;
}

void CLsdEffect::ConSpeed(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_Speed = pResult->GetFloat(0);
}

void CLsdEffect::ConIntensity(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_Intensity = pResult->GetFloat(0);
}

void CLsdEffect::ConWobble(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_WobbleAmount = pResult->GetFloat(0);
}
