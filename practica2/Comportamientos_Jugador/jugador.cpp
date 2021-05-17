#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"

#include <iostream>
#include <cmath>
#include <set>
#include <stack>
#include <queue>
#include <map>

// Declaración de la función, la implementación está más abajo.
int manhattanDist(const estado &a, const estado &b);

/**
 * @brief Función auxiliar utilizada para ordenar la lista de objetivos en orden del más cercano al agente.
 * @param	a	Un objetivo cualquiera
 * @param	b	Otro objetibo cualquiera
 * @returns Verdadero si a está más cerca que b del agente, Falso en caso contrario.
 */
bool objSort(const objective &a, const objective &b)
{
	return a.distToPlayer < b.distToPlayer;
}

/**
 * @brief Función que se encarga de quitar el objetivo por el que el agente ha pasado, utilizado debido a que
 * 		  cuando el agente va a recargar baterías, puede que en su ruta se consiga con un objetivo.
 * @param curssPos	Estado del agente
 * @param objList	Lista de objetivos
 */
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

	// Obtener la batería actual
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

	// Se actualiza si el agente pasa por una casilla de bikini o de zapatillas.
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

	// La funcionalidad del nivel 4 está contenida aquí dentro, en caso contrario se ignora.
	if(sensores.nivel == 4)
	{
		//	Rellenar el mapa y devuelve verdadero si se está pasando por una zona desconocida del mapa (Existe al menos un '?').
		if(clearFog(actual, sensores.terreno))
		{
			/* 
			 * Si se está pasando por una zona recién descubierta y los sensores de proximidad más 
			 * cercanos detectan: agua, bosques, muros o precipicios; entonces recalcular el plan.
			 */
			for (int i = 1; i <= 3; i++)
			{
				if (sensores.terreno[i] == 'A' || 
					sensores.terreno[i] == 'B' || 
					sensores.terreno[i] == 'M' || 
					sensores.terreno[i] == 'P')
				{
					hayPlan = false;
				}
			}
		}
	
		// Verificar el sensor de superficie por la presencia de aldeanos.
		checkEnemies(actual, sensores.superficie);

		// Si hay aldeanos cercanos...
		if (!enemiesNearby.empty())
		{
			//... tomarlo como objeto inamovible
			evadeEnemies();

			// ¿Está frente mío? Pues le paso por un lado.
			if (sensores.colision)
			{
				hayPlan = false;
			}
		}
		
		// Si estoy en mi objetivo y tengo baja batería, entonces estoy en una estación de recarga.
		if (actual.fila == currObj.status.fila && actual.columna == currObj.status.columna && lowBattery)
		{
			// Me quedo quieto...
			plan.push_back(actIDLE);

			// ... hasta que tenga batería, y sigo con los objetivos.
			if(batteryLevel > 2000)
			{
				lowBattery = false;
				hayPlan = false;
			}				
		}

		// ¿Me estoy quedando sin baterías?
		if (batteryLevel < 1500)
		{
			// Busco en el mapa estaciones de recarga...
			checkForEnergyStations();

			// ... si hay estaciones ...
			if (!energyStations.empty())
			{
				// ... elijo la más cercana a mí
				currObj = energyStations.front();
				lowBattery = true;
			}
		}

		// ¿He llegado a un objetivo? Quitarlo y reordenar la lista de objetivos.
		checkIfOnObjective(actual, actualObjectives);

		// Si no tengo batería baja ...
		if(!lowBattery)
		{
			// ... voy a por los objetivos.

			// Si he llegado a los 3 objetivos, entonces obtener los siguientes 3.
			if (actualObjectives.empty())
			{
				actualObjectives = objetivos;
				actualObjectives.sort(objSort);
			}
			// Tomo el más cercano.
			currObj = actualObjectives.front();
		}
	}


	// Si no tengo plan, generarlo. 
	// Adicionalmente, si el plan está vacío y estoy en el nivel 4, también volverlo a regenerar.
	if(!hayPlan || (plan.empty() && sensores.nivel == 4))
	{
		plan.clear();
		hayPlan = pathFinding(sensores.nivel, actual, objetivos, currObj, plan);
	}

	Action sigAccion;

	if(hayPlan && plan.size() > 0)
	{
		sigAccion = plan.front();
		plan.erase(plan.begin());
	}

	// Si hay enemigos en el sensor de superficie, ahora restauro el mapa a como estaba antes.
	if(!enemiesNearby.empty())
	{
		restoreMap(sensores.terreno);
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

/**
 * @brief Obtener una ruta por medio del Búsqueda en Anchura
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
 * @brief Función auxiliar utilizada por la clase aStarNode para comparar los objetivos que tienen dentro los nodos. 
 * 		  Utilizado en el nivel 3, pero está aquí porque la clase es la misma para el nivel 2 y 3.
 * @param a Lista de objetivos
 * @param b Lista de objetivos
 * @returns	Verdadero si a y b son idénticos o si están vacíos, Falso en caso contrario.
 */
bool haveSameObjectivesOpen(const list<objective> &a, const list<objective> &b)
{

	if(a.empty() && b.empty())
	{
		return true;
	}
	
	if (a.size() != b.size())
	{
		return false;
	}
	
	list<objective>::const_iterator bIter = b.begin();

	for (list<objective>::const_iterator aIter = a.begin(); aIter != a.end(); ++aIter) 
	{
		if (aIter->status.fila != bIter->status.fila || aIter->status.columna != bIter->status.columna)
		{
			return false;
		}

		++bIter;	
	}
	return true;
}

/**
 * @brief Clase que se utiliza con el algoritmo A* para mantener los nodos.
 */
class aStarNode
{
	private:
		estado nodeStatus;					//	Estado del nodo
		list<Action> routeSoFar;			//	Ruta que ha tomado para llegar al nodo.
		list<objective> currentObjectives;	//	Lista de objetivos que tiene el nodo, usado en el nivel 3.
		int totalCost;						//  Costo total del camino, f(n) = g(n) + h(n)
		int accumCost;						// 	Costo acumulado, lo que ha costado llegar hasta donde está el nodo, g(n)
		int expectedCost;					//  Costo estimado, la heurística, hacia el objetivo, h(n)
		bool hasBikini;						//  Si se tiene el bikini equipado en el nodo.
		bool hasShoes;						//	Si se tienen las zapatillas equipadas en el nodo.

	public:
		aStarNode(){}

		/**
		 * @brief Constructor con parámetros para el A* original
		 * @param arg_nodeStatus	Estado del nodo
		 * @param arg_accumCost		Costo acumulado hasta el momento, es decir, el gasto de la batería.
		 * @param arg_expectedCost	Costo estimado de batería hasta el objetivo.
		 * @param arg_routeSoFar	Lista de acciones que han llevado hasta el estado actual.
		 * @param arg_bikini		Indica si se tiene equipado o no el bikini.
		 * @param arg_shoes			Indica si se tienen equipadas o no las zapatillas.
		 */
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

		/**
		 * @brief Constructor con parámetros para el A* multiobjetivo
		 * @param arg_nodeStatus	Estado del nodo
		 * @param arg_accumCost		Costo acumulado hasta el momento, es decir, el gasto de la batería.
		 * @param arg_expectedCost	Costo estimado de batería hasta el objetivo.
		 * @param arg_routeSoFar	Lista de acciones que han llevado hasta el estado actual.
		 * @param arg_bikini		Indica si se tiene equipado o no el bikini.
		 * @param arg_shoes			Indica si se tienen equipadas o no las zapatillas.
		 * @param arg_obj			Lista de objetivos que posee el nodo.
		 */
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

		/**
		 * @brief Retornar el estado del nodo
		 * @returns Estado
		 */
		estado getStatus() const
		{
			return this->nodeStatus;
		}

		/**
		 * @brief Obtener el coste total del nodo, f(n) = g(n) + h(n)
		 * @returns Coste total
		 */
		int getTotalCost() const
		{
			return this->totalCost;
		}

		/**
		 * @brief Obtener el coste acumulado del camino, g(n)
		 * @returns Coste acumulado.
		 */
		int getAccumCost() const
		{
			return this->accumCost;
		}

		/**
		 * @brief Obtener el coste esperado del camino, h(n)
		 * @returns Coste esperado.
		 */
		int getExpectedCost() const
		{
			return this->expectedCost;
		}

		/**
		 * @brief Saber si el nodo posee bikini.
		 * @return Verdadero si tiene bikini, Falso en caso contrario.
		 */
		bool getIfHasBikini() const
		{
			return this->hasBikini;
		}

		/**
		 * @brief Saber si el nodo posee zapatillas.
		 * @return Verdadero si tiene zapatillas, Falso en caso contrario.
		 */
		bool getIfHasShoes() const
		{
			return this->hasShoes;
		}

		/**
		 * @brief Compara si un estado posee las mismas coordenadas que el nodo.
		 * @param argStatus Estado con el cual comparar coordenadas.
		 * @return Verdadero si las coordenadas coinciden, Falso en caso contrario.
		 */
		bool equalCoords(estado argStatus) const
		{
			return this->nodeStatus.fila == argStatus.fila && this->nodeStatus.columna == argStatus.columna;
		}

		/**
		 * @brief Compara si un nodo es idéntico a otro nodo, salvo por el coste que posean.
		 * @param arg Nodo A* con que comparar el estado.
		 * @returns Verdadero si el nodo posee la misma posición, orientación, bikini, zapatillas y objetivos, Falso en caso contrario.
		 */
		bool equalNode(const aStarNode &arg) const
		{
			return 	this->nodeStatus.fila == arg.nodeStatus.fila && 
					this->nodeStatus.columna == arg.nodeStatus.columna && 
					this->nodeStatus.orientacion == arg.nodeStatus.orientacion &&
					this->hasBikini == arg.hasBikini &&
					this->hasShoes == arg.hasShoes &&
					haveSameObjectivesOpen(this->currentObjectives, arg.currentObjectives);
		}

		/**
		 * @brief Devuelve la ruta del nodo
		 * @returns Lista de acciones
		 */
		list<Action> getRoute() const
		{
			return this->routeSoFar;
		}

		/**
		 * @brief Devuelve la lista de objetivos del nodo
		 * @returns Lista de objetivos
		 */
		list<objective> getObjectives() const
		{
			return this->currentObjectives;
		}

		/**
		 * @brief Devuelve el objetivo actual del nodo
		 * @returns Objetivo actual
		 */
		objective getActualObjective() const
		{
			return this->currentObjectives.front();
		}

		/**
		 * @brief Devuelve el número de objetivos por visitar del nodo.
		 * @returns Número de objetivos.
		 */
		int getNumObjectives() const
		{
			return this->currentObjectives.size();
		}

		/**
		 * @brief Quita el objetivo actual del nodo.
		 */
		void removeActualObjective()
		{
			if(!currentObjectives.empty())
			{
				this->currentObjectives.pop_front();
			}
		}

		/**
		 * @brief Copia los datos de un nodo a otro.
		 * @param arg	Nodo A* al que se le copiarán los atributos.
		 */
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

		// Métodos de Depuración, imprimen los contenidos del nodo.
		void printContents() const
		{
			cout<<"\tNode ["<<nodeStatus.fila<<"]["<<nodeStatus.columna<<"]("<<nodeStatus.orientacion<<") K("<<hasBikini<<") S("<<hasShoes<<") | f("<<totalCost<<")=g("<<accumCost<<")+h("<<expectedCost<<") Road size="<<routeSoFar.size()<<" Objectives="<<currentObjectives.size()<<endl;
		}

		void printObjs()
		{
			cout<<"| Objs [ ";
			for (list<objective>::iterator aIter = this->currentObjectives.begin(); aIter != this->currentObjectives.end(); ++aIter) 
			{
				cout<<"["<<aIter->status.fila<<"]["<<aIter->status.columna<<"] "; 
			}
			cout<<"]"<<endl;
		}

};


/**
 * @brief Comparador para la lista de abiertos del A*, ordena los nodos por el coste que poseen
 * @param a  Nodo de A*
 * @param b  Nodo de A*
 * @returns Verdadero si 'a' posee un coste total menor que 'b', o si son iguales, posee un coste acumulado menor que b. Falso en caso contrario.
 */
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
		/**
		 * @brief Comparador para la lista de cerrados del A*
		 * @param a Nodo de A*
		 * @param b Nodo de A*
		 * @returns Verdadero si 'a' es en cierta manera diferente de 'b', Falso en caso contrario.
		 */
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

/**
 * @brief Calcular la distancia Manhattan entre dos puntos.
 * @param origin		Estado de la posición de origen.
 * @param destination	Estado de la posición de destino.
 * @return	Distancia Manhattan entre origin y destination.
 */
int manhattanDist(const estado &origin, const estado &destination)
{
	return abs(origin.fila - destination.fila) + abs(origin.columna - destination.columna);
}

/**
 * @brief Implementación del algoritmo A* para un único objetivo
 * @param origin		Estado origen
 * @param destination	Estado destino
 * @param theRoad		Lista de acciones para llegar del origen al destino.
 * @returns	Verdadero si se ha encontrado ruta, falso en caso contrario.
 */
bool ComportamientoJugador::pathFinding_Aestrella(const estado &origin, const estado &destination, list<Action> &theRoad)
{
	bool hasRoute = false, 
		 validNode,
		 isOnClosed,
		 isOnOpen,
		 actHasBikini = this->hasBikini, // Obtener si el agente posee ya bikinis.
		 actHasShoes = this->hasShoes;   // Ídem con las zapatillas.

	multiset<aStarNode,bool(*)(const aStarNode&, const aStarNode&)> open (costCompare);		// "Lista" de ABIERTOS
	multiset<aStarNode>::iterator openFinder;

	multiset<aStarNode, isSameNode> closed;													// "Lista" de CERRADOS
	multiset<aStarNode>::iterator closedFinder;
	
	list<Action> actualRoad;

	estado actStatus;
	int actAccumCost;
	Action actAction;
	
	// Se crea el nodo raíz y se inserta de una vez en ABIERTOS, se declaran adicionalmente un nodo auxiliar.
	aStarNode actNode, root(origin, 0, manhattanDist(origin, destination), theRoad, actHasBikini, actHasShoes);
	open.insert(root);

	// Mientras existan nodos en ABIERTOS, explorar...
	while (!open.empty())
	{
		// Obtener el nodo más prometedor y quitarlo de ABIERTOS.
		openFinder = open.begin();
		actNode = *openFinder;
		open.erase(openFinder);

		// Si el nodo está en el destino, hay ruta y finaliza el bucle.
		if(actNode.equalCoords(destination))
		{
			hasRoute = true;
			break;
		}

		// Sino, lo meto en CERRADOS y genero los hijos.
		closed.insert(actNode);

		// Generar un hijo implica o bien rotar hacia la derecha o la izquierda o moverse hacia delante, se hace un bucle para ello.
		for (int i = 0; i < 3; i++)
		{
			// Se inicializan las variables auxiliares al estado actual del nodo padre.
			validNode = false;
			actStatus = actNode.getStatus();
			actAccumCost = actNode.getAccumCost();
			actHasBikini = actNode.getIfHasBikini();
			actHasShoes = actNode.getIfHasShoes();
			actAction = (Action)i;

			// Estas variables están sujetas a ser modificadas dependiendo de las acciones que tome el nodo hijo.

			// Girar a la derecha o izquierda
			if (i == actTURN_R || i == actTURN_L)
			{
				actStatus.orientacion = ( i == actTURN_L ? (actStatus.orientacion+3)%4 : (actStatus.orientacion+1)%4);

				// Se aumenta el coste transcurrido, es decir g(n), dependiendo del terreno en que se encuentra el agente.
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

				// El nodo siempre será válido cuando se gira ya que no es posible girar en un muro o un precipicio.
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
				
				// Si para donde se mueve no es un muro o precipicio, el nodo es válido.
				if(!HayObstaculoDelante(actStatus))
				{
					validNode = true;
				}
			}

			// Si el nodo hijo es un nodo válido, crearlo propiamente.
			if (validNode)
			{
				// El camino se aumenta en uno, añadiendo la última acción que se hizo.
				actualRoad = actNode.getRoute();
				actualRoad.push_back(actAction);

				// Se crea propiamente el nodo.
				aStarNode newNode(actStatus, actAccumCost, manhattanDist(actStatus, destination), actualRoad, actHasBikini, actHasShoes);

				isOnClosed = false;
				isOnOpen = false;

				closedFinder = closed.find(newNode);

				// Si el nodo está en CERRADOS ...
				if(closedFinder != closed.end())
				{
					isOnClosed = true;
					// ... y el nodo actual posee un mejor g(n) que su copia en CERRADOS, quitarlo y añadirlo en ABIERTOS.
					if(newNode.getAccumCost() < closedFinder->getAccumCost())
					{
						closed.erase(closedFinder);
						open.insert(newNode);
					}
				}

				// Si el nodo no está en CERRADOS, buscarlo en ABIERTOS.
				if(!isOnClosed)
				{		
					for (multiset<aStarNode>::iterator it = open.begin(); it != open.end(); ++it)
					{
						// Si el nodo está en ABIERTOS...
						if (newNode.equalNode(*it))
						{
							isOnOpen = true;
							// ... y el nodo actual posee una mejor g(n) que su copia de ABIERTOS, "reemplazar" la copia en ABIERTOS.
							if (newNode.getAccumCost() < it->getAccumCost())
							{
								open.erase(it);
								open.insert(newNode);
							}
							break;
						}	
					}
				}

				// Sino está ni en CERRADOS ni en ABIERTOS, insertarlo en ABIERTOS.
				if(!isOnClosed && !isOnOpen)
				{
					open.insert(newNode);
				}			
			}
		}
	}

	// Si hay ruta, visualizarla e igualar a la variable por referencia del procedimiento.
	if(hasRoute)
	{
		cout<<"Ruta encontrada."<<endl;
		cout<<"Coste en Batería: "<<actNode.getAccumCost()<<" ("<<this->batteryLevel-actNode.getAccumCost()<<")"<<endl;
		theRoad = actNode.getRoute();
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
 * 		[NIVEL 3]
 */

/**
 * @brief Comparador auxiliar para la lista de cerrados, compara si dos listas de objetivos son iguales o poseen alguna diferencia.
 * @param a  Lista de objetivos
 * @param b  Lista de objetivos
 * @returns Verdadero si 'a' es en cierta forma diferente de 'b', Falso en caso contrario.
 */
bool haveSameObjectivesClosed(const list<objective> &a, const list<objective> &b)
{
	if (a.size() > b.size())
	{
		return true;
	}
	else if(a.size() == b.size())
	{
		list<objective>::const_iterator bIter = b.begin();
		
		for (list<objective>::const_iterator aIter = a.begin(); aIter != a.end(); ++aIter) 
		{
			if ((aIter->status.fila > bIter->status.fila) || 
			    (aIter->status.fila == bIter->status.fila && aIter->status.columna > bIter->status.columna))
			{
				return true;
			}

			++bIter;	
		}
		return false;
	}
	else return false;
} 


class isSameNodeMulti
{
	public:
		/**
		 * @brief Comparador para la lista de cerrados del A* Multiobjetivo
		 * @param a Nodo de A*
		 * @param b Nodo de A*
		 * @returns Verdadero si 'a' es en cierta manera diferente de 'b', Falso en caso contrario.
		 */
		bool operator()(const aStarNode &a, const aStarNode &b)
		{			
			if ((a.getStatus().fila > b.getStatus().fila) or 
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna > b.getStatus().columna) or
	      		(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion > b.getStatus().orientacion) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && (int)a.getIfHasBikini() > (int)b.getIfHasBikini()) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && a.getIfHasBikini() == b.getIfHasBikini() && (int)a.getIfHasShoes() > (int)b.getIfHasShoes()) ||
				(a.getStatus().fila == b.getStatus().fila and a.getStatus().columna == b.getStatus().columna and a.getStatus().orientacion == b.getStatus().orientacion && a.getIfHasBikini() == b.getIfHasBikini() && a.getIfHasShoes() == b.getIfHasShoes() && haveSameObjectivesClosed(a.getObjectives(), b.getObjectives())))
				return true;
			else
				return false;
		}
};

