#include "stdafx.h"
#include "GameLogic/Automation/Attack.h"
#include "GameLogic/Automation/Movement.h"
#include "GameLogic/Automation/Pickup.h"
#include "GameLogic/Automation/Skill.h"
#include "GameLogic/Combat/SkillExecution.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <climits>
#include <cmath>

#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/PlayerActionState.h"
#include "UI/NewUI/NewUISystem.h"
#include "Core/Utilities/Log/muConsoleDebug.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Skills/SkillManager.h"
#include "GameLogic/Social/PartyManager.h"
#include "World/MapInfra/MapManager.h"
#include "Network/Server/WSclient.h"
#include "GameLogic/Items/ItemCategories.h"

#include "MuHelper.h"

constexpr int MAX_ACTIONABLE_DISTANCE = 10;
constexpr int DEFAULT_DURABILITY_THRESHOLD = 50;
constexpr int MANA_POTION_COOLDOWN_LOOPS = 3;  // timer loops (250 ms each) between two mana potions
constexpr int LOOPS_PER_SECOND = 4;            // the timer loops every 250 ms
constexpr int ORIGINAL_POSITION_TOLERANCE = 2; // tiles from the original position which don't count as away

SpinLock _targetsLock;
SpinLock _itemsLock;

// Movement/target globals are defined in ZzzInterface.cpp.
extern MovementSkill g_MovementSkill;
extern int SelectedCharacter;
extern int TargetX;
extern int TargetY;

namespace MUHelper
{
	MovementSkill& g_MovementSkill = ::g_MovementSkill;
	int& SelectedCharacter = ::SelectedCharacter;
	int& TargetX = ::TargetX;
	int& TargetY = ::TargetY;

    CMuHelper g_MuHelper;

    void CALLBACK CMuHelper::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
    {
        g_MuHelper.WorkLoop(hwnd, uMsg, idEvent, dwTime);
    }

    void CMuHelper::Save(const ConfigData& config)
    {
        m_config = config;

        PRECEIVE_MUHELPER_DATA netData;
        ConfigDataSerDe::Serialize(m_config, netData);

        SocketClient->ToGameServer()->SendMuHelperSaveDataRequest(reinterpret_cast<BYTE*>(&netData), sizeof(netData));
    }

    void CMuHelper::Load(const ConfigData& config)
    {
        m_config = config;
    }

    ConfigData CMuHelper::GetConfig() const {
        return m_config;
    }

    void CMuHelper::Toggle()
    {
        if (m_bActive)
        {
            TriggerStop();

            // Stop the client-driven bot immediately instead of waiting for the
            // server's status reply. After an auto-reconnect the server's new
            // session doesn't have the helper marked active, so it never replies
            // and the bot would otherwise keep running with no way to stop it.
            Stop();
        }
        else
        {
            TriggerStart();
        }
    }

    void CMuHelper::TriggerStart()
    {
        if (!Hero->SafeZone)
            SocketClient->ToGameServer()->SendMuHelperStatusChangeRequest(0);
    }

    void CMuHelper::TriggerStop()
    {
        SocketClient->ToGameServer()->SendMuHelperStatusChangeRequest(1);
    }

    void CMuHelper::Start()
    {
        if (m_bActive)
        {
            return;
        }

        m_iTotalCost = 0;
        m_iComboState = 0;
        m_iCurrentBuffIndex = 0;
        m_iCurrentBuffPartyIndex = 0;
        m_iCurrentHealPartyIndex = 0;
        m_iCurrentTarget = -1;
        m_iCurrentSkill = (ActionSkillType)m_config.aiSkill[0];
        m_iCurrentItem = MAX_ITEMS;
        m_posOriginal = { Hero->PositionX, Hero->PositionY };

        // The range of the UI, in tiles: the diagonal would turn a range of 6 into 9.
        m_iHuntingDistance = std::max(1, m_config.iHuntingRange);
        m_iObtainingDistance = ComputeDistanceByRange(m_config.iObtainingRange);

        m_iSecondsElapsed = 0;
        m_iSecondsAway = 0;

        m_bTimerActivatedBuffOngoing = false;
        m_bPetActivated = false;
        m_bTargetInReach = false;

        m_iLoopCounter = 0;
        m_iManaPotionCooldown = 0;
        m_bSwingInProgress = false;

        m_bActive = true;
        g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Started");
    }

    void CMuHelper::Stop()
    {
        m_bActive = false;
        g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Stopped");
    }

    void CMuHelper::WorkLoop(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
    {
        if (!m_bActive)
        {
            return;
        }

        if (Hero->SafeZone)
        {
            g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Entered safezone. Stopping.");
            TriggerStop();
            return;
        }

        Work();

        if (m_iManaPotionCooldown > 0)
        {
            m_iManaPotionCooldown--;
        }

        if (++m_iLoopCounter == LOOPS_PER_SECOND)
        {
            m_iSecondsElapsed++;

            // Time away only counts with nothing to hit: the fight isn't left for a timer.
            if (ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, m_posOriginal) > ORIGINAL_POSITION_TOLERANCE
                && !m_bTargetInReach)
            {
                m_iSecondsAway++;
            }
            else
            {
                m_iSecondsAway = 0;
            }

            m_iLoopCounter = 0;
        }
    }

