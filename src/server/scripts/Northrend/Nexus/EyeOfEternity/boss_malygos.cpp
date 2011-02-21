/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Script Data Start
SDName: Boss malygos
SDAuthor: LordVanMartin
SD%Complete:
SDComment:
SDCategory:
Script Data End */

/*
Implemented:
Phase1:
  Arcane Breath - implemented normal, heroic
  Vortex - Visual AOE dummy, debuff (dealt 2k dmg every sec, for 5sec(?))
  Power Spark - Following Malygos and if they are in distance of malygos, starts to buff him

Phase2:
  Nexus Lord - casts Arcane Shock (normal, heroic) on random target, casts haste (not blizzlike timer)
  Scion of Eternity - flying, after Arcane Barrage they go at another place
  Anti magic zones - spawnning at random place, protect player with buff (-50% damage from spells)
  Arcane Pulse - implemented for normal, heroic
  Arcane Storm - implemented for normal, heroic

Phase 3:
  visual effect for destroying platform by malygos
  after malygos destroy platform, raid fall into drakes, drake stats are modified based player's stats [hp = drakeHP + (PlayerHP*2)]
  Static Field   - implemented
  Surge of Power  - implemented for normal, hero

--------------

Need to be implemented:
Rewrite count of Nexus Lord, Scion of Eternity to blizzlike values.
*/

#include "ScriptPCH.h"
#include "eye_of_eternity.h"
#include "WorldPacket.h"
#include "ObjectAccessor.h"


/*
vortex: vehicle 214
flags 1073741827 = 40000003
seats:
2187 flags:
2204 flags:
2205 flags:
2304 flags:
2305 flags:

*/
enum
{
    // ******************************** SPELLS ******************************** //
    SPELL_BERSERK                  = 64238, // Maybe wrong but enought :)
    //////////////// ALL PHASES ////////////////
    SPELL_ARCANE_STORM             = 61693, // AOE - used in all phases (every 10s phase 1, about every 8 in ph3)
    SPELL_ARCANE_STORM_H           = 61694,
    //SPELL_ARCANE_STORM_2           = 57459, // not sure what's that for

    //////////////// PHASE 1 ////////////////
    SPELL_ARCANE_BREATH            = 56272,
    SPELL_ARCANE_BREATH_H          = 60072,
    SPELL_VORTEX_DUMMY             = 56105, // This is for addons, actualy does nothing
    SPELL_VORTEX                   = 56266, // Cast on trigger in middle of the room, this interrupt their cast and deal dmg
    SPELL_VORTEX_AOE_VISUAL        = 55873,    // visual effect around platform
    SPELL_PORTAL_BEAM              = 56046,
    SPELL_POWER_SPARK              = 56152, // If spark reach malygos then buff him, if killed then to players
    SPELL_POWER_SPARK_PLAYERS      = 55852, // This one is propably for players
    SPELL_POWER_SPARK_VISUAL       = 55845,

    //////////////// PHASE 2 ////////////////
    SPELL_ARCANE_OVERLOAD          = 56432, // Cast this on arcane overload NPCs
    SPELL_ARCANE_BOMB              = 56431, // Cast by arcane overload
    SPELL_ARCANE_OVERLOAD_PROTECT  = 56438,
    SPELL_SURGE_OF_POWER_BREATH    = 56505, // omfg, they say deep breath, but its this!
    SPELL_DESTROY_PLATFORM_PRE     = 58842, // lights all over the platform
    SPELL_DESTROY_PLATFROM_BOOM    = 59084, // Big Blue boom
    //NPCs spells
    SPELL_ARCANE_SHOCK             = 57058,
    SPELL_ARCANE_SHOCK_H           = 60073,
    SPELL_HASTE                    = 57060,
    //SPELL_ARCANE_BARRAGE           = 63934, // - normal ... damage to self This one has strange data in DBC 56397
    //SPELL_ARCANE_BARRAGE           = 60071, // found in combat log, but not exist in DBC..maybe from previous patch?
    SPELL_ARCANE_BARRAGE           = 58456, // I have to modify basepoints in this spell...
    BP_BARRAGE0                    = 14138,
    BP_BARRAGE0_H                  = 16965,

    //////////////// PHASE 3 ////////////////
    SPELL_SUMMON_STATIC_FIELD      = 57431, // yet unused -- summons 30592 on target location ... apparently with a very long duration
    SPELL_STATIC_FIELD_MISSILE     = 57430, // yet unused -- cast at 30592
    SPELL_STATIC_FIELD             = 57428, // Summon trigger and cast this on them should be enought
    SPELL_SURGE_OF_POWER           = 57407, // this is on one target
    SPELL_SURGE_OF_POWER_H         = 60936, // this is on unlimited tagets, must limit it in mangos
    //SPELL_SURGE_OF_POWER_DUMMY     = 60939, // maybe used for targetting ?

    SPELL_ARCANE_PULSE             = 57432,
    //Dragons spells
    SPELL_FLAME_SPIKE              = 56091,
    SPELL_ENGULF_IN_FLAMES         = 56092,
    SPELL_REVIVIFY                 = 57090,
    SPELL_LIFE_BURST               = 57143,
    SPELL_FLAME_SHIELD             = 57108,
    SPELL_BLAZING_SPEED            = 57092,
    //////////////// PHASE 4 ////////////////
    //Alexstrasza's Gift Beam 61028
    //Alexstrasza's Gift Visual 61023


    // ******************************** NPCs & GObjects ******************************** //
    //ITEM_KEY_TO_FOCUSING_IRIS      = 44582,
    //ITEM_KEY_TO_FOCUSING_IRIS_H    = 44581,
    //////////////// PHASE 1 ////////////////
    NPC_AOE_TRIGGER                = 22517,
    NPC_VORTEX                     = 30090,
    NPC_POWER_SPARK                = 30084,
    NPC_SPARK_PORTAL               = 30118, // For power sparks
    //VEHICLE_VORTEX                 = 168,
    NPC_SURGE_OF_POWER             = 30334,

    //////////////// PHASE 2 ////////////////
    NPC_HOVER_DISC_0               = 30248, // Maybe wrong, two following NPC flying on them (vehicle) -- NPCS fly on it
    NPC_HOVER_DISC                 = 30234, // Maybe wrong, two following NPC flying on them (vehicle)
    DISPLAY_HOVER_DISC             = 26876, // DisplayID of hover disc
    NPC_NEXUS_LORD                 = 30245, // two (?) of them are spawned on beginning of phase 2
    NPC_SCION_OF_ETERNITY          = 30249, // same, but unknow count
    NPC_ARCANE_OVERLOAD            = 30282, // Bubles
    //GO_PLATFORM                    = 193070,

    //////////////// PHASE 3 ////////////////
    NPC_STATIC_FIELD               = 30592, // Trigger for that spell. Hope its fly

