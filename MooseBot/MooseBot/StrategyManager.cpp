/*

This file is based on StrategyManager.cpp from UAlbertaBot by David Churchill. See: https://code.google.com/p/ualbertabot/
Modified August 2014.
Distributed under GPL v3, see LICENSE for details.
*/

#include "Common.h"
#include "StrategyManager.h"

// constructor
StrategyManager::StrategyManager() 
	: currentStrategy(0)
	, selfRace(BWAPI::Broodwar->self()->getRace())
	, enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	addStrategies();
	populateCompositionLookup();
}

// get an instance of this
StrategyManager & StrategyManager::Instance() 
{
	static StrategyManager instance;
	return instance;
}

void StrategyManager::addStrategies() 
{
	protossOpeningBook = std::vector<std::string>(NumProtossStrategies);
	terranOpeningBook  = std::vector<std::string>(NumTerranStrategies);
	zergOpeningBook    = std::vector<std::string>(NumZergStrategies);

    protossOpeningBook[ProtossZealots]	= "0 0 0 0 1 0  0 3 0 3 0 4 1 4 4 0 4 4 0 1 4 3 0 1";
	protossOpeningBook[ProtossDragoons]		= "0 0 0 0 1 0 0 3 0 7 0 0 5 0 0 3 8 6 1 6 6 0 3 1 0 6 6 6";
	terranOpeningBook[TerranMM]		= "0 0 0 0 0 1 0 0 3 0 0 3 0 1 0 4 0 0 0 6";
	terranOpeningBook[TerranMech]	= "0 0 0 0 0 1 0 0 3 4 0 0 0 0 0 9 1 0 0 9 13 5 0 5 0 13 5 15";
	zergOpeningBook[ZergZerglingRush]		= "0 0 0 0 0 1 0 0 0 2 3 5 0 0 0 0 0 0 1";
	zergOpeningBook[ZergHydras]				= "0 0 0 0 0 1 0 0 0 3 5 0 0 0 7";
	zergOpeningBook[ZergMutas]				= "0 0 0 0 0 3 5 0 0 1 6 0 0 0 0 8";

	if (selfRace == BWAPI::Races::Protoss)
	{
		usableStrategies.push_back(ProtossZealots);
		usableStrategies.push_back(ProtossDragoons);
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		usableStrategies.push_back(TerranMM);
		usableStrategies.push_back(TerranMech);
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		usableStrategies.push_back(ZergZerglingRush);
		usableStrategies.push_back(ZergHydras);
		usableStrategies.push_back(ZergMutas);
	}
}

const std::string StrategyManager::getOpeningBook() const
{
	if (selfRace == BWAPI::Races::Protoss)
	{
		if(enemyRace != NULL)
		{
			if(enemyRace == BWAPI::Races::Terran)
			{
				return protossOpeningBook[ProtossDragoons];
			}
			else
			{
				return protossOpeningBook[ProtossZealots];
			}
		}
		else
		{
			return protossOpeningBook[ProtossZealots];
		}
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		if(enemyRace != NULL)
		{
			if(enemyRace == BWAPI::Races::Zerg)
			{
				return terranOpeningBook[TerranMM];
			}
			else
			{
				return terranOpeningBook[TerranMech];
			}
		}
		else
		{
			return terranOpeningBook[TerranMM];
		}
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		return zergOpeningBook[ZergHydras];
	} 

	// something wrong, return the protoss one
	return protossOpeningBook[currentStrategy];
}

const MetaPairVector StrategyManager::getBuildOrderGoal()
{
	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss)
	{
		return getProtossBuildOrderGoal();
	}
	else if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		return getTerranBuildOrderGoal();
	}
	else
	{
		return getZergBuildOrderGoal();
	}
}

const MetaPairVector StrategyManager::getProtossBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;

