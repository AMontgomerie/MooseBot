/*
ArmyManager keeps track of all military units under the player's control.
A pointer to each unit is stored in allArmy.
From here, individual units can be accessed so that they can be given orders,
or orders can be issued to all units, or all units of a given UnitType.
*/

#include "ArmyManager.h"
using namespace BWAPI;

ArmyManager::ArmyManager(void) : armyStatus(scout),
								regroupOrdered(false), 
								regroupFrame(0),
								rallyPoint(Position(0,0)),
								attackIssued(false),
								retreatIssued(false)
{
	srand((unsigned int)time(NULL));
}

/*
Should be called each frame.
*/
void ArmyManager::update()
{
	updateStatus();
	drawArmyStatus(320, 10);

	if((armyStatus == attack) && (Broodwar->getFrameCount() % 24 == 0))
	{
		executeAttack();
	}
	if(armyStatus == defend && (Broodwar->getFrameCount() % 24 == 0))
	{
		executeDefence();
	}
}

/*
issues commands to army units during an attack
*/
void ArmyManager::executeAttack()
{
	BWAPI::Unit* closestEnemy = NULL;
	BWAPI::Unit* closestUnit = NULL;

	for(std::set<std::pair<Unit*,int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
	{
		//if this is a unit that can't attack but that we want to have with our army (e.g. observer)
		//tell it to follow a friendly army unit that can attack
		if(!(*i).first->getType().canAttack() || ((*i).first->getType() == BWAPI::UnitTypes::Protoss_Arbiter))
		{
			//find the closest unit to the attack position
			for(std::set<std::pair<Unit*, int>>::const_iterator j = allArmy.begin(); j != allArmy.end(); j++)
			{
				if((closestUnit == NULL) || attackPosition.getDistance((*j).first->getPosition()) < attackPosition.getDistance(closestUnit->getPosition()) && (*j).first->getType().canAttack())
				{
					closestUnit = (*j).first;
				}
			}
			if(closestUnit != NULL)
			{
				(*i).first->attack(closestUnit->getPosition(), false);
			}
		}
		//find a target for our army units that can attack
		else
		{
			closestEnemy = getClosestEnemy((*i).first);
			if((closestEnemy != NULL) && closestEnemy->isVisible())
			{
				if((*i).first->isUnderAttack())
				{
					(*i).first->attack(closestEnemy, false);
				}
				else if((*i).first->isIdle())
				{
					(*i).first->attack(closestEnemy, false);
				}
			}
			else
			{
				if((*i).first->isIdle() && (attackPosition != Position(0,0)) && (attackPosition != NULL))
				{
					(*i).first->attack(attackPosition, false);
				}
			}
		}
	}
}

/*
issues commands to our army units during a defence and checks if we still need to defend
once there are no more visible enemy units, return to retreat status
*/
void ArmyManager::executeDefence()
{
	regroupOrdered = false;
	BWAPI::Unit* closestEnemy = NULL;

	if(defendPosition != NULL)
	{
		//if we don't have any fighting units, use workers to defend
		if(allArmy.size() == 0)
		{
			workerCombat();
		}
		//otherwise, use our army to defend
		else
		{
			clearCombatWorkers();

			for(std::set<std::pair<Unit*,int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
			{
				//if this is a unit that can't attack but that we want to have with our army (e.g. observer)
				//tell it to follow a friendly army unit that can attack
				if(!(*i).first->getType().canAttack())
				{
					//find the closest unit to the attack position
					for(std::set<std::pair<Unit*, int>>::const_iterator j = allArmy.begin(); j != allArmy.end(); j++)
					{
						if((closestEnemy == NULL) || attackPosition.getDistance((*j).first->getPosition()) < attackPosition.getDistance(closestEnemy->getPosition()) && (*j).first->getType().canAttack())
						{
							closestEnemy = (*j).first;
						}
					}
					if(closestEnemy != NULL)
					{
						(*i).first->move(closestEnemy->getPosition(), false);
					}
				}
				closestEnemy = getClosestEnemy((*i).first);
				if(closestEnemy != NULL && closestEnemy->isVisible())
				{
					if((*i).first->isUnderAttack())
					{
						(*i).first->attack(closestEnemy, false);
					}
					else if((*i).first->isIdle())
					{
						(*i).first->attack(closestEnemy, false);
					}
				}
				else if((*i).first->isIdle())
				{
					(*i).first->attack(defendPosition, false);
				}
			}
		}
		if((getClosestEnemy(defendPosition) == NULL) || //if there are no enemies nearby
			((getClosestEnemy(defendPosition)->getType().isBuilding()) && !(getClosestEnemy(defendPosition)->getType().canAttack())) || //or the enemy is a building that can't attack
			(getClosestEnemy(defendPosition)->getDistance(defendPosition) > 1000)) //or the closest enemy is too far away
		{
			//clear defense status and return to retreat (will automatically switch to attack if appropriate)
			armyStatus = retreat;
			retreatIssued = true;
			attackIssued = false;
		}
	}
}

/*
finds the closest enemy unit to the given unit and returns a pointer to them
*/
BWAPI::Unit* ArmyManager::getClosestEnemy(BWAPI::Unit* unit)
{
	BWAPI::Unit* closestEnemy = NULL;

//	for(std::set<Unit*>::const_iterator i = Broodwar->enemy()->getUnits().begin(); i != Broodwar->enemy()->getUnits().end(); i++)
	for(std::set<Unit*>::const_iterator i = visibleEnemies.begin(); i != visibleEnemies.end(); i++)
	{
		//find an enemy who...
		if ((closestEnemy == NULL || unit->getDistance(*i) < unit->getDistance(closestEnemy))	//is closer than previous enemies we have checked
			&& (*i)->getType().canAttack()														//can attack (so we prioritise fighting units over workers or buildings
			&& (*i)->isVisible()																//we can see
			&& !(((*i)->isBurrowed() || (*i)->isCloaked()) && !haveDetection()))				//if they are cloaked or burrowed then only target them if we have detection with our army
		closestEnemy = (*i);
	}
	if (closestEnemy == NULL)
	{
		//Broodwar->printf("ArmyManager Error: no enemies found");
	}
	return closestEnemy;
}

/*
finds the closest enemy unit to the given unit and returns a pointer to them
*/
BWAPI::Unit* ArmyManager::getClosestEnemy(BWAPI::Position position)
{
	BWAPI::Unit* closestEnemy = NULL;

	for(std::set<Unit*>::const_iterator i = Broodwar->enemy()->getUnits().begin(); i != Broodwar->enemy()->getUnits().end(); i++)
	{
		if ((closestEnemy == NULL || position.getDistance((*i)->getPosition()) < position.getDistance(closestEnemy->getPosition())) && (*i)->isVisible())
		closestEnemy = (*i);
	}
	if (closestEnemy == NULL)
	{
		//Broodwar->printf("ArmyManager Error: no enemies found");
	}
	return closestEnemy;
}

/*
add a unit to the army
*/
void ArmyManager::addUnit(BWAPI::Unit* unit)
{
	allArmy.insert(std::make_pair(unit, 0));
	if(rallyPoint != Position(0,0))
	{
		if(regroupOrdered)
		{
			unit->attack(regroupPosition, false);
		}
		else
		{
			unit->attack(rallyPoint, false);
		}
	}
}

/*
remove a unit from the army
*/
void ArmyManager::removeUnit(BWAPI::Unit* unit)
{
	for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
	{
		if((*i).first == unit)
		{
			allArmy.erase(*i);
			break;
		}
	}
	//check through units that have been issued a regroup command to make sure we aren't going to be waiting for a dead unit to arrive
	for(std::set<Unit*>::const_iterator i = regroupingUnits.begin(); i != regroupingUnits.end(); i++)
	{
		if((*i) == unit)
		{
			regroupingUnits.erase(*i);
			break;
		}
	}
}

/*
returns the number of military units under the player's control
*/
int ArmyManager::getUnitCount()
{
	return allArmy.size();
}

/*
returns the number of units of type unitType under the player's control.
*/
int ArmyManager::getUnitCount(BWAPI::UnitType unitType)
{
	int count = 0;

	if(unitType != NULL)
	{
		for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
		{
			if((*i).first->getType() == unitType)
			{
				count++;
			}
		}
		return count;
	}
	else
	{
		Broodwar->printf("ArmyManager Error: cannot return count for NULL Unit Type.");
		return 0;
	}
}

/*
tell every unit in the army to attack-move to a position on the map
*/
void ArmyManager::allAttack(BWAPI::Position position)
{
	if(position != NULL)
	{
		for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
		{
			(*i).first->attack(position, false);
		}
		armyStatus = attack;
		attackPosition = position;
	}
	else
	{
		Broodwar->printf("ArmyManager Error: invalid attack position");
	}
}

/*
tell every unit in the army to attack a specific unit
*/
void ArmyManager::allAttack(BWAPI::Unit* target)
{
	if(target != NULL)
	{
		for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
		{
			(*i).first->attack(target, false);
		}
		armyStatus = attack;
	}
	else
	{
		Broodwar->printf("ArmyManager Error: invalid attack target");
	}
}

void ArmyManager::allMove(BWAPI::Position position)
{
	if(position != NULL)
	{
		for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
		{
			(*i).first->move(position, false);
		}
	}
	else
	{
		Broodwar->printf("ArmyManager Error: invalid move position");
	}
}

/*
returns a single unit of the given UnitType that is under the player's control
*/
BWAPI::Unit* ArmyManager::getUnit(BWAPI::UnitType unitType)
{
	for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
	{
		if((*i).first->getType() == unitType)
		{
			return (*i).first;
		}
	}
	Broodwar->printf("ArmyManager: you do not control any units of type '%s'", unitType.getName().c_str());
	return NULL;
}

/*
returns a set of all the units of the given UnitType that are under the player's control
*/
std::set<BWAPI::Unit*> ArmyManager::getAllUnitType(BWAPI::UnitType unitType)
{
	std::set<BWAPI::Unit*> units;
	for(std::set<std::pair<Unit*, int>>::const_iterator i=allArmy.begin();i!=allArmy.end();i++)
	{
		if((*i).first->getType() == unitType)
		{
			units.insert((*i).first);
		}
	}
	return units;
}

/*
sets a rally point for new units to gather at
*/
void ArmyManager::setRallyPoint(BWAPI::Position position)
{
	rallyPoint = position;
}

/*
calculates the total supply value of our army
*/
int ArmyManager::getArmySupply()
{
	int armySupply = 0;

	for(std::set<std::pair<BWAPI::Unit*, int>>::const_iterator i = allArmy.begin(); i != allArmy.end(); i++)
	{
		if((*i).first->isCompleted())
		{
			armySupply += (*i).first->getType().supplyRequired();
		}
	}

	return armySupply;
}

/*
returns a sparcraft gamestate including all friendly military units and all visible enemy units
*/
/*
SparCraft::GameState ArmyManager::getState()
{
    SparCraft::GameState state;	
	std::vector<SparCraft::Unit> friendlyUnits;

	//get the current map
	state.setMap(&map);

	int count = 0;
	//add units under our control
	for(std::set<std::pair<Unit*, int>>::iterator i = allArmy.begin(); i != allArmy.end(); i++)
	{
		/*
		//friendlyUnits.push_back(SparCraft::Unit((*i)->getType(), SparCraft::Players::Player_One, (*i)->getPosition()).setUnitID((*i)->getID()));

		SparCraft::Unit u((*i)->getType(), SparCraft::Players::Player_One, (*i)->getPosition());
		u.setUnitID((*i)->getID());
		friendlyUnits.push_back(u);
		//state.addUnitWithID(u);
		*/

/*
		state.addUnit((*i).first->getType(), SparCraft::Players::Player_One, (*i).first->getPosition());
		(*i).second = count;

		count++;
		//Broodwar->printf("adding friendly %s", (*i)->getType().getName().c_str());
	}

	//add enemy units
	for(std::set<Unit*>::const_iterator i = visibleEnemies.begin(); i != visibleEnemies.end(); i++)
	{
		state.addUnit((*i)->getType(), SparCraft::Players::Player_Two, (*i)->getPosition());
		//Broodwar->printf("adding enemy %s", (*i)->getType().getName().c_str());
	}
	
    return state;
}
*/

/*
adds a unit to the set of visible enemies
*/
void ArmyManager::addEnemy(BWAPI::Unit* unit)
{
	visibleEnemies.insert(unit);
}

/*
removes a unit from the set of visible enemies
*/
void ArmyManager::removeEnemy(BWAPI::Unit* unit)
{
	visibleEnemies.erase(unit);
}

void ArmyManager::allRetreat()
{
	if((rallyPoint != NULL) && (rallyPoint != Position(0,0)))
	{
		allMove(rallyPoint);
	}
	else
	{
		for(std::set<Unit*>::const_iterator i=Broodwar->self()->getUnits().begin();i!=Broodwar->self()->getUnits().end();i++)
		{
			if ((*i)->getType().isResourceDepot())
			{
				rallyPoint = (*i)->getPosition();
				break;
			}
		}

		allMove(rallyPoint);
	}
	armyStatus = retreat;
}

void ArmyManager::setArmyStatus(int status)
{
	armyStatus = status;
}

void ArmyManager::setDefendPosition(BWAPI::Position position)
{
	defendPosition = position;
}

int ArmyManager::getArmyStatus()
{
	return armyStatus;
}

/*
orders all units to regroup at the location of the unit closest to the attack point
returns true once all units have arrived at that location, false otherwise
*/
bool ArmyManager::regroup()
{
	BWAPI::Unit* closestUnit = NULL;
	bool regroupComplete = false;

	//if we haven't yet ordered the regroup to commence
	if(!regroupOrdered && !allArmy.empty())
	{
		regroupPosition = rallyPoint;
		allMove(regroupPosition);

		//feed all current units into the set of units that are regrouping so we can check if they arrive
		for(std::set<std::pair<Unit*, int>>::const_iterator i = allArmy.begin(); i != allArmy.end(); i++)
		{
			regroupingUnits.insert((*i).first);
		}
		regroupOrdered = true;
		regroupFrame = BWAPI::Broodwar->getFrameCount();
	}
	//if the regroup command has already been set
	else
	{
		//check how long it has been since we ordered the regroup command
		if((regroupFrame + MAXREGROUPTIME) < BWAPI::Broodwar->getFrameCount())
		{
			//if it has taken too long then cancel the regroup and continue with the attack
			//this is to prevent us from getting stuck in a situation where we are waiting for a unit that is stuck
			//(for example if we accidently walled it in, or if it is stuck on an island)
			regroupComplete = true;
			regroupOrdered = false;
			return regroupComplete;
		}
		//check that each unit that has been ordered to regroup has arrived at the regroup position
		regroupComplete = true;
		for(std::set<Unit*>::const_iterator i = regroupingUnits.begin(); i != regroupingUnits.end(); i++)
		{
			//check if this unit has arrived
			if(regroupPosition.getDistance((*i)->getPosition()) > REGROUPDIST)
			{
				regroupComplete = false;
			}
			//if the unit is idle and not at the regroup point, reissue the move command
			if((*i)->isIdle() && (regroupPosition.getDistance((*i)->getPosition()) > REGROUPDIST))
			{
				(*i)->move(regroupPosition, false);
			}
		}
	}
	//if the regroup command is successful then reset the variables
	if(regroupComplete)
	{
		regroupOrdered = false;
		regroupingUnits.clear();
	}
	return regroupComplete;
}

void ArmyManager::setAttackPosition(BWAPI::Position position)
{
	if(attackPosition != NULL)
	{
		attackPosition = position;
	}
}

void ArmyManager::drawArmyStatus(int x, int y) 
{
	int i = 0;

	BWAPI::Broodwar->drawTextScreen(x, y-10, "Army Status:");

	if(regroupOrdered)
	{
		BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Regrouping");
	}
	else
	{
		switch(armyStatus)
		{
		case scout:
			BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Scouting");
			break;
		case retreat:
			BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Retreating");
			break;
		case attack:
			BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Attacking");
			break;
		case defend:
			BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Defending");
			break;
		}
	}

}

/*
when we cannot find any enemies, send our units randomly around the map until we find the enemy base
*/
void ArmyManager::findEnemyBase()
{
	BWAPI::Position position;

	armyStatus = scout;

	for(std::set<std::pair<Unit*, int>>::const_iterator i = allArmy.begin(); i != allArmy.end(); i++)
	{
		if((*i).first->isIdle())
		{
			position = Position((rand() % (BWAPI::Broodwar->mapWidth() * 32) + 1), (rand() % (BWAPI::Broodwar->mapHeight() * 32) + 1)).makeValid();

			if((*i).first->hasPath(position))
			{
				(*i).first->attack(position, false);
			}
		}
	}
}

void ArmyManager::updateStatus()
{
	if((armyStatus != defend) && (attackPosition == Position(0,0)))
	{
		findEnemyBase();
		armyStatus = scout;
		attackIssued = false;
		retreatIssued = false;
	}

	if((attackPosition != Position(0,0)) && (attackPosition != NULL))
	{
		if(armyStatus != defend)
		{
			if((enemyArmySupply < getArmySupply()) && !attackIssued)
			{

				if(regroup())
				{
					allAttack(attackPosition);
					attackIssued = true;
					retreatIssued = false;
				}
			}
			else if(enemyArmySupply >= getArmySupply() && !retreatIssued)
			{
				allRetreat();
				attackIssued = false;
				retreatIssued = true;
			}
		}
	}
}

void ArmyManager::setEnemyArmySupply(int supply)
{
	enemyArmySupply = supply;
}

/*
get workers and tell them to fight
*/
void ArmyManager::workerCombat()
{
	//calculate how many workers we will need to fight
	size_t threat;

	for(std::set<BWAPI::Unit*>::iterator i = visibleEnemies.begin(); i != visibleEnemies.end(); i++)
	{
		if(!(*i)->getType().isBuilding())
		{
			if((*i)->getType().isWorker())
			{
				threat += (*i)->getType().supplyRequired() / 2;
			}
			else
			{
				threat += (*i)->getType().supplyRequired();
			}
		}
		else
		{
			if((*i)->getType().canAttack())
			{
				threat += 10;
			}
		}
	}

	threat = threat * 2;

	//get the workers
	getCombatWorkers(threat);

	//tell the workers to attack
	if(defendPosition != NULL)
	{
		for(std::set<BWAPI::Unit*>::iterator i = combatWorkers.begin(); i != combatWorkers.end(); i++)
		{
			(*i)->attack(defendPosition);
		}
	}
}

/*
get workers equal to the level of enemy threat
*/
void ArmyManager::getCombatWorkers(size_t threat)
{
	for(std::set<BWAPI::Unit*>::const_iterator i = BWAPI::Broodwar->self()->getUnits().begin(); i != BWAPI::Broodwar->self()->getUnits().end(); i++)
	{
		if(combatWorkers.size() >= threat)
		{
			break;
		}
		if((*i)->getType().isWorker())
		{
			combatWorkers.insert(*i);
		}
	}
}

/*
tell the combat workers to stop fighting so they can return to mining
*/
void ArmyManager::clearCombatWorkers()
{
	for(std::set<BWAPI::Unit*>::iterator i = combatWorkers.begin(); i != combatWorkers.end(); i++)
	{
		(*i)->stop();
	}
	combatWorkers.clear();
}

/*
void ArmyManager::kite()
{
	SparCraft::MoveArray moveArray;
	std::vector<SparCraft::UnitAction> move;
	SparCraft::GameState state;

	SparCraft::PlayerPtr kiter(new SparCraft::Player_Kiter(SparCraft::Players::Player_One));

	if(!allArmy.empty() && !visibleEnemies.empty())// && (BWAPI::Broodwar->getFrameCount() % 24 == 0))
	{
		state = getState();
		state.generateMoves(moveArray, SparCraft::Players::Player_One);
		kiter->getMoves(state, moveArray, move);		
		if(move.empty())
		{
			Broodwar->printf("no moves generated");
		}
		else
		{
			for(std::vector<SparCraft::UnitAction>::iterator unitMove = move.begin(); unitMove != move.end(); unitMove++)
			{
			//	Broodwar->printf("%s", unitMove->moveString().c_str());
				Broodwar->printf("%s", unitMove->debugString().c_str());

				for(std::set<std::pair<Unit*, int>>::const_iterator unit = allArmy.begin(); unit != allArmy.end(); unit++)
				{
					BWAPI::UnitCommand currentCommand((*unit).first->getLastCommand());

					if((*unit).first->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
					{
					//	Broodwar->printf("unit id %d, unitaction id %d", (*unit).first->getID(), (*unitMove).unit());
						if(((*unitMove).unit() == (*unit).second))
						{
						//	Broodwar->printf("%s issued command %d", (*unit).first->getType().getName().c_str(), (*unitMove).type());
							switch((*unitMove).type())
							{
							case 0:	//none
								break;
							case 1: //attack
								if(!(currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move))
								{
									(*unit).first->attack(BWAPI::Position((*unitMove).pos().x(), (*unitMove).pos().y()), false);
								}
							//	Broodwar->printf("attack!");
								break;
							case 2: //reload
								break;
							case 3: //move
								if(!(currentCommand.getType() == BWAPI::UnitCommandTypes::Move))
								{
									(*unit).first->move(BWAPI::Position((*unitMove).pos().x(), (*unitMove).pos().y()), false);
								}
							//	Broodwar->printf("move!");
								break;
							case 4: //pass
								break;
							case 5: //heal
								break;
							}
							break;
						}
					}
				}
			}
		}
	}
}
*/

bool ArmyManager::haveDetection()
{
	std::set<BWAPI::Unit*> detectors;

	if(BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss)
	{
		detectors = getAllUnitType(BWAPI::UnitTypes::Protoss_Observer);
	}

	if(detectors.size() > 0)
	{
		return true;
	}
	return false;
}