#include "MooseBot.h"
using namespace BWAPI;

bool analyzed;
bool analysis_just_finished;
BWTA::Region* home;
BWTA::Region* enemy_base;
bool scouted = false;
bool attackIssued = false;
bool retreatIssued = false;
bool cloakDetected = false;
bool expoScout = false;

/*
called at the start of the game
*/
void MooseBot::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	//read map information into BWTA so terrain analysis can be done in another thread
	BWTA::readMap();
	analyzed = false;
	analysis_just_finished = false;

	show_bullets = false;
	show_visibility_data = false;

	//create map analysis thread
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);

	//give starting units to production manager to deal with
	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
	{	
		productionManager.addUnit(*i);
	}
}

/*
Called each in-game frame (~24 calls/sec)
*/
void MooseBot::onFrame()
{
	BuildOrderSearch::Timer timer;

	timer.start();

	armyManager.setAttackPosition(scoutManager.getEnemyBase()); 
	armyManager.setEnemyArmySupply(scoutManager.getEnemyArmySupply() + (scoutManager.getTotalEnemyStaticD() * 2)); //enemy army supply + (number of static defence * 4) 

	productionManager.update();
	armyManager.update();
	scoutManager.update();
	StrategyManager::Instance().setEnemyComposition(scoutManager.getEnemyComposition());
	checkUnderAttack();

	if(scoutManager.enemyHasCloak() && !cloakDetected)
	{
		productionManager.produceDetection();
		cloakDetected = true;
	}

	//if map analysis is complete
	if(analyzed)
	{
		drawTerrainData();

		//assign a worker as a scout and send them to find the enemy base
		if((Broodwar->self()->supplyUsed() >= 18) && (!scouted) && (scoutManager.getEnemyBase() == Position(0,0)))
		{
			scoutManager.setScout(productionManager.getWorker());
			scoutManager.sendScout();
			scouted = true;
		}

		//set a defensive rally point for our army at the choke to our base

		//get the chokepoints linked to our home region
		std::set<BWTA::Chokepoint*> chokepoints = home->getChokepoints();
		double min_length = 10000;
		BWTA::Chokepoint* choke = NULL;

		//iterate through all chokepoints and look for the one with the smallest gap (least width)
		for(std::set<BWTA::Chokepoint*>::iterator c = chokepoints.begin(); c != chokepoints.end(); c++)
		{
			double length = (*c)->getWidth();
			if (length < min_length || choke == NULL)
			{
				min_length = length;
				choke = *c;
			}
		}
		//if we have a choke facing towards the enemy base then rally units to there
		if((scoutManager.getEnemyBase() != Position(0,0)) && (scoutManager.getEnemyBase() != NULL) && (choke->getCenter().getDistance(scoutManager.getEnemyBase()) < home->getCenter().getDistance(scoutManager.getEnemyBase())))
		{
			armyManager.setRallyPoint(choke->getCenter());
		}
		//otherwise rally to the middle of our base
		else
		{
			armyManager.setRallyPoint(home->getCenter());
		}
	}

    if (analysis_just_finished)
    {
		Broodwar->printf("Finished analyzing map.");
		analysis_just_finished=false;

		//if we aren't zerg then set the starting build position to the middle of our region
		//if zerg we want to leave it at our hatchery so we can build on creep
		if((Broodwar->self()->getRace() != BWAPI::Races::Zerg) && (Broodwar->mapFileName() != "(4)Andromeda.scx"))
		{
			productionManager.setCentre(TilePosition(home->getCenter()));
		}
	}
	
	timer.stop();
	drawTimerInformation(520, 300, timer.getElapsedTimeInMilliSec());
}

