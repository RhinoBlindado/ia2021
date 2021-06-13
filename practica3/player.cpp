#include <iostream>
#include <cstdlib>
#include <vector>
#include <queue>
#include "player.h"
#include "environment.h"
#include <map>
#include <list>
#include <limits>
using namespace std;

const double masinf=9999999999.0, menosinf=-9999999999.0;


// Constructor
Player::Player(int jug){
    jugador_=jug;
}

// Actualiza el estado del juego para el jugador
void Player::Perceive(const Environment & env){
    actual_=env;
}

double Puntuacion(int jugador, const Environment &estado){
    double suma=0;

    for (int i=0; i<7; i++)
      for (int j=0; j<7; j++){
         if (estado.See_Casilla(i,j)==jugador){
            if (j<3)
               suma += j;
            else
               suma += (6-j);
         }
      }

    return suma;
}


// Funcion de valoracion para testear Poda Alfabeta
double ValoracionTest(const Environment &estado, int jugador){
    int ganador = estado.RevisarTablero();

    if (ganador==jugador)
       return 99999999.0; // Gana el jugador que pide la valoracion
    else if (ganador!=0)
            return -99999999.0; // Pierde el jugador que pide la valoracion
    else if (estado.Get_Casillas_Libres()==0)
            return 0;  // Hay un empate global y se ha rellenado completamente el tablero
    else
          return Puntuacion(jugador,estado);
}

// ------------------- Los tres metodos anteriores no se pueden modificar

/**
 *  [CASTELLANO]
 * 
 * Practica 3 - Busqueda con Adversario & Juegos
 * Asignatura: Inteligencia Artificial
 * Autor: Valentino Lugli (Github: @RhinoBlindado)
 * Mayo, Junio 2021
 */

/**
 *  [ENGLISH]
 *
 * Practice 3 - Adversary Search and Games
 * Course: Artificial Intelligence
 * Author: Valentino Lugli (Github: @RhinoBlindado)
 * May, June 2021
 */


// Matriz para evaluar la posición de una ficha en el tablero.
const int baseGrid[7][7]=
{
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1},
   {1, 2, 3, 4, 3, 2, 1}
};

/**
 * @brief Registro para almacenar una ficha.
 * @param i Posición en la fila del tablero.
 * @param j Posición en la columna del tablero.
 * @param score Valoración dependiendo de su localización.
 */
struct token
{
   int i;
   int j;
   double score;
};

/**
 * @brief Funtor utilizado para buscar las fichas en el map.
 */
class compareTokens
{
   public:
      bool operator()(const pair<int, int> &a, const pair<int, int> &b)
      {
         if ((a.first > b.first) ||
             (a.first == b.first && a.second > b.second))
         {
            return true;
         }
         else
         {
            return false;
         }
   }
};

/**
 * @brief Chequear que efectos produce la ficha bomba en el tablero.
 * @param gameBoard  El tablero de juego.
 * @param actPlayer  El jugador.
 */
double checkBoomEffects(const Environment &gameBoard, int actPlayer)
{
   int boomAct = 7;

   double accumValue = 0;

   Environment newGame = gameBoard.GenerateNextMove(boomAct);

   if (newGame.JuegoTerminado())
   {
      if (newGame.RevisarTablero() == actPlayer)
      {
         accumValue = 100;
      }
      else
      if (newGame.RevisarTablero() != 0)
      {
         accumValue = -100;
      }
   }
   return accumValue;
}

/**
 * @brief Revisar si las fichas están conectadas entre sí, ya sea horizontal, vertical, o diagonalmente.
 * @param gameBoard  El tablero.
 * @param actToken   Vector que contiene las fichas del jugador.
 * @param actTokenMap Map que posee las fichas del jugador, utilizado para encontrarlas.
 * @param actEnemyMap Map que posee las fichas enemigas para encontrarlas.
 * @returns Valoración heurística
 */