//	StrategyManager::Instance().printEnemyComposition();

	int numZealots =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numReavers =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Reaver);
	int numDarkTemplars =		BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar);
	int numCarriers =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Carrier);
	int numObservers =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Observer);
	int numArbiters =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Arbiter);
	int numArchons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Archon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numCorsairs =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Corsair);
	int numHighTemplar =		BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_High_Templar);

	int zealotsWanted = numZealots;
	int dragoonsWanted = numDragoons;
	int reaversWanted = numReavers;
	int darkTemplarsWanted = numDarkTemplars;
	int carriersWanted = numCarriers;
	int observersWanted = numObservers;
	int arbitersWanted = numArbiters;
	int archonsWanted = numArchons;
	int probesWanted = numProbes;
	int corsairsWanted = numCorsairs;
	int highTemplarsWanted = numHighTemplar;

	std::set<std::pair<BWAPI::UnitType, int>> goalComp = getGoalComposition();

	int nexusCount = 0;

	for(std::set<BWAPI::Unit*>::const_iterator i=BWAPI::Broodwar->self()->getUnits().begin();i!=BWAPI::Broodwar->self()->getUnits().end();i++)
	{
		if((*i)->getType().isResourceDepot())
		{
			nexusCount++;
		}
	}

	if((numProbes < (nexusCount * 20)) && (numProbes < WORKERMAX))
	{
		probesWanted += nexusCount * 4;
		if(probesWanted > WORKERMAX)
		{
			probesWanted = WORKERMAX;
		}
	}

	for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator i = goalComp.begin(); i != goalComp.end(); i++)
	{

		if((*i).first == BWAPI::UnitTypes::Protoss_Zealot)
		{
			zealotsWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Dragoon)
		{
			dragoonsWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Reaver)
		{
			reaversWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Dark_Templar)
		{
			darkTemplarsWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Carrier)
		{
			carriersWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Observer)
		{
			observersWanted += ((*i).second / 10);
			if(observersWanted > 4)
			{
				observersWanted = 4;
			}
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Arbiter)
		{
			arbitersWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Archon)
		{
			archonsWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_Corsair)
		{
			corsairsWanted += ((*i).second / 10);
		}
		if((*i).first == BWAPI::UnitTypes::Protoss_High_Templar)
		{
			highTemplarsWanted += ((*i).second / 10);
		}
	}

	/*
	if(enemyRace == BWAPI::Races::Zerg)
	{
		//go through the currently known enemy unit composition and create a set of counter units
		//the enemy unit composition is stored as a percentage
		//we will create a set of 10 units of types equivalent to the 
		if(enemyComposition.empty())
		{
			zealotsWanted += 10;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if((*unit).first == BWAPI::UnitTypes::Zerg_Mutalisk)
				{
					dragoonsWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable() || 
					((*unit).first == BWAPI::UnitTypes::Zerg_Lurker) || 
					((*unit).first == BWAPI::UnitTypes::Zerg_Lurker_Egg))
				{
					observersWanted += 1;
				}
				else
				{
					zealotsWanted += ((*unit).second / 10);
				}
			}
		}
	}
	else if(enemyRace == BWAPI::Races::Terran)
	{
		//go through the currently known enemy unit composition and create a set of counter units
		//the enemy unit composition is stored as a percentage
		//we will create a set of 10 units of types equivalent to the 
		if(enemyComposition.empty())
		{
			dragoonsWanted += 8;
			zealotsWanted += 2;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if(((*unit).first == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode) || ((*unit).first == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode))
				{
					zealotsWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable())
				{
					if(observersWanted < 4)
					{
						observersWanted += 1;
					}
				}
				else
				{
					dragoonsWanted += (((*unit).second * 0.8) / 10);
					zealotsWanted += (((*unit).second * 0.2) / 10);
				}
			}
		}
	}
	else if(enemyRace == BWAPI::Races::Protoss)
	{
		//go through the currently known enemy unit composition and create a set of counter units
		//the enemy unit composition is stored as a percentage
		//we will create a set of 10 units of types equivalent to the 
		if(enemyComposition.empty())
		{
			zealotsWanted += 10;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if((*unit).first.isFlyer())
				{
					dragoonsWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable() || (*unit).first.hasPermanentCloak())
				{
					if(observersWanted < 4)
					{
						observersWanted += 1;
					}
				}
				else
				{
					zealotsWanted += ((*unit).second / 10);
				}
			}
		}
	}
	else
	{
		if(enemyComposition.empty())
		{
			zealotsWanted += 5;
			dragoonsWanted += 5;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if((*unit).first.isFlyer() || ((*unit).first == BWAPI::UnitTypes::Terran_Marine))
				{
					dragoonsWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable() || (*unit).first.hasPermanentCloak())
				{
					if(observersWanted < 4)
					{
						observersWanted += 1;
					}
				}
				else
				{
					zealotsWanted += ((*unit).second / 10);
				}
			}
		}
	}
	*/

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Zealot, zealotsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Reaver, reaversWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Dark_Templar, darkTemplarsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Carrier, carriersWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Observer, observersWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Arbiter, arbitersWanted));
//	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Archon, archonsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Probe, probesWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_Corsair, corsairsWanted));
	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Protoss_High_Templar, highTemplarsWanted));

	/*
	BWAPI::Broodwar->printf("goal units:");
	for(std::vector<std::pair<MetaType, UnitCountType>>::iterator unit = goal.begin(); unit != goal.end(); unit++)
	{
		BWAPI::Broodwar->printf("%d %s", unit->second, unit->first.getName().c_str());
	}
	*/

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

