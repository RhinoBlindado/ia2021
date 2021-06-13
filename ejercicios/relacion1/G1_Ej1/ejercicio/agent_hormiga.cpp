#include "agent_hormiga.h"
#include "environment.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <utility>

using namespace std;

// -----------------------------------------------------------
Agent::ActionType Agent::Think()
{
	ActionType accion;

/**
 * Apartado (a)
 */

/*	if (FEROMONA_)
	{
		accion = actFORWARD;
	}
	else
	{
		accion = actTURN_R;
	}
*/

/**
 * Apartado (b)
 */

/*
	if (FEROMONA_)
	{
		accion = actFORWARD;
		turn = 0;
	}
	else if(turn < 2)
	{
		accion = actTURN_R;
		turn++;
	}
	else
	{
		accion = actTURN_L;
	}
*/

/**
 * Apartado (c)
 */

	if (FEROMONA_)
	{
		accion = actFORWARD;
		turn = 0;
	}
	else if(turn == 0)
	{
		accion = actTURN_R;
		turn++;
	}
	else
	{
		accion = actTURN_L;
	}

	return accion;

}
// -----------------------------------------------------------
void Agent::Perceive(const Environment &env)
{
	FEROMONA_ = env.isFeromona();
}
// -----------------------------------------------------------
string ActionStr(Agent::ActionType accion)
{
	switch (accion)
	{
	case Agent::actFORWARD: return "FORWARD";
	case Agent::actTURN_L: return "TURN LEFT";
	case Agent::actTURN_R: return "TURN RIGHT";
	case Agent::actIDLE: return "IDLE";
	default: return "????";
	}
}
