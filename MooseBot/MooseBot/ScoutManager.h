/*
Source code by Adam Montgomerie.
Distributed under GPL v3, see LICENSE for details.
*/

#pragma once
#include <BWAPI.h>
#include <BWTA.h>

class ScoutManager
{
	BWAPI::Unit* scout;
	std::set<BWAPI::Position> knownEnemyBases;
	std::set<std::pair<BWAPI::Unit*, BWAPI::UnitType>> knownEnemyUnits;
	int enemyStaticD;
	int enemyArmySupply;
	int enemyBaseCount;
	bool enemyCloak;
public:
	ScoutManager(void);
	void ScoutManager::update();
	void ScoutManager::setScout(BWAPI::Unit* scout);
	BWAPI::Unit* ScoutManager::getScout();
	void ScoutManager::sendScout();
	void ScoutManager::addEnemyBase(BWAPI::Unit* enemyBase);
	void ScoutManager::removeEnemyBase(BWAPI::Unit* enemyBase);
	BWAPI::Position ScoutManager::getClosestEnemyBase(BWAPI::Unit* unit);
	BWAPI::Position ScoutManager::getEnemyBase();
	void ScoutManager::addEnemyUnit(BWAPI::Unit* unit);
	void ScoutManager::removeEnemyUnit(BWAPI::Unit* unit);
	std::set<std::pair<BWAPI::UnitType, int>> ScoutManager::getEnemyComposition();
	bool ScoutManager::enemyHasCloak();
	int ScoutManager::getEnemyArmySupply();
	void ScoutManager::addEnemyStaticD();
	void ScoutManager::removeEnemyStaticD();
	int ScoutManager::getTotalEnemyStaticD();
	void ScoutManager::scoutExpos();
private:
	void ScoutManager::calculateEnemyArmySupply();
};