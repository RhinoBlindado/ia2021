#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"

#include <iostream>
#include <cmath>
#include <set>
#include <stack>
#include <queue>
#include <map>

int manhattanDist(estado a, estado b);

bool objSort(const objective &a, const objective &b)
{
	return a.distToPlayer < b.distToPlayer;
}

void checkIfOnObjective(estado currPos, list<objective> &objList)
{
	for (list<objective>::iterator it = objList.begin(); it != objList.end(); ++it) 
	{
		if (currPos.fila == it->status.fila && currPos.columna == it->status.columna)
		{
			it = objList.erase(it);
			objList.sort(objSort);
			break;
		}	
	}

}

// Este es el método principal que se piden en la practica.
// Tiene como entrada la información de los sensores y devuelve la acción a realizar.
// Para ver los distintos sensores mirar fichero "comportamiento.hpp"
Action ComportamientoJugador::think(Sensores sensores) 
{
	// Actualizar la variable global.
	actual.fila = sensores.posF;
	actual.columna = sensores.posC;
	actual.orientacion = sensores.sentido;

	batteryLevel = sensores.bateria;
	// Capturar destinos.
	objetivos.clear();
	for (int i = 0; i < sensores.num_destinos; i++)
	{
		objective aux;
		aux.status.fila = sensores.destino[2*i];
		aux.status.columna = sensores.destino[2*i+1];
		aux.distToPlayer = manhattanDist(actual, aux.status);
		objetivos.push_back(aux);
	}

	if(mapaResultado[actual.fila][actual.columna] == 'K')
	{
		hasBikini = true;
		hasShoes = false;
	}

	if(mapaResultado[actual.fila][actual.columna] == 'D')
	{
		hasBikini = false;
		hasShoes = true;
	}

	if(sensores.nivel == 4)
	{
		if (actual.fila == currObj.status.fila && actual.columna == currObj.status.columna && lowBattery)
		{
			plan.push_back(actIDLE);

			if(batteryLevel > 2000)
			{
				lowBattery = false;
				hayPlan = false;
				actualObjectives.sort(objSort);
			}				
		}

		if(isEnteringFog(actual, sensores.terreno))
		{
			removeFog(actual, sensores.terreno);
			hayPlan = false;
		}

		if (batteryLevel < 1700)
		{
			checkForEnergyStations();
			if (!energyStations.empty())
			{
				currObj = energyStations.front();
				lowBattery = true;
			}
		}

		checkIfOnObjective(actual, actualObjectives);

		if(!lowBattery)
		{
			if (actualObjectives.empty())
			{
				actualObjectives = objetivos;
				actualObjectives.sort(objSort);
			}
			currObj = actualObjectives.front();
			cout<<"OBJETIVO ACTUAL ES"<<currObj.status.fila<<" "<<currObj.status.columna<<endl;
		}
	}

	if(!hayPlan || plan.empty())
	{
		plan.clear();
		hayPlan = pathFinding(sensores.nivel, actual, objetivos, currObj, plan);
	}

	Action sigAccion;

	if (sensores.colision)
	{
		cout<<"COLISION!"<<endl;
		hayPlan = false;
		sigAccion = actIDLE;
	}

	if(hayPlan && plan.size() > 0)
	{
		sigAccion = plan.front();
		plan.erase(plan.begin());
	}
	else
	{
		cout<<"ERROR: No se pudo encontrar un plan."<<endl;
	}	

  	return sigAccion;
}


// Llama al algoritmo de busqueda que se usara en cada comportamiento del agente
// Level representa el comportamiento en el que fue iniciado el agente.
bool ComportamientoJugador::pathFinding (int level, const estado &origen, const list<objective> &destino, objective survivalObj, list<Action> &plan)
{
	switch (level)
	{
		case 0: 
			cout << "Demo\n";
			objective un_objetivo;
			un_objetivo = destino.front();
			cout << "fila: " << un_objetivo.status.fila << " col:" << un_objetivo.status.columna << endl;
			return pathFinding_Profundidad(origen,un_objetivo.status,plan);
		break;

		case 1: // Incluir aqui la llamada al busqueda en anchura
			cout << "Optimo numero de acciones\n";
			return pathFinding_Anchura(origen, destino.front().status, plan);
		break;

		case 2: // Incluir aqui la llamada al busqueda de costo uniforme
			cout << "Optimo en coste 1 Objetivo\n";
			return pathFinding_Aestrella(origen, destino.front().status, plan);
		break;

		case 3: // Incluir aqui la llamada al algoritmo de busqueda para 3 objetivos
		cout << "Optimo en coste 3 Objetivos\n";
			return  pathFinding_AestrellaMulti(origen, destino, plan);
		break;

		case 4: // Incluir aqui la llamada al algoritmo de busqueda usado en el nivel 2
			return pathFinding_Aestrella(origen, survivalObj.status, plan);
		break;
	}
	return false;
}


