/*

This file is taken from UAlbertaBot by David Churchill. See: https://code.google.com/p/ualbertabot/

*/

#pragma once

#include "Common.h"
#include "BWTA.h"
#include "BuildOrderQueue.h"
#include "StarcraftBuildOrderSearchManager.h"
#include <sys/stat.h>
#include <cstdlib>

#include "..\..\StarcraftBuildOrderSearch1\Source\starcraftsearch\StarcraftData.hpp"

#define WORKERMAX 60

typedef std::pair<int, int> IntPair;
typedef std::pair<MetaType, UnitCountType> MetaPair;
typedef std::vector<MetaPair> MetaPairVector;

class StrategyManager 
{
	StrategyManager();
	~StrategyManager() {}

	std::vector<std::string>	protossOpeningBook;
	std::vector<std::string>	terranOpeningBook;
	std::vector<std::string>	zergOpeningBook;
	std::vector<int>			usableStrategies;
	int							currentStrategy;
	std::set<std::pair<BWAPI::UnitType, int>> enemyComposition;
	std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> compositionLookup;

	BWAPI::Race					selfRace;
	BWAPI::Race					enemyRace;

	void	addStrategies();
	void	populateCompositionLookup();

	const	MetaPairVector		getProtossBuildOrderGoal() const;
	const	MetaPairVector		getTerranBuildOrderGoal() const;
	const	MetaPairVector		getZergBuildOrderGoal() const;

	const std::set<std::pair<BWAPI::UnitType, int>> getGoalComposition() const;

	const	MetaPairVector		getProtossOpeningBook() const;
	const	MetaPairVector		getTerranOpeningBook() const;
	const	MetaPairVector		getZergOpeningBook() const;

public:

	enum { ProtossZealots=0, ProtossDragoons=1, NumProtossStrategies=2 };
	enum { TerranMM=0, TerranMech=1, NumTerranStrategies=2 };
	enum { ZergZerglingRush=0, ZergHydras=1, ZergMutas=2, NumZergStrategies=3 };

	static	StrategyManager &	Instance();
	
	const	bool				regroup(int numInRadius);
	const	bool				rushDetected();

	const	int					getCurrentStrategy();

	const	MetaPairVector		getBuildOrderGoal();
	const	std::string			getOpeningBook() const;

	const	void				setEnemyComposition(std::set<std::pair<BWAPI::UnitType, int>> composition);
	const   void				drawEnemyInformation(int x, int y) ;

private:
    const	void				printEnemyComposition();
	const   void				drawGoalInformation(int x, int y);
	const std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> limitByUnitType() const;
	const std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>
		limitByRatio(std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> table) const;
	const std::set<std::pair<std::set<std::pair<BWAPI::UnitType, int>>, std::set<std::pair<BWAPI::UnitType, int>>>> findClosest() const;
	const std::set<std::pair<BWAPI::UnitType, int>> getDefaultSolution() const;

};
