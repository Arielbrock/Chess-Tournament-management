#ifndef _CHESSTOURNAMENT_H_
#define _CHESSTOURNAMENT_H_

#include "chessPlayer.h"
#include <stdio.h>

typedef struct chess_tournament_t *Tournament;

/**
 * Create and return an empty map of tournaments
 * */
Map tournamentCreateMap(void);

/**
 * Create a new tournament and add it to a map.
 * Return false if an error occured (can only happen if malloc fails).
 * */
bool tournamentAddToMap(Map map, int tournament_id, int max_games_per_player, const char* location);

/**
 * Add a new game to a tournament.
 * Return false if an error occured (can only happen if malloc fails)
 * */
bool tournamentAddGame(Tournament tournament, Player first_player, Player second_player, int winners_id, int play_time);

/**
 * Calculate the winner of a tournament and save the result.
 * */
void tournamentEnd(Tournament tournament, Map players);

/**
 * Update the players' statistics when removing a tournament.
 * */
void tournamentUpdateStatisticsBeforeRemove(Tournament tournament, Map players);

// Getters

int tournamentGetNumOfGames(Tournament tournament);
int tournamentGetMaxGamesPerPlayer(Tournament tournament);

// Functions whose names' explain their purposes

bool tournamentHasEnded(Tournament tournament);
bool tournamentGameExists(Tournament tournament, int player1_id, int player2_id);
bool tournamentPrintStatistics(Tournament tournament, FILE* stream, Map players);
void tournamentRemovePlayer(Tournament tournament, Player player, Map players);
void tournamentRemoveGame(Tournament tournament, int first_player, int second_player);

#endif