//---------------------- Implementación de la busqueda en profundidad ---------------------------

// Dado el codigo en caracter de una casilla del mapa dice si se puede
// pasar por ella sin riegos de morir o chocar.
bool EsObstaculo(unsigned char casilla){
	if (casilla=='P' or casilla=='M')
		return true;
	else
	  return false;
}


// Comprueba si la casilla que hay delante es un obstaculo. Si es un
// obstaculo devuelve true. Si no es un obstaculo, devuelve false y
// modifica st con la posición de la casilla del avance.
bool ComportamientoJugador::HayObstaculoDelante(estado &st){
	int fil=st.fila, col=st.columna;

  // calculo cual es la casilla de delante del agente
	switch (st.orientacion) {
		case 0: fil--; break;
		case 1: col++; break;
		case 2: fil++; break;
		case 3: col--; break;
	}

	// Compruebo que no me salgo fuera del rango del mapa
	if (fil<0 or fil>=mapaResultado.size()) return true;
	if (col<0 or col>=mapaResultado[0].size()) return true;

	// Miro si en esa casilla hay un obstaculo infranqueable
	if (!EsObstaculo(mapaResultado[fil][col])){
		// No hay obstaculo, actualizo el parametro st poniendo la casilla de delante.
    st.fila = fil;
		st.columna = col;
		return false;
	}
	else{
	  return true;
	}
}

struct nodo{
	estado st;
	list<Action> secuencia;
};

struct ComparaEstados{
	bool operator()(const estado &a, const estado &n) const{
		if ((a.fila > n.fila) or (a.fila == n.fila and a.columna > n.columna) or
	      (a.fila == n.fila and a.columna == n.columna and a.orientacion > n.orientacion))
			return true;
		else
			return false;
	}
};

// Implementación de la busqueda en profundidad.
// Entran los puntos origen y destino y devuelve la
// secuencia de acciones en plan, una lista de acciones.
bool ComportamientoJugador::pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan) {
	//Borro la lista
	cout << "Calculando plan\n";
	plan.clear();
	set<estado,ComparaEstados> Cerrados; // Lista de Cerrados
	stack<nodo> Abiertos;				 // Lista de Abiertos

  nodo current;
	current.st = origen;
	current.secuencia.empty();

	Abiertos.push(current);

  while (!Abiertos.empty() and (current.st.fila!=destino.fila or current.st.columna != destino.columna)){

		Abiertos.pop();
		Cerrados.insert(current.st);

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;
		if (Cerrados.find(hijoTurnR.st) == Cerrados.end()){
			hijoTurnR.secuencia.push_back(actTURN_R);
			Abiertos.push(hijoTurnR);

		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;
		if (Cerrados.find(hijoTurnL.st) == Cerrados.end()){
			hijoTurnL.secuencia.push_back(actTURN_L);
			Abiertos.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;
		if (!HayObstaculoDelante(hijoForward.st)){
			if (Cerrados.find(hijoForward.st) == Cerrados.end()){
				hijoForward.secuencia.push_back(actFORWARD);
				Abiertos.push(hijoForward);
			}
		}

		// Tomo el siguiente valor de la Abiertos
		if (!Abiertos.empty()){
			current = Abiertos.top();
		}
	}

  cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna){
		cout << "Cargando el plan\n";
		plan = current.secuencia;
		cout << "Longitud del plan: " << plan.size() << endl;
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No encontrado plan\n";
	}


	return false;
}

// Sacar por la consola la secuencia del plan obtenido
void ComportamientoJugador::PintaPlan(list<Action> plan) {
	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			cout << "A ";
		}
		else if (*it == actTURN_R){
			cout << "D ";
		}
		else if (*it == actTURN_L){
			cout << "I ";
		}
		else {
			cout << "- ";
		}
		it++;
	}
	cout << endl;
}


// Funcion auxiliar para poner a 0 todas las casillas de una matriz
void AnularMatriz(vector<vector<unsigned char> > &m){
	for (int i=0; i<m[0].size(); i++){
		for (int j=0; j<m.size(); j++){
			m[i][j]=0;
		}
	}
}