double checkConnectedPieces(const Environment &gameBoard, int actPlayer, vector<token> actToken, map< pair<int, int>, token, compareTokens> actTokenMap, map< pair<int, int>, token, compareTokens> actEnemyMap)
{
   pair<int, int> locator;
   bool contiguous = false;
   map<pair<int, int>, token>::iterator tokenIter, enemyIter;
   double accumValue = 0;
   int ii, jj, foundCounter;

   for (int i = 0; i < actToken.size(); i++)
   {
      accumValue += actToken[i].score;
      ii = actToken[i].i;
      jj = actToken[i].j;

      contiguous = true;
      foundCounter = 0;
      locator.first = ii;
      for (int j = 0; j < 3 && jj + j + 1 < 7; j++)
      {
         locator.second = jj + j + 1;
         tokenIter = actTokenMap.find(locator);

         if (tokenIter != actTokenMap.end())
         {

            accumValue += tokenIter->second.score;
            if (contiguous)
            {
               accumValue += 10;
            }
            contiguous = true;
         }
         else
         {
            contiguous = false;
            enemyIter = actEnemyMap.find(locator);
            if (enemyIter != actEnemyMap.end())
            {

               break;
            }
         }
      }
      contiguous = true;
      locator.second = jj;
      for (int j = 0; j < 3 && ii + j + 1 < 7; j++)
      {
         locator.first = ii + j + 1;
         tokenIter = actTokenMap.find(locator);

         if (tokenIter != actTokenMap.end())
         {
            accumValue += tokenIter->second.score;
            if (contiguous)
            {
               accumValue += 10;
            }
            contiguous = true;
         }
         else
         {
            contiguous = false;
            enemyIter = actEnemyMap.find(locator);
            if (enemyIter != actEnemyMap.end())
            {

               break;
            }
         }
      }

      contiguous = true;
      locator.first = ii;
      locator.second = jj;
      for (int j = 0; j < 3 && ii + j + 1 < 7 && jj + j + 1 < 7; j++)
      {
         locator.first = ii + j + 1;
         locator.second = jj + j + 1;
         tokenIter = actTokenMap.find(locator);

         if (tokenIter != actTokenMap.end())
         {

            accumValue += tokenIter->second.score;
            if (contiguous)
            {
               accumValue += 10;
            }
            contiguous = true;
         }
         else
         {
            contiguous = false;
            enemyIter = actEnemyMap.find(locator);
            if (enemyIter != actEnemyMap.end())
            {

               break;
            }
         }
      }

      contiguous = true;
      locator.first = ii;
      locator.second = jj;
      for (int j = 0; j < 3 && ii - j - 1 >= 0 && jj + j + 1 < 7; j++)
      {
         locator.first = ii - j - 1;
         locator.second = jj + j + 1;
         tokenIter = actTokenMap.find(locator);

         if (tokenIter != actTokenMap.end())
         {

            accumValue += tokenIter->second.score;
            if (contiguous)
            {
               accumValue += 10;
            }
            contiguous = true;
         }
         else
         {
            contiguous = false;
            enemyIter = actEnemyMap.find(locator);
            if (enemyIter != actEnemyMap.end())
            {
               break;
            }
         }
      }

   }

   return accumValue;
}

/**
 * @brief Realiza el análisis del tablero
 * @param gameBoard El tablero.
 * @param player    El jugador que se está valorando.
 */
double checkGrid(const Environment &gameBoard, int player)
{

   int enemy;

   bool bombTokenFound = false;

   double playerSum = 0, enemySum = 0;

   token auxToken, playerBoom;

   pair<int, int> auxPair;

   map<pair<int, int>, token, compareTokens> playerTokensMap, enemyTokensMap;

   vector<token> playerTokens, enemyTokens;

   if (player == 1)
      enemy = 2;
   else
      enemy = 1;
   
   for (int i = 0; i < 7; i++)
   {
      for (int j = 0; j < 7; j++)
      {
         if (gameBoard.See_Casilla(i,j) != 0)
         {
            auxToken.i = i;
            auxPair.first = i;
            auxToken.j = j;
            auxPair.second = j;
            auxToken.score = baseGrid[i][j];


            if (gameBoard.See_Casilla(i,j) == (char)player || gameBoard.See_Casilla(i,j) == (char)(player+3))
            {
               playerTokensMap.insert(pair<pair<int,int>, token>(auxPair, auxToken));
               playerTokens.push_back(auxToken);
               
               if (gameBoard.See_Casilla(i,j) == (char)(player+3))
               {
                  playerBoom = auxToken;
               }
            }
            else
            {
               enemyTokensMap.insert(pair<pair<int,int>, token>(auxPair, auxToken));
               enemyTokens.push_back(auxToken);
            }  
         }
      }
   }

   playerSum = checkConnectedPieces(gameBoard, player, playerTokens, playerTokensMap, enemyTokensMap);
   enemySum = checkConnectedPieces(gameBoard, enemy, enemyTokens, enemyTokensMap, playerTokensMap);
   
   if (gameBoard.Have_BOOM(player))
   {
      playerSum += checkBoomEffects(gameBoard, player);
   }

   return playerSum - enemySum;
}