/**
 * @brief Cálculo de la distancia Manhattan entre un origen y múltiples objetivos
 * @param origin	Estado origen
 * @param destination Lista de objetivos 
 * @returns Distancia Manhattan del origen hacia los objetivos en el orden en que se encuentran.
 */
int multiManhattanDist(const estado &origin, const list<objective> &objectives)
{
	int sum = 0;
	list<objective>::const_iterator destination;

	destination = objectives.begin();

	sum = manhattanDist(origin, destination->status);

	for (list<objective>::const_iterator i = ++destination; i != objectives.end(); ++i) 
	{
		sum += manhattanDist(destination->status, i->status);
		destination = i;
	}

	return sum;
}

/**
 * @brief Implementación del algoritmo A* para multiples objetivos
 * @param origin		Estado origen.
 * @param destination	Lista de destinos.
 * @param theRoad		Ruta desde el origen pasando por cada destino.
 * @returns Verdadero si se encuentra la ruta, falso en caso contrario.
 */
bool ComportamientoJugador::pathFinding_AestrellaMulti(const estado &origin, const list<objective> &destination, list<Action> &theRoad)
{
	bool hasRoute = false, 
		 validNode,
		 isOnClosed,
		 isOnOpen,
		 actHasBikini = this->hasBikini, // Obtener si el agente ya posee bikini.
		 actHasShoes = this->hasShoes,	 // Ídem con las zapatillas.
		 scrambleObjectives = true;	     // Barajear los objetivos, al principio siempre se realiza.

	multiset<aStarNode,bool(*)(const aStarNode&, const aStarNode&)> open (costCompare);  // "Lista" de ABIERTOS
	multiset<aStarNode>::iterator openFinder;

	multiset<aStarNode, isSameNodeMulti> closed;												 // "Lista" de CERRADOS
	multiset<aStarNode>::iterator closedFinder;
	
	list<Action> actualRoad;
	list<objective> actObjectives; // Se tiene adicionalmente una lista de objetivos para el nodo actual que se está analizando.

	estado actStatus;

	objective actDestination;

	int actAccumCost, 
		i = 0, 
		maxScram = destination.size();

	Action actAction;
	/**
	 *  Se crea el nodo raíz y se inserta de una vez en ABIERTOS, se declaran adicionalmente un nodo auxiliar, notar que se utiliza ahora la heurística
	 *  de distancia Manhattan múltiple en vez de la variante original.
	 */ 
	aStarNode actNode, root(origin, 0, multiManhattanDist(origin, destination), theRoad, actHasBikini, actHasShoes, destination), auxNode;
	open.insert(root);

	// Mientras existan nodos en ABIERTOS, explorar...
	while (!open.empty())
	{
		// Obtener el nodo más prometedor y quitarlo de ABIERTOS.
		openFinder = open.begin();
		actNode = *openFinder;
		open.erase(openFinder);

		// Obtener el destino al que se dirige este nodo en particular
		actDestination = actNode.getActualObjective();

		// Si el nodo está en el destino...
		if(actNode.equalCoords(actDestination.status))
		{
			// ... y en la lista de objetivos solo está ese destino, ya se han encontrado los 3 objetivos. Hay ruta y finaliza el bucle.
			if (actNode.getNumObjectives() == 1)
			{
				hasRoute = true;
				break;
			}
			// ... sino, quitar el objetivo, y en los hijos barajear los objetivos que restan.
			else
			{
				actNode.removeActualObjective();
				// Se vuelven a barajear los objetivos restantes.
				scrambleObjectives = true;
				i = 0;
				maxScram = actNode.getNumObjectives();
			}

		}

		// Sino, lo meto en CERRADOS y genero los hijos.
		closed.insert(actNode);

		/**
		 *  - La mayor diferencia con el A* original es que, para obtener la ruta óptima de visitar varios objetivos, se deben generar hijos adicionales
		 *    que ramifican la búsqueda, es decir, hijos que van a diferentes objetivos y entre ellos se obtendrá la ruta óptima. Naturalmente,
		 *    una vez que se llega a un destino, los hijos de ese nodo padre deberán ramificar también los objetivos restantes.
		 * 
		 *  - Esto se realiza con el do-while adicional: se poseen en este caso 3 objetivos (aunque la implementación soporta
		 *    n objetivos también) entonces, en vez de generarse 3 hijos solamente, se generan 9 hijos, cada trío posee un objetivo para el cual ir diferente.
		 * 
		 *  - Esto no se realiza en cada iteración, ya que es muy ineficiente porque se generan muchos nodos repetidos, se realiza cada vez que un nodo o bien
		 *    es el nodo raíz o el nodo padre ha llegado a un objetivo, por que allí tiene sentido ramificar la búsqueda, mientras un nodo está en búsqueda de ese
		 *    objetivo, no tendría sentido ramificar, ya que existen ya otros nodos hijo que están en búsqueda de los otros objetivos.
		 * 
		 *  - Se utiliza un do-while ya que en el caso de que no se necesiten más iteraciones, se generarán 3 hijos y el bucle no se repetirá, ya que la condición
		 *    se comprueba al final y estará en Falso.
		 */

		actObjectives = actNode.getObjectives();

		do 
		{
			// Si es el baraje, para cada hijo, recibirá un objetivo diferente, ya que el frente de la lista irá cambiando.
			if(scrambleObjectives && actObjectives.size() > 1)
			{
				actObjectives.push_back(actObjectives.front());
				actObjectives.pop_front();
			}
			
			i++;
			// Generar un hijo implica o bien rotar hacia la derecha o la izquierda o moverse hacia delante, se hace un bucle para ello.
			for (int j = 0; j < 3; j++)
			{
				// Se inicializan las variables auxiliares al estado actual del nodo padre.
				validNode = false;
				actStatus = actNode.getStatus();
				actAccumCost = actNode.getAccumCost();
				actHasBikini = actNode.getIfHasBikini();
				actHasShoes = actNode.getIfHasShoes();
				actAction = (Action)j;

				// Estas variables están sujetas a ser modificadas dependiendo de las acciones que tome el nodo hijo.

				// Girar a la derecha o izquierda
				if (j == actTURN_R || j == actTURN_L)
				{
					actStatus.orientacion = ( j == actTURN_L ? (actStatus.orientacion+3)%4 : (actStatus.orientacion+1)%4);

					// Se aumenta el coste transcurrido, es decir g(n), dependiendo del terreno en que se encuentra el agente.
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

					// El nodo siempre será válido cuando se gira ya que no es posible girar en un muro o un precipicio.
					validNode = true;
				}

				// Avanzar
				if (j == actFORWARD)
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

					// Si para donde se mueve no es un muro o precipicio, el nodo es válido.
					if(!HayObstaculoDelante(actStatus))
					{
						validNode = true;
					}

				}

				// Si el nodo hijo es un nodo válido, crearlo propiamente.
				if (validNode)
				{
					// El camino se aumenta en uno, añadiendo la última acción que se hizo.
					actualRoad = actNode.getRoute();
					actualRoad.push_back(actAction);

					// Se crea propiamente el nodo.
					aStarNode newNode(actStatus, actAccumCost, multiManhattanDist(actStatus, destination), actualRoad, actHasBikini, actHasShoes, actObjectives);

					isOnClosed = false;
					isOnOpen = false;

					closedFinder = closed.find(newNode);

					// Si el nodo está en CERRADOS ...
					if(closedFinder != closed.end())
					{
						isOnClosed = true;
						// ... y el nodo actual posee un mejor g(n) que su copia en CERRADOS, quitarlo y añadirlo en ABIERTOS.
						if(newNode.getAccumCost() < closedFinder->getAccumCost())
						{
							closed.erase(closedFinder);
							open.insert(newNode);
						}
					}

					// Si el nodo no está en CERRADOS, buscarlo en ABIERTOS.
					if(!isOnClosed)
					{					
						for (multiset<aStarNode>::iterator it = open.begin(); it != open.end(); ++it)
						{
							// Si el nodo está en ABIERTOS...
							if (newNode.equalNode(*it))
							{
								isOnOpen = true;
								// ... y el nodo actual posee una mejor g(n) que su copia de ABIERTOS, "reemplazar" la copia en ABIERTOS.
								if (newNode.getAccumCost() < it->getAccumCost())
								{
									open.erase(it);
									open.insert(newNode);
								}
								break;
							}	
						}
					}
					// Sino está ni en CERRADOS ni en ABIERTOS, insertarlo en ABIERTOS.
					if(!isOnClosed && !isOnOpen)
					{
						open.insert(newNode);
					}			

				}
			}

			// Si ya se ha dado vuelta entera a la lista, entonces dejar de barajear.
			if (i == maxScram && scrambleObjectives)
			{
				scrambleObjectives = false;
				i = 0;
			}

		}while(scrambleObjectives);
	}

	// Si hay ruta, visualizarla e igualar a la variable por referencia del procedimiento.
	if(hasRoute)
	{
		cout<<"Ruta encontrada."<<endl;
		cout<<"Coste en Batería: "<<actNode.getAccumCost()<<" ("<<this->batteryLevel-actNode.getAccumCost()<<")"<<endl;
		theRoad = actNode.getRoute();
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
 * 		[NIVEL 4]
 */

/**
 * @brief Chequea el mapa por si se han descubierto estaciones de recarga, las almacena en la lista 'energyStations'
 * 		  y la lista de ordena teniendo de primero a la estación más cercana.
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

/**
 * @brief Va descubriendo el mapa a medida que el agente atraviesa el mundo.
 * @param status	Posición y orientación actual del agente
 * @param visualField Sensores de terreno
 * @returns Verdadero si se está atravesando por una zona desconocida, o sea, se detecta 
 * 			al menos una casilla desconocida, Falso en caso contrario.
 */
bool ComportamientoJugador::clearFog(const estado &status, const vector<unsigned char> &visualField)
{
	bool silentHill = false;
	int k = 1;

	if (mapaResultado[status.fila][status.columna] == '?')
	{
		mapaResultado[status.fila][status.columna] = visualField[0];
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
						mapaResultado[status.fila-i][status.columna+j] =  visualField[k];
					}
					k++;
					
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
						mapaResultado[status.fila+j][status.columna+i] = visualField[k];
					}
					k++;
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
						mapaResultado[status.fila+i][status.columna+j] = visualField[k];
					}
					k++;
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
						mapaResultado[status.fila+j][status.columna-i] = visualField[k];
					}
					k++;
				}

			}
		break;
	}
	return silentHill;
}

