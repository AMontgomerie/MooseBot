#include "WorkerManager.h"
using namespace BWAPI;

/*
*/

WorkerManager::WorkerManager()
{
	currentBuilder = NULL;
	expansionBuilder = NULL;
}

bool WorkerManager::update()
{
	updateSaturation();

	drawBaseInformation(520, 150);

	//these are only called every 10 seconds to allow workers time to transfer to the correct location before they are counted again
	if(BWAPI::Broodwar->getFrameCount() % 240 == 0)
	{
		transferWorkers();
		updateGasWorkers();
	}


	if((currentBuilder != NULL) && (currentBuilder->getHitPoints() == 0))
	{
		unsetBuilder();
	}
	if((expansionBuilder != NULL) && (expansionBuilder->getHitPoints() == 0))
	{
		unsetExpansionBuilder();
	}
	//find idle workers, make them available for command issuing and command them to mine minerals
	for(std::set<Unit*>::const_iterator i=allWorkers.begin();i!=allWorkers.end();i++)
	{
		if(!((*i)->getType().isWorker()))
		{
			removeWorker(*i);
		}
		if((*i)->isIdle() && (*i)->isCompleted() && ((*i) != expansionBuilder))
		{
			makeAvailable((*i));
			returnToMining((*i));
		}
	}
	if(bases.size() == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/*
Returns a worker that is available for having commands issued to it
*/
BWAPI::Unit* WorkerManager::getWorker()
{
	BWAPI::Unit* worker;

	for(std::set<Unit*>::const_iterator i=availableWorkers.begin();i!=availableWorkers.end();i++)
	{
		if((*i)->getType().isWorker() && !(*i)->isConstructing() && !(*i)->isGatheringGas())
		{
			worker = (*i);
			if(availableWorkers.erase((*i)))
			{
				makeUnavailable(worker);
				return worker;
			}
			else
			{
				Broodwar->printf("WorkerManager Error: failed to remove worker from set of available workers");
				return NULL;
			}
		}
	}
	Broodwar->printf("WorkerManager Error: No available workers");
	return NULL;
}

int WorkerManager::getWorkerCount()
{
	return allWorkers.size();
}

/*
Adds a new worker to the set of all workers that the player controls
*/
bool WorkerManager::addWorker(BWAPI::Unit* newWorker)
{
	if(allWorkers.insert(newWorker).second)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
Removes a worker from the set of all workers that the player controls
*/
bool WorkerManager::removeWorker(BWAPI::Unit* worker)
{
	for(std::set<Unit*>::const_iterator i=allWorkers.begin();i!=allWorkers.end();i++)
	{
		if((*i) == worker)
		{
			if(allWorkers.erase(worker))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	Broodwar->printf("WorkerManager Error: unable to locate worker that is to be removed.");
	return false;
}

/*
Add a worker that the player controls to the set of workers that are available
*/
bool WorkerManager::makeAvailable(BWAPI::Unit* worker)
{
	if(availableWorkers.insert(worker).second)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
Removes a worker from the set of available workers.
The worker will remain in the set of allWorkers.
*/
bool WorkerManager::makeUnavailable(BWAPI::Unit* worker)
{
	if(availableWorkers.erase(worker))
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
Issues a command to a worker to mine the nearest mineral patch
*/
bool WorkerManager::returnToMining(BWAPI::Unit* worker)
{
	Unit* closestMineral=NULL;
	for(std::set<Unit*>::iterator m=Broodwar->getMinerals().begin();m!=Broodwar->getMinerals().end();m++)
	{
		if (closestMineral==NULL || worker->getDistance(*m) < worker->getDistance(closestMineral))
		closestMineral=*m;
	}
	if (closestMineral!=NULL)
	{
		if(worker->rightClick(closestMineral))
		{
			return true;
		}
		else
		{
			Broodwar->printf("WorkerManager Error: command to mine minerals failed");
			return false;
		}
	}
	Broodwar->printf("WorkerManager Error: unable to find mineral field");
	return false;
}

/*
Finds an available worker and commands them to mine the specified gas. 
Removes the worker from the set of available workers.
Should be called 3 times to saturate 1 geyser.
*/
bool WorkerManager::mineGas(BWAPI::Unit* gas)
{
	for(std::set<Unit*>::const_iterator i=availableWorkers.begin();i!=availableWorkers.end();i++)
	{
		if((*i) != NULL)
		{
			(*i)->rightClick(gas);
			if(!makeUnavailable((*i)))
			{
				Broodwar->printf("WorkerManager Error: removal from available workers unsuccessful");
				return false;
			}
			return true;
		}
		Broodwar->printf("WorkerManager Error: no available workers found");
		return false;
	}
	Broodwar->printf("WorkerManager Error: unable to find vespine geyser");
	return false;
}

/*
Calls mineGas 3 times in order to fully saturate a gas geyser
if terran then calls twice because refinery builder automatically becomes a gas miner
*/
bool WorkerManager::saturateGas(BWAPI::Unit* gas)
{
	int i;
	int satCount = 3;

	if(Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		satCount = 2;
	}

	for(i = 0; i < satCount; i++)
	{
		if(!mineGas(gas))
		{
			Broodwar->printf("WorkerManager Error: assigning worker # %d to gas failed", i);
			return false;
		}
	}
	return true;
}

/*
assign a worker the task of constructing an expansion
*/
void WorkerManager::setExpansionBuilder()
{
	expansionBuilder = getWorker();
}

/*
unassigns a worker from being the dedicated expansion builder, allowing it to return to mining
*/
void WorkerManager::unsetExpansionBuilder()
{
	expansionBuilder = NULL;
}

/*
returns the worker assigned as expansion builder
*/
BWAPI::Unit* WorkerManager::getExpansionBuilder()
{
	return expansionBuilder;
}

/*
returns the worker assigned as the current builder
*/
BWAPI::Unit* WorkerManager::getCurrentBuilder()
{
	return currentBuilder;
}

/*
assigns a worker to be a builder
*/
void WorkerManager::setBuilder()
{
	currentBuilder = getWorker();
}

/*
unassigns the current builder
*/
void WorkerManager::unsetBuilder()
{
	currentBuilder = NULL;
}

int WorkerManager::getNumMineralWorkers()
{
	int count = 0;

	for(std::set<Unit*>::const_iterator i=allWorkers.begin();i!=allWorkers.end();i++)
	{
		if((*i)->isGatheringMinerals())
		{
			count++;
		}
	}
	return count;
}

int WorkerManager::getNumGasWorkers()
{
	int count = 0;

	for(std::set<Unit*>::const_iterator i=allWorkers.begin();i!=allWorkers.end();i++)
	{
		if((*i)->isGatheringGas())
		{
			count++;
		}
	}
	return count;
}

void WorkerManager::addExpansion(BWAPI::Unit* expansion)
{
	std::set<BWAPI::Unit*> workers;

	bases.insert(std::make_pair(expansion, workers));
}

void WorkerManager::removeExpansion(BWAPI::Unit* expansion)
{
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator b = bases.begin(); b != bases.end(); b++)
	{
		if((*b).first == expansion)
		{
			bases.erase(*b);
			break;
		}
	}
}


void WorkerManager::updateSaturation()
{
	bool active;

	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator b = bases.begin(); b != bases.end(); b++)
	{
		active = false;

		//first check that the base isn't mined out
		for(std::set<BWAPI::Unit*>::const_iterator m = BWAPI::Broodwar->getMinerals().begin(); m != BWAPI::Broodwar->getMinerals().end(); m++)
		{
			if(((*m)->getDistance((*b).first) < BASERADIUS) && ((*m)->getResources() > 0))
			{
				active = true;
				break;
			}
		}

		//if the base is mined out or has been destroyed then we should remove it from the set of active bases
		if(((*b).first->getHitPoints() == 0) || !active)
		{
			removeExpansion((*b).first);
			break;
		}
	}

	//update the worker count at each remaining base
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator b = bases.begin(); b != bases.end(); b++)
	{
		for(std::set<BWAPI::Unit*>::const_iterator w = allWorkers.begin(); w != allWorkers.end(); w++)
		{
			if(((*w)->getDistance((*b).first) < BASERADIUS) && (*w)->isGatheringMinerals())
			{
				(*b).second.insert(*w);
			}
			else
			{
				(*b).second.erase(*w);
			}
			
			//remove any pointers to workers that have died
			if((*w)->getHitPoints() == 0)
			{
				(*b).second.erase(*w);
			}
		}
	}
}

//checks how many workers we have at each base
//if we have too many then try to find a base with a lower worker count to move the surplus workers to
void WorkerManager::transferWorkers()
{
	std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>> transferTo;

	//populate copy of bases
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator b = bases.begin(); b != bases.end(); b++)
	{
		transferTo.insert(*b);
	}
	
	//go through each base
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator b = bases.begin(); b != bases.end(); b++)
	{
		//check if we have too many workers
		if((*b).second.size() > 20)
		{
			//if we do then find an undersaturated base
			for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator t = transferTo.begin(); t != transferTo.end(); t++)
			{
				if((*b).second.size() <= 20)
				{
					break;
				}
				if(((*t).first != (*b).first) && ((*t).second.size() < 20))
				{
					//transfer workers from one base to another
					int difference = (*b).second.size() - (*t).second.size();
					while(difference > 0)
					{
						for(std::set<BWAPI::Unit*>::iterator transferWorker = (*b).second.begin(); transferWorker != (*b).second.end(); transferWorker++)
						{
							(*transferWorker)->move((*t).first->getPosition(), false);
							break;
						}
						difference--;
					}
				}
			}
		}
	}
}

//draws information about worker counts at each mining base
void WorkerManager::drawBaseInformation(int x, int y)
{
	int i = 1;

	BWAPI::Broodwar->drawTextScreen(x, y," Mining Bases:");

	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator base = bases.begin(); base != bases.end(); base++)
	{
		BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "\x04 Base %d: %d workers", i, base->second.size());
		i++;
	}
}

