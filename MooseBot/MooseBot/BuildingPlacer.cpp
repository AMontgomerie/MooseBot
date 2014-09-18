/*
Source code by Adam Montgomerie. 
Distributed under GPL v3, see LICENSE for details.

Manager for finding locations for structures. placeBuilding takes a pointer to a worker (builder), 
a structure type (building), and a starting TileLocation that the structure should be located near
to (approxLocation).
*/

#include "BuildingPlacer.h"
using namespace BWAPI;

#define SPIRALLIMIT 200
#define MINERALDIST 7

BuildingPlacer::BuildingPlacer()
{
}

/*
placeBuilding will spiral outwards from approxLocation, attempting to build the structure at each
location until it succeeds or reaches SPIRALLIMIT.
Use placeExpansion() instead when building an expansion.
*/
bool BuildingPlacer::placeBuilding(BWAPI::Unit* builder, BWAPI::UnitType building, BWAPI::TilePosition approxLocation)
{
	bool closeToMinerals = false;
	bool closeToGas = false;
	int count = 0;
	int spiralCount = 1;
	bool isX = true;
	BWAPI::TilePosition buildPosition;
	BWAPI::TilePosition shiftPositionX(1,0);
	BWAPI::TilePosition shiftPositionY(0,1);

	//check parameters are acceptable
	if(builder->getType() != Broodwar->self()->getRace().getWorker())
	{
		Broodwar->printf("Building Placement Error: unit type '%s' unit cannot create structures", builder->getType().getName().c_str());
		return false;
	}
	else if(!building.isBuilding())
	{
		Broodwar->printf("Building Placement Error: type: '%s' is not a valid structure", building.getName().c_str());
		return false;
	}
	else if(!approxLocation.isValid())
	{
		approxLocation = approxLocation.makeValid();
	}
	
	//begin search for valid structure placement at approxLocation and then spiral outwards until /
	//acceptable location is found
	buildPosition = approxLocation;
	while(!builder->build(buildPosition, building))
	{
		do{
			//check the location of the closest minerals - continue to increment position without /
			//attempting to construct until the minerals are no longer too close
			//This is to avoid constructing structures in mineral lines
			Unit* closestMineral=NULL;
			for(std::set<BWAPI::Unit*>::iterator m=Broodwar->getMinerals().begin();m!=Broodwar->getMinerals().end();m++)
			{
				if (closestMineral==NULL || buildPosition.getDistance((*m)->getTilePosition()) < buildPosition.getDistance(closestMineral->getTilePosition()))
				closestMineral=*m;
			}
			if (closestMineral!=NULL)
			{
				if(buildPosition.getDistance(closestMineral->getTilePosition()) < MINERALDIST)
				{
					closeToMinerals = true;
				}
				else
				{
					closeToMinerals = false;
				}
			}
			
			//check we aren't blocking gas too
			Unit* closestGas=NULL;
			for(std::set<BWAPI::Unit*>::iterator g=Broodwar->getGeysers().begin();g!=Broodwar->getGeysers().end();g++)
			{
				if (closestGas==NULL || buildPosition.getDistance((*g)->getTilePosition()) < buildPosition.getDistance(closestGas->getTilePosition()))
				closestGas=*g;
			}
			if (closestGas!=NULL)
			{
				if(buildPosition.getDistance(closestMineral->getTilePosition()) < MINERALDIST)
				{
					closeToGas= true;
				}
				else
				{
					closeToGas = false;
				}
			}

			if(count % spiralCount == 0)
			{
				if(isX) 
				{
					isX = false;
					count = 0;
				}
				else {
					spiralCount++;
					count = 0;
					isX = true;
				}
			}
			count++;
			if(spiralCount % 2 == 0)
			{
				if(isX)
				{
					buildPosition -= shiftPositionX;
				}
				else
				{
					buildPosition -= shiftPositionY;
				}
			}
			else 
			{
				if(isX)
				{
					buildPosition += shiftPositionX;
				}
				else
				{
					buildPosition += shiftPositionY;
				}
			}
			//search is cut off at SPIRALLIMIT to prevent it from taking too long or placing building to far from approxLocation
			if(spiralCount == SPIRALLIMIT)
			{
			//	Broodwar->printf("Building Placement Error: no acceptable build location found for %s", building.getName().c_str());
				return false;
			}
		} while(closeToMinerals || closeToGas);
	}
	return true;
}

/*
find the closest available base location to the given unit
*/
BWAPI::TilePosition BuildingPlacer::getClosestBase(BWAPI::Unit* unit)
{
	BWAPI::TilePosition buildPosition = TilePosition(0,0);
	double minDist = 0;
	bool taken;

	for(std::set<BWTA::BaseLocation*>::const_iterator i = BWTA::getBaseLocations().begin(); i != BWTA::getBaseLocations().end(); i++)
	{
		taken = false;
		for(std::set<BWAPI::Unit*>::const_iterator j = expansions.begin(); j != expansions.end(); j++)
		{
			if((*j)->getTilePosition() == (*i)->getTilePosition())
			{
				taken = true;
			}
		}
		if(!taken)
		{
			if((minDist == 0) || (unit->getPosition().getDistance((*i)->getPosition()) < minDist))
			{
				minDist = unit->getPosition().getDistance((*i)->getPosition());
				buildPosition = (*i)->getTilePosition();
			}
		}
	}
	if(buildPosition == TilePosition(0,0))
	{
		Broodwar->printf("cant find new expansion location");
	}
	return buildPosition;
}

