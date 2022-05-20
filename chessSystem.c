#include "chessSystem.h"

#include "chessTournament.h"
#include "chessPlayer.h"
#include "chessGame.h"
#include "utils.h"
#include "map.h"
#include <stdlib.h>
#include <string.h>

// ------------------ DEFINES ---------------- //

#define FAULT_AVERAGE_TIME 0.0
#define MIN_ID_VALUE 1

struct chess_system_t {
    Map tournaments;  // <(int)id, (Tournament)tournament>
    Map players;      // <(int)id, <(Player) player>
    int num_of_games; // number of games in the system.
};

// ------------------ FUNCTIONS DECLARATIONS ---------------- //

static void freeMapFunction(MapDataElement data);
static MapDataElement copyMapFunction(MapDataElement data);
static int compareDecending(MapKeyElement key1, MapKeyElement key2);
static MapKeyElement copyDoubleKey(MapKeyElement key);

static MapKeyElement (*copyIntKey)(MapKeyElement) = copyInt;
static MapKeyElement (*copyDouKey)(MapKeyElement) = copyDoubleKey;
static MapDataElement (*copyMap)(MapDataElement) = copyMapFunction;
static void (*freeIntKey)(MapKeyElement) = freeSimple;
static void (*freeMap)(MapDataElement) = freeMapFunction;
static int (*compareIntsAscending)(MapKeyElement, MapKeyElement) = compare;
static int (*compareDoublesDecending)(MapKeyElement, MapKeyElement) = compareDecending;

static bool isLocationValid(const char* location);
static bool addPlayersToMap(Map players, Player* player1, Player* player2, int first_player, int second_player);
static bool exceededMaxGames(Map players, Player player1, Player player2, Tournament tournament, int tournament_id);
static bool updatePlayersStatistics(Map players, Player* player1, Player* player2,
                                    Tournament tournament, int tournament_id, Winner winner, int play_time);
static bool printLevelsToFile(Map levels_map, FILE* file);
static bool addPlayersWithLevelToMap(Map all_players, Map players_map, double level);
static bool fillLevelsMap(Map players, Map levels_map);
static bool printTournamentStatistics(Map tournaments, Map players, FILE* stream, int* ended_tournaments);

// ------------------ FUNCTIONS IMPLEMENTATIONS ---------------- //

ChessSystem chessCreate()
{
    ChessSystem system = (ChessSystem)malloc(sizeof(*system));
    if (system == NULL)
    {
        return NULL;
    }
    Map tournaments = tournamentCreateMap();
    if (tournaments == NULL)
    {
        free(system);
        return NULL;
    }
    Map players = playerCreateMap();
    if (players == NULL)
    {
        mapDestroy(tournaments);
        free(system);
        return NULL;
    }

    system->tournaments = tournaments;
    system->players = players;
    system->num_of_games = 0;
    return system;
}

void chessDestroy(ChessSystem system)
{
    if (system == NULL)
    {
        return;
    }
    mapDestroy(system->tournaments);
    mapDestroy(system->players);
    free(system);
}

