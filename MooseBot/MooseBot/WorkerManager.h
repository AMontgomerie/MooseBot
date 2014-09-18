#pragma once
#include <BWAPI.h>
#include <BWTA.h>

#define BASERADIUS 200

class WorkerManager
{
	std::set<BWAPI::Unit*> allWorkers;
	std::set<BWAPI::Unit*> availableWorkers;
	std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>> bases;
	std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>> gases;
	BWAPI::Unit* expansionBuilder;
	BWAPI::Unit* currentBuilder;
public:
	WorkerManager::WorkerManager();
	bool WorkerManager::update();
	BWAPI::Unit* WorkerManager::getWorker();
	bool WorkerManager::addWorker(BWAPI::Unit* newWorker);
	bool WorkerManager::removeWorker(BWAPI::Unit* worker);
	bool WorkerManager::mineGas(BWAPI::Unit* gas);
	bool WorkerManager::saturateGas(BWAPI::Unit* gas);
	int WorkerManager::getWorkerCount();
	void WorkerManager::setExpansionBuilder();
	void WorkerManager::unsetExpansionBuilder();
	BWAPI::Unit* WorkerManager::getExpansionBuilder();
	BWAPI::Unit* WorkerManager::getCurrentBuilder();
	void WorkerManager::setBuilder();
	void WorkerManager::unsetBuilder();
	int WorkerManager::getNumMineralWorkers();
	int WorkerManager::getNumGasWorkers();
	void WorkerManager::addExpansion(BWAPI::Unit* expansion);
	void WorkerManager::removeExpansion(BWAPI::Unit* expansion);
	void WorkerManager::addGas(BWAPI::Unit* gas);
	void WorkerManager::removeGas(BWAPI::Unit* gas);
private:
	bool WorkerManager::makeAvailable(BWAPI::Unit* worker);
	bool WorkerManager::makeUnavailable(BWAPI::Unit* worker);
	bool WorkerManager::returnToMining(BWAPI::Unit* worker);
	void WorkerManager::updateSaturation();
	void WorkerManager::transferWorkers();
	void WorkerManager::drawBaseInformation(int x, int y);
	void WorkerManager::updateGasWorkers();
};