/*
updates relevant manager whenever a unit is destroyed
*/
void MooseBot::onUnitDestroy(BWAPI::Unit* unit)
{
	if(unit->getPlayer() == Broodwar->self())
	{
		if((unit->getType().isWorker()) || (unit->getType().isBuilding()))
		{
			productionManager.removeUnit(unit);
		}
		if((!unit->getType().isWorker()) && (!unit->getType().isBuilding()))
		{
			armyManager.removeUnit(unit);
		}
	}
	else
	{
		if(unit->getType().isBuilding())
		{
			scoutManager.removeEnemyBase(unit);
			if(unit->getType().canAttack() || (unit->getType() == BWAPI::UnitTypes::Terran_Bunker))
			{
				scoutManager.removeEnemyStaticD();
			}
		}
		scoutManager.removeEnemyUnit(unit);
	}
}

/*
updates relevent manager whenever a unit is morphed
*/
void MooseBot::onUnitMorph(BWAPI::Unit* unit)
{
	if(unit->getPlayer() == Broodwar->self())
	{
		if(!productionManager.emptyQueue())
		{
			BuildOrderItem<PRIORITY_TYPE>& nextElement = productionManager.getNextElement();
		//	Broodwar->printf("%s morphed", unit->getType().getName().c_str());
			if(unit->getBuildType() == nextElement.metaType.unitType)
			{
				productionManager.productionStarted(unit);
			}
		}
		else
		{
			Broodwar->printf("Nothing in production queue to match new unit to");
		}
		//if the new unit is a zerg egg then we need to check what is inside it
		if(unit->getType() == BWAPI::UnitTypes::Zerg_Egg)
		{
			if(unit->getBuildType().isWorker())
			{
				productionManager.addUnit(unit);
			}
			else
			{
				armyManager.addUnit(unit);
			}
		}
		//otherwise we can just check the type of the unit
		else
		{
			if((unit->getType() == Broodwar->self()->getRace().getRefinery()))
			{
				productionManager.addGas(unit);
			}
			else if(unit->getType().isBuilding())
			{
				productionManager.addBuilding(unit);	
			}
			else if(unit->getType().isWorker())
			{
				productionManager.addUnit(unit);
			}
			else if((!unit->getType().isWorker()) && (!unit->getType().isBuilding()))
			{
				armyManager.addUnit(unit);
			}
		}
	}
	else
	{
		//for storing/removing positions of morphing enemy zerg buildings
		if(unit->getBuildType().isBuilding())
		{
			scoutManager.addEnemyBase(unit);
		}
		if(unit->getType().isWorker())
		{
			scoutManager.removeEnemyBase(unit);
		}
	}
}

/*
updates relevant manager whenever a new unit is created
(not used when playing as zerg because all new units are morphed)
*/
void MooseBot::onUnitCreate(BWAPI::Unit* unit)
{
	if(Broodwar->self()->getRace() != BWAPI::Races::Zerg && Broodwar->getFrameCount() > 1)
	{
		if(unit->getPlayer() == Broodwar->self())
		{
			if(unit->getType().isBuilding())
			{
				productionManager.addBuilding(unit);	
			}
			if(!productionManager.emptyQueue())
			{
				BuildOrderItem<PRIORITY_TYPE>& nextElement = productionManager.getNextElement();
			//	Broodwar->printf("%s created", unit->getType().getName().c_str());
				if(unit->getType() == nextElement.metaType.unitType)
				{
					productionManager.productionStarted(unit);
				}
			}
			else
			{
				Broodwar->printf("Nothing in production queue to match new unit to");
			}
			if(unit->getType().isWorker())
			{
				productionManager.addUnit(unit);
			}
			else
			{
				//Broodwar->printf("Nothing in production queue to match new unit to");
			}
		}
	}
}

/*
updates relevant manager whenever a unit is completed
*/
void MooseBot::onUnitComplete(BWAPI::Unit* unit)
{
	if(unit->getPlayer() == Broodwar->self())
	{
		if(unit->getType().isBuilding())
		{
			productionManager.addBuilding(unit);	
		}
		else if(unit->getType().isWorker() && (Broodwar->self()->getRace() != BWAPI::Races::Zerg))
		{
			productionManager.addUnit(unit);
		}
		else if((!unit->getType().isWorker()) && (!unit->getType().isBuilding()) && (Broodwar->self()->getRace() != BWAPI::Races::Zerg))
		{
			armyManager.addUnit(unit);
		}
	}
}