    //////////////// PHASE 4 ////////////////
    NPC_ALEXSTRASZA                = 32295, // The Life-Binder
    GO_ALEXSTRASZAS_GIFT           = 193905, // Loot chest
    GO_ALEXSTRASZAS_GIFT_H         = 193967, // Loot chest

    CHASE_MOTION_TYPE              = 5,

    SAY_INTRO1                     = -1616000,
    SAY_INTRO2                     = -1616001,
    SAY_INTRO3                     = -1616002,
    SAY_INTRO4                     = -1616003,
    SAY_INTRO5                     = -1616004,
    SAY_INTRO_PHASE3               = -1616018,
    SAY_AGGRO1                     = -1616005,
    SAY_AGGRO2                     = -1616013,
    SAY_AGGRO3                     = -1616019,
    SAY_VORTEX                     = -1616006,
    SAY_POWER_SPARK                = -1616035,
    SAY_POWER_SPARK_BUFF           = -1616007,
    SAY_KILL1_1                    = -1616008,
    SAY_KILL1_2                    = -1616009,
    SAY_KILL1_3                    = -1616010,
    SAY_KILL2_1                    = -1616020,
    SAY_KILL2_2                    = -1616021,
    SAY_KILL2_3                    = -1616022,
    SAY_KILL3_1                    = -1616023,
    SAY_KILL3_2                    = -1616024,
    SAY_KILL3_3                    = -1616025,
    SAY_END_PHASE1                 = -1616012,
    SAY_END_PHASE2                 = -1616017,
    SAY_ARCANE_PULSE               = -1616014,
    SAY_ARCANE_PULSE_WARN          = -1616015,
    SAY_ARCANE_OVERLOAD            = -1616016,
    SAY_SURGE_OF_POWER             = -1616026,
    SAY_CAST_SPELL1                = -1616027,
    SAY_CAST_SPELL2                = -1616028,
    SAY_CAST_SPELL3                = -1616029,
    SAY_OUTRO1                     = -1616030,
    SAY_OUTRO2                     = -1616031,
    SAY_OUTRO3                     = -1616032,
    SAY_OUTRO4                     = -1616033,
    SAY_OUTRO5                     = -1616034,

    SHELL_MIN_X                    = 722,
    SHELL_MAX_X                    = 768,
    SHELL_MIN_Y                    = 1290,
    SHELL_MAX_Y                    = 1339,

    NEXUS_LORD_COUNT               = 2,
    NEXUS_LORD_COUNT_H             = 4,
    SCION_OF_ETERNITY_COUNT        = 4,
    SCION_OF_ETERNITY_COUNT_H      = 6,

    PHASE_NOSTART                  = 0,
        SUBPHASE_FLY_DOWN1         = 04,
        SUBPHASE_FLY_DOWN2         = 05,
        SUBPHASE_WAIT              = 06,
    PHASE_FLOOR                    = 1,
        SUBPHASE_VORTEX            = 11,
    PHASE_ADDS                     = 2,
        SUBPHASE_TALK              = 21,
    PHASE_DRAGONS                  = 3,
        SUBPHASE_DESTROY_PLATFORM1 = 31,
        SUBPHASE_DESTROY_PLATFORM2 = 32,
        SUBPHASE_DESTROY_PLATFORM3 = 33,
    PHASE_OUTRO                    = 4,
        SUBPHASE_STOP_COMBAT       = 41,
        SUBPHASE_DIE               = 42,
};

#define SURGE_MAX_TARGETS     RAID_MODE<size_t>(1,3)

struct Locations
{
    float x, y, z, o;
    uint32 id;
};
struct LocationsXY
{
    float x, y;
    uint32 id;
};
static Locations GOPositions[]=
{
    {754.346f, 1300.87f, 256.249f, 3.14159f},   // Raid Platform position
    {754.731f, 1300.12f, 266.171f, 5.01343f},   // Focusing iris and Alexstrazas gift
    {724.684f, 1332.92f, 267.234f, -0.802851f}, // Exit Portal
};
static LocationsXY SparkLoc[]=
{
    {652.417f, 1200.52f},
    {847.67f,  1408.05f},
    {647.675f, 1403.8f},
    {843.182f, 1215.42f},
};

//Also spawn locations for scions of eternity
static LocationsXY VortexLoc[]=
{
    {754, 1311},
    {734, 1334},
    {756, 1339},
    {781, 1329},
    {791, 1311},
    {790, 1283},
    {768, 1264},
    {739, 1261},
    {720, 1280},
    {714, 1299},
    {716, 1318},
    {734, 1334},
    {756, 1339},
    {781, 1329},
    {791, 1311},
    {790, 1283},
    {768, 1264},
    {739, 1261},
    {720, 1280},
    {714, 1299},
    {716, 1318},
};
static Locations OtherLoc[]=
{
    {808, 1301, 298, 0},          // Phase 3 position
    {749, 1244, 332, 1.544f},      // Vortex FarSight loc ... unused
    {754.29f, 1301.18f, 266.17f, 0}, // Center of the platform, ground.
    {823, 1241, 319, 0},          // Alexstrasza's  position
    {749, 1244, 266.17f, 5.33f},       // Aggro position after Subphase fly down
};

#define FLOOR_Z                 268.17f
#define AIR_Z                   297.24f

/*######
## boss_malygos
######*/

