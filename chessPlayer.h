#ifndef _CHESSPLAYER_H_
#define _CHESSPLAYER_H_

#include "map.h"

typedef struct chess_player_t *Player;

typedef enum chess_player_state_t {
    PLAYER_WINNER,
    PLAYER_LOSER,
    PLAYER_DRAW
} PlayerStatus;

/**
 * Create a map of players
 * */
Map playerCreateMap(void);

/**
 * Create a new player.
 * Add that player to the required map.
 * Return false if an error occured (malloc failed), otherwise return true.
 * */
bool playerAddToMap(Map map, int player_id);

/**
 * Update the statistics of a player when adding a new game.
 * Return false if an error occured (malloc failed), otherwise return true.
 * */
bool playerUpdate(Player player, int player_id, int tournament_id, PlayerStatus status, int play_time);

/**
 * Update the statistics of a player when removing a game. 
 * Only happens if we tried to add a new game, player1Update went successful, but player2Update failed,
 * so we have to "downdate" player1.
 * */
void playerDowndate(Player player, int player_id, int tournament_id, PlayerStatus status, int play_time);

// Functions that return general information about the player

int playerGetID(Player player);
bool playerExists(Player player);
double playerGetLevel(Player player);
double playerGetAveragePlayTime(Player player);
bool playerPlayedInTournament(Player player, int tournament_id);
int playerGetGamesInTournament(Player player, int tournament_id);

// Functions that serve tournamentEnd

int playerCompareScores(Player player1, Player player2, int tournament_id, int* max_score, int* min_loses, int* max_wins);
int playerCompareLoses(Player player1, Player player2, int tournament_id, int* min_loses, int* max_wins);
int playerCompareWins(Player player1, Player player2, int tournament_id, int* max_wins);

/**
 * Update player's statistics after removing a game from system.
 * Game removal only happens when a tournament is removed,
 * so the function also deletes the tournament from player->score_per_tournament map
 * */
void playerRemoveFromGame(Player player, PlayerStatus status, int game_length, int tournament_id);

// Functions for score recalculation when a player is removed

void playerSwitchLoseToVictory(Player player, int tournament_id);
void playerSwitchDrawToVictory(Player player, int tournament_id);

/**
 * Reset the player's statistics to 0.
 * Is done instead of removing it from the system as a requirement of ex1-version3,
 * in order to track players that once played but were removed from the system.
 * NOTE: after playerResetStatistics is called, playerExists will return FALSE.
 * */
void playerResetStatistics(Player player);

#endif