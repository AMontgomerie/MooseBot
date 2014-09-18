#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <cstdlib>
#include <ctime>
/*
#include "..\..\SparCraft1\source\GameState.h"
#include "..\..\SparCraft1\source\Game.h"
#include "..\..\SparCraft1\source\Unit.h"
#include "..\..\SparCraft1\source\AllPlayers.h"
#include "..\..\SparCraft1\source\Player_Kiter_NOKDPS.h"
*/

#define REGROUPDIST 250	// the pixel distance a unit must be within to count as having arrived at the regroup location
#define MAXREGROUPTIME 720	//the frame count that we are willing to wait for units to arrive at the regroup location

class ArmyManager
{
	std::set<std::pair<BWAPI::Unit*, int>> allArmy;
	std::set<BWAPI::Unit*> regroupingUnits;
	std::set<BWAPI::Unit*> visibleEnemies;
	std::set<BWAPI::Unit*> combatWorkers;
	BWAPI::Position attackPosition;
	BWAPI::Position defendPosition;
	BWAPI::Position rallyPoint;
	BWAPI::Position regroupPosition;
//	SparCraft::Map map;
	int armyStatus;
	bool regroupOrdered;
	bool attackIssued;
	bool retreatIssued;
	int regroupFrame;
	int enemyArmySupply;
public:
	ArmyManager(void);
	void ArmyManager::update();
	BWAPI::Unit* ArmyManager::getClosestEnemy(BWAPI::Unit* unit);
	BWAPI::Unit* ArmyManager::getClosestEnemy(BWAPI::Position position);
	void ArmyManager::addUnit(BWAPI::Unit* unit);
	void ArmyManager::removeUnit(BWAPI::Unit* unit);
	int ArmyManager::getUnitCount();
	int ArmyManager::getArmySupply();
	int ArmyManager::getUnitCount(BWAPI::UnitType unitType);
	void ArmyManager::allAttack(BWAPI::Position position);
	void ArmyManager::allAttack(BWAPI::Unit* target);
	void ArmyManager::allMove(BWAPI::Position position);
	BWAPI::Unit* ArmyManager::getUnit(BWAPI::UnitType unitType);
	std::set<BWAPI::Unit*> ArmyManager::getAllUnitType(BWAPI::UnitType unitType);
	void ArmyManager::setRallyPoint(BWAPI::Position position);
	void ArmyManager::addEnemy(BWAPI::Unit* unit);
	void ArmyManager::removeEnemy(BWAPI::Unit* unit);
	void ArmyManager::allRetreat();
	void ArmyManager::setDefendPosition(BWAPI::Position position);
	void ArmyManager::setArmyStatus(int status);
	void ArmyManager::setArmyStatus();
	int ArmyManager::getArmyStatus();
	bool ArmyManager::regroup();
	void ArmyManager::setAttackPosition(BWAPI::Position position);
	void ArmyManager::findEnemyBase();
	void ArmyManager::setEnemyArmySupply(int supply);

	enum {scout = 0, retreat = 1, attack = 2, defend = 3};

private:
//	SparCraft::GameState ArmyManager::getState();
	void ArmyManager::executeAttack();
	void ArmyManager::executeDefence();
	void ArmyManager::drawArmyStatus(int x, int y);
	void ArmyManager::updateStatus();
//	void ArmyManager::makeArchon();
	void ArmyManager::workerCombat();
	void ArmyManager::getCombatWorkers(size_t threat);
	void ArmyManager::clearCombatWorkers();
	void ArmyManager::kite();
	bool ArmyManager::haveDetection();
};