class boss_malygos : public CreatureScript
{
public:
    boss_malygos() : CreatureScript("boss_malygos") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_malygosAI(pCreature);
    }

    struct boss_malygosAI : public ScriptedAI
    {
        boss_malygosAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
            m_uiIs10Man = RAID_MODE(true, false);
        }

        InstanceScript* m_pInstance;

        uint64 m_AlexstraszaGUID;

        uint8 m_uiPhase; //Fight Phase
        uint8 m_uiSubPhase; //Subphase if needed
        uint8 m_uiSpeechCount;
        uint8 m_uiVortexPhase;
        std::list<uint64> m_lDiscGUIDList;
        std::list<std::pair<uint64, uint64> > m_uiMounts; // Vehicle(GUID)-Player(GUID) pairs
        std::vector<std::pair<uint64, uint64> > m_uiSurgeTargets; // Vehicle(GUID)-Player(GUID) pairs

        bool m_uiIsMounted;
        bool m_uiIs10Man;

        uint32 m_uiFallToMountTimer;
        uint32 m_uiEnrageTimer;
        uint32 m_uiSpeechTimer[5];
        uint32 m_uiTimer;
        uint32 m_uiVortexTimer;
        uint32 m_uiArcaneBreathTimer;
        uint32 m_uiPowerSparkTimer;
        uint32 m_uiDeepBreathTimer;
        uint32 m_uiShellTimer;
        uint32 m_uiArcaneStormTimer;
        uint32 m_uiStaticFieldTimer;
        uint32 m_uiArcanePulseTimer;
        uint32 m_uiSurgeOfPowerTimer;
        uint32 m_uiCheckDisksTimer;
        uint32 m_uiWipeCheckTimer;

        void Reset()
        {
            if(m_pInstance)
                m_pInstance->SetData(TYPE_MALYGOS, NOT_STARTED);
            else
                me->ForcedDespawn();

            me->SetFlying(true);
            me->AddUnitMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION); // or MOVEMENTFLAG_LEVITATING ?

            m_uiPhase = PHASE_NOSTART;
            m_uiSubPhase = 0;
            m_uiSpeechCount = 0;
            m_uiVortexPhase = 0;
            m_uiMounts.clear();

            m_uiIsMounted = false;

            m_uiFallToMountTimer = 3000;
            m_uiEnrageTimer = 600000;
            m_uiSpeechTimer[0] = 15000;
            m_uiSpeechTimer[1] = 18000;
            m_uiSpeechTimer[2] = 19000;
            m_uiSpeechTimer[3] = 21000;
            m_uiSpeechTimer[4] = 18000;
            m_uiSpeechTimer[5] = 17000;
            m_uiTimer = 7000;
            m_uiVortexTimer = 10000;//60000;
            m_uiArcaneBreathTimer = 15000;
            m_uiPowerSparkTimer = 30000;
            m_uiDeepBreathTimer = 70000;
            m_uiShellTimer = 0;
            m_uiArcaneStormTimer = 15000;
            m_uiStaticFieldTimer = 15000;
            m_uiArcanePulseTimer = 1000;
            m_uiSurgeOfPowerTimer = 30000;
            m_uiCheckDisksTimer = 2500;
            m_uiWipeCheckTimer = 2500;
            m_AlexstraszaGUID = 0;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetSpeed(MOVE_FLIGHT, 3.5f, true);
        }

        void MoveInLineOfSight(Unit* /*who*/)
        {}

        void JustReachedHome()
        {
            Reset();

            //Summon Platform
            if(GameObject* pGo = GetClosestGameObjectWithEntry(me, GO_PLATFORM, 200.0f))
            {
                pGo->Respawn();
                m_pInstance->SetData(TYPE_DESTROY_PLATFORM, NOT_STARTED);
            }
            //Despawn all summoned creatures
            DespawnCreatures(NPC_POWER_SPARK);
            DespawnCreatures(NPC_ARCANE_OVERLOAD);
            DespawnCreatures(NPC_NEXUS_LORD);
            DespawnCreatures(NPC_SCION_OF_ETERNITY);
            DespawnCreatures(NPC_STATIC_FIELD);
            DespawnCreatures(NPC_HOVER_DISC_0);
            DespawnCreatures(NPC_HOVER_DISC);
            me->setActive(false);
        }

        void AttackStart(Unit* pWho)
        {
            if (me->Attack(pWho, true) && (m_uiPhase == PHASE_FLOOR || me->HasAura(SPELL_BERSERK)))
                me->GetMotionMaster()->MoveChase(pWho);
        }

        void EnterCombat(Unit* /*pWho*/)
        {
            //me->SetInCombatWithZone();
            me->setActive(true);
            m_pInstance->SetData(TYPE_MALYGOS, IN_PROGRESS);
            DoScriptText(SAY_AGGRO1, me);

            if(m_pInstance->GetData(TYPE_OUTRO_CHECK) == 1) //Should be enought to trigger outro immediatly
            {
                me->SetFlying(true);
                //Destroy Platform
                CastSpellToTrigger(SPELL_DESTROY_PLATFROM_BOOM, false);
                m_pInstance->SetData(TYPE_DESTROY_PLATFORM, IN_PROGRESS);

                //Mount Players
                PrepareMounts();
                MountPlayers();

                m_uiPhase = PHASE_OUTRO;
                m_uiSubPhase = SUBPHASE_STOP_COMBAT;
            }
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 &uiDamage)
        {
            if (m_uiPhase == PHASE_OUTRO && m_uiSubPhase != SUBPHASE_DIE)
            {
                uiDamage = 0;
                return;
            }

            if (uiDamage >= me->GetHealth() && m_uiSubPhase != SUBPHASE_DIE)
            {
                m_uiPhase = PHASE_OUTRO;
                m_uiSubPhase = SUBPHASE_STOP_COMBAT;
                uiDamage = 0;
            }
        }

        void JustDied(Unit* /*pKiller*/)
        {
            m_pInstance->SetData(TYPE_MALYGOS, DONE);
            m_pInstance->SetData(TYPE_OUTRO_CHECK, 0);
        }

        void KilledUnit(Unit* pVictim)
        {
            uint8 text = 0;
            switch(m_uiPhase)
            {
                case PHASE_FLOOR:
                    text = urand(0, 2);
                    break;
                case PHASE_ADDS:
                    text = urand(3, 5);
                    break;
                case PHASE_DRAGONS:
                    text = urand(6, 8);
                    break;
                default:
                    return;
            }
            switch(text)
            {
                case 0: DoScriptText(SAY_KILL1_1, me); break;
                case 1: DoScriptText(SAY_KILL1_2, me); break;
                case 2: DoScriptText(SAY_KILL1_3, me); break;

                case 3: DoScriptText(SAY_KILL2_1, me); break;
                case 4: DoScriptText(SAY_KILL2_2, me); break;
                case 5: DoScriptText(SAY_KILL2_3, me); break;

                case 6: DoScriptText(SAY_KILL3_1, me); break;
                case 7: DoScriptText(SAY_KILL3_2, me); break;
                case 8: DoScriptText(SAY_KILL3_3, me); break;
            }

            if(m_uiPhase == PHASE_DRAGONS)
                if(pVictim->GetEntry() == NPC_WYRMREST_SKYTALON)
                    for(std::list<std::pair<uint64, uint64> >::iterator iter = m_uiMounts.begin(); iter != m_uiMounts.end(); ++iter)
                        if(pVictim->GetGUID() == (*iter).first)
                        {
                            Unit* pPlayer=Unit::GetUnit(*me, (*iter).second);
                            if (pPlayer && pPlayer->GetTypeId() == TYPEID_PLAYER && pPlayer->isAlive())
                            {
                                pPlayer->GetMotionMaster()->MoveFall(100.0f, EVENT_FALL_GROUND); //mozna nejprve vyhodit z draka? ale die vozidla se zavola pred killedunit ... 
                                pPlayer->ToPlayer()->EnvironmentalDamage(DAMAGE_FALL, pPlayer->GetHealth());
                            }
                            m_uiMounts.erase(iter);
                            return;
                        }

        }

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell)
        {
            if(pSpell->Id == SPELL_POWER_SPARK && m_uiPhase == PHASE_FLOOR)
                DoScriptText(SAY_POWER_SPARK_BUFF, me);
            else if(pSpell->Id == SPELL_POWER_SPARK && m_uiPhase != PHASE_FLOOR)
                me->RemoveAurasDueToSpell(SPELL_POWER_SPARK);
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry * pSpell)
        {
            bool found=false;
            if(pSpell->Id == SPELL_SURGE_OF_POWER_H)
            {
                for(std::vector<std::pair<uint64, uint64> >::iterator iter = m_uiSurgeTargets.begin(); iter != m_uiSurgeTargets.end(); ++iter)
                {
                    Creature* target = (Creature*)Unit::GetUnit(*me, (*iter).first);
                    if (pTarget == target)
                    {
                        found=true;
                        break;
                    }
                }
                if (!found)
                    pTarget->RemoveAurasDueToSpell(SPELL_SURGE_OF_POWER_H);
                    
            }
        }

        void HoverDisk(Position &pos)
        {
            pos.m_positionZ = FLOOR_Z;
            Creature * pDisc = me->SummonCreature(NPC_HOVER_DISC, pos, TEMPSUMMON_DEAD_DESPAWN);
            if (pDisc)
            {
                //pDisc->SetSpeed(MOVE_FLIGHT, 3.5f, true);
                m_lDiscGUIDList.push_back(pDisc->GetGUID());
            }
        }

        void SummonedCreatureDespawn(Creature* pWho)
        {
            uint32 entry = pWho->GetEntry();
            if ((entry == NPC_SCION_OF_ETERNITY) || (entry == NPC_NEXUS_LORD))
            {
                Position pos;
                pWho->GetPosition(&pos);
                HoverDisk(pos);
            }
        }

        void CastSpellToTrigger(uint32 uiSpellId, bool triggered = true, bool triggerCast = false)
        {
            if(Creature *pTrigger = me->SummonCreature(NPC_AOE_TRIGGER, OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z, 0, TEMPSUMMON_TIMED_DESPAWN, 10000))
            { //TODO: use prespawned trigger?
               if(!triggerCast)
               {
                 pTrigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                 pTrigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                 pTrigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                 pTrigger->setFaction(me->getFaction());
                 pTrigger->CastSpell(pTrigger, uiSpellId, triggered);
               }else
               {
                   pTrigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                   DoCast(pTrigger, uiSpellId, triggered);
               }
            }
        }
        
        void DoVortex(uint8 phase)
        {        
            switch(phase)
            {
                case 0: 
                    if(m_pInstance)
                        m_pInstance->SetData(TYPE_VORTEX, 1);
                    me->GetMotionMaster()->Clear(false);
                    //me->HandleEmoteCommand(EMOTE_ONESHOT_FLY_SIT_GROUND_UP);    
                    break;
                case 1: 
                    me->SetFlying(true);
                    me->GetMotionMaster()->MovePoint(0, OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z+25);
                    DoCast(me, SPELL_VORTEX_DUMMY);
                    break;
                case 2:
                    {
                        me->GetMotionMaster()->Clear(false);
                        me->Relocate(OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z+25);
                        me->SendMovementFlagUpdate();
                        Map* pMap = me->GetMap();
                        if(!pMap)
                            return;
        
                        uint8 i=0;
                        Creature* pVortex=NULL;
                        Map::PlayerList const &lPlayers = pMap->GetPlayers();
                        for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                        {
                            if(!itr->getSource()->isAlive())
                                continue;
                            if(i%5==0)
                            {
                                pVortex = me->SummonCreature(NPC_VORTEX, OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z+13+(i/5*2), 0, TEMPSUMMON_TIMED_DESPAWN, 10000);//todo maybe use prespawned vortices?
                                if (!pVortex) 
                                    return;
                                pVortex->CastSpell(pVortex, SPELL_VORTEX_AOE_VISUAL, true);
                            }
                            itr->getSource()->CastSpell(itr->getSource(), SPELL_VORTEX, true, NULL, NULL, me->GetGUID());
                            itr->getSource()->EnterVehicle(pVortex->GetVehicleKit(),i);
                            ++i;
                        }
                    }
                    break;
                case 30:
                //drop players?
                    if(m_pInstance)
                        m_pInstance->SetData(TYPE_VORTEX, 0);
                    break;
                case 31:
                    //me->HandleEmoteCommand(EMOTE_ONESHOT_FLY_SIT_GROUND_DOWN);
                    me->Relocate(OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z);
                    me->SetFlying(false);
                    me->SendMovementFlagUpdate();
                    if(me->getVictim())
                        me->GetMotionMaster()->MoveChase(me->getVictim());
                    m_uiSubPhase = 0;
                    break;                
                default:
                    break;        
            }
        }

        void PowerSpark(uint8 /*action*/)
        {
            if (m_pInstance && m_pInstance->instance)
            {
                Map::PlayerList const &PlayerList = m_pInstance->instance->GetPlayers();
                if (!PlayerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        DoScriptText(/*WHISPER_POWER_SPARK*/SAY_POWER_SPARK, me, i->getSource());
            }
            uint8 random = urand(0, 3);
            if(Creature *pSpark = me->SummonCreature(NPC_POWER_SPARK, SparkLoc[random].x, SparkLoc[random].y, FLOOR_Z+10, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
            {
                pSpark->CastSpell(pSpark, SPELL_POWER_SPARK_VISUAL, false);
            }
        }

        void DoSpawnAdds()
        {
            //Nexus lords
            int max_lords = m_uiIs10Man ? NEXUS_LORD_COUNT :NEXUS_LORD_COUNT_H;
            for(int i=0; i < max_lords;i++)
            {
                if(Creature *pLord = me->SummonCreature(NPC_NEXUS_LORD, me->getVictim()->GetPositionX()-5+rand()%10, me->getVictim()->GetPositionY()-5+rand()%10, me->getVictim()->GetPositionZ(), 0, TEMPSUMMON_CORPSE_DESPAWN, 0))
                {
                    pLord->AI()->AttackStart(me->getVictim());
                    pLord->SetInCombatWithZone();//is it neccessary?
                }
            }
            //Scions of eternity
            int max_scions = m_uiIs10Man ? SCION_OF_ETERNITY_COUNT : SCION_OF_ETERNITY_COUNT_H;
            for(int i=0; i < max_scions;i++)
            {
                uint32 tmp = urand(1, 10);
                if(Creature *pScion = me->SummonCreature(NPC_SCION_OF_ETERNITY, VortexLoc[tmp].x, VortexLoc[tmp].y, FLOOR_Z+10, 0, TEMPSUMMON_CORPSE_DESPAWN, 0))
                {
                    //pScion->SetInCombatWithZone();//is it neccessary?
                    if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                    {
                        pScion->AI()->AttackStart(pTarget);
                    }
                }
            }
        }

        bool IsThereAnyAdd()
        {
            //Search for Nexus lords
            if(GetClosestCreatureWithEntry(me, NPC_NEXUS_LORD, 180.0f))
                return true;

            //Search for Scions of eternity
            if(GetClosestCreatureWithEntry(me, NPC_SCION_OF_ETERNITY, 180.0f))
                return true;

            return false;
        }

        void DoSpawnShell()
        {
            uint32 x = urand(SHELL_MIN_X, SHELL_MAX_X);
            uint32 y = urand(SHELL_MIN_Y, SHELL_MAX_Y);
            if(Creature *pShell = me->SummonCreature(NPC_ARCANE_OVERLOAD, x, y, FLOOR_Z, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 45000))
            {
                pShell->CastSpell(pShell, SPELL_ARCANE_BOMB, false);
            }
        }

        void PrepareMounts()
        {
            Map *pMap = me->GetMap();

            if(!pMap)
                return;

            Map::PlayerList const &lPlayers = pMap->GetPlayers();
            if (lPlayers.isEmpty())
                return;

            for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if(Creature *pTemp = pPlayer->SummonCreature(NPC_WYRMREST_SKYTALON, pPlayer->GetPositionX(), pPlayer->GetPositionY(), 210, 0))
                    {
                        m_uiMounts.push_back(std::pair<uint64, uint64>(pTemp->GetGUID(), pPlayer->GetGUID()));
                    }
                }
            }
        }

        void MountPlayers()
        {
            Map *pMap = me->GetMap();

            if(!pMap)
                return;

            for(std::list<std::pair<uint64, uint64> >::iterator iter = m_uiMounts.begin(); iter != m_uiMounts.end(); ++iter)
            {
                Creature *pTemp = (Creature*)Unit::GetUnit(*me, (*iter).first);
                Player *pPlayer = (Player*)Unit::GetUnit(*me, (*iter).second);

                if(!pTemp)
                    continue;

                if(!pPlayer)
                    continue;

                pTemp->SetCreatorGUID(pPlayer->GetGUID());
                pPlayer->EnterVehicle(pTemp, 0);
                me->AddThreat(pTemp, 1.0f);
                pTemp->SetInCombatWith(me);
                pTemp->SetFacingToObject(me);
            }
        }

        void FillSurgeTargets()
        {
            m_uiSurgeTargets.clear();
            for (std::list<std::pair<uint64, uint64> >::const_iterator itr = m_uiMounts.begin(); itr != m_uiMounts.end(); ++itr)
            {
                m_uiSurgeTargets.push_back(*itr);
            }

            random_shuffle(m_uiSurgeTargets.begin(), m_uiSurgeTargets.end());

            if (m_uiSurgeTargets.size()>SURGE_MAX_TARGETS)
                m_uiSurgeTargets.resize(SURGE_MAX_TARGETS);
        }

        void DespawnCreatures(uint32 entry, float distance=400.0f)
        {
            std::list<Creature*> m_pCreatures;
            GetCreatureListWithEntryInGrid(m_pCreatures, me, entry, distance);

            if (m_pCreatures.empty())
                return;

            for(std::list<Creature*>::iterator iter = m_pCreatures.begin(); iter != m_pCreatures.end(); ++iter)
                (*iter)->ForcedDespawn();
        }
        
        void DoAction(const int32 id)
        {
            if (id==0)
                m_uiSubPhase = SUBPHASE_FLY_DOWN1;
        }

        void MovementInform(uint32, uint32 id)
        {
            switch (id)
            {
            case 1:
                m_uiSubPhase = SUBPHASE_FLY_DOWN2;
                break;
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if(m_uiPhase == PHASE_NOSTART)
            {
            if (m_uiSubPhase == SUBPHASE_WAIT)
                return;

                if(m_uiSubPhase == SUBPHASE_FLY_DOWN1)
                {
                    me->InterruptNonMeleeSpells(false);
                    //me->RemoveAurasDueToSpell(SPELL_PORTAL_BEAM);
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MovePoint(1, OtherLoc[2].x, OtherLoc[2].y, FLOOR_Z+25);
                    m_uiSubPhase = SUBPHASE_WAIT;
                }
                else if(m_uiSubPhase == SUBPHASE_FLY_DOWN2)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    m_uiSubPhase = 0;
                    m_uiPhase = PHASE_FLOOR;
                    me->Relocate(OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z);
                    me->SetFlying(false);
                    me->SendMovementFlagUpdate();
                    me->SetInCombatWithZone();
                    return;
                }
                else
                {
                    //Speech
                    if(m_uiSpeechTimer[m_uiSpeechCount] <= uiDiff)
                    {
                        DoScriptText(SAY_INTRO1-m_uiSpeechCount, me);
                        m_uiSpeechCount++;
                        if(m_uiSpeechCount == 5)
                        {
                            m_uiSpeechCount = 0;
                            m_uiSpeechTimer[0] = 15000;
                            m_uiSpeechTimer[1] = 18000;
                            m_uiSpeechTimer[2] = 19000;
                            m_uiSpeechTimer[3] = 21000;
                            m_uiSpeechTimer[4] = 18000;
                            m_uiSpeechTimer[5] = 17000;
                        }
                    }else m_uiSpeechTimer[m_uiSpeechCount] -= uiDiff;
                    //Random movement over platform
                    if(m_uiTimer <= uiDiff)
                    {
                        uint8 tmp = urand(0,3);
                        me->GetMotionMaster()->MovePoint(0, SparkLoc[tmp].x, SparkLoc[tmp].y, AIR_Z);
                        m_uiTimer = 25000;
                    }else m_uiTimer -= uiDiff;
                }
            }
            if (m_uiSubPhase==0 && !UpdateVictim())// TODO upravit
                return;

            //Enrage timer.....
            if(m_uiEnrageTimer <= uiDiff)
            {
                DoCast(me, SPELL_BERSERK);
                m_uiEnrageTimer = 600000;
                me->SetSpeed(MOVE_FLIGHT, 3.5f, true);
                me->SetSpeed(MOVE_RUN, 3.5f, true);
                me->GetMotionMaster()->MoveChase(me->getVictim());
            }else m_uiEnrageTimer -= uiDiff;

            if(m_uiPhase == PHASE_FLOOR)
            {
                if(m_uiSubPhase == SUBPHASE_VORTEX)
                {
                    if(m_uiTimer <= uiDiff)
                    {//TODO move stuff from here into DoVortex 
                        DoVortex(m_uiVortexPhase);
                        m_uiTimer = 500;
                        m_uiVortexPhase++;
                    }else m_uiTimer -= uiDiff;
                    return;
                }

                //Vortex
                if(m_uiVortexTimer <= uiDiff)
                {
                    DoVortex(0);
                    m_uiVortexPhase = 1;
                    m_uiSubPhase = SUBPHASE_VORTEX;
                    m_uiVortexTimer = 56000;
                    m_uiTimer = 6000;
                    DoScriptText(SAY_VORTEX, me);
                    return;
                }else m_uiVortexTimer -= uiDiff;

                if(m_uiArcaneStormTimer <= uiDiff)
                {
                    DoCast(me, m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H);
                    //me->CastCustomSpell(m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H, SPELLVALUE_MAX_TARGETS, m_uiIs10Man(5,10));
                    m_uiArcaneStormTimer = 10000;
                }else m_uiArcaneStormTimer -= uiDiff;

                //Arcane Breath
                if(m_uiArcaneBreathTimer <= uiDiff)
                {
                    DoCast(me, m_uiIs10Man ? SPELL_ARCANE_BREATH : SPELL_ARCANE_BREATH_H);
                    m_uiArcaneBreathTimer = 10000 + urand(2000, 8000);
                }else m_uiArcaneBreathTimer -= uiDiff;

                //PowerSpark
                if(m_uiPowerSparkTimer<= uiDiff)
                {
                    PowerSpark(1);
                    m_uiPowerSparkTimer = 30000;
                }else m_uiPowerSparkTimer -= uiDiff;

                //Health check
                if(m_uiTimer<= uiDiff)
                {
                    uint8 health = me->GetHealth()*100 / me->GetMaxHealth();
                    if(health <= 50) // !me->IsNonMeleeSpellCasted(false) ?
                    {
                        me->InterruptNonMeleeSpells(true);
                        me->GetMotionMaster()->Clear(false);
                        me->SetFlying(true);
                        DoScriptText(SAY_END_PHASE1, me);
                        me->GetMotionMaster()->MovePoint(0, OtherLoc[2].x, OtherLoc[2].y, OtherLoc[2].z+40);
                        //Despawn power sparks
                        DespawnCreatures(NPC_POWER_SPARK);
                        m_uiPhase = PHASE_ADDS;
                        m_uiSubPhase = SUBPHASE_TALK;
                        m_uiTimer = 23000;
                        return;
                    }
                    m_uiTimer = 1500;
                }else m_uiTimer -= uiDiff;

                DoMeleeAttackIfReady();
            }else if(m_uiPhase == PHASE_ADDS)
            {
                if(m_uiSubPhase == SUBPHASE_TALK)
                {
                    if(m_uiTimer <= uiDiff)
                    {
                        DoScriptText(SAY_AGGRO2, me);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        DoSpawnAdds();
                        DoSpawnShell();
                        m_uiShellTimer = 20000;
                        m_uiSubPhase = 0;
                        m_uiTimer = 15000;
                    }else m_uiTimer -= uiDiff;
                    return;
                }

                //Arcane overload (bubble)
                if(m_uiShellTimer <= uiDiff)
                {
                    DoSpawnShell();
                    DoScriptText(SAY_ARCANE_OVERLOAD, me);
                    m_uiShellTimer = 20000;
                }else m_uiShellTimer -= uiDiff;

                // Deep breath
                if(m_uiDeepBreathTimer <= uiDiff)
                {
                    DoScriptText(SAY_ARCANE_PULSE, me);
                    DoScriptText(SAY_ARCANE_PULSE_WARN, me);
                    DoCast(SPELL_SURGE_OF_POWER_BREATH); // should not hit players on discs :-(
                    m_uiDeepBreathTimer = 60000;
                }else m_uiDeepBreathTimer -= uiDiff;

                // Arcane Storm
                if(m_uiArcaneStormTimer <= uiDiff)
                {
                    DoCast(me, m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H);
                    //me->CastCustomSpell(m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H, SPELLVALUE_MAX_TARGETS, m_uiIs10Man(5,10));
                    m_uiArcaneStormTimer = 20000;
                }else m_uiArcaneStormTimer -= uiDiff;

                if(m_uiTimer <= uiDiff)
                {
                    if(!IsThereAnyAdd() && !me->IsNonMeleeSpellCasted(false))
                    {
                        if(m_pInstance)
                            m_pInstance->SetData(TYPE_PLAYER_HOVER, DATA_DROP_PLAYERS);
                        DespawnCreatures(NPC_HOVER_DISC_0);
                        DespawnCreatures(NPC_HOVER_DISC);
                        m_uiPhase = PHASE_DRAGONS;
                        m_uiSubPhase = SUBPHASE_DESTROY_PLATFORM1;
                        DoScriptText(SAY_END_PHASE2, me);
                        CastSpellToTrigger(SPELL_DESTROY_PLATFORM_PRE, false);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        m_uiTimer = 6500;
                        return;
                    }
                    m_uiTimer = 5000;
                }else m_uiTimer -= uiDiff;

                if(me->HasAura(SPELL_BERSERK))
                    DoMeleeAttackIfReady(); // this is there just for case of enrage
            }
            else if(m_uiPhase == PHASE_DRAGONS)
            {
                if(m_uiSubPhase == SUBPHASE_DESTROY_PLATFORM1)
                {
                    if(m_uiTimer<= uiDiff)
                    {
                        //Destroy Platform
                        CastSpellToTrigger(SPELL_DESTROY_PLATFROM_BOOM, false);
                        //Despawn bubbles
                        DespawnCreatures(NPC_ARCANE_OVERLOAD);

                        m_uiTimer = 2000;
                        m_uiSubPhase = SUBPHASE_DESTROY_PLATFORM2;
                    }else m_uiTimer -= uiDiff;
                    return;
                }
                else if(m_uiSubPhase == SUBPHASE_DESTROY_PLATFORM2)
                {
                    if(m_uiTimer <= uiDiff)
                    {
                        m_pInstance->SetData(TYPE_DESTROY_PLATFORM, IN_PROGRESS);
                        //Prepare Mounts
                        DoResetThreat(); //drakes need to have more threat than players, otherwise boss will evade
                        PrepareMounts();
                        me->GetMotionMaster()->MovePoint(0, OtherLoc[0].x, OtherLoc[0].y, OtherLoc[0].z);
                        DoScriptText(SAY_INTRO_PHASE3, me);
                        m_uiTimer = 14900;
                        m_uiFallToMountTimer = 1000;
                    }else
                        m_uiTimer -= uiDiff;

                    if(!m_uiIsMounted && m_uiFallToMountTimer <= uiDiff)
                    {
                        MountPlayers();
                        m_uiSubPhase = SUBPHASE_DESTROY_PLATFORM3;
                        m_uiIsMounted = true;
                    }else
                        m_uiFallToMountTimer -= uiDiff;
                    return;

                }else if(m_uiSubPhase == SUBPHASE_DESTROY_PLATFORM3)
                {
                    if(m_uiTimer<= uiDiff)
                    {
                        m_uiSubPhase = 0;
                        me->GetMotionMaster()->Clear(false);        // No moving!
                        me->GetMotionMaster()->MoveIdle();
                        DoScriptText(SAY_AGGRO3, me);
                    }else m_uiTimer -= uiDiff;
                    return;
                }
                //Arcane Pulse
                if(m_uiArcanePulseTimer <= uiDiff)
                {
                    DoCast(me, SPELL_ARCANE_PULSE);
                    m_uiArcanePulseTimer = 1000;
                }else m_uiArcanePulseTimer -= uiDiff;

                if(m_uiArcaneStormTimer <= uiDiff)
                {
                    DoCast(me, m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H);
                    //me->CastCustomSpell(m_uiIs10Man ? SPELL_ARCANE_STORM : SPELL_ARCANE_STORM_H, SPELLVALUE_MAX_TARGETS, m_uiIs10Man(5,10));
                    m_uiArcaneStormTimer = 10000;
                }else m_uiArcaneStormTimer -= uiDiff;

                //Static field
                if(m_uiStaticFieldTimer <= uiDiff)
                {
                    if(Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        if(Creature *pField = me->SummonCreature(NPC_STATIC_FIELD, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 25000))
                        {
                            pField->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                            pField->Attack(pTarget, false); //maybe change to docombatwithzone ?
                            pField->CastSpell(pField, SPELL_STATIC_FIELD, true);
                        }
                    DoScriptText(SAY_CAST_SPELL1-urand(0,2), me);
                    m_uiStaticFieldTimer = 10000+rand()%10000;
                }else m_uiStaticFieldTimer -= uiDiff;

                //Surge of power
                if(m_uiSurgeOfPowerTimer <= uiDiff)
                {
                    FillSurgeTargets();
                    Creature* pTarget = NULL;
                    for(std::vector<std::pair<uint64, uint64> >::iterator iter = m_uiSurgeTargets.begin(); iter != m_uiSurgeTargets.end(); ++iter)
                    {
                        pTarget = (Creature*)Unit::GetUnit(*me, (*iter).first);
                        Player* pPlayer = (Player*)Unit::GetUnit(*me, (*iter).second);
                        if (!pTarget || !pPlayer) return; // just in case ... perhaps if player gets disconnected or somehow ports out
                        me->MonsterWhisper("Malygos fixes his eyes on you!", pPlayer->GetGUID(), true);
                    }
                    DoScriptText(SAY_SURGE_OF_POWER, me);
                    DoCast(pTarget, m_uiIs10Man ? SPELL_SURGE_OF_POWER : SPELL_SURGE_OF_POWER_H); //in fact, on 25men the spell is cast on self

                    m_uiSurgeOfPowerTimer = 9000+rand()%6000;
                }else m_uiSurgeOfPowerTimer -= uiDiff;

                if(me->HasAura(SPELL_BERSERK))
                    DoMeleeAttackIfReady();
            }
            //Outro!
            else if(m_uiPhase == PHASE_OUTRO)
            {
                if(m_uiSubPhase == SUBPHASE_STOP_COMBAT)
                {
                    m_pInstance->SetData(TYPE_OUTRO_CHECK, 1);

                    if (me->IsNonMeleeSpellCasted(false))
                        me->InterruptNonMeleeSpells(false);

                    me->RemoveAllAuras();

                    DespawnCreatures(NPC_STATIC_FIELD);

                    me->SetHealth(1);
                    //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);//??blbost :)
                    me->GetMotionMaster()->Clear(false);        // No moving!
                    me->GetMotionMaster()->MoveIdle();
                    m_uiSpeechCount = 0;
                    m_uiSpeechTimer[0] = 2000;
                    m_uiSpeechTimer[1] = 8500;
                    m_uiSpeechTimer[2] = 5000;
                    m_uiSpeechTimer[3] = 3000;
                    m_uiSpeechTimer[4] = 22000;

                    if(Creature *pTemp = me->SummonCreature(NPC_ALEXSTRASZA, OtherLoc[3].x, OtherLoc[3].y, OtherLoc[3].z, 0, TEMPSUMMON_CORPSE_DESPAWN, 0))//maybe used prespawned trogger for loc ... ?
                    {
                        pTemp->SetFlying(true);
                        pTemp->SetFacingToObject(me);
                        pTemp->SetVisible(false);
                        m_AlexstraszaGUID = pTemp->GetGUID();
                    }
                    m_uiSubPhase = 0;
                    return;
                }
                if(m_uiSpeechCount >= 5 || m_uiSubPhase == SUBPHASE_DIE)
                    return;

                if(m_uiSpeechTimer[m_uiSpeechCount] <= uiDiff)
                {
                    Creature* pAlexstrasza = (Creature*)Unit::GetUnit(*me, m_AlexstraszaGUID);

                    switch(m_uiSpeechCount)
                    {
                        case 1:
                            pAlexstrasza->SetVisible(true);
                            pAlexstrasza->SetFacingToObject(me->getVictim());
                            break;
                        case 4:
                            m_uiSubPhase = SUBPHASE_DIE;
                            if(GameObject *pGift = pAlexstrasza->SummonGameObject( m_uiIs10Man ? GO_ALEXSTRASZAS_GIFT : GO_ALEXSTRASZAS_GIFT_H, GOPositions[1].x, GOPositions[1].y, GOPositions[1].z+4, GOPositions[2].o, 0, 0, 0, 0, 0))
                                pAlexstrasza->SetFacingToObject(pGift);

                            //Summon platform for the looters to stay on...
                            //me->SummonGameObject( 190023 , 757.0f, 1297.4f, 267.09f, 2.26684f, 0, 0, 0.905866f, 0.423563f, 600000);

                            Player* pPlayer = GetPlayerAtMinimumRange(1.0f);
                            if(pPlayer)
                                pPlayer->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                            else
                                me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                            break;
                    }
                    if(m_uiSpeechCount || pAlexstrasza)
                        DoScriptText(SAY_OUTRO1-m_uiSpeechCount, ( m_uiSpeechCount == 0 ) ? me : pAlexstrasza);

                    m_uiSpeechCount++;
                }else m_uiSpeechTimer[m_uiSpeechCount] -= uiDiff;
            }
        }
    };
};