const MetaPairVector StrategyManager::getTerranBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;

//	StrategyManager::Instance().printEnemyComposition();

	int numMarines =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Marine);
	int numMedics =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Medic);
	int numWraith =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Wraith);
	int numVultures =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Vulture);
	int numTanks =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode);
	int numGoliaths =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Goliath);
	int numScienceVessels =		BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Science_Vessel);
	int numFirebats =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Firebat);

	if(enemyRace == BWAPI::Races::Zerg)
	{
		int marinesWanted = numMarines;
		int medicsWanted = numMedics;
		int firebatsWanted = numTanks;
		int scienceVesselsWanted = numScienceVessels;
		int tanksWanted = numTanks;

		//go through the currently known enemy unit composition and create a set of counter units
		//the enemy unit composition is stored as a percentage
		//we will create a set of 10 units of types equivalent to the 
		if(enemyComposition.empty())
		{
			marinesWanted += 8;
			medicsWanted += 2;
		//	scienceVesselsWanted += 1;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if((*unit).first == BWAPI::UnitTypes::Zerg_Zergling)
				{
					firebatsWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable() || 
					((*unit).first == BWAPI::UnitTypes::Zerg_Lurker) || 
					((*unit).first == BWAPI::UnitTypes::Zerg_Lurker_Egg))
				{
					scienceVesselsWanted += ((*unit).second / 10);
				}
				else if((*unit).first == BWAPI::UnitTypes::Zerg_Hydralisk)
				{
					tanksWanted += ((*unit).second / 10);
				}
				else
				{
					marinesWanted += (((*unit).second * 0.8) / 10);
					medicsWanted += (((*unit).second * 0.2) / 10);
				}
			}
		}

		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, marinesWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Medic,	medicsWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Firebat, firebatsWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Vessel, scienceVesselsWanted));
	}
	else
	{
		int vulturesWanted = numVultures;
		int tanksWanted = numTanks;
		int goliathsWanted = numGoliaths;
		int scienceVesselsWanted = 0;

		//go through the currently known enemy unit composition and create a set of counter units
		//the enemy unit composition is stored as a percentage
		//we will create a set of 10 units of types equivalent to the 
		if(enemyComposition.empty())
		{
			vulturesWanted += 10;
		//	tanksWanted += 3;
		//	goliathsWanted += 3;
		}
		else
		{
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
			{
				if((*unit).first.isFlyer())
				{
					goliathsWanted += ((*unit).second / 10);
				}
				else if((*unit).first == BWAPI::UnitTypes::Protoss_Dragoon)
				{
					tanksWanted += ((*unit).second / 10);
				}
				else if((*unit).first.isCloakable())
				{
					scienceVesselsWanted = 1;
				}
				else 
				{
					vulturesWanted += ((*unit).second / 10);
				}
			}
		}

		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Vulture, vulturesWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, tanksWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Goliath, goliathsWanted));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Vessel, scienceVesselsWanted));
	}

	/*
	BWAPI::Broodwar->printf("goal units:");
	for(std::vector<std::pair<MetaType, UnitCountType>>::iterator unit = goal.begin(); unit != goal.end(); unit++)
	{
		BWAPI::Broodwar->printf("%d %s", unit->second, unit->first.getName().c_str());
	}
	*/

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