// Funcion heuristica (ESTA ES LA QUE TENEIS QUE MODIFICAR)
/**
 * @brief Función heurística que valora el estado del tablero.
 * @param estado  El estado del tablero a valorar.
 * @param jugador Para quién se está realizando la valoración.
 * @return Valor heurístico, mientras más positivo, mejor para el jugador, mientras más negativo, peor.
 */
double Valoracion(const Environment &estado, int jugador)
{
   double valorH = 0;
   // Si el juego ha terminado, revisar quién ganó.
   if (estado.JuegoTerminado())
   {
      int winner = estado.RevisarTablero();
      if (winner == jugador)
      {
         valorH = 9999;
      }
      else
      if (jugador != 0)
      {
         valorH = -9999;
      }
   }
   // Realizar una valoración del tablero.
   valorH += checkGrid(estado, jugador);

   return valorH;
}


// Esta funcion no se puede usar en la version entregable
// Aparece aqui solo para ILUSTRAR el comportamiento del juego
// ESTO NO IMPLEMENTA NI MINIMAX, NI PODA ALFABETA
void JuegoAleatorio(bool aplicables[], int opciones[], int &j){
    j=0;
    for (int i=0; i<8; i++){
        if (aplicables[i]){
           opciones[j]=i;
           j++;
        }
    }
}

/**
 * @brief Implementa el Algoritmo Alpha-Beta para el problema del Conecta-4 BOOM, retorna la mejor acción para la jugada actual.
 * @param actGame    Tablero de juego actual.
 * @param actPlayer  Jugador actual.
 * @param bestAction Devuelve la mejor acción. (Por Referencia)
 * @param actDepth   Profundidad de la recursión actual.
 * @param maxDepth   Profundidad máxima de la recursión.
 * @param alpha      Valor actual de alpha, cota para MAX.
 * @param beta       Valor actual de beta, cota para MIN.
 * @param MAX        Indica que jugador es MAX.
 * @returns El valor alpha o beta del nodo hijo a padre.
 */
double alphaBetaPruning(Environment actGame, int actPlayer, Environment::ActionType &bestAction, int actDepth, int maxDepth, double alpha, double beta, int MAX)
{
   // Caso base: Se ha llegado al máximo de profundidad o bien el juego ha terminado.
   if (actDepth == maxDepth || actGame.JuegoTerminado())
   {
      // El nodo hoja retorna su valoración heurística.
      return Valoracion(actGame, MAX);
   }
   else
   {
      int nextAction = -1,
          localAction = -1;
      bool possible[8];
      double actValue;
      Environment nextGame;

      // Obtener las acciones posibles desde la configuración actual del tablero.
      actGame.possible_actions(possible);

      // Mientras hayan acciones posibles y alpha sea menor que beta...
      while (nextAction < 8 && alpha < beta)
      {
         // Generar una jugada.
         nextGame = actGame.GenerateNextMove(nextAction);

         // Si es posible, y la acción es menor que 8; realizarla.
         if (possible[nextAction] && nextAction < 8)
         {
            actValue = alphaBetaPruning(nextGame, nextGame.JugadorActivo(), bestAction, actDepth + 1, maxDepth, alpha, beta, MAX);

            // Si el valor actual es de MAX y es mejor que el alfa de este nodo, actualizarlo y guardar la acción.
            if (actPlayer == MAX && actValue > alpha)
            {               
               localAction = nextAction;
               alpha = actValue;
            }
            
            // Si no es MAX, y es menor que beta, actualizar beta.
            if (actPlayer != MAX && actValue < beta)
            {
               beta = actValue;
            }

         }
      }

      // Una vez se termina el bucle o sucede una poda se guarda la mejor acción para pasarla por referencia.
      bestAction = static_cast< Environment::ActionType > (localAction);

      // Dependiendo si es MAX o no, devolver alfa o beta.
      if (actGame.JugadorActivo() == MAX)
      {
         return alpha;
      }
      else
      {
         return beta;
      }
   }
}