// Pinta sobre el mapa del juego el plan obtenido
void ComportamientoJugador::VisualizaPlan(const estado &st, const list<Action> &plan){
  AnularMatriz(mapaConPlan);
	estado cst = st;

	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			switch (cst.orientacion) {
				case 0: cst.fila--; break;
				case 1: cst.columna++; break;
				case 2: cst.fila++; break;
				case 3: cst.columna--; break;
			}
			mapaConPlan[cst.fila][cst.columna]=1;
		}
		else if (*it == actTURN_R){
			cst.orientacion = (cst.orientacion+1)%4;
		}
		else {
			cst.orientacion = (cst.orientacion+3)%4;
		}
		it++;
	}
}



int ComportamientoJugador::interact(Action accion, int valor){
  return false;
}

/**
 *  [CASTELLANO]
 * 
 * Practica 2 - Agentes Deliberativos y Reactivos
 * Asignatura: Inteligencia Artificial
 * Autor: Valentino Lugli (Github: @RhinoBlindado)
 * Abril, Mayo 2021
 */

/**
 *  [ENGLISH]
 *
 * Practice 2 - Deliberative and Reactive Agents
 * Course: Artificial Intelligence
 * Author: Valentino Lugli (Github: @RhinoBlindado)
 * April, May 2021
 */


/**
 *		[NIVEL 1]
 */

/*
 * @brief Obtener una ruta por medio del búsqueda de anchura
 * @param	origen		Nodo origen para realizar la búsqueda.
 * @param	destino		Nodo destino para realizar la búsqueda.
 * @param	plan		Lista de acciones a rellenarse que llevan del origen al destino.
 * @returns Indica si se ha encontrado un camino o no.
 */
bool ComportamientoJugador::pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan) 
{
	cout << "Calculando plan\n";
	plan.clear();

	set<estado,ComparaEstados> Cerrados; // Lista de Cerrados
	queue<nodo> Abiertos;				 // Lista de Abiertos

  	nodo current;
	current.st = origen;
	current.secuencia.empty();

	Abiertos.push(current);

  	while (!Abiertos.empty() and (current.st.fila!=destino.fila or current.st.columna != destino.columna))
	{

		Abiertos.pop();
		Cerrados.insert(current.st);

		// Generar descendiente de girar a la derecha
		nodo hijoTurnR = current;
		hijoTurnR.st.orientacion = (hijoTurnR.st.orientacion+1)%4;
		if (Cerrados.find(hijoTurnR.st) == Cerrados.end())
		{
			hijoTurnR.secuencia.push_back(actTURN_R);
			Abiertos.push(hijoTurnR);
		}

		// Generar descendiente de girar a la izquierda
		nodo hijoTurnL = current;
		hijoTurnL.st.orientacion = (hijoTurnL.st.orientacion+3)%4;
		if (Cerrados.find(hijoTurnL.st) == Cerrados.end())
		{
			hijoTurnL.secuencia.push_back(actTURN_L);
			Abiertos.push(hijoTurnL);
		}

		// Generar descendiente de avanzar
		nodo hijoForward = current;
		if (!HayObstaculoDelante(hijoForward.st))
		{
			if (Cerrados.find(hijoForward.st) == Cerrados.end())
			{
				hijoForward.secuencia.push_back(actFORWARD);
				Abiertos.push(hijoForward);
			}
		}

		// Tomo el siguiente valor de la Abiertos
		if (!Abiertos.empty())
		{
			current = Abiertos.front();
		}
	}

  	cout << "Terminada la busqueda\n";

	if (current.st.fila == destino.fila and current.st.columna == destino.columna)
	{
		cout << "Cargando el plan\n";
		plan = current.secuencia;
		cout << "Longitud del plan: " << plan.size() << endl;
		PintaPlan(plan);
		// ver el plan en el mapa
		VisualizaPlan(origen, plan);
		return true;
	}
	else 
	{
		cout << "No encontrado plan\n";
	}
	return false;
}

/**
 * 		[NIVEL 2] | [NIVEL 3]
 */

/**
 * 
 */
class aStarNode
{
	private:
		estado nodeStatus;
		list<Action> routeSoFar;
		list<objective> currentObjectives;
		int totalCost;
		int accumCost;
		int expectedCost;
		bool hasBikini;
		bool hasShoes;

	public:
		aStarNode(){}

		aStarNode(estado arg_nodeStatus, int arg_accumCost, int arg_expectedCost, list<Action> arg_routeSoFar, bool arg_bikini, bool arg_shoes)
		{
			this->nodeStatus = arg_nodeStatus;
			this->accumCost = arg_accumCost;
			this->expectedCost = arg_expectedCost;
			this->totalCost = this->accumCost + this->expectedCost;
			this->routeSoFar = arg_routeSoFar;
			this->hasBikini = arg_bikini;
			this->hasShoes = arg_shoes;
		}

