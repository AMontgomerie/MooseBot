#pragma once
#include <BWAPI.h>
#include <BWTA.h>

#define XMAX 2000
#define YMAX 2000

class BuildingPlacer
{
	std::set<BWAPI::Unit*> expansions;
	public:
		BuildingPlacer::BuildingPlacer();
		bool BuildingPlacer::placeBuilding(BWAPI::Unit* builder, BWAPI::UnitType building, BWAPI::TilePosition approxLocation);
		BWAPI::TilePosition BuildingPlacer::getClosestBase(BWAPI::Unit* unit);
		BWAPI::TilePosition BuildingPlacer::getNextClosestBase(BWAPI::Unit* unit);
		bool BuildingPlacer::placeGas(BWAPI::Unit* builder, BWAPI::UnitType building);
		bool BuildingPlacer::placeExpansion(BWAPI::Unit* builder, BWAPI::UnitType building, BWAPI::TilePosition location);
		bool BuildingPlacer::placeAddon(BWAPI::Unit* builder, BWAPI::UnitType building);
		void BuildingPlacer::addExpansion(BWAPI::Unit* expansion);
		void BuildingPlacer::removeExpansion(BWAPI::Unit* expansion);
		std::set<BWAPI::Unit*> BuildingPlacer::getExpansions();
};