class mob_power_spark : public CreatureScript
{
public:
    mob_power_spark() : CreatureScript("mob_power_spark") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_power_sparkAI(pCreature);
    }

    struct mob_power_sparkAI : public ScriptedAI
    {
        mob_power_sparkAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        bool isDead;
        uint32 m_uiCheckTimer;
        uint64 pMalygosGUID;

        void Reset()
        {
            isDead = false;

            if(Creature* pMalygos = GetClosestCreatureWithEntry(me, NPC_MALYGOS, 200.0f))
            {
                pMalygosGUID = pMalygos->GetGUID();
            }else pMalygosGUID = 0;

            m_uiCheckTimer = 2500;
            me->SetSpeed(MOVE_RUN, 1.0f);
        }

        void MoveInLineOfSight(Unit* /*pWho*/) {}

        void DamageTaken(Unit* /*pDoneBy*/, uint32 &uiDamage)
        {
            if (isDead)
            {
                uiDamage = 0;
                return;
            }

            if (uiDamage > me->GetHealth())
            {
                isDead = true;

                if (me->IsNonMeleeSpellCasted(false))
                    me->InterruptNonMeleeSpells(false);

                me->RemoveAllAuras();
                me->AttackStop();

                me->GetMotionMaster()->MovementExpired();

                uiDamage = 0;
                m_uiCheckTimer = 250;
                me->SetHealth(1);
                me->AddAura(SPELL_POWER_SPARK_PLAYERS, me);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                me->ForcedDespawn(60000);
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if(isDead)
                return;

            if(m_uiCheckTimer <= uiDiff)
            {
                if(m_pInstance && m_pInstance->GetData(TYPE_VORTEX))
                {
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == TARGETED_MOTION_TYPE)
                    {
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->MoveIdle();
                    }
                    return;
                }

                if(Creature* pMalygos = (Creature*)Unit::GetUnit(*me, pMalygosGUID))
                {
                    if(!pMalygos->isAlive())
                    {
                        me->ForcedDespawn();
                        return;
                    }

                    if(me->IsWithinDist3d(pMalygos->GetPositionX(), pMalygos->GetPositionY(), pMalygos->GetPositionZ(), 5.0f))
                    {
                        pMalygos->AddAura(SPELL_POWER_SPARK, pMalygos);
                        me->ForcedDespawn();
                    }else
                    {
                        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() < MAX_DB_MOTION_TYPE)
                            me->GetMotionMaster()->MoveFollow(pMalygos, me->GetObjectSize() - pMalygos->GetObjectSize(),0.0f);
                    }
                }
                m_uiCheckTimer = 2000;
            }else m_uiCheckTimer -= uiDiff;
        }
    };
};

