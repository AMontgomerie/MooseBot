#include "ProductionManager.h"
using namespace BWAPI;

ProductionManager::ProductionManager()
{
	expanding = false;
	expansionQueued = false;
	setBuildOrder(StarcraftBuildOrderSearchManager::Instance().getOpeningBuildOrder());
	deadlockfound = false;
	lastProductionFrame = 0;
	lastExpansionFrame = 0;

	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
	{
		if((*i)->getType().isResourceDepot())
		{
			workerManager.addExpansion(*i);
		}
	}
}

/*
taken from UAlbertaBot
Copies the build order described in the parameter into the production queue
*/
void ProductionManager::setBuildOrder(const std::vector<MetaType> & buildOrder)
{
	// clear the current build order
	production.clearAll();

	// for each item in the results build order, add it
	for (size_t i(0); i<buildOrder.size(); ++i)
	{
		// queue the item
	//	Broodwar->printf("adding %s to production queue", buildOrder[i].unitType.getName().c_str());
		production.queueAsLowestPriority(buildOrder[i], true);
	}
}

/*
taken from UAlbertaBot
Takes an inputted goal consisting of a vector of pairs made up of unit types and unit counts
Performs a search to find a build order which results in the goal units being created
When a build order is found, it is added to the production queue
*/
void ProductionManager::performBuildOrderSearch(const std::vector< std::pair<MetaType, UnitCountType> > & goal)
{	
	std::vector<MetaType> buildOrder = StarcraftBuildOrderSearchManager::Instance().findBuildOrder(goal);
	// set the build order
	setBuildOrder(buildOrder);
}

void ProductionManager::update()
{
	StarcraftBuildOrderSearchManager::Instance().update(35); //update the current build order search to allow it to run for 35ms
	//if(!workerManager.update())
	//{
	//	production.queueAsHighestPriority(MetaType(BWAPI::UnitTypes::Protoss_Nexus), true);
	//}
	workerManager.update();
//	checkGas(); //check if we need to put workers in gas (now handled in WorkerManager)
	checkMinerals();
	production.drawQueueInformation(10, 10);
	StrategyManager::Instance().drawEnemyInformation(180, 10);

	checkForDeadlock();

	//if the production queue is empty
	if(emptyQueue())
	{
		//get a new build order goal
		const std::vector< std::pair<MetaType, UnitCountType> > goal = StrategyManager::Instance().getBuildOrderGoal();
		drawGoalInformation(10, 100, goal);
		//perform a new build order search
		performBuildOrderSearch(goal);
	}
	else
	{
		removeUnwantedItems();
		BuildOrderItem<PRIORITY_TYPE> element = ProductionManager::getNextElement(); //get the next element in the queue	
		if(Broodwar->getFrameCount() % 30 == 0)
		{
		//	Broodwar->printf("next element is %s", element.metaType.getName().c_str());
		}
		//if we can afford to start production then do so
		if(canAfford(element))
		{
			beginProduction(element);
		}
		//if we can't afford to start production and the current item isn't blocking 
		//then check the rest of the queue to see if we can afford to build anything else
		else if(!element.blocking)
		{
			do
			{
				element = production.getNextHighestPriorityItem();
			}while(!canAfford(element) && !element.blocking);
			if(canAfford(element))
			{
				beginProduction(element);
			}
		}
	}
}

/*
calls the correct method to begin production of the type of item specified in element
*/
void ProductionManager::beginProduction(BuildOrderItem<PRIORITY_TYPE> element)
{
	//if the item is a unit then call the appropriate method to begin construction
	if(element.metaType.isUnit())
	{
		if(element.metaType.whatBuilds().isBuilding())
		{
			if(element.metaType.unitType.isAddon())
			{
				createAddon(element);
			}
			else
			{
				createUnit(element);
			}
		}
		else if(element.metaType.whatBuilds() == BWAPI::UnitTypes::Zerg_Larva)
		{
			morphUnit(element);
		}
		else if(element.metaType.whatBuilds() == Broodwar->self()->getRace().getWorker())
		{
			createBuilding(element);
		}
		else
		{
			Broodwar->printf("ProductionManager Error: Production this type of unit is not supported");
		}
	}
	//if the task is an upgrade rather than a unit then start the upgrade
	else if(element.metaType.isUpgrade())
	{
		startUpgrade(element);
	}
	else 
	{
		Broodwar->printf("ProductionManager Error: Unknown production error");
	}
}

