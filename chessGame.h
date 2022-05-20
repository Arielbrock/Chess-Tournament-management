#ifndef _CHESSGAME_H_
#define _CHESSGAME_H_

#include "chessPlayer.h"

#define GAME_DRAW 0 // winners_id if the game ended with a draw.

typedef struct chess_game_t *Game;

/**
 * Create a map of players.
 * */
Map gameCreateMap(void);

/**
 * Create a new game.
 * Add that game to the required map.
 * Return false if an error occured (malloc failed), otherwise return true.
 * */
bool gameAddToMap(Map map, int game_id, int length, int player1_id, int player2_id, int winners_id);

/**
 * Remove a player from a game.
 * Update statistics of the other player if needed.
 * */
void gameRemovePlayer(Game game, Player player, Player other_player, int tournament_id);

/**
 * Update the statistics of the players when removing a game from the system.
 * */
void gameRemove(Game game, Player player1, Player player2, int tournament_id);

/**
 * Return game's key, if such game exist on the list.
 * If game does not exist, return 0 (reminder: key > 0). 
 * NOTE: The order of the players doesn't matter.
 * */
int gameExists(Map games, int player1_id, int player2_id);

// Simple getters.

int gameGetPlayer1ID(Game game);
int gameGetPlayer2ID(Game game);
bool gameHasPlayer(Game game, int player_id);

#endif