		aStarNode(estado arg_nodeStatus, int arg_accumCost, int arg_expectedCost, list<Action> arg_routeSoFar, bool arg_bikini, bool arg_shoes, list<objective> arg_obj)
		{
			this->nodeStatus = arg_nodeStatus;
			this->accumCost = arg_accumCost;
			this->expectedCost = arg_expectedCost;
			this->totalCost = this->accumCost + this->expectedCost;
			this->routeSoFar = arg_routeSoFar;
			this->hasBikini = arg_bikini;
			this->hasShoes = arg_shoes;
			this->currentObjectives = arg_obj;
		}

		estado getStatus() const
		{
			return this->nodeStatus;
		}

		int getTotalCost() const
		{
			return this->totalCost;
		}

		int getAccumCost() const
		{
			return this->accumCost;
		}

		int getExpectedCost() const
		{
			return this->expectedCost;
		}

		bool getIfHasBikini() const
		{
			return this->hasBikini;
		}

		bool getIfHasShoes() const
		{
			return this->hasShoes;
		}

		void copy(aStarNode arg)
		{
			this->nodeStatus = arg.nodeStatus;
			this->accumCost = arg.accumCost;
			this->expectedCost = arg.expectedCost;
			this->totalCost = arg.totalCost;
			this->routeSoFar = arg.routeSoFar;
			this->hasBikini = arg.hasBikini;
			this->hasShoes = arg.hasShoes;
			this->currentObjectives = arg.currentObjectives;
		}

		void printContents() const
		{
			cout<<"\tNode ["<<nodeStatus.fila<<"]["<<nodeStatus.columna<<"]("<<nodeStatus.orientacion<<") K("<<hasBikini<<") S("<<hasShoes<<") | f("<<totalCost<<")=g("<<accumCost<<")+h("<<expectedCost<<") Road size="<<routeSoFar.size()<<" Objectives="<<currentObjectives.size()<<endl;
		}

		bool equalCoords(estado argStatus) const
		{
			return this->nodeStatus.fila == argStatus.fila && this->nodeStatus.columna == argStatus.columna;
		}

		bool equalNode(const aStarNode &arg) const
		{
			return 	this->nodeStatus.fila == arg.nodeStatus.fila && 
					this->nodeStatus.columna == arg.nodeStatus.columna && 
					this->nodeStatus.orientacion == arg.nodeStatus.orientacion &&
					this->hasBikini == arg.hasBikini &&
					this->hasShoes == arg.hasShoes &&
					this->currentObjectives.size() == arg.currentObjectives.size();
		}

		list<Action> getRoute() const
		{
			return this->routeSoFar;
		}

		list<objective> getObjectives() const
		{
			return this->currentObjectives;
		}

		objective getActualObjective() const
		{
			return this->currentObjectives.front();
		}

		int getNumObjectives() const
		{
			return this->currentObjectives.size();
		}

		void removeActualObjective()
		{
			if(!currentObjectives.empty())
			{
				this->currentObjectives.pop_front();
			}
		}

};


bool costCompare(const aStarNode &a, const aStarNode &b)
{
	bool eval = false;

	if (a.getTotalCost() < b.getTotalCost())
	{
		eval = true;
	}
	else if(a.getTotalCost() == b.getTotalCost())
	{
		if (a.getAccumCost() < b.getAccumCost())
		{
			eval = true;
		}
		
	}

	return eval;
}

class isSameNode
{
	public:
		bool operator()(const aStarNode &a, const aStarNode &b)
		{			
			if ((a.getStatus().fila > b.getStatus().fila) or 
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna > b.getStatus().columna) or
	      		(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion > b.getStatus().orientacion) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && (int)a.getIfHasBikini() > (int)b.getIfHasBikini()) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && a.getIfHasBikini() == b.getIfHasBikini() && (int)a.getIfHasShoes() > (int)b.getIfHasShoes()))
				return true;
			else
				return false;
		}
};

class isSameNodeMulti
{
	public:
		bool operator()(const aStarNode &a, const aStarNode &b)
		{			
			if ((a.getStatus().fila > b.getStatus().fila) or 
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna > b.getStatus().columna) or
	      		(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion > b.getStatus().orientacion) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && (int)a.getIfHasBikini() > (int)b.getIfHasBikini()) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && a.getIfHasBikini() == b.getIfHasBikini() && (int)a.getIfHasShoes() > (int)b.getIfHasShoes()) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && a.getIfHasBikini() == b.getIfHasBikini() && a.getIfHasShoes() == b.getIfHasShoes() && a.getNumObjectives() > b.getNumObjectives()))
				return true;
			else
				return false;
		}
};

/**
 * @brief Calcular la distancia Manhattan entre dos puntos.
 * @param origin		Estado de la posición de origen.
 * @param destination	Estado de la posición de destino.
 * @return	Distancia Manhattan
 */
