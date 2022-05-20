#include "chessTournament.h"

#include "chessGame.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// ------------------ DEFINES ---------------- //

struct chess_tournament_t {
    unsigned int id;
    unsigned int winners_id; // NOTE: players_id > 0, therefore (winners_id = 0) means tournament unfinished.
    unsigned int max_games_per_player;
    char* location;
    Map games;               // <(int)id, <(Game) game>

    int num_of_players;      // number of players ever participated in tournament
    double average_game_time;
    unsigned int longest_game_time;
};

// ------------------ FUNCTIONS DECLARATIONS ---------------- //

static void freeTournament(MapDataElement tournament);
static MapDataElement copyTournament(MapDataElement tournament);
// the next functions are implemented in utils.h
void freeSimple(MapKeyElement key);
MapKeyElement copyInt(MapKeyElement key);               
int compare(MapKeyElement key1, MapKeyElement key2);

static void (*freeTournamentKey)(MapDataElement) = freeSimple;
static void (*freeTournamentData)(MapDataElement) = freeTournament;
static MapKeyElement (*copyTournamentKey)(MapKeyElement) = copyInt;
static int (*compareTournamentKeys)(MapKeyElement, MapKeyElement) = compare;
static MapDataElement (*copyTournamentData)(MapDataElement) = copyTournament;

// ------------------ FUNCTIONS IMPLEMENTATION ---------------- //

Map tournamentCreateMap()
{
    return mapCreate(copyTournamentData, copyTournamentKey, freeTournamentData, freeTournamentKey, compareTournamentKeys);
}

bool tournamentAddToMap(Map map, int tournament_id, int max_games_per_player, const char* location)
{
    Tournament tournament = (Tournament)malloc(sizeof(*tournament));
    if (tournament == NULL)
    {
        return false;
    }

    tournament->games = gameCreateMap();
    if (tournament->games == NULL)
    {
        free(tournament);
        return false;
    }

    tournament->location = (char*)malloc(strlen(location) + 1);
    if (tournament->location == NULL)
    {
        mapDestroy(tournament->games);
        free(tournament);
        return false;
    }
    strcpy(tournament->location, location);

    tournament->id = tournament_id;
    tournament->max_games_per_player = max_games_per_player;
    tournament->winners_id = 0; // NOTE: players_id > 0, therefore (winners_id = 0) means tournament unfinished.

    // our additional data
    tournament->average_game_time = 0.0;
    tournament->longest_game_time = 0;
    tournament->num_of_players = 0;
    if (mapPut(map, &tournament_id, tournament) != MAP_SUCCESS)
    {
        freeTournament(tournament);
        return false;
    }

    freeTournament(tournament);

    return true;
}

int tournamentGetNumOfGames(Tournament tournament)
{
    return mapGetSize(tournament->games);
}

int tournamentGetMaxGamesPerPlayer(Tournament tournament)
{
    return tournament->max_games_per_player;
}

bool tournamentAddGame(Tournament tournament, Player first_player,
                        Player second_player, int winners_id, int play_time)
{
    int game_id = tournamentGetNumOfGames(tournament) + 1;
    int player1_id = playerGetID(first_player);
    int player2_id = playerGetID(second_player);
    if (!gameAddToMap(tournament->games, game_id, play_time, player1_id, player2_id, winners_id))
    {
        return false;
    }

    tournament->average_game_time = (tournament->average_game_time * (game_id - 1) + play_time) / (game_id);
    if (play_time > tournament->longest_game_time)
    {
        tournament->longest_game_time = play_time;
    }

    if (!playerPlayedInTournament(first_player, tournament->id))
    {
        tournament->num_of_players++;
    }
    if (!playerPlayedInTournament(second_player, tournament->id))
    {
        tournament->num_of_players++;
    }

    return true;
}