/*
checks if we can afford to build an element, returns true if we can
*/
bool ProductionManager::canAfford(BuildOrderItem<PRIORITY_TYPE> element)
{
	if((Broodwar->self()->minerals() >= element.metaType.mineralPrice()) && (Broodwar->self()->gas() >= element.metaType.gasPrice()))
	{
		return true;
	}
	return false;
}

/*
adds a new element to production
*/
void ProductionManager::addElement(BuildOrderItem<PRIORITY_TYPE> element)
{
	production.queueItem(element);
	return;
}

/*
removes an element from production
*/

void ProductionManager::removeElement()
{
	//Broodwar->printf("removing current task from production queue");
	production.removeHighestPriorityItem();
	return;
}

/*
Returns the next production element in the queue to be executed
*/
BuildOrderItem<PRIORITY_TYPE>& ProductionManager::getNextElement()
{
	return production.getHighestPriorityItem();
}

/*
adds a building to the set of player controlled buildings
*/
void ProductionManager::addBuilding(Unit* building)
{
	if(!buildings.insert(building).second)
	{
	//	Broodwar->printf("ProductionManager Error: unable to insert new building in to set");
	}
	return;
}

/*
removes a building from the set of player controlled buildings
*/
void ProductionManager::removeBuilding(Unit* building)
{
	//clear the production queue and search for a new goal in case this was a key structure
	clearProductionQueue();

	if(!buildings.erase(building))
	{
	//	Broodwar->printf("ProductionManager Error: unable to find destroyed building in building set");
	}
	return;
}

/*
returns an idle of building of the type indicated by the parameter buildingType.
If no such Idle building exists, NULL is returned.
*/

BWAPI::Unit* ProductionManager::getBuilding(BWAPI::UnitType buildingType)
{
	for(std::set<Unit*>::const_iterator i=buildings.begin();i!=buildings.end();i++)
	{
		if(((*i)->getType() == buildingType) && ((*i)->isIdle()) && ((*i)->isCompleted()))
		{
			return (*i);
		}
		//incase we are looking for a hatch that is currently morphing
		else if((buildingType == BWAPI::UnitTypes::Zerg_Hatchery) ||
			(buildingType == BWAPI::UnitTypes::Zerg_Lair) ||
			(buildingType == BWAPI::UnitTypes::Zerg_Hive))
		{
			if((*i)->getBuildType() == buildingType)
			{
				return (*i);
			}
		}
	}
	return NULL;
}

/*
Overloaded getBuilding method.
The second parameter indicates whether we are searching for a building with or without an addon
If we are searching for a building that cannot have an addon (e.g. hatchery) then use the other getBuilding method
*/
BWAPI::Unit* ProductionManager::getBuilding(BWAPI::UnitType buildingType, bool addon)
{
	for(std::set<Unit*>::const_iterator i=buildings.begin();i!=buildings.end();i++)
	{
		if(!addon)
		{
			if(((*i)->getType() == buildingType) && (*i)->isIdle() && ((*i)->getAddon() == NULL) && ((*i)->isCompleted()))
			{
				return (*i);
			}
		}
		//if we are looking for a building with an addon
		else
		{
			if(((*i)->getType() == buildingType) && ((*i)->isIdle()) && ((*i)->getAddon() != NULL) && ((*i)->isCompleted()))
			{
				return (*i);
			}
		}
	}
	return NULL;
}

/*
returns a building even if it's not idle or not completed
*/
BWAPI::Unit* ProductionManager::getUncompletedBuilding(BWAPI::UnitType buildingType)
{
	for(std::set<Unit*>::const_iterator i=buildings.begin();i!=buildings.end();i++)
	{
		if((*i)->getType() == buildingType)
		{
			return (*i);
		}
		//incase we are looking for a hatch that is currently morphing
		else if((buildingType == BWAPI::UnitTypes::Zerg_Hatchery) ||
			(buildingType == BWAPI::UnitTypes::Zerg_Lair) ||
			(buildingType == BWAPI::UnitTypes::Zerg_Hive))
		{
			if((*i)->getBuildType() == buildingType)
			{
				return (*i);
			}
		}
	}
	return NULL;
}

