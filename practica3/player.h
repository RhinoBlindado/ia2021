#ifndef PLAYER_H
#define PLAYER_H

#include "environment.h"

class Player{
    public:
      Player(int jug);
      Environment::ActionType Think();
      void Perceive(const Environment &env);
      double MiniMax(Environment env, int jugador_actual, int profundidad_maxima, Environment::ActionType & accionElegida);

    private:
      int jugador_;
      Environment actual_;
};
#endif