int manhattanDist(estado origin, estado destination)
{
	return abs(origin.fila - destination.fila) + abs(origin.columna - destination.columna);
}

int multiManhattanDist(estado origin, list<objective> objectives)
{
	int sum = 0;
	objective destination;

	destination = objectives.front();
	objectives.pop_front();

	sum = manhattanDist(origin, destination.status);

	while (!objectives.empty())
	{
		sum += manhattanDist(destination.status, objectives.front().status);
		destination = objectives.front();
		objectives.pop_front();
	}

	return sum;
}


/**
 * 
 */
bool ComportamientoJugador::pathFinding_Aestrella(const estado &origin, const estado &destination, list<Action> &theRoad)
{
	bool hasRoute = false, 
		 validNode,
		 isOnClosed,
		 isOnOpen,
		 actHasBikini = hasBikini,
		 actHasShoes = hasShoes;

	list<aStarNode> open;
	list<aStarNode>::iterator openFinder;

	map<aStarNode, aStarNode, isSameNode> closed;
	map<aStarNode, aStarNode>::iterator closedFinder;
	
	list<Action> actualRoad;

	estado actStatus;
	int actAccumCost;
	Action actAction;
	

	aStarNode actNode, root(origin, 0, manhattanDist(origin, destination), theRoad, actHasBikini, actHasShoes), auxNode;
	open.push_back(root);

	while (!open.empty())
	{
		actNode = open.front();
		open.pop_front();

	//	cout<<"PARENT NODE:"; actNode.printContents();
	//	cout<<"ACTUAL OBJECTIVE: ["<<actDestination.fila<<"]["<<actDestination.columna<<"]"<<endl;

		if(actNode.equalCoords(destination))
		{
			hasRoute = true;
			break;
		}

		closed.insert(pair<aStarNode, aStarNode>(actNode, actNode));

		for (int i = 0; i < 3; i++)
		{
			validNode = false;
			actStatus = actNode.getStatus();
			actAccumCost = actNode.getAccumCost();
			actHasBikini = actNode.getIfHasBikini();
			actHasShoes = actNode.getIfHasShoes();
			actAction = (Action)i;

			// Girar a la derecha o izquierda
			if (i == actTURN_R || i == actTURN_L)
			{
				actStatus.orientacion = ( i == actTURN_L ? (actStatus.orientacion+3)%4 : (actStatus.orientacion+1)%4);
				switch (mapaResultado[actStatus.fila][actStatus.columna])
				{
					case 'A':
						if (actHasBikini)
						{
							actAccumCost += 5;
						}
						else
						{
							actAccumCost += 500;
						}
					break;
					
					case 'B':
						if (actHasShoes)
						{
							actAccumCost += 1;
						}
						else
						{
							actAccumCost += 3;
						}
					break;

					case 'K':
						actAccumCost += 1;
						actHasBikini = true;
						actHasShoes = false;
					break;

					case 'D':
						actAccumCost += 1;
						actHasBikini = false;
						actHasShoes = true;
					break;

					case 'T':
						actAccumCost += 2;
					break;

					default:
						actAccumCost += 1;
					break;
				}
				validNode = true;
			}

			// Avanzar
			if (i == actFORWARD)
			{
				switch (mapaResultado[actStatus.fila][actStatus.columna])
				{
					case 'A':
						if (actHasBikini)
						{
							actAccumCost += 10;
						}
						else
						{
							actAccumCost += 200;
						}
					break;
					
					case 'B':
						if (actHasShoes)
						{
							actAccumCost += 15;
						}
						else
						{
							actAccumCost += 100;
						}
					break;

					case 'T':
						actAccumCost += 2;
					break;

					case 'K':
						actAccumCost += 1;
						actHasBikini = true;
						actHasShoes = false;
					break;

					case 'D':
						actAccumCost += 1;
						actHasBikini = false;
						actHasShoes = true;
					break;

					default:
						actAccumCost += 1;
					break;
				}
				if(!HayObstaculoDelante(actStatus))
				{
					validNode = true;
				}

			}

			if (validNode)
			{
				actualRoad = actNode.getRoute();
				actualRoad.push_back(actAction);

				aStarNode newNode(actStatus, actAccumCost, manhattanDist(actStatus, destination), actualRoad, actHasBikini, actHasShoes);

				isOnClosed = false;
				isOnOpen = false;

		//		cout<<"CHILD NODE";newNode.printContents();

				closedFinder = closed.find(newNode);

				if(closedFinder != closed.end())
				{
					isOnClosed = true;
					if(newNode.getAccumCost() < closedFinder->second.getAccumCost())
					{
						newNode.copy(closedFinder->second);
						closed.erase(closedFinder);
						open.push_back(newNode);
					}
				}

				if(!isOnClosed)
				{					
					for (list<aStarNode>::iterator it = open.begin(); it != open.end(); ++it)
					{

						if (newNode.equalNode(*it))
						{
							isOnOpen = true;
							if (newNode.getAccumCost() < it->getAccumCost())
							{
								it->copy(newNode);
								break;
							}	
						}	
					}
				}

				if(!isOnClosed && !isOnOpen)
				{
					open.push_back(newNode);
				}			

			}
		}
		open.sort(costCompare);
	}

	if(hasRoute)
	{
		theRoad = actNode.getRoute();
	//	cout<<"### Ruta encontrada ###\nLongitud: "<<theRoad.size()<<endl;
	//	cout<<"Batería: "<<actNode.getAccumCost()<<" ("<<(batteryLevel-actNode.getAccumCost())<<")"<<endl;
	//	PintaPlan(theRoad);
		VisualizaPlan(origin, theRoad);
	}
	else
	{
		cout<<"Ruta no encontrada."<<endl;
	}
	
	return hasRoute;
}