/*
takes a newly created unit as a parameter and checks if this unit is the next expected unit in the production queue
if it is then connect the unit pointer to the production element so that it's progress can be tracked
*/
void ProductionManager::productionStarted(BWAPI::Unit* unit)
{
	removeElement();
	lastProductionFrame = BWAPI::Broodwar->getFrameCount();
	deadlockfound = false;

	if(unit->getBuildType().isResourceDepot())
	{
		workerManager.unsetExpansionBuilder();
		buildingPlacer.addExpansion(unit);
		workerManager.addExpansion(unit);
		lastExpansionFrame = BWAPI::Broodwar->getFrameCount();
	}
	if(unit->getBuildType().isRefinery())
	{
		workerManager.addGas(unit);
	}
	else
	{
		workerManager.unsetBuilder();
	}
	
}

/*
assigns a new unit to the correct manager
*/
void ProductionManager::addUnit(BWAPI::Unit* unit)
{
	if(unit->getType().isWorker())
	{
		workerManager.addWorker(unit);
	}
	else if(unit->getType().isResourceDepot())
	{
		buildingPlacer.addExpansion(unit);
	}
	/*
	else if(unit->getType().isBuilding())
	{
		addBuilding(unit);
	}
	*/
}

/*
removes a destroyed unit from the relevant manager
*/
void ProductionManager::removeUnit(BWAPI::Unit* unit)
{
	if(unit->getType().isWorker())
	{
		workerManager.removeWorker(unit);
	}
	else if(unit->getType().isBuilding())
	{
		removeBuilding(unit);
	}
	else if(unit->getType().isResourceDepot())
	{
		buildingPlacer.removeExpansion(unit);
	}
}

/*
puts workers in gas if needed
*/
void ProductionManager::checkGas()
{

	for(std::vector<Unit*>::const_iterator i = gas.begin(); i != gas.end();)
	{
		if((*i)->isCompleted() && ((*i)->getPlayer() == Broodwar->self()))
		{
		//	Broodwar->printf("saturating gas");
			workerManager.saturateGas((*i));
			gas.erase(i);
		}
		else 
		{
			++i;
		}
	}
}

/*
retrieves a worker from WorkerManager
*/
BWAPI::Unit* ProductionManager::getWorker()
{
	return workerManager.getWorker();
}

/*
adds a new geyser to the set of controlled gas geysers
*/
void ProductionManager::addGas(BWAPI::Unit* unit)
{
	gas.push_back(unit);
}

/*
checks if the production queue is empty
*/
bool ProductionManager::emptyQueue()
{
	if(production.isEmpty())
	{
		return true;
	}
	return false;
}

void ProductionManager::clearProductionQueue()
{
	production.clearAll();
}

/*
creates the unit type specified in the build order item
*/
void ProductionManager::createUnit(BuildOrderItem<PRIORITY_TYPE> element)
{
	BWAPI::Unit* structure;
	bool addon = false;

	//check that we can afford to train the new unit
	if((Broodwar->self()->minerals() >= element.metaType.mineralPrice()) && (Broodwar->self()->gas() >= element.metaType.gasPrice()))
	{
		for(std::map<UnitType, int>::const_iterator i = element.metaType.unitType.requiredUnits().begin(); i != element.metaType.unitType.requiredUnits().end(); i++)
		{
			if((*i).first.isAddon() && element.metaType.whatBuilds().canBuildAddon())
			{
				addon = true;
			}
		}

		if(addon)
		{
			structure = getBuilding(element.metaType.whatBuilds(), addon);
		}
		else
		{
			structure = getBuilding(element.metaType.whatBuilds());
		}

		if(structure != NULL && structure->isCompleted())
		{
			structure->train(element.metaType.unitType);
		}
	}
}