/**
 * @brief Verifica el sensor de superficie por enemigos, los guarda en un vector llamado 'enemiesNearby'.
 * @param status		Estado actual del jugador.
 * @param enemyRadar	Sensor de superficie
 */
void ComportamientoJugador::checkEnemies(const estado &status, const vector<unsigned char> &enemyRadar)
{
	enemiesNearby.clear();
	int k = 1;
	enemy aux;

	switch (status.orientacion)
	{
		case 0:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = -i; j <= i; j++)
				{
					if (enemyRadar[k] == 'a')
					{
						aux.status.fila = status.fila - i;
						aux.status.columna = status.columna + j;
						aux.visualFieldIndex = k;
						enemiesNearby.push_back(aux);
					}
					k++;
				}

			}
		break;

		case 1:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = -i; j <= i; j++)
				{
					if (enemyRadar[k] == 'a')
					{
						aux.status.fila = status.fila + j;
						aux.status.columna = status.columna + i;
						aux.visualFieldIndex = k;
						enemiesNearby.push_back(aux);
					}
					k++;
				}
			}
		break;

		case 2:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = i; j >= -i; j--)
				{
					if (enemyRadar[k] == 'a')
					{
						aux.status.fila = status.fila + i;
						aux.status.columna = status.columna + j;
						aux.visualFieldIndex = k;
						enemiesNearby.push_back(aux);
					}
					k++;		
				}
			}
		break;

		case 3:
			for (int i = 1; i <= 3; i++)
			{
				for (int j = i; j >= -i; j--)
				{
					if (enemyRadar[k] == 'a')
					{
						aux.status.fila = status.fila + j;
						aux.status.columna = status.columna - i;
						aux.visualFieldIndex = k;
						enemiesNearby.push_back(aux);
					}
					k++;
				}
			}
		break;
	}
}

/**
 * @brief Evade los enemigos cercanos, toma la posición donde está el enemigo y lo marca como si fuera un muro.
 * 		  De esta manera el A* hará que el agente le pase por un lado.
 */
void ComportamientoJugador::evadeEnemies()
{
	for (int i = 0; i < enemiesNearby.size(); i++)
	{
		mapaResultado[enemiesNearby[i].status.fila][enemiesNearby[i].status.columna] = 'M';
	}
	
}

/**
 * @brief Luego de generar la ruta, se restuara el mapa, se obtiene del sensor de terreno que terreno hay en donde está
 *        parado el enemigo.
 * @param visualField	Sensor de terreno del agente.
 */
void ComportamientoJugador::restoreMap(const vector<unsigned char> &visualField)
{
	for (int i = 0; i < enemiesNearby.size(); i++)
	{
		mapaResultado[enemiesNearby[i].status.fila][enemiesNearby[i].status.columna] = visualField[enemiesNearby[i].visualFieldIndex];
	}
}