/**
 * [NIVEL 3]
 */



bool ComportamientoJugador::pathFinding_AestrellaMulti(const estado &origin, const list<objective> &destination, list<Action> &theRoad)
{
	bool hasRoute = false, 
		 validNode,
		 isOnClosed,
		 isOnOpen,
		 actHasBikini = hasBikini,
		 actHasShoes = hasShoes;

	list<aStarNode> open;
	list<aStarNode>::iterator openFinder;

	map<aStarNode, aStarNode, isSameNodeMulti> closed;
	map<aStarNode, aStarNode>::iterator closedFinder;
	
	list<Action> actualRoad;
	list<objective> actObjectives;

	estado actStatus;
	objective actDestination;
	int actAccumCost;
	Action actAction;
	

	aStarNode actNode, root(origin, 0, multiManhattanDist(origin, destination), theRoad, actHasBikini, actHasShoes, destination), auxNode;
	open.push_back(root);

	while (!open.empty())
	{
		actNode = open.front();
		actDestination = actNode.getActualObjective();
		open.pop_front();

	//	cout<<"PARENT NODE:"; actNode.printContents();
	//	cout<<"ACTUAL OBJECTIVE: ["<<actDestination.fila<<"]["<<actDestination.columna<<"]"<<endl;

		if(actNode.equalCoords(actDestination.status) && actNode.getAccumCost() <= 3000)
		{
	//		cout<<"-------------------- OBJECTIVO ENCONTRADO--------------------"<<endl;
			if (actNode.getNumObjectives() == 1)
			{
				hasRoute = true;
				break;
			}
			else
			{
				actNode.removeActualObjective();
			}

		}

		closed.insert(pair<aStarNode, aStarNode>(actNode, actNode));

		for (int i = 0; i < 3; i++)
		{
			validNode = false;
			actStatus = actNode.getStatus();
			actAccumCost = actNode.getAccumCost();
			actHasBikini = actNode.getIfHasBikini();
			actHasShoes = actNode.getIfHasShoes();
			actObjectives = actNode.getObjectives();
			actAction = (Action)i;

			// Girar a la derecha o izquierda
			if (i == actTURN_R || i == actTURN_L)
			{
				actStatus.orientacion = ( i == actTURN_L ? (actStatus.orientacion+3)%4 : (actStatus.orientacion+1)%4);
				switch (mapaResultado[actStatus.fila][actStatus.columna])
				{
					case 'A':
						if (actHasBikini)
						{
							actAccumCost += 5;
						}
						else
						{
							actAccumCost += 500;
						}
					break;
					
					case 'B':
						if (actHasShoes)
						{
							actAccumCost += 1;
						}
						else
						{
							actAccumCost += 3;
						}
					break;

					case 'K':
						actAccumCost += 1;
						actHasBikini = true;
						actHasShoes = false;
					break;

					case 'D':
						actAccumCost += 1;
						actHasBikini = false;
						actHasShoes = true;
					break;

					case 'T':
						actAccumCost += 2;
					break;

					default:
						actAccumCost += 1;
					break;
				}
				validNode = true;
			}

			// Avanzar
			if (i == actFORWARD)
			{
				switch (mapaResultado[actStatus.fila][actStatus.columna])
				{
					case 'A':
						if (actHasBikini)
						{
							actAccumCost += 10;
						}
						else
						{
							actAccumCost += 200;
						}
					break;
					
					case 'B':
						if (actHasShoes)
						{
							actAccumCost += 15;
						}
						else
						{
							actAccumCost += 100;
						}
					break;

					case 'T':
						actAccumCost += 2;
					break;

					case 'K':
						actAccumCost += 1;
						actHasBikini = true;
						actHasShoes = false;
					break;

					case 'D':
						actAccumCost += 1;
						actHasBikini = false;
						actHasShoes = true;
					break;

					default:
						actAccumCost += 1;
					break;
				}
				if(!HayObstaculoDelante(actStatus))
				{
					validNode = true;
				}

			}

			if (validNode)
			{
				actualRoad = actNode.getRoute();
				actualRoad.push_back(actAction);

				aStarNode newNode(actStatus, actAccumCost, multiManhattanDist(actStatus, destination), actualRoad, actHasBikini, actHasShoes, actObjectives);

				isOnClosed = false;
				isOnOpen = false;

		//		cout<<"CHILD NODE";newNode.printContents();

				closedFinder = closed.find(newNode);

				if(closedFinder != closed.end())
				{
					isOnClosed = true;
					if(newNode.getAccumCost() < closedFinder->second.getAccumCost())
					{
						newNode.copy(closedFinder->second);
						closed.erase(closedFinder);
						open.push_back(newNode);
					}
				}

				if(!isOnClosed)
				{					
					for (list<aStarNode>::iterator it = open.begin(); it != open.end(); ++it)
					{

						if (newNode.equalNode(*it))
						{
							isOnOpen = true;
							if (newNode.getAccumCost() < it->getAccumCost())
							{
								it->copy(newNode);
								break;
							}	
						}	
					}
				}

				if(!isOnClosed && !isOnOpen)
				{
					open.push_back(newNode);
				}			

			}
		}
		open.sort(costCompare);
	}

	if(hasRoute)
	{
		theRoad = actNode.getRoute();
	//	cout<<"### Ruta encontrada ###\nLongitud: "<<theRoad.size()<<endl;
	//	cout<<"Batería: "<<actNode.getAccumCost()<<" ("<<(batteryLevel-actNode.getAccumCost())<<")"<<endl;
		PintaPlan(theRoad);
		VisualizaPlan(origin, theRoad);
	}
	else
	{
		cout<<"Ruta no encontrada."<<endl;
	}
	
	return hasRoute;
}
/**
 * [NIVEL 4]
 */