// Invoca el siguiente movimiento del jugador
Environment::ActionType Player::Think(){
   const int PROFUNDIDAD_MINIMAX = 6;  // Umbral maximo de profundidad para el metodo MiniMax
   const int PROFUNDIDAD_ALFABETA = 8; // Umbral maximo de profundidad para la poda Alfa_Beta

   Environment::ActionType accion; // acci�n que se va a devolver
   bool aplicables[8]; // Vector bool usado para obtener las acciones que son aplicables en el estado actual. La interpretacion es
                     // aplicables[0]==true si PUT1 es aplicable
                     // aplicables[1]==true si PUT2 es aplicable
                     // aplicables[2]==true si PUT3 es aplicable
                     // aplicables[3]==true si PUT4 es aplicable
                     // aplicables[4]==true si PUT5 es aplicable
                     // aplicables[5]==true si PUT6 es aplicable
                     // aplicables[6]==true si PUT7 es aplicable
                     // aplicables[7]==true si BOOM es aplicable



   double valor; // Almacena el valor con el que se etiqueta el estado tras el proceso de busqueda.
   double alpha, beta; // Cotas de la poda AlfaBeta

   int n_act; //Acciones posibles en el estado actual


   n_act = actual_.possible_actions(aplicables); // Obtengo las acciones aplicables al estado actual en "aplicables"
   int opciones[10];

   // Muestra por la consola las acciones aplicable para el jugador activo
   //actual_.PintaTablero();
   cout << " Acciones aplicables ";
   (jugador_==1) ? cout << "Verde: " : cout << "Azul: ";
   for (int t=0; t<8; t++)
   if (aplicables[t])
      cout << " " << actual_.ActionStr( static_cast< Environment::ActionType > (t)  );
   cout << endl;


   //--------------------- COMENTAR Desde aqui
/*   cout << "\n\t";
   int n_opciones=0;
   JuegoAleatorio(aplicables, opciones, n_opciones);

   if (n_act==0){
   (jugador_==1) ? cout << "Verde: " : cout << "Azul: ";
   cout << " No puede realizar ninguna accion!!!\n";
   //accion = Environment::actIDLE;
   }
   else if (n_act==1){
         (jugador_==1) ? cout << "Verde: " : cout << "Azul: ";
         cout << " Solo se puede realizar la accion "
               << actual_.ActionStr( static_cast< Environment::ActionType > (opciones[0])  ) << endl;
         accion = static_cast< Environment::ActionType >opciones[0]);

   }
   else { // Hay que elegir entre varias posibles acciones
         int aleatorio = rand()%n_opciones;
         cout << " -> " << actual_.ActionStr( static_cast< Environment::ActionType > (opciones[aleatorio])  ) << endl;
         accion = static_cast< Environment::ActionType > (opciones[aleatorio]);
   }
*/
   //--------------------- COMENTAR Hasta aqui

   //--------------------- AQUI EMPIEZA LA PARTE A REALIZAR POR EL ALUMNO ------------------------------------------------

   // Opcion: Poda AlfaBeta
   // NOTA: La parametrizacion es solo orientativa
   // valor = Poda_AlfaBeta(actual_, jugador_, 0, PROFUNDIDAD_ALFABETA, accion, alpha, beta);
   //cout << "Valor MiniMax: " << valor << "  Accion: " << actual_.ActionStr(accion) << endl;
   alpha = menosinf;
   beta = masinf;
   valor = alphaBetaPruning(actual_, jugador_, accion, 0, PROFUNDIDAD_ALFABETA, alpha, beta, jugador_);
   cout << "Valor MiniMax: " << valor << "  Accion: " << actual_.ActionStr(accion) << endl;
   return accion;
}