/*
creates the addon type specified in the build order item
*/
void ProductionManager::createAddon(BuildOrderItem<PRIORITY_TYPE> element)
{
	BWAPI::Unit* structure;

	//check that we can afford to train the new unit
	if((Broodwar->self()->minerals() >= element.metaType.mineralPrice()) && (Broodwar->self()->gas() >= element.metaType.gasPrice()))
	{
		structure = getBuilding(element.metaType.whatBuilds(), false);
		if(structure != NULL)
		{
			if(structure->getLastCommand().getType() != BWAPI::UnitCommandTypes::Build_Addon)
			{
				buildingPlacer.placeAddon(structure, element.metaType.unitType);
			}
		}
	}
}

/*
morphs the unit type specified in the build order item
*/
void ProductionManager::morphUnit(BuildOrderItem<PRIORITY_TYPE> element)
{
	BWAPI::Unit* structure;

	//check that we can afford to train the new unit
	if((Broodwar->self()->minerals() >= element.metaType.mineralPrice()) && (Broodwar->self()->gas() >= element.metaType.gasPrice()))
	{
		structure = getBuilding(BWAPI::UnitTypes::Zerg_Hatchery);
		if(structure == NULL)
		{
			structure = getBuilding(BWAPI::UnitTypes::Zerg_Lair);
		}
		if(structure == NULL)
		{
			structure = getBuilding(BWAPI::UnitTypes::Zerg_Hive);
		}
		if(structure != NULL)
		{
			std::set<BWAPI::Unit*> larvae = structure->getLarva();
			//if there are any idle larvae, then train the unit
			if (larvae.size() > 0)
			{
				BWAPI::Unit* larva = *larvae.begin();
				larva->morph(element.metaType.unitType);
			}
			else
			{
				return;
			}
		}
		else
		{
			Broodwar->printf("ProductionManager Error: No available hatcheries");
			return;
		}
	}
}

/*
creates the building type specified in the build order item
*/
void ProductionManager::createBuilding(BuildOrderItem<PRIORITY_TYPE> element)
{
	BWAPI::TilePosition buildPosition;

	//if we have analyzed the map then we will set the buildPosition at the centre of our region, otherwise it will be set to our main base
	if(centre != TilePosition(0,0))
	{
		buildPosition = centre;
	}
	else
	{
		for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
		{
			if ((*i)->getType().isResourceDepot())
			{
				buildPosition = (*i)->getTilePosition();
				break;
			}
		}
	}

	//check that we can afford to build the new structure
	if((Broodwar->self()->minerals() >= element.metaType.mineralPrice()) && (Broodwar->self()->gas() >= element.metaType.gasPrice()))
	{
		//when constructing an expansion
		if(element.metaType.unitType.isResourceDepot())
		{
			if(workerManager.getExpansionBuilder() == NULL)
			{
				workerManager.setExpansionBuilder();
				nextExpansionLocation = buildingPlacer.getClosestBase(workerManager.getExpansionBuilder());
				//if we can't find any bases, move on to the next item
				if(nextExpansionLocation == TilePosition(0,0))
				{
					production.removeHighestPriorityItem();
					return;
				}
				//if we can't reach the closest base (e.g. its on an island)
				else if(!workerManager.getExpansionBuilder()->hasPath(Position(nextExpansionLocation)))
				{
					//get the next closest
					nextExpansionLocation = buildingPlacer.getNextClosestBase(workerManager.getExpansionBuilder());
					//if the next closest is non-existent or also unreachable then give up and move on to the next build order item
					if((nextExpansionLocation == TilePosition(0,0)) || !workerManager.getExpansionBuilder()->hasPath(Position(nextExpansionLocation)))
					{
						production.removeHighestPriorityItem();
						return;
					}
				}
				expanding = true;
			}
			if(workerManager.getExpansionBuilder() != NULL)
			{	
				if(expanding && (nextExpansionLocation != NULL))
				{
					workerManager.getExpansionBuilder()->move(Position(nextExpansionLocation), false);
					expanding = false;
				}						
				if(workerManager.getExpansionBuilder()->isIdle())
				{
					buildingPlacer.placeExpansion(workerManager.getExpansionBuilder(), element.metaType.unitType, nextExpansionLocation);
				}
						
			}
			else
			{
				Broodwar->printf("Error: no available to workers to build expansion");
			}
		}
		//when constructing gas
		else if(element.metaType.isRefinery())
		{
			if(workerManager.getCurrentBuilder() == NULL)
			{
				workerManager.setBuilder();
			}
			if(workerManager.getCurrentBuilder() != NULL)
			{
				buildingPlacer.placeGas(workerManager.getCurrentBuilder(), element.metaType.unitType);
			}
			else
			{
				Broodwar->printf("Error: no available to workers to build gas");
			}
		}
		//when constructing all other buildings
		else
		{
			//check that we have the required tech to make this building
			int requiredTech = 0;
			for(std::map<UnitType, int>::const_iterator i = element.metaType.unitType.requiredUnits().begin(); i != element.metaType.unitType.requiredUnits().end(); i++)
			{
				for(std::set<Unit*>::const_iterator j = Broodwar->self()->getUnits().begin(); j != Broodwar->self()->getUnits().end(); j++)
				{
					if(((*j)->getType() == (*i).first) && ((*j)->isCompleted()))
					{
						requiredTech++;
						break;
					}
				}
			}
			if(element.metaType.unitType.requiredUnits().size() == requiredTech)
			{
				if(workerManager.getCurrentBuilder() == NULL)
				{
					workerManager.setBuilder();
				}
				if(workerManager.getCurrentBuilder() != NULL)
				{
					if((workerManager.getCurrentBuilder()->getLastCommand().getType() != BWAPI::UnitCommandTypes::Build) || (workerManager.getCurrentBuilder()->isGatheringMinerals()))
					{
						buildingPlacer.placeBuilding(workerManager.getCurrentBuilder(), element.metaType.unitType, buildPosition);
					}
				}
				else
				{
					Broodwar->printf("Error: no available to workers to build");
				}
			}
		}
	}
	//if we cant afford it then wait until the next frame
	else
	{
		return;
	}
}

