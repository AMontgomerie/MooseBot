#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include "BuildOrderQueue.h"
#include "BuildingPlacer.h"
#include "WorkerManager.h"
#include "StarcraftBuildOrderSearchManager.h"

#define MAXWAIT 720

class ProductionManager
{
	WorkerManager						workerManager;
	BuildingPlacer						buildingPlacer;
	BuildOrderQueue						production;

	std::set<BWAPI::Unit*>				buildings;
	std::vector<BWAPI::Unit*>			gas;
	BWAPI::TilePosition					nextExpansionLocation;
	bool								expanding;
	bool								deadlockfound;
	bool								expansionQueued;
	BWAPI::TilePosition					centre;
	int									lastProductionFrame;
	int									lastExpansionFrame;

	std::vector< std::pair<MetaType, UnitCountType> > goal;
public:
	ProductionManager::ProductionManager();
	void ProductionManager::setBuildOrder(const std::vector<MetaType> & buildOrder);
	void ProductionManager::performBuildOrderSearch(const std::vector< std::pair<MetaType, UnitCountType> > & goal);
	void ProductionManager::update();
	void ProductionManager::checkGas();
	void ProductionManager::addElement(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::removeElement();
	BuildOrderItem<PRIORITY_TYPE> & ProductionManager::getNextElement();
	bool ProductionManager::emptyQueue();
	void ProductionManager::addBuilding(BWAPI::Unit* building);
	void ProductionManager::removeBuilding(BWAPI::Unit* building);
	void ProductionManager::productionStarted(BWAPI::Unit* unit);
	BWAPI::Unit* ProductionManager::getBuilding(BWAPI::UnitType buildingType);
	BWAPI::Unit* ProductionManager::getBuilding(BWAPI::UnitType buildingType, bool addon);
	void ProductionManager::removeUnit(BWAPI::Unit* unit);
	void ProductionManager::addUnit(BWAPI::Unit* unit);
	BWAPI::Unit* ProductionManager::getWorker();
	void ProductionManager::addGas(BWAPI::Unit* unit);
	void ProductionManager::produceDetection();
	void ProductionManager::setCentre(BWAPI::TilePosition centre);
	void ProductionManager::clearProductionQueue();
private:
	void ProductionManager::createUnit(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::createAddon(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::morphUnit(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::createBuilding(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::startUpgrade(BuildOrderItem<PRIORITY_TYPE> element);
	bool ProductionManager::canAfford(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::beginProduction(BuildOrderItem<PRIORITY_TYPE> element);
	void ProductionManager::drawGoalInformation(int x, int y, std::vector< std::pair<MetaType, UnitCountType> > goal);
	void ProductionManager::checkForDeadlock();
	void ProductionManager::checkMinerals();
	void ProductionManager::removeUnwantedItems();
	bool ProductionManager::isTechBuilding(BuildOrderItem<PRIORITY_TYPE> element);
	BWAPI::Unit* ProductionManager::getUncompletedBuilding(BWAPI::UnitType buildingType);
};