class mob_scion_of_eternity : public CreatureScript
{
public:
    mob_scion_of_eternity() : CreatureScript("mob_scion_of_eternity") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_scion_of_eternityAI(pCreature);
    }

    struct mob_scion_of_eternityAI : public ScriptedAI
    {
        mob_scion_of_eternityAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        uint32 m_uiArcaneBarrageTimer;
        uint32 m_uiMoveTimer;
        uint8 m_uiMovePoint;
        Creature* m_pDisc;

        void Reset()
        {
            m_pDisc=me->SummonCreature(NPC_HOVER_DISC_0,0,0,0,0,TEMPSUMMON_DEAD_DESPAWN);
            me->EnterVehicle(m_pDisc);
            m_pDisc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);            
            DoNextMovement();
            m_uiMovePoint = 0;
            m_uiMoveTimer = 10000;
            m_uiArcaneBarrageTimer = 5000 + rand()%15000;
        }

        void JustDied(Unit* /*pKiller*/)
        {
            m_pDisc->ForcedDespawn();
        }
        
        void AttackStart(Unit *pWho)
        {
            if(pWho->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->Attack(pWho, false))
            {
                me->SetInCombatWithZone();//is it neccessary?
            }
        }

        void DoNextMovement()
        {
            me->SendMovementFlagUpdate();
            m_uiMovePoint++;
            //AFAIK it should NOT be random
            uint32 x = urand(SHELL_MIN_X, SHELL_MAX_X);
            uint32 y = urand(SHELL_MIN_Y, SHELL_MAX_Y);
            m_pDisc->GetMotionMaster()->MovePoint(m_uiMovePoint, x, y, FLOOR_Z+10);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if(m_uiMoveTimer <= uiDiff) // -> movementinform
            {
                m_uiMoveTimer = 10000;
                DoNextMovement();
            }else m_uiMoveTimer -= uiDiff;

            if(m_uiArcaneBarrageTimer <= uiDiff)
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                {
                    if (pTarget->GetVehicle())
                        return;

                    int32 bpoints0 = RAID_MODE(int32(BP_BARRAGE0), int32(BP_BARRAGE0_H));
                    me->CastCustomSpell(pTarget, SPELL_ARCANE_BARRAGE, &bpoints0, 0, 0, false);
                }
                m_uiArcaneBarrageTimer = 3000 + rand()%7000;
            }else m_uiArcaneBarrageTimer -= uiDiff;
        }
    };
};