/*
starts the upgrade specified in the build order item
*/
void ProductionManager::startUpgrade(BuildOrderItem<PRIORITY_TYPE> element)
{
	BWAPI::Unit* structure;

	structure = getBuilding(element.metaType.whatBuilds());
	if(structure != NULL && structure->isCompleted())
	{
		structure->upgrade(element.metaType.upgradeType);
		removeElement();
	}
	else
	{
		Broodwar->printf("ProductionManager Error: No structure available to research '%s'", element.metaType.getName().c_str());
	}
}

/*
queues a static detection building to the production queue as highest priority
*/
void ProductionManager::produceDetection()
{
	BWAPI::UnitType detector;

	if(Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		detector = BWAPI::UnitTypes::Terran_Missile_Turret;
	}
	else if(Broodwar->self()->getRace() == BWAPI::Races::Protoss)
	{
		detector = BWAPI::UnitTypes::Protoss_Photon_Cannon;
	}
	else if(Broodwar->self()->getRace() == BWAPI::Races::Zerg)
	{
		detector = BWAPI::UnitTypes::Zerg_Spore_Colony;
	}

	production.queueAsHighestPriority(detector, true);
	production.queueAsHighestPriority(detector, true);

	if((Broodwar->self()->getRace() == BWAPI::Races::Terran) && (getBuilding(BWAPI::UnitTypes::Terran_Engineering_Bay) == NULL))
	{
		production.queueAsHighestPriority(BWAPI::UnitTypes::Terran_Engineering_Bay, true);
	}
	if((Broodwar->self()->getRace() == BWAPI::Races::Protoss) && (getBuilding(BWAPI::UnitTypes::Protoss_Forge) == NULL))
	{
		production.queueAsHighestPriority(BWAPI::UnitTypes::Protoss_Forge, true);
	}
	if((Broodwar->self()->getRace() == BWAPI::Races::Zerg) && (getBuilding(BWAPI::UnitTypes::Zerg_Evolution_Chamber) == NULL))
	{
		production.queueAsHighestPriority(BWAPI::UnitTypes::Zerg_Evolution_Chamber, true);
	}
}

/*
prints information about the current build order goal on to the screen
*/
void ProductionManager::drawGoalInformation(int x, int y, std::vector< std::pair<MetaType, UnitCountType> > goal)
{
	int i = 0;

	BWAPI::Broodwar->drawTextScreen(x, y-10,"Goal Units");

	if(goal.empty())
	{
		BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "\x04No Goal");
	}

	for(std::vector<std::pair<MetaType, UnitCountType>>::iterator unit = goal.begin(); unit != goal.end(); unit++)
	{
		BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "\x04%d %s", (*unit).second, (*unit).first.getName().c_str());
		i++;
	}
}

