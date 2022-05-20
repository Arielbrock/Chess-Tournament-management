#include "chessPlayer.h"

#include <stdlib.h>

// ------------------ STRUCT FUNCTIONS ---------------- //

static void freePlayer(MapDataElement player);
static MapDataElement copyPlayer(MapDataElement player);
// the next functions are implemented in utils.h
void freeSimple(MapKeyElement key);
MapKeyElement copyInt(MapKeyElement key);
int compare(MapKeyElement key1, MapKeyElement key2);

static void (*freePlayerKey)(MapDataElement) = freeSimple;
static void (*freePlayerData)(MapDataElement) = freePlayer;
static MapKeyElement (*copyPlayerKey)(MapKeyElement) = copyInt;
static MapDataElement (*copyPlayerData)(MapDataElement) = copyPlayer;
static int (*comparePlayerKeys)(MapKeyElement, MapKeyElement) = compare;

// ------------------ DEFINES ---------------- //

struct chess_player_t {
    unsigned int id;
    unsigned int num_of_wins;
    unsigned int num_of_loses;
    unsigned int num_of_draws;
    Map score_per_tournament; // <(int) tournament_id, (int)score>
    Map games_per_tournament; // <(int) tournament_id, (int)num_of_games>
    unsigned int total_time;
};

#define VALUE_WIN 6
#define VALUE_LOSE -10
#define VALUE_DRAW 2

#define POINTS_WIN 1
#define POINTS_DRAW 2

// ------------------ FUNCTIONS DECLARATIONS ---------------- //

static int playerGetTotalGames(Player player);
static int playerGetScore(Player player, int tournament_id);

// ------------------ FUNCTIONS IMPLEMENTATIONS ---------------- //

Map playerCreateMap()
{
    return mapCreate(copyPlayerData, copyPlayerKey, freePlayerData, freePlayerKey, comparePlayerKeys);
}

bool playerAddToMap(Map map, int player_id)
{
    Player player = (Player)malloc(sizeof(*player));
    if (player == NULL)
    {
        return false;
    }

    if ((player->score_per_tournament = 
	    mapCreate(copyPlayerKey, copyPlayerKey, freePlayerKey, freePlayerKey, comparePlayerKeys)) == NULL)
    {
        free(player);
        return false;
    }
    if ((player->games_per_tournament = 
	    mapCreate(copyPlayerKey, copyPlayerKey, freePlayerKey, freePlayerKey, comparePlayerKeys)) == NULL)
    {
        mapDestroy(player->score_per_tournament);
        free(player);
        return false;
    }
    player->id = player_id;
    player->num_of_draws = 0;
    player->num_of_loses = 0;
    player->num_of_wins = 0;
    player->total_time = 0;

    if (mapPut(map, &player_id, player) != MAP_SUCCESS)
    {
        freePlayer(player);
        return false;
    }

    freePlayer(player);
    return true;
}

bool playerUpdate(Player player, int player_id, int tournament_id, PlayerStatus status, int play_time)
{
    int* score = mapGet(player->score_per_tournament, &tournament_id);
    int* games = mapGet(player->games_per_tournament, &tournament_id);
    if (score == NULL)
    {
        int new_score = 0;
        if (mapPut(player->score_per_tournament, &tournament_id, &new_score) != MAP_SUCCESS)
        {
            return false;
        }
        score = mapGet(player->score_per_tournament, &tournament_id);
    }
    if (games == NULL)
    {
        int new_games = 0;
        if (mapPut(player->games_per_tournament, &tournament_id, &new_games) != MAP_SUCCESS)
        {
            return false;
        }
        games = mapGet(player->games_per_tournament, &tournament_id);
    }

    switch (status)
    {
        case PLAYER_WINNER: 
            player->num_of_wins++;
            *score += POINTS_WIN;
            break;
        case PLAYER_LOSER:
            player->num_of_loses++;
            // no need to update score
            break;
        case PLAYER_DRAW:
            player->num_of_draws++;
            *score += POINTS_DRAW;
            break;
    }   
    *games += 1;
    player->total_time += play_time;
    
    return true;
}

void playerDowndate(Player player, int player_id, int tournament_id, PlayerStatus status, int play_time)
{
    int* score = mapGet(player->score_per_tournament, &tournament_id);
    int* games = mapGet(player->games_per_tournament, &tournament_id);

    switch (status)
    {
        case PLAYER_WINNER: 
            player->num_of_wins--;
            *score -= POINTS_WIN;
            break;
        case PLAYER_LOSER:
            player->num_of_loses--;
            // no need to update score
            break;
        case PLAYER_DRAW:
            player->num_of_draws--;
            *score -= POINTS_DRAW;
            break;
    }
    *games -= 1;
    player->total_time -= play_time;
}

bool playerExists(Player player)
{
    return (player != NULL && playerGetTotalGames(player));
}

int playerGetID(Player player)
{
    if (player == NULL)
    {
        return 0;
    }
    return player->id;
}

static int playerGetTotalGames(Player player)
{
    return player->num_of_wins + player->num_of_loses + player->num_of_draws;
}

double playerGetLevel(Player player)
{
    if (!playerExists(player))
    {
        return 0.0;
    }
    int num_of_games = playerGetTotalGames(player);
    int wins = player->num_of_wins;
    int lose = player->num_of_loses;
    int draw = player->num_of_draws;
    double level = (VALUE_WIN * wins + VALUE_LOSE * lose + VALUE_DRAW * draw) / (double)num_of_games;
    return level;
}