ChessResult chessAddTournament(ChessSystem chess, int tournament_id, 
                                int max_games_per_player, const char* tournament_location)
{
    // check for errors
    if (chess == NULL || tournament_location == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    if (tournament_id < MIN_ID_VALUE)
    {
        return CHESS_INVALID_ID;
    }
    if (mapGet(chess->tournaments, &tournament_id) != NULL)
    {
        return CHESS_TOURNAMENT_ALREADY_EXISTS;
    }
    if (!isLocationValid(tournament_location))
    {
        return CHESS_INVALID_LOCATION;
    }
    if (max_games_per_player < MIN_ID_VALUE)
    {
        return CHESS_INVALID_MAX_GAMES;
    }

    // add the tournament
    if (!tournamentAddToMap(chess->tournaments, tournament_id, max_games_per_player, tournament_location))
    {
        return CHESS_OUT_OF_MEMORY;
    }

    return CHESS_SUCCESS;
}

static bool isLocationValid(const char* location)
{
    if (location == NULL || strlen(location) < 1 || location[0] > 'Z' || location[0] < 'A')
    {
        return false;
    }
    int i = 0;
    char c;
    while ((c = location[++i]) != '\0')
    {
        if (!(c == ' ' || (c >= 'a' && c <= 'z')))
        {
            return false;
        }
    }
    return true;
}

ChessResult chessAddGame(ChessSystem chess, int tournament_id, int first_player, 
                        int second_player, Winner winner, int play_time)
{
    // basic validations
    if (chess == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    if (tournament_id < MIN_ID_VALUE || first_player < MIN_ID_VALUE 
        || second_player < MIN_ID_VALUE || first_player == second_player)
    {
        return CHESS_INVALID_ID;
    }
    
    Tournament tournament = (Tournament)mapGet(chess->tournaments, &tournament_id);
    if (tournament == NULL)
    {
        return CHESS_TOURNAMENT_NOT_EXIST;
    }
    if (tournamentHasEnded(tournament))
    {
        return CHESS_TOURNAMENT_ENDED;
    }
    if (tournamentGameExists(tournament, first_player, second_player))
    {
        return CHESS_GAME_ALREADY_EXISTS;
    }
    if (play_time < 0)
    {
        return CHESS_INVALID_PLAY_TIME;
    }

    // adding the players to chess->players if needed
    Player player1 = (Player)mapGet(chess->players, &first_player);
    Player player2 = (Player)mapGet(chess->players, &second_player);;
    if (!addPlayersToMap(chess->players, &player1, &player2, first_player, second_player))
    {
        return CHESS_OUT_OF_MEMORY;
    }

    // check one more validaiton
    if (exceededMaxGames(chess->players, player1, player2, tournament, tournament_id))
    {
        return CHESS_EXCEEDED_GAMES;
    }

    // add the game itself
    int winners_id = (winner == FIRST_PLAYER ? first_player : (winner == SECOND_PLAYER ? second_player : GAME_DRAW));
    if (!tournamentAddGame(tournament, player1, player2, winners_id, play_time))
    {
        if (!playerExists(player1))
        {
            mapRemove(chess->players, &first_player);
        }
        if (!playerExists(player2))
        {
            mapRemove(chess->players, &second_player);
        }
        return CHESS_OUT_OF_MEMORY;
    }

    // update statistics for both players
    if (!updatePlayersStatistics(chess->players, &player1, &player2, tournament, tournament_id, winner, play_time))
    {
        return CHESS_OUT_OF_MEMORY;
    }

    chess->num_of_games++;

    return CHESS_SUCCESS;
}

static bool addPlayersToMap(Map players, Player* player1, Player* player2, int first_player, int second_player)
{
    if (*player1 == NULL)
    {
        if (!playerAddToMap(players, first_player))
        {
            return false;
        }
        *player1 = mapGet(players, &first_player);
    }

    if (*player2 == NULL)
    {
        if (!playerAddToMap(players, second_player))
        {
            if (!playerExists(*player1))
            {
                mapRemove(players, &first_player);
            }
            return false;
        }
        *player2 = mapGet(players, &second_player);
    }

    return true;
}

static bool exceededMaxGames(Map players, Player player1, Player player2, Tournament tournament, int tournament_id)
{
    if(player1 == NULL || player2 == NULL || players == NULL || tournament == NULL)
    {
        return false;
    }
    int first_player  = playerGetID(player1);
    int second_player = playerGetID(player2);
    if (playerGetGamesInTournament(player1, tournament_id) >= tournamentGetMaxGamesPerPlayer(tournament)
     || playerGetGamesInTournament(player2, tournament_id) >= tournamentGetMaxGamesPerPlayer(tournament)) 
    {
        if (!playerExists(player1))
        { 
            mapRemove(players, &first_player);
        }
        if (!playerExists(player2))
        { 
            mapRemove(players, &second_player);
        }
        return true;
    }

    return false;
}

static bool updatePlayersStatistics(Map players, Player* player1, Player* player2,
                                    Tournament tournament, int tournament_id, Winner winner, int play_time)
{
    int first_player = playerGetID(*player1);
    int second_player = playerGetID(*player2);

    if (!playerUpdate(*player1, first_player, tournament_id, winner, play_time))
    {
        tournamentRemoveGame(tournament, first_player, second_player);
        if (!playerExists(*player1))
        {
            mapRemove(players, &first_player);
        }
        if (!playerExists(*player2))
        {
            mapRemove(players, &second_player);
        }
        return false;
    }

    if (!playerUpdate(*player2, second_player, tournament_id, winner == DRAW ? DRAW : 1 - winner, play_time))
    {
        playerDowndate(*player1, first_player, tournament_id, winner, play_time);
        tournamentRemoveGame(tournament, first_player, second_player);
        if (!playerExists(*player1))
        {
            mapRemove(players, &first_player);
        }
        if (!playerExists(*player2))
        {
            mapRemove(players, &second_player);
        }
        return false;
    }

    return true;
}

ChessResult chessRemoveTournament(ChessSystem chess, int tournament_id)
{
    if(chess == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    if(tournament_id < 0)
    {
        return CHESS_INVALID_ID;
    }

    Tournament tournament = mapGet(chess->tournaments, &tournament_id);
    if (tournament == NULL)
    {
        return CHESS_TOURNAMENT_NOT_EXIST;
    }

    chess->num_of_games -= tournamentGetNumOfGames(tournament);
    tournamentUpdateStatisticsBeforeRemove(tournament, chess->players);

    mapRemove(chess->tournaments, &tournament_id);
    
    return CHESS_SUCCESS;
}

ChessResult chessRemovePlayer(ChessSystem chess, int player_id)
{
    if (chess == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    if (player_id < MIN_ID_VALUE)
    {
        return CHESS_INVALID_ID; 
    }

    Player player = mapGet(chess->players, &player_id);
    if (!playerExists(player))
    {
        return CHESS_PLAYER_NOT_EXIST;
    }

    // reset all statistics of that player
    // ex1-version3 says not to delete from chess->players
    playerResetStatistics(player);

    // remove the player from the games themselfs (and update statistics)
    MAP_FOREACH(int*, tournament_id, chess->tournaments)
    {
        Tournament tournament = mapGet(chess->tournaments, tournament_id);
        if (!tournamentHasEnded(tournament))
        {
            tournamentRemovePlayer(tournament, player, chess->players);
        }
        free(tournament_id);
    }

    return CHESS_SUCCESS;
}

ChessResult chessEndTournament(ChessSystem chess, int tournament_id)
{
    if (chess == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    if (tournament_id < MIN_ID_VALUE)
    {
        return CHESS_INVALID_ID;
    }
    Tournament tournament = (Tournament)mapGet(chess->tournaments, &tournament_id);
    if (tournament == NULL)
    {
        return CHESS_TOURNAMENT_NOT_EXIST;
    }
    if (tournamentHasEnded(tournament))
    {
        return CHESS_TOURNAMENT_ENDED;
    }
    if (tournamentGetNumOfGames(tournament) < 1)
    {
        return CHESS_NO_GAMES;
    }

    tournamentEnd(tournament, chess->players);

    return CHESS_SUCCESS;
}

double chessCalculateAveragePlayTime(ChessSystem chess, int player_id, ChessResult* chess_result)
{
    if(chess == NULL)
    {
        *chess_result = CHESS_NULL_ARGUMENT;
        return FAULT_AVERAGE_TIME;
    }
    if (player_id < MIN_ID_VALUE)
    {
        *chess_result = CHESS_INVALID_ID;
        return FAULT_AVERAGE_TIME;
    }
    
    // search for a player with that id, and return its average
    MAP_FOREACH(int*, id, chess->players)
    {
        Player player = mapGet(chess->players, id);
        if (!playerExists(player))
        {
            free(id);
            continue;
        }
        if(playerGetID(player) == player_id)
        {
            *chess_result = CHESS_SUCCESS;
            free(id);
            return playerGetAveragePlayTime((Player)mapGet(chess->players, &player_id));
        }
        free(id);
    }

    *chess_result = CHESS_PLAYER_NOT_EXIST;
    return FAULT_AVERAGE_TIME;
}

ChessResult chessSavePlayersLevels(ChessSystem chess, FILE* file)
{
    if(chess == NULL || file == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }

    /**
     * levels_map is a map of maps: <(double)level, (Map)all_players_with_that_level>
     * levels_map is in descending order, while all_players_with_that_level is ascending.
     * */
    Map levels_map = mapCreate(copyMap, copyDouKey, freeMap, freeIntKey, compareDoublesDecending);
    if (levels_map == NULL)
    {
        return CHESS_SAVE_FAILURE;
    }

    if (!fillLevelsMap(chess->players, levels_map))
    {
        mapDestroy(levels_map);
        return CHESS_SAVE_FAILURE;
    }


    if (!printLevelsToFile(levels_map, file))
    {
        mapDestroy(levels_map);
        return CHESS_SAVE_FAILURE;
    }

    mapDestroy(levels_map);

    return CHESS_SUCCESS;
}

static bool fillLevelsMap(Map players, Map levels_map)
{
    MAP_FOREACH(int*, player_id, players)
    {
        Player player = mapGet(players, player_id);
        double level = playerGetLevel(player);
        if (level == 0.0 || mapGet(levels_map, &level) != NULL)
        {
            free(player_id);
            continue;
        }

        /**
         * players_map (aka all_players_with_that_level) is just a list of players.
         * We don't have a LinkedList ADT, so it's just <(int)player_id, (int)player_id>
         * */
        Map players_map = mapCreate(copyIntKey, copyIntKey, freeIntKey, freeIntKey, compareIntsAscending);
        if (players_map == NULL)
        {
	        free(player_id);
            return false;
        }

        if (!addPlayersWithLevelToMap(players, players_map, level))
        {
            free(player_id);
            mapDestroy(players_map);
            return false;
        }

        if (mapPut(levels_map, &level, players_map) != MAP_SUCCESS)
        {
            free(player_id);
            mapDestroy(players_map);
            return false;
        }

        free(player_id);
        mapDestroy(players_map); 
        free(mapGetFirst(players));
    }

    return true;
}

static bool addPlayersWithLevelToMap(Map all_players, Map players_map, double level)
{
    MAP_FOREACH(int*, other_player_id, all_players)
    {
        Player other_player = mapGet(all_players, other_player_id);
        if (playerGetLevel(other_player) == level)
        {
            if (mapPut(players_map, other_player_id, other_player_id) != MAP_SUCCESS)
            {
                free(other_player_id);
                return false;
            }
        }
        free(other_player_id);
    }
    return true;
}

static bool printLevelsToFile(Map levels_map, FILE* file)
{
    MAP_FOREACH(double*, level, levels_map)
    {
        Map players_map = mapGet(levels_map, level);
        MAP_FOREACH(int*, id, players_map)
        {
            if (fprintf(file, "%d %.2lf\n", *id, *level) < 0)
            {
                free(id);
                free(level);
                return false;
            }
            free(id);
        }
        free(level);
    }
    return true;
}

ChessResult chessSaveTournamentStatistics(ChessSystem chess, char* path_file)
{
    if (chess == NULL)
    {
        return CHESS_NULL_ARGUMENT;
    }
    FILE* stream = fopen(path_file, "w+");
    if (stream == NULL)
    {
        return CHESS_SAVE_FAILURE;
    }
    int ended_tournaments = 0;
    if (!printTournamentStatistics(chess->tournaments, chess->players, stream, &ended_tournaments))
    {
        fclose(stream);
        return CHESS_SAVE_FAILURE;
    }

    if (ended_tournaments < 1)
    {
        fclose(stream);
        return CHESS_NO_TOURNAMENTS_ENDED;
    }
    fclose(stream);
    return CHESS_SUCCESS;
}

static bool printTournamentStatistics(Map tournaments, Map players, FILE* stream, int* ended_tournaments)
{
    MAP_FOREACH(int*, tournament_id, tournaments)
    {
        Tournament tournament = mapGet(tournaments, tournament_id);
        if (tournamentHasEnded(tournament))
        {
            (*ended_tournaments)++;
            
            if (!tournamentPrintStatistics(tournament, stream, players))
            {
                free(tournament_id);
                return false;
            }
        }
        free(tournament_id);
    }
    return true;
}

// ------------------ POINTER FUNCTIONS IMPLEMENTATIONS ---------------- //

static MapDataElement copyMapFunction(MapDataElement data)
{
    return mapCopy((Map)data);
}

static void freeMapFunction(MapDataElement data)
{
    mapDestroy((Map)data);
}

static int compareDecending(MapKeyElement key1, MapKeyElement key2)
{
    if (key1 == NULL && key2 == NULL) 
    {
        return 0;
    }
    if (key1 == NULL) 
    {
        return (int)(*((double*)key2));
    }
    if (key2 == NULL)
    {
        return (int)(*((double*)key1));
    }

    return (*((double*)key2) > *((double*)key1)) ? 1 : ((*((double*)key2) == *((double*)key1)) ? 0 : -1);
}

static MapKeyElement copyDoubleKey(MapKeyElement key)
{
    if (key == NULL)
    {
        return NULL;
    }
    MapKeyElement new_key = (MapKeyElement)malloc(sizeof(double));
    if (new_key == NULL)
    {
        return NULL;
    }
    *((double*)new_key) = *((double*)key);
    return new_key;
}