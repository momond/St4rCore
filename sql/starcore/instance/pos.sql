-- insert martin victus + bnerin posisi jd wowheadlike Koornya di 65 55 deket garfrost

SET @GUID :=300000;
DELETE FROM creature WHERE id IN (37591,37592);
INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) VALUES (GUID+0, 37592, 658, 3, 1, 30697, 0, 695.220, -160.275, 528.061, 4.712, 86400, 0, 0, 126000, 0, 0, 0, 0, 0, 0);

-- Set si victus supaya gk agro dll
UPDATE `creature_template` SET `unit_flags`=320 WHERE `entry`=37591 LIMIT 1;