/*
sets the tile position at the centre of our region (used for placing buildings)
*/
void ProductionManager::setCentre(BWAPI::TilePosition tilePosition)
{
	centre = tilePosition;
}

/*
checks if our build has got stuck, if it has then clear production and search again
*/
void ProductionManager::checkForDeadlock()
{
	
	//if we haven't made anything for a while
	if(!deadlockfound && ((BWAPI::Broodwar->getFrameCount() - lastProductionFrame) > MAXWAIT))
	{
		deadlockfound = true;
		clearProductionQueue();

	}
}

/*
if the mineral fields at our base are getting low then construct a new base
*/
void ProductionManager::checkMinerals()
{
	if(!expansionQueued)
	{
		Unit* closestMineral = NULL;
		TilePosition home;

		//find main
		for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
		{
			if ((*i)->getType().isResourceDepot())
			{
				home = (*i)->getTilePosition();
				for(std::set<BWAPI::Unit*>::iterator m = Broodwar->getMinerals().begin(); m != Broodwar->getMinerals().end(); m++)
				{
					if (closestMineral==NULL || home.getDistance((*m)->getTilePosition()) < home.getDistance(closestMineral->getTilePosition()))
					closestMineral=*m;
				}
				if (closestMineral!=NULL)
				{
					if(closestMineral->getResources() < 150)
					{
						production.queueAsHighestPriority(MetaType(Broodwar->self()->getRace().getCenter()), true);
						expansionQueued = true;
					}
				}
			}
		}
	}
}

/*
temporary function to remove build order items that are causing bugs
*/
void ProductionManager::removeUnwantedItems()
{
	bool gas = false;
	int gatewayCount = 0, nexusCount = 0;

	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
	{
		if((*i)->getType().isRefinery())
		{
			gas = true;
		}
		if((*i)->getType() == UnitTypes::Protoss_Gateway)
		{
			gatewayCount++;
		}
		if((*i)->getType().isResourceDepot())
		{
			nexusCount++;
		}
	}

	//in case natural is mineral only
	if(gas && (Broodwar->self()->minerals() > 500) && (production.getHighestPriorityItem().metaType.unitType == UnitTypes::Protoss_Assimilator))
	{
		production.removeHighestPriorityItem();
	}
	/*
	//in case there are no available expansions
	if((Broodwar->self()->minerals() > 1500) && (production.getHighestPriorityItem().metaType.unitType == UnitTypes::Protoss_Nexus))
	{
		production.removeHighestPriorityItem();
	}
	*/

	if((production.getHighestPriorityItem().metaType.unitType == UnitTypes::Protoss_Gateway) && (gatewayCount > 4 * nexusCount))
	{
		production.removeHighestPriorityItem();
	}
	//to prevent it from spamming nexuses every time it returns a new set of instructions
	if((production.getHighestPriorityItem().metaType.unitType == UnitTypes::Protoss_Nexus) && ((BWAPI::Broodwar->getFrameCount() - lastExpansionFrame) < (MAXWAIT * 4)))
	{
		production.removeHighestPriorityItem();
	}
	if(isTechBuilding(production.getHighestPriorityItem()) && (getUncompletedBuilding(production.getHighestPriorityItem().metaType.unitType) != NULL))
	{
		production.removeHighestPriorityItem();
	}
}

bool ProductionManager::isTechBuilding(BuildOrderItem<PRIORITY_TYPE> element)
{
	if((element.metaType.unitType == UnitTypes::Protoss_Arbiter_Tribunal) ||
		(element.metaType.unitType == UnitTypes::Protoss_Citadel_of_Adun) ||
		(element.metaType.unitType == UnitTypes::Protoss_Cybernetics_Core) ||
		(element.metaType.unitType == UnitTypes::Protoss_Fleet_Beacon) ||
		(element.metaType.unitType == UnitTypes::Protoss_Observatory) ||
		(element.metaType.unitType == UnitTypes::Protoss_Robotics_Support_Bay) ||
		(element.metaType.unitType == UnitTypes::Protoss_Templar_Archives))
	{
		return true;
	}
	return false;
}