double playerGetAveragePlayTime(Player player)
{
    return player->total_time / (double)playerGetTotalGames(player);
}

int playerCompareScores(Player player1, Player player2, int tournament_id,
                        int* max_score, int* min_loses, int* max_wins)
{
    int player1_score = player1 == NULL ? 0 : playerGetScore(player1, tournament_id);
    int player2_score = player2 == NULL ? 0 : playerGetScore(player2, tournament_id);
    if (player1 != NULL && player1_score > *max_score)
    {
        *max_score = player1_score;
        *min_loses = player1->num_of_loses;
        *max_wins = player1->num_of_wins;
        return player1->id;
    }
    if (player2 != NULL && player2_score > *max_score)
    {
        *max_score = player2_score;
        *min_loses = player2->num_of_loses;
        *max_wins = player2->num_of_wins;
        return player2->id;
    }
    return ((player1 != NULL && player2 != NULL &&
             player1_score == player2_score && player1_score == *max_score) ? -1 : 0);
}

static int playerGetScore(Player player, int tournament_id)
{
    return *(int*)mapGet(player->score_per_tournament, &tournament_id);
}

int playerCompareLoses(Player player1, Player player2, int tournament_id, int* min_loses, int* max_wins)
{
    if (player1 != NULL && player1->num_of_loses < *min_loses)
    {
        *min_loses = player1->num_of_loses;
        *max_wins = player1->num_of_wins;
        return player1->id;
    }
    if (player2 != NULL && player2->num_of_loses < *min_loses)
    {
        *min_loses = player2->num_of_loses;
        *max_wins = player2->num_of_wins;
        return player2->id;
    }
    return ((player1 != NULL && player2 != NULL &&
            player1->num_of_loses == player2->num_of_loses && player1->num_of_loses == *min_loses) ? -1 : 0);
}

int playerCompareWins(Player player1, Player player2, int tournament_id, int* max_wins)
{
    if (player1 != NULL && player1->num_of_wins > *max_wins)
    {
        *max_wins = player1->num_of_wins;
        return player1->id;
    }
    if (player2 != NULL && player2->num_of_wins > *max_wins)
    {
        *max_wins = player2->num_of_wins;
        return player2->id;
    }
    return ((player1 != NULL && player2 != NULL && 
            player1->num_of_wins == player2->num_of_wins && player2->num_of_wins == *max_wins) ? -1 : 0);
}

void playerSwitchLoseToVictory(Player player, int tournament_id)
{
    player->num_of_loses--;
    player->num_of_wins++;
    int* score = (int*)mapGet(player->score_per_tournament, (MapKeyElement)(&tournament_id));
    *score += POINTS_WIN;
}

void playerSwitchDrawToVictory(Player player, int tournament_id)
{
    player->num_of_draws--;
    player->num_of_wins++;
    int* score = (int*)mapGet(player->score_per_tournament, (MapKeyElement)(&tournament_id));
    *score += POINTS_DRAW;
}

void playerRemoveFromGame(Player player, PlayerStatus status, int game_length, int tournament_id)
{
    if (player == NULL)
    {
        return;
    }
    switch (status)
    {
        case PLAYER_WINNER:
            player->num_of_wins--;
            break;
        case PLAYER_LOSER:
            player->num_of_loses--;
            break;
        case PLAYER_DRAW:
            player->num_of_draws--;
    }

    player->total_time -= game_length;

    mapRemove(player->score_per_tournament, &tournament_id);
    mapRemove(player->games_per_tournament, &tournament_id);
}

bool playerPlayedInTournament(Player player, int tournament_id)
{
    return mapGet(player->games_per_tournament, &tournament_id) != NULL;
}

int playerGetGamesInTournament(Player player, int tournament_id)
{
    int* games = mapGet(player->games_per_tournament, &tournament_id);
    if (games == NULL)
    {
        return 0;
    }
    return *games;
}

void playerResetStatistics(Player player)
{
    player->num_of_draws = 0;
    player->num_of_loses = 0;
    player->num_of_wins = 0;
    player->total_time = 0;
    mapClear(player->games_per_tournament);
    mapClear(player->score_per_tournament);
}

// ------------------ POINTER FUNCTIONS IMPLEMENTATIONS ---------------- //

static MapDataElement copyPlayer(MapDataElement player)
{
    if(player == NULL)
    {
        return NULL;
    }
    Player new_player = (Player)malloc(sizeof(*new_player));
    if(new_player == NULL)
    {
        return NULL;
    }

    new_player->id              = ((Player)player)->id;
    new_player->num_of_draws    = ((Player)player)->num_of_draws;
    new_player->num_of_wins     = ((Player)player)->num_of_wins;
    new_player->num_of_loses    = ((Player)player)->num_of_loses;
    new_player->total_time      = ((Player)player)->total_time;
    new_player->score_per_tournament = mapCopy(((Player)player)->score_per_tournament);
    if (new_player->score_per_tournament == NULL)
    {
	free(new_player);
        return NULL;
    }
    new_player->games_per_tournament = mapCopy(((Player)player)->games_per_tournament);
    if (new_player->games_per_tournament == NULL)
    {
	mapDestroy(new_player->score_per_tournament);
        free(new_player);
        return NULL;
    }

    return (MapDataElement)new_player;
}

static void freePlayer(MapDataElement player)
{
    mapDestroy(((Player)player)->score_per_tournament);
    mapDestroy(((Player)player)->games_per_tournament);
    free(player);
}