const MetaPairVector StrategyManager::getZergBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;
	
	int numMutas  =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk);
	int numHydras  =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk);

	int mutasWanted = numMutas + 6;
	int hydrasWanted = numHydras + 6;

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Hydralisk, hydrasWanted));
	//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Mutalisk, mutasWanted));
	//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Zergling, 6));

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

 const int StrategyManager::getCurrentStrategy()
 {
	 return currentStrategy;
 }

 /*
 sets the current enemy unit composition
 */
 const	void StrategyManager::setEnemyComposition(std::set<std::pair<BWAPI::UnitType, int>> composition)
 {
	 enemyComposition = composition;
	 //StrategyManager::Instance().printEnemyComposition();
 }

 /*
 prints the current enemy unit composition in terms of percentages of each unit type
 */
const void StrategyManager::printEnemyComposition()
 {
	 BWAPI::Broodwar->printf("Enemy Composition:");
	 for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
	 {
		 BWAPI::Broodwar->printf("%d%% %s", (*unit).second, (*unit).first.getName().c_str());
	 }
 }

const void StrategyManager::drawEnemyInformation(int x, int y) 
{
	int i = 0;

	BWAPI::Broodwar->drawTextScreen(x, y-10,"Enemy Unit Composition:");

	if(enemyComposition.empty())
	{
		BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "\x04Unknown");
	}

	for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator unit = enemyComposition.begin(); unit != enemyComposition.end(); unit++)
	{
		BWAPI::Broodwar->drawTextScreen(x, y+(i*10), "\x04%d%% %s", (*unit).second, (*unit).first.getName().c_str());
		i++;
	}
}

/*
populate the composition lookup table with enemy unit compositions and their corresponding solutions
*/
void StrategyManager::populateCompositionLookup()
{
	std::set<std::pair<BWAPI::UnitType, int>> enemyComp;
	std::set<std::pair<BWAPI::UnitType, int>> solution;

	/////////////////////////////PvT//////////////////////////////////////////////

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Firebat, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 80));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Medic, 20));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 70));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 30));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Medic, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 40));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 60));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 30));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Medic, 30));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Firebat, 30));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 50));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 50));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Medic, 25));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Firebat, 25));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 70));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode, 25));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Arbiter, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Vulture, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Arbiter, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Vulture, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Arbiter, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Marine, 70));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, 25));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Arbiter, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

/////////////////////////////PvZ//////////////////////////////////////////////

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Zergling, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Zergling, 80));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Ultralisk, 20));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 90));
//	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Archon, 10));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dark_Templar, 10)); //dt rather than archon because archon morphin not implemented

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Zergling, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Mutalisk, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 40));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 20));
//	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Archon, 10));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Corsair, 40));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Mutalisk, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 100));
//	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Archon, 20)); //dt rather than archon because archon morph is not implemented

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Zergling, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Hydralisk, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 90));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dark_Templar, 10)); //ideally HT here but haven't implemented control for storm

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Hydralisk, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 90));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dark_Templar, 10)); //ideally HT here but haven't implemented control for storm

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Zergling, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Lurker, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 90));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Observer, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Hydralisk, 50));
	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Lurker, 50));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 45));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Observer, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Zerg_Guardian, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 50));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Corsair, 50));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

/////////////////////////////PvP//////////////////////////////////////////////

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 100));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dark_Templar, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 90));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Observer, 10));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

	enemyComp.clear();
	solution.clear();

	enemyComp.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Carrier, 100));

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Corsair, 80));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 20));

	compositionLookup.insert(std::make_pair(enemyComp, solution));

}

/*
checks the current enemy unit composition against the composition lookup table and returns the most appropriate solution
*/
const std::set<std::pair<BWAPI::UnitType, int>> StrategyManager::getGoalComposition() const
{
	std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> tempTable;
	std::set<std::pair<BWAPI::UnitType, int>> solution; 

	//filter the lookup table to only include rows where the set of unit types is the same as the current enemy composition
	tempTable = limitByUnitType();

	if(tempTable.empty())
	{
		tempTable = findClosest();
	}

	//out of the remaining table rows, find the row that has the closest ratio of unit types to the current enemy composition
	solution = limitByRatio(tempTable).second;

	
	if(solution.empty())
	{
		solution = getDefaultSolution();
	}
	

	return solution;
}

