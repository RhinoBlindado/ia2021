#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"

#include <list>
#include <iostream>

struct estado {
  int fila;
  int columna;
  int orientacion;
};

struct objective
{
  estado status;
  int distToPlayer;
};

class ComportamientoJugador : public Comportamiento {
  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size) {
      // Inicializar Variables de Estado
      hayPlan = false;
      hasBikini = false;
      hasShoes = false;
      objectiveCounter = 0;
      for (int i = 0; i < mapaResultado.size(); i++)
      {
        mapaResultado[i][0] = 'P';
        mapaResultado[i][1] = 'P';
        mapaResultado[i][2] = 'P';

        mapaResultado[0][i] = 'P';
        mapaResultado[1][i] = 'P';
        mapaResultado[2][i] = 'P';

        mapaResultado[i][mapaResultado.size()-1] = 'P';
        mapaResultado[i][mapaResultado.size()-2] = 'P';
        mapaResultado[i][mapaResultado.size()-3] = 'P';

        mapaResultado[mapaResultado[0].size()-1][i] = 'P';
        mapaResultado[mapaResultado[0].size()-2][i] = 'P';
        mapaResultado[mapaResultado[0].size()-3][i] = 'P';
      }
      
    }
    ComportamientoJugador(std::vector< std::vector< unsigned char> > mapaR) : Comportamiento(mapaR) {
      // Inicializar Variables de Estado
      hayPlan = false;
      hasBikini = false;
      hasShoes = false;
    }
    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
    void VisualizaPlan(const estado &st, const list<Action> &plan);
    ComportamientoJugador * clone(){return new ComportamientoJugador(*this);}

  private:
    // Declarar Variables de Estado
    estado actual;
    objective currObj;
    list<objective> objetivos, actualObjectives, energyStations;
    list<Action> plan;
    bool hayPlan,
         hasBikini,
         hasShoes,
         hasEnemies,
         onStation,
         lowBattery;

    int batteryLevel,
        objectiveCounter;

    // Métodos privados de la clase
    bool pathFinding(int level, const estado &origen, const list<objective> &destino, objective survivalObj, list<Action> &plan);
    bool pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_Aestrella(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_AestrellaMulti(const estado &origin, const list<objective> &destination, list<Action> &theRoad);

    void PintaPlan(list<Action> plan);
    bool HayObstaculoDelante(estado &st);
    void removeFog(const estado &status, const vector<unsigned char> &visualField);
    bool isEnteringFog(const estado &status, const vector<unsigned char> &visualField);
    void checkForEnergyStations();
};

#endif