void ComportamientoJugador::checkForEnergyStations()
{
	objective energyLocator;
	energyStations.clear();
	for(int i=0; i<mapaResultado.size(); i++)
	{
		for(int j=0; j<mapaResultado[0].size(); j++)
		{
			if (mapaResultado[i][j] == 'X')
			{
				energyLocator.status.fila = i;
				energyLocator.status.columna = j;
				energyLocator.distToPlayer = manhattanDist(actual, energyLocator.status);
				energyStations.push_back(energyLocator);
			}
		}
	}
	energyStations.sort(objSort);
}

bool ComportamientoJugador::isEnteringFog(const estado &status, const vector<unsigned char> &visualField)
{
	bool silentHill = false;

	if (mapaResultado[status.fila][status.columna] == '?')
	{
		silentHill = true;
	}
	
	switch (status.orientacion)
	{
		case 0:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = -i; j <= i; j++)
				{
					if (mapaResultado[status.fila-i][status.columna+j] == '?')
					{
						silentHill = true; 
						break;
					}
					
				}
				if (silentHill)
				{
					break;
				}
			}
		break;

		case 1:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = -i; j <= i; j++)
				{
					if (mapaResultado[status.fila+j][status.columna+i] == '?')
					{
						silentHill = true; 
						break;
					}
				}
				if (silentHill)
				{
					break;
				}
			}
		break;

		case 2:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = i; j >= -i; j--)
				{
					if (mapaResultado[status.fila+i][status.columna+j] == '?')
					{
						silentHill = true; 
						break;
					}
					
				}
				if (silentHill)
				{
					break;
				}
			}
		break;

		case 3:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = i; j >= -i; j--)
				{
					if (mapaResultado[status.fila+j][status.columna-i] == '?')
					{
						silentHill = true; 
						break;
					}
					
				}
				if (silentHill)
				{
					break;
				}
			}
		break;
	}
	return silentHill;
}


