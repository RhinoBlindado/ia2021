#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"

#include <iostream>
#include <cmath>
#include <set>
#include <stack>
#include <queue>

// Este es el método principal que se piden en la practica.
// Tiene como entrada la información de los sensores y devuelve la acción a realizar.
// Para ver los distintos sensores mirar fichero "comportamiento.hpp"
Action ComportamientoJugador::think(Sensores sensores) 
{

	for (int i = 0; i < mapaResultado.size(); i++)
	{
		for (int j = 0; j < mapaResultado[0].size(); j++)
		{
			cout<<mapaResultado[i][j]<<" ";
		}
		cout<<endl;
	}


	// Actualizar la variable global.
	actual.fila = sensores.posF;
	actual.columna = sensores.posC;
	actual.orientacion = sensores.sentido;

	batteryLevel = sensores.bateria;

	// Capturar destinos.
	objetivos.clear();
	for (int i = 0; i < sensores.num_destinos; i++)
	{
		estado aux;
		aux.fila = sensores.destino[2*i];
		aux.columna = sensores.destino[2*i+1];
		objetivos.push_back(aux);
	}
	
	if(!hayPlan)
	{
		hayPlan = pathFinding(sensores.nivel, actual, objetivos, plan);
	}

	Action sigAccion;
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
bool ComportamientoJugador::pathFinding (int level, const estado &origen, const list<estado> &destino, list<Action> &plan){
	switch (level){
		case 0: cout << "Demo\n";
						estado un_objetivo;
						un_objetivo = objetivos.front();
						cout << "fila: " << un_objetivo.fila << " col:" << un_objetivo.columna << endl;
			      return pathFinding_Profundidad(origen,un_objetivo,plan);
						break;

		case 1: // Incluir aqui la llamada al busqueda en anchura
			cout << "Optimo numero de acciones\n";
			estado actualObjetive;
			actualObjetive = objetivos.front();
			return pathFinding_Anchura(origen, actualObjetive, plan);
		break;
		case 2: // Incluir aqui la llamada al busqueda de costo uniforme
			cout << "Optimo en coste 1 Objetivo\n";
			return pathFinding_Aestrella(origen, objetivos.front(), plan);
		break;
		case 3: cout << "Optimo en coste 3 Objetivos\n";
						// Incluir aqui la llamada al algoritmo de busqueda para 3 objetivos
						cout << "No implementado aun\n";
						break;
		case 4: cout << "Algoritmo de busqueda usado en el reto\n";
						// Incluir aqui la llamada al algoritmo de busqueda usado en el nivel 2
						cout << "No implementado aun\n";
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
 * 		[NIVEL 2]
 */

/**
 * 
 */
class aStarNode
{
	private:
		estado nodeStatus;
		list<Action> routeSoFar;
		int totalCost;
		int accumCost;
		int expectedCost;

	public:

		aStarNode(){}

		aStarNode(estado arg_nodeStatus, int arg_accumCost, int arg_expectedCost, list<Action> arg_routeSoFar)
		{
			this->nodeStatus = arg_nodeStatus;
			this->accumCost = arg_accumCost;
			this->expectedCost = arg_expectedCost;
			this->totalCost = this->accumCost + this->expectedCost;
			this->routeSoFar = arg_routeSoFar;
		}

		estado getStatus()
		{
			return this->nodeStatus;
		}

		int getTotalCost()
		{
			return this->totalCost;
		}

		int getAccumCost()
		{
			return this->accumCost;
		}

		int getExpectedCost()
		{
			return this->expectedCost;
		}

		void printContents()
		{
			cout<<"Node ["<<nodeStatus.fila<<"]["<<nodeStatus.columna<<"]("<<nodeStatus.orientacion<<") f("<<totalCost<<")=g("<<accumCost<<")+h("<<expectedCost<<") Road size= "<<routeSoFar.size()<<endl;
		}

		bool equalCoords(estado argStatus)
		{
			return this->nodeStatus.fila == argStatus.fila && this->nodeStatus.columna == argStatus.columna;
		}

		list<Action> getRoute()
		{
			return this->routeSoFar;
		}

};

class costCompare
{
	public:
		bool operator()(aStarNode& a, aStarNode& b)
		{
			return a.getTotalCost() > b.getTotalCost();
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


/**
 * 
 */
bool ComportamientoJugador::pathFinding_Aestrella(const estado &origin, const estado &destination, list<Action> &theRoad)
{
	bool hasRoute = false, validNode;
	priority_queue<aStarNode, vector<aStarNode>, costCompare> open;
	set<estado,ComparaEstados> closed;
	list<Action> actualRoad;

	estado actStatus;
	int actAccumCost;
	Action actAction;

	aStarNode actNode, root(origin, 0, manhattanDist(origin, destination), theRoad);
	open.push(root);

	while (!open.empty())
	{
		actNode = open.top();
		open.pop();

		if(actNode.equalCoords(destination))
		{
			hasRoute = true;
			break;
		}

		closed.insert(actNode.getStatus());

		for (int i = 0; i < 3; i++)
		{
			validNode = false;
			actStatus = actNode.getStatus();
			actAccumCost = actNode.getAccumCost();
			actAction = i;

			// Girar a la derecha o izquierda
			if (i == actTURN_R || i == actTURN_L)
			{
				actStatus.orientacion = ( i == actTURN_L ? (actStatus.orientacion+3)%4 : (actStatus.orientacion+1)%4);

				switch (mapaResultado[actStatus.fila][actStatus.columna])
				{
					case 'A':
						if (hasBikini)
						{
							actAccumCost += 5;
						}
						else
						{
							actAccumCost += 500;
						}
					break;
					
					case 'B':
						if (hasShoes)
						{
							actAccumCost += 1;
						}
						else
						{
							actAccumCost += 3;
						}
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
						if (hasBikini)
						{
							actAccumCost += 5;
						}
						else
						{
							actAccumCost += 500;
						}
					break;
					
					case 'B':
						if (hasShoes)
						{
							actAccumCost += 1;
						}
						else
						{
							actAccumCost += 3;
						}
					break;

					case 'T':
						actAccumCost += 2;
					break;

					case 'K':
						actAccumCost += 1;
						hasBikini = true;
						hasShoes = false;
					break;

					case 'D':
						actAccumCost += 1;
						hasBikini = false;
						hasShoes = true;
					break;

					case 'X':
						actAccumCost -= 10;
					break;

					default:
						actAccumCost += 1;
					break;

					if(!HayObstaculoDelante(actStatus))
					{
						validNode = true;
					}
				}

			}

			if (validNode)
			{
				actualRoad = actNode.getRoute();
				actualRoad.push_back(actAction);
				
				nodo newNode(actStatus, actAccumCost, manhattanDist(actStatus, destination), actualRoad);
			}
		}
	}

	if(hasRoute)
	{
		theRoad = actNode.getRoute();
		cout<<"Ruta encontrada, longitud: "<<theRoad.size()<<endl;
		PintaPlan(theRoad);
		VisualizaPlan(origin, theRoad);
	}
	else
	{
		cout<<"Ruta no encontrada."<<endl;
	}
	
	return hasRoute;
}