    // The timer only comes back every 250 ms: a swing that ends in between waited for it, and the
    // faster the hero swings, the more attacks were lost. The work goes on as soon as the swing ends.
    void CMuHelper::OnFrame()
    {
        const bool bSwingInProgress = m_bActive && GameLogic::Automation::IsSwingInProgress();
        const bool bSwingEnded = m_bSwingInProgress && !bSwingInProgress;
        m_bSwingInProgress = bSwingInProgress;

        // The safe zone is left to the timer, which stops the helper there.
        if (bSwingEnded && !Hero->SafeZone)
        {
            Work();
        }
    }

    void CMuHelper::Work()
    {
        try
        {
            if (!ActivatePet())
            {
                return;
            }

            if (!Buff())
            {
                return;
            }

            if (!RecoverHealth())
            {
                return;
            }

            if (!ObtainItem())
            {
                return;
            }

            if (!Regroup())
            {
                return;
            }

            Attack();

            RepairEquipments();
        }
        catch (...)
        {
            g_ConsoleDebug->Write(MCD_NORMAL, L"[MU Helper] Exception occurred. Ignoring...");
        }
    }

    void CMuHelper::AddTarget(int iTargetId, bool bIsAttacking)
    {
        if (!m_bActive)
        {
            return;
        }

        CHARACTER* pTarget = FindCharacterByKey(iTargetId);
        if (!pTarget || pTarget == Hero)
        {
            return;
        }

        const bool bInHuntingArea = ComputeDistanceFromHuntingCenter(pTarget) <= m_iHuntingDistance;

        if (bInHuntingArea
            || (bIsAttacking && m_config.bLongRangeCounterAttack))
        {
            _targetsLock.lock();

            m_setTargets.insert(iTargetId);

            if (bIsAttacking)
            {
                m_setTargetsAttacking.insert(iTargetId);
            }

            _targetsLock.unlock();
        }

        if (m_config.bUseSelfDefense && bIsAttacking && bInHuntingArea && IsMonster(pTarget))
        {
            m_iCurrentTarget = iTargetId;
        }
    }

    void CMuHelper::DeleteTarget(int iTargetId)
    {
        _targetsLock.lock();

        m_setTargets.erase(iTargetId);
        m_setTargetsAttacking.erase(iTargetId);

        _targetsLock.unlock();

        if (iTargetId == m_iCurrentTarget)
        {
            m_iCurrentTarget = -1;
        }
    }

    void CMuHelper::DeleteAllTargets()
    {
        _targetsLock.lock();

        m_setTargets.clear();
        m_setTargetsAttacking.clear();

        _targetsLock.unlock();
    }

    int CMuHelper::ComputeDistanceByRange(int iRange)
    {
        return ComputeDistanceBetween({ 0, 0 }, { iRange, iRange });
    }

    int CMuHelper::ComputeDistanceFromTarget(CHARACTER* pTarget)
    {
        const POINT posHero = { Hero->PositionX, Hero->PositionY };

        const POINT posCurrent = { pTarget->PositionX, pTarget->PositionY };
        const POINT posNext    = { pTarget->TargetX,   pTarget->TargetY };

        return std::min(
            ComputeDistanceBetween(posHero, posCurrent),
            ComputeDistanceBetween(posHero, posNext)
        );
    }

    // With "Original Position" the hunting area stays around that position; otherwise it goes with the hero.
    int CMuHelper::ComputeDistanceFromHuntingCenter(CHARACTER* pTarget)
    {
        const POINT posCenter = m_config.bReturnToOriginalPosition
            ? m_posOriginal
            : POINT{ Hero->PositionX, Hero->PositionY };

        const POINT posCurrent = { pTarget->PositionX, pTarget->PositionY };
        const POINT posNext    = { pTarget->TargetX,   pTarget->TargetY };

        return std::min(
            ComputeDistanceBetween(posCenter, posCurrent),
            ComputeDistanceBetween(posCenter, posNext)
        );
    }

    int CMuHelper::ComputeDistanceBetween(POINT posA, POINT posB)
    {
        int iDx = posA.x - posB.x;
        int iDy = posA.y - posB.y;

        return static_cast<int>(std::ceil(std::sqrt(iDx * iDx + iDy * iDy)));
    }

    int CMuHelper::GetNearestTarget()
    {
        int iClosestMonsterId = -1;
        int iMinDistance = INT_MAX;
        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargets;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            CHARACTER* pTarget = &CharactersClient[iIndex];

            if (!IsMonster(pTarget) || ComputeDistanceFromHuntingCenter(pTarget) > m_iHuntingDistance)
            {
                continue;
            }

            int iDistance = ComputeDistanceFromTarget(pTarget);
            if (iDistance <= iMinDistance)
            {
                iMinDistance = iDistance;
                iClosestMonsterId = iMonsterId;
            }
        }