//add a geyser to the set of controlled geysers
void WorkerManager::addGas(BWAPI::Unit* gas)
{
	std::set<BWAPI::Unit*> workers;

	//limiting the number of geysers we will mine from to 2
	if(!(gases.size() > 2))
	{
		gases.insert(std::make_pair(gas, workers));
	}
}

//remove a geyser from the set of controlled geysers
void WorkerManager::removeGas(BWAPI::Unit* geyser)
{
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator gas = gases.begin(); gas != gases.end(); gas++)
	{
		if((*gas).first == geyser)
		{
			gases.erase(*gas);
			break;
		}
	}
}

//remove any geysers not under our control anymore and update and manage the number of workers mining from each geyser
void WorkerManager::updateGasWorkers()
{
	int workersNeeded;

	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator g = gases.begin(); g != gases.end(); g++)
	{
		//if the geyser is not under our control then remove it from the set
		if((*g).first->getType().isNeutral())
		{
			removeGas((*g).first);
			break;
		}
	}

	//update the worker count at each remaining geyser
	for(std::set<std::pair<BWAPI::Unit*, std::set<BWAPI::Unit*>>>::iterator g = gases.begin(); g != gases.end(); g++)
	{
		if((*g).first->isCompleted())
		{
			for(std::set<BWAPI::Unit*>::const_iterator w = allWorkers.begin(); w != allWorkers.end(); w++)
			{
				if(((*w)->getDistance((*g).first) < BASERADIUS) && (*w)->isGatheringGas())
				{
					(*g).second.insert(*w);
				}
				else
				{
					(*g).second.erase(*w);
				}
			
				//remove any pointers to workers that have died
				if((*w)->getHitPoints() == 0)
				{
					(*g).second.erase(*w);
				}
			}
			//if we have too few workers then add some more
			if((*g).second.size() < 3)
			{
				workersNeeded = 3 - (*g).second.size();
				while(workersNeeded > 0)
				{
					mineGas((*g).first);
					workersNeeded--;
				}
			}
			//if we have too many then remove some
			if((*g).second.size() > 3)
			{
				workersNeeded = (*g).second.size() - 3;
				for(std::set<BWAPI::Unit*>::iterator w = (*g).second.begin(); w != (*g).second.end(); w++)
				{
					if(workersNeeded <= 0)
					{
						break;
					}
					(*w)->stop(false);
					workersNeeded--;
				}
			}
		}
	}
}