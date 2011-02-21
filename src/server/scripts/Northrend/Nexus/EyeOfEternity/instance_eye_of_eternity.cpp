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

#include "ScriptPCH.h"
#include "eye_of_eternity.h"
#include "WorldPacket.h"

enum
{
    EVENT_OPEN_IRIS                 = 20711,
    SPELL_IRIS_OPENED               = 61012,
};

class instance_eye_of_eternity : public InstanceMapScript
{
public:
    instance_eye_of_eternity() : InstanceMapScript("instance_eye_of_eternity", 616) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_eye_of_eternity_InstanceMapScript(pMap);
    }

    struct instance_eye_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_eye_of_eternity_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {Initialize();}

        std::string strInstData;
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 m_uiOutroCheck;
        uint32 m_uiMalygosPlatformData;

        uint64 m_uiMalygosPlatformGUID;
        uint64 m_uiFocusingIrisGUID;
        uint64 m_uiExitPortalGUID;

        uint64 m_uiMalygosGUID;
        uint64 m_uiPlayerCheckGUID;

        bool m_bVortex;


        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            m_uiMalygosGUID = 0;
            m_uiOutroCheck = 0;
            m_uiMalygosPlatformData = 0;
            m_uiMalygosPlatformGUID = 0;
            m_uiFocusingIrisGUID = 0;
            m_uiExitPortalGUID = 0;
            m_uiPlayerCheckGUID = 0;
            m_bVortex = false;
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            switch(pCreature->GetEntry())
            {
                case NPC_MALYGOS:
                    m_uiMalygosGUID = pCreature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject *pGo)
        {
            switch(pGo->GetEntry())
            {
                case GO_PLATFORM: m_uiMalygosPlatformGUID = pGo->GetGUID(); break;
                case GO_FOCUSING_IRIS: //normal, hero
                case GO_FOCUSING_IRIS_H: m_uiFocusingIrisGUID = pGo->GetGUID(); break;
                case GO_EXIT_PORTAL: m_uiExitPortalGUID = pGo->GetGUID(); break;
                default:
                    break;
            }
        }

        bool IsEncounterInProgress() const
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;

            return false;
        }

        void SetData(uint32 uiType, uint32 uiData)
        {
            switch(uiType)
            {
                case TYPE_MALYGOS:
                    if(uiData == IN_PROGRESS)
                    {
                        if(GameObject* m_uiExitPortal = instance->GetGameObject(m_uiExitPortalGUID))
                            m_uiExitPortal->SetRespawnTime(3600); //+UpdateObjectVisibility(); ?
                        if(GameObject* m_uiFocusingIris = instance->GetGameObject(m_uiFocusingIrisGUID))
                            m_uiFocusingIris->SetRespawnTime(3600); //+UpdateObjectVisibility(); ?
                    }
                    if (uiData == NOT_STARTED)
                    {
                        if(GameObject* m_uiExitPortal = instance->GetGameObject(m_uiExitPortalGUID))
                            m_uiExitPortal->Respawn();
                        if(GameObject* m_uiFocusingIris = instance->GetGameObject(m_uiFocusingIrisGUID))
                            m_uiFocusingIris->Respawn();
                    }
                    if (uiData == DONE)
                    {
                        if(GameObject* m_uiExitPortal = instance->GetGameObject(m_uiExitPortalGUID))
                            m_uiExitPortal->Respawn();
                    }
                    m_auiEncounter[0] = uiData;
                    break;
                case TYPE_OUTRO_CHECK:
                    m_uiOutroCheck = uiData;
                    break;
                case TYPE_DESTROY_PLATFORM:
                    if(uiData == IN_PROGRESS)
                    {
                        if(GameObject* m_uiMalygosPlatform = instance->GetGameObject(m_uiMalygosPlatformGUID))
                            m_uiMalygosPlatform->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    }
                    else if(uiData == NOT_STARTED)
                    {
                        if(GameObject* m_uiMalygosPlatform = instance->GetGameObject(m_uiMalygosPlatformGUID))
                        {
                            m_uiMalygosPlatform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                            m_uiMalygosPlatform->Respawn();
                        }
                    }
                    m_uiMalygosPlatformData = uiData;
                    break;
                case TYPE_VORTEX:
                    if(uiData)
                        m_bVortex = true;
                    else
                        m_bVortex = false;
                    break;
                case TYPE_PLAYER_HOVER:
                    if(uiData == DATA_DROP_PLAYERS)
                        dropAllPlayers();
                    break;
            }
        }

        const char* Save()
        {
            OUT_SAVE_INST_DATA;
            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0] << " " << m_uiOutroCheck;

            strInstData = saveStream.str();
            SaveToDB();
            OUT_SAVE_INST_DATA_COMPLETE;
            return strInstData.c_str();
        }

        void Load(const char* chrIn)
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_uiOutroCheck;

            for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        uint32 GetData(uint32 uiType)
        {
            switch(uiType)
            {
                case TYPE_MALYGOS:
                    return m_auiEncounter[0];
                case TYPE_OUTRO_CHECK:
                    return m_uiOutroCheck;
                case TYPE_DESTROY_PLATFORM:
                    return m_uiMalygosPlatformData;
                case TYPE_VORTEX:
                    return m_bVortex;
            }
            return 0;
        }

        uint64 GetData64(uint32 uiData)
        {
            switch(uiData)
            {
                case NPC_MALYGOS:
                    return m_uiMalygosGUID;
                default:
                    return 0;
            }
        }

        void dropAllPlayers()
        {
            Map::PlayerList const &PlayerList = instance->GetPlayers();

            if (!PlayerList.isEmpty())
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    if (Player *pPlayer = i->getSource())
                        if (Unit* pDisk = pPlayer->GetVehicleBase())
                        {
                            pPlayer->ExitVehicle();
                            pDisk->ToCreature()->ForcedDespawn();
                        }
        }

        void OnPlayerEnter(Player* pPlayer)
        {
            if(GetData(TYPE_MALYGOS) == DONE)
            {
                Creature *pTemp = pPlayer->SummonCreature(NPC_WYRMREST_SKYTALON, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ() - 5, 0);
                if(pTemp)
                {
                    pTemp->SetCreatorGUID(pPlayer->GetGUID());
                    pPlayer->EnterVehicle(pTemp, 0);
                }
            }
        }
 
        void ProcessEvent(GameObject* pGO, uint32 uiEventId)
        {
            switch(uiEventId)
            {
                case EVENT_OPEN_IRIS:
                    if (Creature* Malygos = instance->GetCreature(m_uiMalygosGUID))
                    {
                        Malygos->AI()->DoAction(0);
                        pGO->CastSpell(NULL,SPELL_IRIS_OPENED);                        
                    }
                    break;
            }
        } 
    };
};

void AddSC_instance_eye_of_eternity()
{
    new instance_eye_of_eternity();
}