class npc_arcane_overload : public CreatureScript
{
public:
    npc_arcane_overload() : CreatureScript("npc_arcane_overload") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_arcane_overloadAI(pCreature);
    }

    struct npc_arcane_overloadAI : public ScriptedAI
    {
        npc_arcane_overloadAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;

        uint32 m_uiProtectTimer;
        float range;

        void Reset()
        {
            m_uiProtectTimer = 1000;
            DoCast(me,SPELL_ARCANE_OVERLOAD);
            range = 50.0f;
        }

        void ProtectAllPlayersInRange(float range)//maybe better use spellhittarget
        {
          Map* pMap = me->GetMap();
          if(!pMap)
            return;

          // The range of the bubble is 12 yards, decreases to 0 yards, linearly, over time.
          float realRange = (range/50.0f) * 12.0f;
          Map::PlayerList const &lPlayers = pMap->GetPlayers();
          for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
          {
            if(!itr->getSource()->isAlive())
              continue;

            if(!itr->getSource()->IsWithinDist2d(me->GetPositionX(), me->GetPositionY(), realRange))
            {
                // Remove aura if is within 12 yards (So got the aura via it's default aura range)
                if(itr->getSource()->IsWithinDist2d(me->GetPositionX(), me->GetPositionY(), 12))
                    itr->getSource()->RemoveAurasDueToSpell(SPELL_ARCANE_OVERLOAD_PROTECT,me->GetGUID());
            }
          }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if(m_uiProtectTimer <= uiDiff)
            {
                ProtectAllPlayersInRange(range);
                range -= 0.5f;
                m_uiProtectTimer+=250;
                if (m_uiProtectTimer < uiDiff)
                    m_uiProtectTimer = uiDiff;
            }
            m_uiProtectTimer -= uiDiff;
        }
    };
};