void ComportamientoJugador::removeFog(const estado &status, const vector<unsigned char> &visualField)
{
	mapaResultado[status.fila][status.columna] = visualField[0];

	switch (status.orientacion)
	{
		case 0:
			mapaResultado[status.fila-1][status.columna-1] 	= visualField[1];
			mapaResultado[status.fila-1][status.columna] 	= visualField[2]; 
			mapaResultado[status.fila-1][status.columna+1] 	= visualField[3];

			mapaResultado[status.fila-2][status.columna-2] 	= visualField[4];
			mapaResultado[status.fila-2][status.columna-1] 	= visualField[5];
			mapaResultado[status.fila-2][status.columna] 	= visualField[6];
			mapaResultado[status.fila-2][status.columna+1]	= visualField[7];
			mapaResultado[status.fila-2][status.columna+2] 	= visualField[8];

			mapaResultado[status.fila-3][status.columna-3] 	= visualField[9];
			mapaResultado[status.fila-3][status.columna-2] 	= visualField[10];
			mapaResultado[status.fila-3][status.columna-1] 	= visualField[11];
			mapaResultado[status.fila-3][status.columna]   	= visualField[12];
			mapaResultado[status.fila-3][status.columna+1] 	= visualField[13];
			mapaResultado[status.fila-3][status.columna+2] 	= visualField[14];
			mapaResultado[status.fila-3][status.columna+3] 	= visualField[15];
		break;

		case 1:
			mapaResultado[status.fila-1][status.columna+1] 	= visualField[1];
			mapaResultado[status.fila][status.columna+1] 	= visualField[2]; 
			mapaResultado[status.fila+1][status.columna+1] 	= visualField[3];

			mapaResultado[status.fila-2][status.columna+2] 	= visualField[4];
			mapaResultado[status.fila-1][status.columna+2] 	= visualField[5];
			mapaResultado[status.fila][status.columna+2] 	= visualField[6];
			mapaResultado[status.fila+1][status.columna+2]	= visualField[7];
			mapaResultado[status.fila+2][status.columna+2] 	= visualField[8];

			mapaResultado[status.fila-3][status.columna+3] 	= visualField[9];
			mapaResultado[status.fila-2][status.columna+3] 	= visualField[10];
			mapaResultado[status.fila-1][status.columna+3] 	= visualField[11];
			mapaResultado[status.fila][status.columna+3]   	= visualField[12];
			mapaResultado[status.fila+1][status.columna+3] 	= visualField[13];
			mapaResultado[status.fila+2][status.columna+3] 	= visualField[14];
			mapaResultado[status.fila+3][status.columna+3] 	= visualField[15];
		break;

		case 2:
			mapaResultado[status.fila+1][status.columna+1] 	= visualField[1];
			mapaResultado[status.fila+1][status.columna] 	= visualField[2]; 
			mapaResultado[status.fila+1][status.columna-1] 	= visualField[3];

			mapaResultado[status.fila+2][status.columna+2] 	= visualField[4];
			mapaResultado[status.fila+2][status.columna+1] 	= visualField[5];
			mapaResultado[status.fila+2][status.columna] 	= visualField[6];
			mapaResultado[status.fila+2][status.columna-1]	= visualField[7];
			mapaResultado[status.fila+2][status.columna-2] 	= visualField[8];

			mapaResultado[status.fila+3][status.columna+3] 	= visualField[9];
			mapaResultado[status.fila+3][status.columna+2] 	= visualField[10];
			mapaResultado[status.fila+3][status.columna+1] 	= visualField[11];
			mapaResultado[status.fila+3][status.columna]   	= visualField[12];
			mapaResultado[status.fila+3][status.columna-1] 	= visualField[13];
			mapaResultado[status.fila+3][status.columna-2] 	= visualField[14];
			mapaResultado[status.fila+3][status.columna-3] 	= visualField[15];
		break;

		case 3:
			mapaResultado[status.fila+1][status.columna-1] 	= visualField[1];
			mapaResultado[status.fila][status.columna-1] 	= visualField[2]; 
			mapaResultado[status.fila-1][status.columna-1] 	= visualField[3];

			mapaResultado[status.fila+2][status.columna-2] 	= visualField[4];
			mapaResultado[status.fila+1][status.columna-2] 	= visualField[5];
			mapaResultado[status.fila][status.columna-2] 	= visualField[6];
			mapaResultado[status.fila-1][status.columna-2]	= visualField[7];
			mapaResultado[status.fila-2][status.columna-2] 	= visualField[8];

			mapaResultado[status.fila+3][status.columna-3] 	= visualField[9];
			mapaResultado[status.fila+2][status.columna-3] 	= visualField[10];
			mapaResultado[status.fila+1][status.columna-3] 	= visualField[11];
			mapaResultado[status.fila][status.columna-3]   	= visualField[12];
			mapaResultado[status.fila-1][status.columna-3] 	= visualField[13];
			mapaResultado[status.fila-2][status.columna-3] 	= visualField[14];
			mapaResultado[status.fila-3][status.columna-3] 	= visualField[15];
		break;
	}
}