-- ke Pos
UPDATE `creature_template` SET `faction_A` = 534,`faction_H` = 534 WHERE `entry` in (37597); 

-- bebasin slave horde
UPDATE quest_template SET ReqCreatureOrGOId1 = -201969 WHERE entry = 24507;

-- slave ally
UPDATE quest_template SET ReqCreatureOrGOId1 = -201969 WHERE entry = 24498;