class mob_nexus_lord : public CreatureScript
{
public:
    mob_nexus_lord() : CreatureScript("mob_nexus_lord") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_nexus_lordAI(pCreature);
    }

    struct mob_nexus_lordAI : public ScriptedAI
    {
        mob_nexus_lordAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
            m_uiIs10Man = RAID_MODE(true, false);
        }

        InstanceScript* m_pInstance;

        bool m_uiIs10Man;

        uint32 m_uiArcaneShockTimer;
        uint32 m_uiHasteTimer;
        uint32 attackNow;

        void Reset()
        {
            m_uiHasteTimer = 20000;
            m_uiArcaneShockTimer = 5000 + rand()%15000;
            attackNow = 4000;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if(m_uiArcaneShockTimer <= uiDiff)
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                    DoCast(pTarget, m_uiIs10Man ? SPELL_ARCANE_SHOCK : SPELL_ARCANE_SHOCK_H);
                m_uiArcaneShockTimer = 3000 + rand()%19000;
            }else m_uiArcaneShockTimer -= uiDiff;

            if(m_uiHasteTimer <= uiDiff)
            {
              DoCast(me, SPELL_HASTE);
              m_uiHasteTimer = 10000 + rand()%10000;
            }else m_uiHasteTimer -= uiDiff;

            if(attackNow)
            {
                if(attackNow < uiDiff)
                    attackNow = 0;
                else attackNow -= uiDiff;
            }

            if(!attackNow)
                DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_malygos()
{
    new boss_malygos();
    new mob_power_spark();
    new mob_scion_of_eternity();
    new mob_nexus_lord();
    new npc_arcane_overload();
}
