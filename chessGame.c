#include "chessGame.h"

#include <stdlib.h>
#include <stdbool.h>

// ------------------ STRUCT FUNCTIONS ---------------- //

static MapDataElement copyGame(MapDataElement game);
// the next functions are implemented in utils.h
void freeSimple(MapDataElement game);
MapKeyElement copyInt(MapKeyElement key);
int compare(MapKeyElement key1, MapKeyElement key2);

static void (*freeGameKey)(MapDataElement) = freeSimple;
static void (*freeGameData)(MapDataElement) = freeSimple; // no mallocs in Game
static MapKeyElement (*copyGameKey)(MapKeyElement) = copyInt;
static MapDataElement (*copyGameData)(MapDataElement) = copyGame;
static int (*compareGameKeys)(MapKeyElement, MapKeyElement) = compare;

// ------------------ DEFINES ---------------- //

struct chess_game_t {
    unsigned int length;
    unsigned int player1_id;
    unsigned int player2_id;
    unsigned int winners_id; // 0 = DRAW
};

// ------------------ FUNCTIONS IMPLEMENTATION ---------------- //

Map gameCreateMap(void)
{
    return mapCreate(copyGameData, copyGameKey, freeGameData, freeGameKey, compareGameKeys);
}

int gameGetPlayer1ID(Game game)
{
    return game->player1_id;
}

int gameGetPlayer2ID(Game game)
{
    return game->player2_id;
}

bool gameAddToMap(Map map, int game_id, int length, int player1_id, int player2_id, int winners_id)
{
    Game game = (Game)malloc(sizeof(*game));
    if (game == NULL)
    {
        return false;
    }

    game->length     = length;
    game->player1_id = player1_id;
    game->player2_id = player2_id;
    game->winners_id = winners_id;

    if (mapPut(map, &game_id, game) != MAP_SUCCESS)
    {
        free(game);
        return false;
    } 

    free(game);
    return true;
}

int gameExists(Map games, int player1_id, int player2_id)
{
    MAP_FOREACH(int*, key, games)
    {
        Game game = mapGet(games, key);
        if((game->player1_id == player1_id && game->player2_id == player2_id)
        || (game->player2_id == player1_id && game->player1_id == player2_id))
        {
            int n = *key;
            free(key);
            return n;
        }
        free(key);
    }

    return 0;
}

bool gameHasPlayer(Game game, int player_id)
{
    return (player_id == game->player1_id || player_id == game->player2_id);
}

void gameRemovePlayer(Game game, Player player, Player other_player, int tournament_id)
{
    int player_to_remove = playerGetID(player);
    if (!gameHasPlayer(game, player_to_remove))
    {
        return;
    }

    int last_winner = game->winners_id;
    // remove player from game, update winner.
    if (player_to_remove == game->player1_id)
    {
        game->player1_id = 0;
        game->winners_id = game->player2_id;
    }
    else // player_to_remove == game->player2_id
    {
        game->player2_id = 0;
        game->winners_id = game->player1_id;
    }

    if (game->winners_id == 0) // both players were removed
    {
        return;
    }

    // update the other player's statistics
    if (last_winner == player_to_remove) // the winner was removed
    {
        playerSwitchLoseToVictory(other_player, tournament_id);
    }
    else if (!last_winner) // there was a draw
    {
        playerSwitchDrawToVictory(other_player, tournament_id);
    }
    // else the winner stays winner, do nothing.
}

void gameRemove(Game game, Player player1, Player player2, int tournament_id)
{
    if (!game->winners_id)
    {
        playerRemoveFromGame(player1, PLAYER_DRAW, game->length, tournament_id);
        playerRemoveFromGame(player2, PLAYER_DRAW, game->length, tournament_id);
        return;
    }
    Player winner = game->winners_id == playerGetID(player1) ? player1 : player2;
    Player loser  = game->winners_id == playerGetID(player1) ? player2 : player1; 
    playerRemoveFromGame(winner, PLAYER_WINNER, game->length, tournament_id);
    playerRemoveFromGame(loser, PLAYER_LOSER, game->length, tournament_id);
}

// ------------------ STRUCT FUNCTIONS IMPLEMENTATION ---------------- //

MapDataElement copyGame(MapDataElement game)
{
    if(game == NULL)
    {
        return NULL;
    }
    Game new_game = (Game)malloc(sizeof(*new_game));
    if(new_game == NULL)
    {
        return NULL;
    }

    new_game->length     = ((Game)game)->length;
    new_game->player1_id = ((Game)game)->player1_id;
    new_game->player2_id = ((Game)game)->player2_id;
    new_game->winners_id = ((Game)game)->winners_id;

    return (MapDataElement)new_game;
}