        return iClosestMonsterId;
    }

    int CMuHelper::GetFarthestAttackingTarget()
    {
        int iFarthestMonsterId = -1;
        int iMaxDistance = -1;

        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargetsAttacking;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            CHARACTER* pTarget = &CharactersClient[iIndex];

            if (!IsMonster(pTarget))
            {
                continue;
            }

            int iDistance = ComputeDistanceFromTarget(pTarget);
            if (iDistance > iMaxDistance)
            {
                iMaxDistance = iDistance;
                iFarthestMonsterId = iMonsterId;
            }
        }

        return iFarthestMonsterId;
    }

    void CMuHelper::CleanupTargets()
    {
        std::set<int> setTargets;
        {
            _targetsLock.lock();
            setTargets = m_setTargets;
            _targetsLock.unlock();
        }

        for (const int& iMonsterId : setTargets)
        {
            int iIndex = FindCharacterIndex(iMonsterId);
            if (iIndex == MAX_CHARACTERS_CLIENT)
            {
                DeleteTarget(iMonsterId);
                continue;
            }

            CHARACTER* pTarget = &CharactersClient[iIndex];
            if (pTarget->Dead > 0 || !pTarget->Object.Live)
            {
                DeleteTarget(iMonsterId);
            }
        }
    }

    bool CMuHelper::IsTargetHuntable(int iTargetId)
    {
        const int iIndex = FindCharacterIndex(iTargetId);
        if (iIndex == MAX_CHARACTERS_CLIENT)
        {
            return false;
        }

        CHARACTER* pTarget = &CharactersClient[iIndex];
        if (pTarget->Dead > 0 || !pTarget->Object.Live)
        {
            return false;
        }

        if (m_config.bLongRangeCounterAttack)
        {
            _targetsLock.lock();
            const bool bIsAttacking = m_setTargetsAttacking.count(iTargetId) > 0;
            _targetsLock.unlock();

            if (bIsAttacking)
            {
                return true;
            }
        }

        return ComputeDistanceFromHuntingCenter(pTarget) <= m_iHuntingDistance;
    }

    bool CMuHelper::IsTargetInReach(int iTargetId, float fReach)
    {
        const int iIndex = FindCharacterIndex(iTargetId);
        return iIndex != MAX_CHARACTERS_CLIENT && GameLogic::Automation::IsCharacterInReach(iIndex, fReach);
    }

    // Picks what to hit with a skill or weapon of that reach. A target that left the hunting area is let go,
    // and one out of reach gives way to a monster in reach: walking is only for when nothing can be hit from here.
    void CMuHelper::RefreshCurrentTarget(float fReach)
    {
        CleanupTargets();

        if (m_iCurrentTarget != -1 && !IsTargetHuntable(m_iCurrentTarget))
        {
            m_iCurrentTarget = -1;
        }

        if (m_iCurrentTarget == -1 && m_config.bLongRangeCounterAttack)
        {
            m_iCurrentTarget = GetFarthestAttackingTarget();
        }

        // Going after a far attacker is what the long distance counter attack asks for.
        if (m_iCurrentTarget != -1
            && (m_config.bLongRangeCounterAttack || IsTargetInReach(m_iCurrentTarget, fReach)))
        {
            return;
        }

        const int iNearestTarget = GetNearestTarget();
        if (iNearestTarget != -1 && (m_iCurrentTarget == -1 || IsTargetInReach(iNearestTarget, fReach)))
        {
            m_iCurrentTarget = iNearestTarget;
        }
    }

    // A skill aimed at someone out of its range would walk the hero there, so party support waits for them to be in it.
    // Skills without a range are cast around the hero and walk nowhere.
    bool CMuHelper::CanSupportFromHere(CHARACTER* pChar, ActionSkillType iSkill)
    {
        if (pChar == Hero)
        {
            return true;
        }

        const float fSkillDistance = gSkillManager.GetSkillDistance(iSkill, Hero);
        if (fSkillDistance <= 0.f)
        {
            return ComputeDistanceFromTarget(pChar) <= MAX_ACTIONABLE_DISTANCE;
        }

        return IsTargetInReach(pChar->Key, fSkillDistance);
    }

    int CMuHelper::ActivatePet()
    {
        if (!m_config.bUseDarkRaven)
        {
            return 1;
        }

        if (m_bPetActivated)
        {
            return 1;
        }

        if (m_config.iDarkRavenMode == PET_ATTACK_CEASE)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::Normal, 0xFFFF);
        }
        else if (m_config.iDarkRavenMode == PET_ATTACK_AUTO)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::AttackRandom, 0xFFFF);
        }
        else if (m_config.iDarkRavenMode == PET_ATTACK_TOGETHER)
        {
            SocketClient->ToGameServer()->SendPetCommandRequest(PetType::DarkRaven, PetCommandMode::AttackWithOwner, 0xFFFF);
        }

        m_bPetActivated = true;
        return 1;
    }

    // A skill saved in the configuration before it evolved is cast as what it became.
    static ActionSkillType ResolveEvolvedSkill(ActionSkillType iSkill)
    {
        if (iSkill <= AT_SKILL_UNDEFINED || g_pSkillList->GetSkillIndex(iSkill) != -1)
        {
            return iSkill;
        }

        const int iIndex = gSkillManager.FindSkillOrEvolutionIndex(iSkill);
        return iIndex != -1 ? CharacterAttribute->Skill[iIndex] : iSkill;
    }

    // Whether the hero can cast the buff at all: it's learned, and its level and stat requirements are met.
    // Mana isn't part of it - running short of it is temporary, and the cast drinks a potion.
    static bool CanEverCastBuff(ActionSkillType iBuffSkill)
    {
        return g_pSkillList->GetSkillIndex(iBuffSkill) != -1
            && gSkillManager.AreSkillAttributeRequirementsMet(iBuffSkill);
    }

    int CMuHelper::Buff()
    {
        if (!HasAssignedBuffSkill())
        {
            return 1;
        }

        // A buff the hero can't cast (yet), e.g. one which needs a higher level, would be tried again and again,
        // and as a buff which isn't done holds up the rest of the work, the helper would stop attacking. It's skipped.
        const auto iBuffSkill = ResolveEvolvedSkill(static_cast<ActionSkillType>(m_config.aiBuff[m_iCurrentBuffIndex]));
        if (iBuffSkill > AT_SKILL_UNDEFINED && !CanEverCastBuff(iBuffSkill))
        {
            m_iCurrentBuffPartyIndex = 0;
            m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();
            if (m_iCurrentBuffIndex == 0)
            {
                m_bTimerActivatedBuffOngoing = false;
            }

            return 1;
        }

        if (m_config.bSupportParty && g_pPartyManager->IsPartyActive())
        {
            m_iCurrentBuffPartyIndex %= PartyNumber;

            PARTY_t* pMember = &Party[m_iCurrentBuffPartyIndex];
            CHARACTER* pChar = g_pPartyManager->GetPartyMemberChar(pMember);
            int iBuffResult = 1;

            if (pChar != NULL
                && CanSupportFromHere(pChar, iBuffSkill))
            {
                if (!m_config.bBuffDurationParty
                    && m_config.iBuffCastInterval != 0
                    && m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
                {
                    m_bTimerActivatedBuffOngoing = true;
                }

                iBuffResult = BuffTarget(pChar, iBuffSkill);
            }

            m_iCurrentBuffPartyIndex = (m_iCurrentBuffPartyIndex + 1) % PartyNumber;

            if (m_iCurrentBuffPartyIndex == 0)
            {
                m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();

                // Reaching this branch means everyone's been buffed, 
                // so we're resetting the timer activated buff flag
                if (m_iCurrentBuffIndex == 0)
                {
                    m_bTimerActivatedBuffOngoing = false;
                }
            }

            return iBuffResult;
        }
        else
        {
            m_iCurrentBuffPartyIndex = 0;

            if (!m_config.bBuffDuration
                && m_config.iBuffCastInterval != 0
                && m_iSecondsElapsed % m_config.iBuffCastInterval == 0)
            {
                m_bTimerActivatedBuffOngoing = true;
            }

            if (!BuffTarget(Hero, iBuffSkill))
            {
                return 0;
            }
        }

        if (m_iCurrentBuffPartyIndex == 0)
        {
            m_iCurrentBuffIndex = (m_iCurrentBuffIndex + 1) % m_config.aiBuff.size();

            // Reaching this branch means everyone's been buffed, 
            // so we're resetting the timer activated buff flag
            if (m_iCurrentBuffIndex == 0)
            {
                m_bTimerActivatedBuffOngoing = false;
            }
        }

        return 1;
    }

    int CMuHelper::BuffTarget(CHARACTER* pTargetChar, ActionSkillType iBuffSkill)
    {
        OBJECT* obj = &pTargetChar->Object;

        auto CastIfMissing = [&](bool bBuffActive, bool bTimerRespected, bool bNeedsTarget) -> int
        {
            if (!bBuffActive || (bTimerRespected && m_bTimerActivatedBuffOngoing))
                return SimulateSkill(iBuffSkill, bNeedsTarget, pTargetChar->Key);
            return 1;
        };

        switch (iBuffSkill)
        {
        case AT_SKILL_ATTACK:
        case AT_SKILL_ATTACK_STR:
        case AT_SKILL_ATTACK_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Attack), true, true);

        case AT_SKILL_DEFENSE:
        case AT_SKILL_DEFENSE_STR:
        case AT_SKILL_DEFENSE_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Defense), true, true);

        case AT_SKILL_INFINITY_ARROW:
        case AT_SKILL_INFINITY_ARROW_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_InfinityArrow) || g_isCharacterBuff(obj, EFFECT_INFINITY_ARROW_IMPROVED), false, false);

        case AT_SKILL_SOUL_BARRIER:
        case AT_SKILL_SOUL_BARRIER_STR:
        case AT_SKILL_SOUL_BARRIER_PROFICIENCY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_WizDefense), true, true);

        case AT_SKILL_SWELL_LIFE:
        case AT_SKILL_SWELL_LIFE_STR:
        case AT_SKILL_SWELL_LIFE_PROFICIENCY:
            if (m_iComboState == 2)
            {
                return 1;
            }
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Life) || g_isCharacterBuff(obj, EFFECT_GREATER_LIFE_ENHANCED) || g_isCharacterBuff(obj, EFFECT_GREATER_LIFE_MASTERED), true, false);

        case AT_SKILL_EXPANSION_OF_WIZARDRY:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_STR:
        case AT_SKILL_EXPANSION_OF_WIZARDRY_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_SwellOfMagicPower) || g_isCharacterBuff(obj, EFFECT_MAGIC_CIRCLE_IMPROVED) || g_isCharacterBuff(obj, EFFECT_MAGIC_CIRCLE_ENHANCED), false, false);

        case AT_SKILL_ADD_CRITICAL:
        case AT_SKILL_ADD_CRITICAL_STR1:
        case AT_SKILL_ADD_CRITICAL_STR2:
        case AT_SKILL_ADD_CRITICAL_STR3:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_AddCriticalDamage) || g_isCharacterBuff(obj, EFFECT_GREATER_CRITICAL_DAMAGE_MASTERED) || g_isCharacterBuff(obj, EFFECT_GREATER_CRITICAL_DAMAGE_EXTENDED), false, false);

        case AT_SKILL_ALICE_BERSERKER:
        case AT_SKILL_ALICE_BERSERKER_STR:
        case AT_SKILL_BerserkerProficiency:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Berserker), false, false);

        case AT_SKILL_ALICE_THORNS:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Thorns), false, false);

        // Rage Fighter party buffs — self/party AoE, no explicit target needed.
        case AT_SKILL_ATT_UP_OURFORCES:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Att_up_Ourforces), true, false);

        case AT_SKILL_HP_UP_OURFORCES:
        case AT_SKILL_HP_UP_OURFORCES_STR:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Hp_up_Ourforces), true, false);

        case AT_SKILL_DEF_UP_OURFORCES:
        case AT_SKILL_DEF_UP_OURFORCES_STR:
        case AT_SKILL_DEF_UP_OURFORCES_MASTERY:
            return CastIfMissing(g_isCharacterBuff(obj, eBuff_Def_up_Ourforces), true, false);

        default:
            return 1;
        }
    }


    int CMuHelper::ConsumePotion()
    {
        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;

        if (m_config.bUseHealPotion && iLifeMax > 0 && iLife > 0)
        {
            int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;
            if (iRemaining <= m_config.iPotionThreshold)
            {
                int iPotionIndex = g_pMyInventory->FindHealingItemIndex();
                if (iPotionIndex != -1)
                {
                    SendRequestUse(iPotionIndex, 0);
                }
            }
        }

        ConsumeManaPotion();

        return 1;
    }

    // Refills mana below the "MP Status" of the Auto Recovery window (30% until the player sets it).
    // Without this, mana was only refilled by the skill code once a cast had already failed for lack
    // of it, losing casts while hunting.
    void CMuHelper::ConsumeManaPotion()
    {
        // The new mana arrives with the server's answer: wait a moment so one dip doesn't drink several potions.
        // Counted down by the timer: the work also runs between two of its loops.
        if (m_iManaPotionCooldown > 0)
        {
            return;
        }

        if (!m_config.bUseHealPotion)
        {
            return;
        }

        const int64_t iMana = CharacterAttribute->Mana;
        const int64_t iManaMax = CharacterAttribute->ManaMax;
        if (iManaMax <= 0 || iMana * 100 > iManaMax * m_config.iManaPotionThreshold)
        {
            return;
        }

        int iPotionIndex = g_pMyInventory->FindManaItemIndex();
        if (iPotionIndex != -1)
        {
            SendRequestUse(iPotionIndex, 0);
            m_iManaPotionCooldown = MANA_POTION_COOLDOWN_LOOPS;
        }
    }

    int CMuHelper::RecoverHealth()
    {
        if (!Heal())
        {
            return 0;
        }
        
        if (!DrainLife())
        {
            return 0;
        }

        if (!ConsumePotion())
        {
            return 0;
        }

        return 1;
    }

    int CMuHelper::Heal()
    {
        if (!m_config.bAutoHeal)
        {
            return 1;
        }

        auto iHealingSkill = GetHealingSkill();
        if (iHealingSkill == AT_SKILL_UNDEFINED)
        {
            return 1;
        }

        if (m_config.bAutoHealParty && g_pPartyManager->IsPartyActive())
        {
            m_iCurrentHealPartyIndex %= PartyNumber;

            PARTY_t* pMember = &Party[m_iCurrentHealPartyIndex];
            CHARACTER* pChar = g_pPartyManager->GetPartyMemberChar(pMember);
            int iHealResult = 1;

            if (pChar != NULL)
            {
                if (pChar == Hero)
                {
                    iHealResult = HealSelf(iHealingSkill);
                }
                else if (pMember->stepHP * 10 <= m_config.iHealPartyThreshold
                    && CanSupportFromHere(pChar, iHealingSkill))
                {
                    iHealResult = SimulateSkill(iHealingSkill, true, pChar->Key);
                }
            }

            m_iCurrentHealPartyIndex = (m_iCurrentHealPartyIndex + 1) % PartyNumber;

            return iHealResult;
        }
        else
        {
            m_iCurrentHealPartyIndex = 0;
            return HealSelf(iHealingSkill);
        }

        return 1;
    }

    int CMuHelper::HealSelf(ActionSkillType iHealingSkill)
    {
        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;
        int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

        if (iRemaining <= m_config.iHealThreshold)
        {
            return SimulateSkill(iHealingSkill, true, HeroKey);
        }

        return 1;
    }

    int CMuHelper::DrainLife()
    {
        if (!m_config.bUseDrainLife)
        {
            return 1;
        }

        auto iDrainLife = GetDrainLifeSkill();
        if (iDrainLife == AT_SKILL_UNDEFINED)
        {
            return 1;
        }

        int64_t iLife = CharacterAttribute->Life;
        int64_t iLifeMax = CharacterAttribute->LifeMax;
        int64_t iRemaining = (iLife * 100 + iLifeMax - 1) / iLifeMax;

        if (iRemaining <= m_config.iHealThreshold)
        {
            m_iCurrentTarget = GetNearestTarget();
            if (m_iCurrentTarget != -1)
            {
                return SimulateSkill(iDrainLife, true, m_iCurrentTarget);
            }
        }

        return 1;
    }

    int CMuHelper::RepairEquipments()
    {
        if (m_config.bRepairItem)
        {
            for (int i = 0; i < MAX_EQUIPMENT; i++)
            {
                ITEM* pItem = &CharacterMachine->Equipment[i];
                if (!pItem || pItem->Type == -1)
                {
                    continue;
                }

                ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
                if (!pAttr)
                {
                    continue;
                }

                int iLevel = pItem->Level;
                int iDurability = pItem->Durability;
                int iMaxDurability = CalcMaxDurability(pItem, pAttr, iLevel);

                int64_t iHealth = (iDurability * 100 + iMaxDurability - 1) / iMaxDurability;

                if (iHealth <= DEFAULT_DURABILITY_THRESHOLD)
                {
                    int64_t iGoldCost = CalcSelfRepairCost(ItemValue(pItem, 2), iDurability, iMaxDurability, pItem->Type);
                    if (iGoldCost <= CharacterMachine->Gold)
                    {
                        SocketClient->ToGameServer()->SendRepairItemRequest(i, 1);
                    }
                }
            }
        }

        return 1;
    }

    int CMuHelper::Attack()
    {
        if (m_config.bUseCombo)
        {
            m_iCurrentSkill = ResolveEvolvedSkill(static_cast<ActionSkillType>(m_config.aiSkill[m_iComboState]));
        }
        else
        {
            m_iCurrentSkill = SelectAttackSkill();
        }

        const float fReach = m_iCurrentSkill > AT_SKILL_UNDEFINED
            ? gSkillManager.GetSkillDistance(m_iCurrentSkill, Hero)
            : GameLogic::Automation::EquippedAttackRange();

        RefreshCurrentTarget(fReach);

        m_bTargetInReach = m_iCurrentTarget != -1 && IsTargetInReach(m_iCurrentTarget, fReach);

        if (m_iCurrentTarget == -1)
        {
            m_iComboState = 0;
            return 0;
        }

        if (m_config.bUseCombo)
        {
            return SimulateComboAttack();
        }

        if (m_iCurrentSkill > AT_SKILL_UNDEFINED)
        {
            const float fSkillDistance = gSkillManager.GetSkillDistance(m_iCurrentSkill, Hero);
            if (GameLogic::Combat::CanExecuteSkill(Hero, m_iCurrentSkill, fSkillDistance))
            {
                return SimulateAttack(m_iCurrentSkill);
            }
        }

        if (m_config.bFallbackBasicAttack)
        {
            // A skill that can't be paid for right now is no reason to walk up to the target: only hit what's at hand.
            const bool bWalksForFallback = m_iCurrentSkill > AT_SKILL_UNDEFINED
                && !IsTargetInReach(m_iCurrentTarget, GameLogic::Automation::EquippedAttackRange());

            if (!Hero->Movement && !bWalksForFallback)
            {
                return SimulateBasicAttack(m_iCurrentTarget);
            }
        }

        return 1;
    }

    ActionSkillType CMuHelper::SelectAttackSkill()
    {
        const size_t safeSize = std::min({m_config.aiSkill.size(), m_config.aiSkillCondition.size(), m_config.aiSkillInterval.size()});
        for (int i = 1; i < (int)safeSize; i++)
        {
            const int iSkillId = m_config.aiSkill[i];
            if (iSkillId <= 0 || iSkillId >= MAX_SKILLS)
            {
                continue;
            }

            if ((m_config.aiSkillCondition[i] & ON_TIMER)
                && m_config.aiSkillInterval[i] != 0
                && m_iSecondsElapsed > 0
                && m_iSecondsElapsed % m_config.aiSkillInterval[i] == 0)
            {
                return ResolveEvolvedSkill((ActionSkillType)iSkillId);
            }

            if (m_config.aiSkillCondition[i] & ON_CONDITION)
            {
                int iCount = 0;
                if (m_config.aiSkillCondition[i] & ON_MOBS_NEARBY)
                {
                    iCount = (int)m_setTargets.size();
                }
                else if (m_config.aiSkillCondition[i] & ON_MOBS_ATTACKING)
                {
                    iCount = (int)m_setTargetsAttacking.size();
                }
                else
                {
                    continue;
                }

                if (((m_config.aiSkillCondition[i] & ON_MORE_THAN_TWO_MOBS)   && iCount >= 2)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_THREE_MOBS) && iCount >= 3)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_FOUR_MOBS)  && iCount >= 4)
                    || ((m_config.aiSkillCondition[i] & ON_MORE_THAN_FIVE_MOBS)  && iCount >= 5))
                {
                    return ResolveEvolvedSkill((ActionSkillType)iSkillId);
                }
            }
        }

        if (m_config.aiSkill[0] > 0)
        {
            return ResolveEvolvedSkill((ActionSkillType)m_config.aiSkill[0]);
        }

        return AT_SKILL_UNDEFINED;
    }

    int CMuHelper::SimulateComboAttack()
    {
        for (int i = 0; i < m_config.aiSkill.size(); i++)
        {
            if (m_config.aiSkill[i] == 0)
            {
                return 0;
            }
        }

        if (SimulateAttack(ResolveEvolvedSkill((ActionSkillType)m_config.aiSkill[m_iComboState])))
        {
            m_iComboState = (m_iComboState + 1) % 3;
        }

        return 1;
    }

    int CMuHelper::SimulateAttack(ActionSkillType iSkill)
    {
        return SimulateSkill(iSkill, true, m_iCurrentTarget);
    }

    int CMuHelper::SimulateSkill(ActionSkillType iSkill, bool bTargetRequired, int iTarget)
    {
        using GameLogic::Automation::SkillResult;

        // Players are allowed here, and must be: the helper casts its buffs and
        // heals on party members and on the hero itself. Only the plain attack
        // is monster-only, as it always was. A target that is not attackable is
        // still refused by the primitive when it is dead.
        const SkillResult result =
            GameLogic::Automation::CastSkill(iSkill, bTargetRequired, iTarget, true, m_iHuntingDistance);

        if (iTarget != -1 && GameLogic::Automation::ShouldForgetTarget(result))
        {
            DeleteTarget(iTarget);
        }

        return result == SkillResult::Cast ? 1 : 0;
    }

    int CMuHelper::SimulateBasicAttack(int iTarget)
    {
        using GameLogic::Automation::AttackResult;

        // No target is not a failed attack: without this the helper asks the
        // automation for object -1 and then forgets a target it never had.
        if (iTarget == -1)
        {
            return 0;
        }

        const AttackResult result = GameLogic::Automation::AttackObject(iTarget, false, m_iHuntingDistance);

        if (GameLogic::Automation::ShouldForgetTarget(result))
        {
            DeleteTarget(iTarget);
        }

        return result == AttackResult::Attacked ? 1 : 0;
    }

    int CMuHelper::Regroup()
    {
        if (!m_config.bReturnToOriginalPosition)
        {
            return 1;
        }

        // Past the hunting area there's nothing left to do out there, whatever the timer says.
        // A far attacker can take the hero there on purpose (long distance counter attack).
        const bool bLeftHuntingArea = !m_config.bLongRangeCounterAttack
            && ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, m_posOriginal) > m_iHuntingDistance;

        if (bLeftHuntingArea || m_iSecondsAway > m_config.iMaxSecondsAway)
        {
            // Otherwise the auto-attack of the engine would go on with the monster left behind.
            Attacking = -1;
            SelectedCharacter = -1;
            m_bTargetInReach = false;

            if (!SimulateMove(m_posOriginal))
            {
                return 0;
            }

            m_iSecondsAway = 0;
            m_iComboState = 0;
            m_iCurrentTarget = -1;
        }

        return 1;
    }

    int CMuHelper::SimulateMove(POINT posMove)
    {
        const GameLogic::Automation::MoveResult result =
            GameLogic::Automation::WalkTo(static_cast<int>(posMove.x), static_cast<int>(posMove.y));

        return result == GameLogic::Automation::MoveResult::Arrived ? 1 : 0;
    }

    bool CMuHelper::HasAssignedBuffSkill()
    {
        for (int i = 0; i < m_config.aiBuff.size(); i++)
        {
            if (m_config.aiBuff[i] != 0)
            {
                return true;
            }
        }

        return false;
    }

    ActionSkillType CMuHelper::GetHealingSkill()
    {
        std::vector<ActionSkillType> aiHealingSkills =
        {
            AT_SKILL_HEALING,
            AT_SKILL_HEALING_STR,
        };

        for (int i = 0; i < aiHealingSkills.size(); i++)
        {
            int iSkillIndex = g_pSkillList->GetSkillIndex(aiHealingSkills[i]);
            if (iSkillIndex != -1)
            {
                return aiHealingSkills[i];
            }
        }

        return AT_SKILL_UNDEFINED;
    }

    bool CMuHelper::IsSelfPositionSkill(ActionSkillType iSkill)
    {
        return GameLogic::Automation::IsSelfPositionSkill(iSkill);
    }

    ActionSkillType CMuHelper::GetDrainLifeSkill()
    {
        std::vector<ActionSkillType> aiDrainLifeSkills =
        {
            AT_SKILL_ALICE_DRAINLIFE,
            AT_SKILL_ALICE_DRAINLIFE_STR
        };

        for (int i = 0; i < aiDrainLifeSkills.size(); i++)
        {
            int iSkillIndex = g_pSkillList->GetSkillIndex(aiDrainLifeSkills[i]);
            if (iSkillIndex != -1)
            {
                return aiDrainLifeSkills[i];
            }
        }

        return AT_SKILL_UNDEFINED;
    }

    int CMuHelper::ObtainItem()
    {
        if (m_iCurrentItem == MAX_ITEMS)
        {
            m_iCurrentItem = SelectItemToObtain();
            if (m_iCurrentItem == MAX_ITEMS)
            {
                return 1;
            }
        }

        using GameLogic::Automation::PickupResult;
        const PickupResult result = GameLogic::Automation::PickUpItem(m_iCurrentItem, m_iObtainingDistance);

        switch (result)
        {
        case PickupResult::Requested:
            // The request is out; stop tracking the drop, as before.
            DeleteItem(m_iCurrentItem);
            return 1;
        case PickupResult::Gone:
            DeleteItem(m_iCurrentItem);
            return 1;
        case PickupResult::Approaching:
        case PickupResult::NoPath:
            // Walking towards it: the helper's other work waits a frame.
            return 0;
        case PickupResult::TooFar:
        case PickupResult::Busy:
            return 1;
        }

        return 1;
    }

    bool CMuHelper::ShouldObtainItem(int iItemId)
    {
        ITEM_t* pDrop = &Items[iItemId];
        ITEM* pItem = &pDrop->Item;

        if ((m_config.bPickZen && IsMoneyItem(pItem))
            || (m_config.bPickJewel && GameLogic::Items::IsJewelItem(pItem))
            || (m_config.bPickAncient && IsAncientItem(pItem))
            || (m_config.bPickExcellent && IsExcellentItem(pItem)))
        {
            return true;
        }

        if (m_config.bPickExtraItems)
        {
            std::wstring strDisplayName = GetItemDisplayName(pItem);

            for (const auto& str : m_config.aExtraItems)
            {
                // Check if the search keyword is in the item's display name
                if (strDisplayName.find(str) != std::wstring::npos)
                {
                    return true;
                }
            }
        }

        return m_config.bPickAllItems;
    }

    void CMuHelper::AddItem(int iItemId, POINT posWhere)
    {
        _itemsLock.lock();
        m_setItems.insert(iItemId);
        _itemsLock.unlock();
    }

    void CMuHelper::DeleteItem(int iItemId)
    {
        _itemsLock.lock();
        m_setItems.erase(iItemId);
        _itemsLock.unlock();

        if (iItemId == m_iCurrentItem)
        {
            m_iCurrentItem = MAX_ITEMS;
        }
    }

    int CMuHelper::SelectItemToObtain()
    {
        int iClosestItemId = MAX_ITEMS;
        int iMinDistance = m_config.iObtainingRange;

        std::set<int> setItems;
        {
            _itemsLock.lock();
            setItems = m_setItems;
            _itemsLock.unlock();
        }

        for (const int& iItemId : setItems)
        {
            if (!ShouldObtainItem(iItemId))
            {
                continue;
            }

            int iItemX = (int)(Items[iItemId].Object.Position[0] / TERRAIN_SCALE);
            int iItemY = (int)(Items[iItemId].Object.Position[1] / TERRAIN_SCALE);

            int iDistance = ComputeDistanceBetween({ Hero->PositionX, Hero->PositionY }, { iItemX, iItemY });
            if (iDistance <= iMinDistance)
            {
                iMinDistance = iDistance;
                iClosestItemId = iItemId;
            }
        }

        return iClosestItemId;
    }
}