/*
construct the given building type at the given builder's current location
this method should be used rather than placeBuilding() for expansions to avoid the expansion being misplaced
*/
bool BuildingPlacer::placeExpansion(BWAPI::Unit* builder, BWAPI::UnitType building, BWAPI::TilePosition location)
{
	if(builder->build(TilePosition(builder->getPosition()), building))
	{
		return true;
	}
	else 
	{
		return false;
	}
}

void BuildingPlacer::addExpansion(BWAPI::Unit* expansion)
{
	expansions.insert(expansion);
}

void BuildingPlacer::removeExpansion(BWAPI::Unit* expansion)
{
	expansions.erase(expansion);
}

/*
construct a refinery/assimilator/extractor at the nearest available gas geyser with the given builder
*/
bool BuildingPlacer::placeGas(BWAPI::Unit* builder, BWAPI::UnitType building)
{
	BWAPI::Unit* closestGas = NULL;
	for(std::set<BWAPI::Unit*>::iterator i = Broodwar->getGeysers().begin(); i != Broodwar->getGeysers().end(); i++)
	{
		if (closestGas == NULL || builder->getDistance(*i) < builder->getDistance(closestGas))
		closestGas=*i;
	}
	if (closestGas != NULL)
	{
		if(builder->build(closestGas->getTilePosition(), building))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	Broodwar->printf("BuildingPlacer Error: unable to find any gas geysers");
	return false;
}

bool BuildingPlacer::placeAddon(BWAPI::Unit* builder, BWAPI::UnitType building)
{
	bool closeToMinerals = false;
	bool addonStarted = false;
	int count = 0;
	int spiralCount = 1;
	bool isX = true;
	BWAPI::TilePosition buildPosition = builder->getTilePosition();
	BWAPI::TilePosition shiftPositionX(1,0);
	BWAPI::TilePosition shiftPositionY(0,1);

	if(builder->buildAddon(building))
	{
		return true;
	}
	else
	{
		while(!addonStarted)
		{
			builder->lift();

			if(count % spiralCount == 0)
			{
				if(isX) 
				{
					isX = false;
					count = 0;
				}
				else {
					spiralCount++;
					count = 0;
					isX = true;
				}
			}
			count++;
			if(spiralCount % 2 == 0)
			{
				if(isX)
				{
					buildPosition -= shiftPositionX;
				}
				else
				{
					buildPosition -= shiftPositionY;
				}
			}
			else 
			{
				if(isX)
				{
					buildPosition += shiftPositionX;
				}
				else
				{
					buildPosition += shiftPositionY;
				}
			}

			if(builder->land(buildPosition))
			{
				if(builder->buildAddon(building))
				{
					addonStarted = true;
				}
			}

			//search is cut off at SPIRALLIMIT to prevent it from taking too long or placing building to far from approxLocation
			if(spiralCount == SPIRALLIMIT)
			{
			//	Broodwar->printf("Building Placement Error: no acceptable build location found for %s", building.getName().c_str());
				return false;
			}
		}
	}
	return true;
}

std::set<BWAPI::Unit*> BuildingPlacer::getExpansions()
{
	return expansions;
}

/*
returns the second closest base, if the closest is unavailable for some reason
*/
BWAPI::TilePosition BuildingPlacer::getNextClosestBase(BWAPI::Unit* unit)
{
	BWAPI::TilePosition buildPosition = TilePosition(0,0);
	double minDist = 0;
	bool taken;

	for(std::set<BWTA::BaseLocation*>::const_iterator i = BWTA::getBaseLocations().begin(); i != BWTA::getBaseLocations().end(); i++)
	{
		taken = false;
		for(std::set<BWAPI::Unit*>::const_iterator j = expansions.begin(); j != expansions.end(); j++)
		{
			if((*j)->getTilePosition() == (*i)->getTilePosition())
			{
				taken = true;
			}
		}
		if(!taken)
		{																							
			if((minDist == 0) || (unit->getPosition().getDistance((*i)->getPosition()) < minDist) &&
				(unit->getTilePosition().getDistance((*i)->getTilePosition()) != unit->getTilePosition().getDistance((getClosestBase(unit)))))
			{
				minDist = unit->getPosition().getDistance((*i)->getPosition());
				buildPosition = (*i)->getTilePosition();
			}
		}
	}
	if(buildPosition == TilePosition(0,0))
	{
		Broodwar->printf("cant find new expansion location");
	}
	return buildPosition;
}