/*
returns a set of solutions where the enemy composition has the same types of units as the current enemy composition
*/
const std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> StrategyManager::limitByUnitType() const
{
	std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> matchingComps;

	//for each row in the lookup table
	for(std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>>::const_iterator 
		lookupRow = compositionLookup.begin(); lookupRow != compositionLookup.end(); lookupRow++)
	{
		int matchCount = 0;

		//for each element in the vector containing the enemy composition for this row of the lookup table
		for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyKey = lookupRow->first.begin(); enemyKey != lookupRow->first.end(); enemyKey++)
		{
			//for each type of unit in the current enemy composition
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyUnit = enemyComposition.begin(); enemyUnit != enemyComposition.end(); enemyUnit++)
			{
				//if the unitType is found in the lookup table and in the current composition, we have a match
				if(enemyUnit->first == enemyKey->first)
				{
					matchCount++;
				}
			}
		}
		//if the unit composition in this row of the table has the same unit types as the current enemy composition then add it to the set of matching compositions
		if(matchCount == lookupRow->first.size() && (matchCount == enemyComposition.size()))
		{
			matchingComps.insert(*lookupRow);
		}
	}

	return matchingComps;
}

/*
find the unit composition that has the closest match of unit types to the current enemy composition
*/
const std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> StrategyManager::findClosest() const
{
	std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> closestMatches;
	
	int smallestDifference = -1;

	//for each row in the lookup table
	for(std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>>::const_iterator 
		lookupRow = compositionLookup.begin(); lookupRow != compositionLookup.end(); lookupRow++)
	{
		int matchCount = 0;
		int difference = lookupRow->first.size();

		//for each element in the vector containing the enemy composition for this row of the lookup table
		for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyKey = lookupRow->first.begin(); enemyKey != lookupRow->first.end(); enemyKey++)
		{
			//for each type of unit in the current enemy composition
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyUnit = enemyComposition.begin(); enemyUnit != enemyComposition.end(); enemyUnit++)
			{
				//if the unitType is found in the lookup table and in the current composition, we have a match
				if(enemyUnit->first == enemyKey->first)
				{
					matchCount++;
				}
			}
		}

		difference -= matchCount; //the difference is the total number of unit types in the composition - the number of matching units

		if((smallestDifference == -1) || (difference < smallestDifference))
		{
			smallestDifference = difference;
			closestMatches.clear();
			closestMatches.insert(*lookupRow);
		}
		else if(difference == smallestDifference)
		{
			closestMatches.insert(*lookupRow);
		}
		
	}

	//we will return a set containing the closest match which will be 1 or more rows
	return closestMatches;

}

/*
takes a set of rows from the composition lookup table that have matching unit types to the 
*/
const std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>
	StrategyManager::limitByRatio(std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> table) const
{
	std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>> closestMatch;

	int minimumDifference = 0;

	//for each row in the table
	for(std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>>::const_iterator 
		row = table.begin(); row != table.end(); row++)
	{
		int totalDifference = 0;

		//for each element in the enemy composition stored in this table row
		for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyKey = row->first.begin(); enemyKey != row->first.end(); enemyKey++)
		{
			int difference = -1;

			//for each element in the current enemy composition
			for(std::set<std::pair<BWAPI::UnitType, int>>::const_iterator enemyUnit = enemyComposition.begin(); enemyUnit != enemyComposition.end(); enemyUnit++)
			{
				//the difference is the difference between the % of the unit in the two compositions
				//e.g. marine 20% and marine 10% difference will be 10
				if(enemyUnit->first == enemyKey->first)
				{
					if(enemyUnit->second > enemyKey->second)
					{
						difference = enemyUnit->second - enemyKey->second;
					}
					else
					{
						difference = enemyKey->second - enemyUnit->second;
					}
				}
			}
			//if the unit is not in the current enemy composition at all then the difference is the % of the unit in the composition from the table
			if(difference == -1)
			{
				difference = enemyKey->second;
			}

			totalDifference += difference;
		}

		if((minimumDifference == 0) || (totalDifference < minimumDifference))
		{
			minimumDifference = totalDifference;
			closestMatch = (*row);
		}
	}

	return closestMatch;
}

/*
in the situation that we are unable to find an appropriate solution from the lookup table, return a pre-determined default solution
*/
const std::set<std::pair<BWAPI::UnitType, int>> StrategyManager::getDefaultSolution() const
{
	std::set<std::pair<BWAPI::UnitType, int>> solution;

	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Dragoon, 50));
	solution.insert(std::make_pair(BWAPI::UnitTypes::Protoss_Zealot, 50));

	BWAPI::Broodwar->printf("No solution found. Returning default solution");

	return solution;
}