/*
updates relevent manager whenever a new unit is shown
*/
void MooseBot::onUnitShow(BWAPI::Unit* unit)
{
	if(unit->getPlayer()->isEnemy(Broodwar->self()) && !unit->getType().isBuilding())
	{
		armyManager.addEnemy(unit);
	}
	if((unit->getPlayer()->isEnemy(Broodwar->self())) && (unit->getType().isBuilding()))
	{
		scoutManager.addEnemyBase(unit);
		if(unit->getType().canAttack() || (unit->getType() == BWAPI::UnitTypes::Terran_Bunker))
		{
			scoutManager.addEnemyStaticD();
		}
	}
	else if(unit->getPlayer()->isEnemy(Broodwar->self()) 
		&& (!unit->getType().isWorker())
		&& !(unit->getType() == BWAPI::UnitTypes::Zerg_Overlord)
		&& !(unit->getType() == BWAPI::UnitTypes::Zerg_Egg)
		&& !(unit->getType() == BWAPI::UnitTypes::Zerg_Larva))
	{
		//Broodwar->printf("enemy %s discovered", unit->getType( ).getName().c_str());
		scoutManager.addEnemyUnit(unit);
	}
}

/*
updates relevant manager whenever a unit becomes hidden
*/
void MooseBot::onUnitHide(BWAPI::Unit* unit)
{
	if(unit->getPlayer()->isEnemy(Broodwar->self()))
	{
		armyManager.removeEnemy(unit);
	}
}

//function taken from ExampleAIModule
DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	//self start location only available if the map has base locations
	if (BWTA::getStartLocation(BWAPI::Broodwar->self())!=NULL)
	{
		home = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
	}
	//enemy start location only available if Complete Map Information is enabled.
	if (BWTA::getStartLocation(BWAPI::Broodwar->enemy())!=NULL)
	{
		enemy_base = BWTA::getStartLocation(BWAPI::Broodwar->enemy())->getRegion();
	}
	analyzed   = true;
	analysis_just_finished = true;
	return 0;
}

//function taken from ExampleAIModule
void MooseBot::drawTerrainData()
{
	//we will iterate through all the base locations, and draw their outlines.
	for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
	{
		TilePosition p=(*i)->getTilePosition();
		Position c=(*i)->getPosition();

		//draw outline of center location
		Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);

		//draw a circle at each mineral patch
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
		{
			Position q=(*j)->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
		}

		//draw the outlines of vespene geysers
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
		{
			TilePosition q=(*j)->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
		}

		//if this is an island expansion, draw a yellow circle around the base location
		if ((*i)->isIsland())
		{
			Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
		}
	}

	//we will iterate through all the regions and draw the polygon outline of it in green.
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		BWTA::Polygon p=(*r)->getPolygon();
		for(int j=0;j<(int)p.size();j++)
		{
			Position point1=p[j];
			Position point2=p[(j+1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
		}
	}

	//we will visualize the chokepoints with red lines
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
		{
			Position point1=(*c)->getSides().first;
			Position point2=(*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
		}
	}
}

/*
if one of our buildings or workers is under attack then set the army to defend
*/
void MooseBot::checkUnderAttack()
{
	for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
	{
		if ((*i)->isUnderAttack() && ((*i)->getType().isBuilding()))// || (*i)->getType().isWorker()))
		{
			armyManager.setArmyStatus(defend);
			armyManager.setDefendPosition((*i)->getPosition());
		}
	}
}

void MooseBot::drawTimerInformation(int x, int y, double t)
{
	BWAPI::Broodwar->drawTextScreen(x, y, "\x04 %f ms", t);
	if(t * 0.001 > 55)
	{
		BWAPI::Broodwar->drawTextScreen(x, y-10, "\x04 WARNING", t);
	}
}