void tournamentEnd(Tournament tournament, Map players)
{
    int max_score = 0;
    int min_loses = INT_MAX;
    int max_wins = 0;
    MAP_FOREACH(int*, game_id, tournament->games)
    {
        Game game = mapGet(tournament->games, game_id);
        int player1_id = gameGetPlayer1ID(game);
        int player2_id = gameGetPlayer2ID(game);
        Player player1 = mapGet(players, &player1_id);
        Player player2 = mapGet(players, &player2_id);
        int compare_result;

        // search for highest score
        compare_result = playerCompareScores(player1, player2, tournament->id, &max_score, &min_loses, &max_wins);
        if (compare_result >= 0)
        {
            tournament->winners_id = compare_result ? compare_result : tournament->winners_id;
            free(game_id);
            continue;
        }
        // else compare_result == -1, search for minimum loses

        compare_result = playerCompareLoses(player1, player2, tournament->id, &min_loses, &max_wins);
        if (compare_result >= 0)
        {
            tournament->winners_id = compare_result ? compare_result : tournament->winners_id;
            free(game_id);
            continue;
        }
        // else compare_result == -1, search for maximux wins

        compare_result = playerCompareWins(player1, player2, tournament->id, &max_wins);
        if (compare_result >= 0)
        {
            tournament->winners_id = compare_result ? compare_result : tournament->winners_id;
            free(game_id);
            continue;
        }
        // else compare_result == -1, search for lowest id

        if (player2 != NULL && player1_id < tournament->winners_id)
        {
            tournament->winners_id = player1_id;
        }
        if (player2 != NULL && player2_id < tournament->winners_id)
        {
            tournament->winners_id = player2_id;
        }

        free(game_id);
    }
}

bool tournamentHasEnded(Tournament tournament)
{
    return (tournament->winners_id != 0);
}

bool tournamentGameExists(Tournament tournament, int player1_id, int player2_id)
{
    return gameExists(tournament->games, player1_id, player2_id);
}

bool tournamentPrintStatistics(Tournament tournament, FILE* stream, Map players)
{
    return (fprintf(stream, "%d\n%d\n%.2lf\n%s\n%d\n%d\n",
                            tournament->winners_id,
                            tournament->longest_game_time,
                            tournament->average_game_time,
                            tournament->location,
                            tournamentGetNumOfGames(tournament),
                            tournament->num_of_players)
            >= 0);
}

void tournamentRemovePlayer(Tournament tournament, Player player, Map players)
{
    MAP_FOREACH(int*, game_id, tournament->games)
    {
        Game game = mapGet(tournament->games, game_id);
        int player_id = playerGetID(player);
        if (gameHasPlayer(game, player_id))
        {
            int other_player_id = (player_id == gameGetPlayer1ID(game) ? gameGetPlayer2ID(game) : gameGetPlayer1ID(game)); 
            Player player2 = (Player)mapGet(players, &other_player_id);
            gameRemovePlayer(game, player, player2, tournament->id);
        }
        free(game_id);
    }
}

void tournamentRemoveGame(Tournament tournament, int first_player, int second_player)
{
    int key = gameExists(tournament->games, first_player, second_player);
    mapRemove(tournament->games, &key);
}

void tournamentUpdateStatisticsBeforeRemove(Tournament tournament, Map players)
{
    MAP_FOREACH(int*, game_id, tournament->games)
    {
        Game game = mapGet(tournament->games, game_id);
        int player1_id = gameGetPlayer1ID(game);
        int player2_id = gameGetPlayer2ID(game);
        Player player1 = mapGet(players, &player1_id);
        Player player2 = mapGet(players, &player2_id);
        gameRemove(game, player1, player2, tournament->id);
        free(game_id);
    }
}

// ------------------ STRUCT FUNCTIONS IMPLEMENTATION ---------------- //

static MapDataElement copyTournament(MapDataElement tournament)
{
    if (tournament == NULL)
    {
        return NULL;
    }
    
    Tournament new_tournament = (Tournament)malloc(sizeof(*new_tournament));
    if (new_tournament == NULL)
    {
        return NULL;
    }
    Map new_games = mapCopy(((Tournament)tournament)->games);
    if (new_games == NULL)
    {
        free(new_tournament);
        return NULL;
    }

    new_tournament->location = (char*)malloc(strlen(((Tournament)tournament)->location) + 1);
    if (new_tournament->location == NULL)
    {
        mapDestroy(new_games);
        return NULL;
    }
    strcpy(new_tournament->location, ((Tournament)tournament)->location);

    new_tournament->games = new_games;
    new_tournament->id = ((Tournament)tournament)->id;
    new_tournament->winners_id = ((Tournament)tournament)->winners_id;
    new_tournament->max_games_per_player= ((Tournament)tournament)->max_games_per_player;

    new_tournament->average_game_time = ((Tournament)tournament)->average_game_time;
    new_tournament->longest_game_time = ((Tournament)tournament)->longest_game_time;
    new_tournament->num_of_players = ((Tournament)tournament)->num_of_players;

    return (MapDataElement)new_tournament;
}

static void freeTournament(MapDataElement tournament)
{
    mapDestroy(((Tournament)tournament)->games);
    free(((Tournament)tournament)->location);
    free(tournament);
}