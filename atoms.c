#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "atoms.h"

#define MAX_WORDS MAX_LINE / 2

// track whether the game is started.
int isGameStarted = 0;
int isGameLoaded = 0;
int isPlayFromRun = 0;
int gameWidth, gameHeight, gamePlayer, currentPlayer = -1;
grid_t ***gameBoardPtr;
player_t players[] = {
    {"Red", 0},
    {"Green", 0},
    {"Purple", 0},
    {"Blue", 0},
    {"Yellow", 0},
    {"White", 0}};
int isPlayerStarted[] = {0, 0, 0, 0, 0, 0};
game_t *game;
int loadedMovesSize;
save_file_t *loaded_file = NULL;

void repeatPrint(char c, int times);
void tokenize(char *line, char **words, int *nwords);
int isPosstiveNumber(char *word);
void runCommandQuit();
void runCommandHelp();
void runCommandPlace(char **words, int nwords);
void initBoard();
void runCommandStart(char **words, int nwords);
void runCommandDisplay();
void runCommand(char **words, int nwords);
void nextTurn();
void place(int row, int col);
void initGame();
void addMove(move_t *m);
void removeLastMove();
void freeGameBoardMemory();
void freeMemory();
void reloadMoves();

void repeatPrint(char c, int times)
{
    for (int i = 0; i < times; i++)
    {
        printf("%c", c);
    }
}

//prints cell content
void printGrid(grid_t *g)
{
    if (g->owner == NULL)
    {
        printf("  ");
    }
    else
    {
        printf("%c%d", g->owner->colour[0], g->atom_count);
    }
}

void tokenize(char *line, char **words, int *nwords)
{
    *nwords = 1;
    for (words[0] = strtok(line, " \t\n");
         (*nwords < MAX_WORDS) && (words[*nwords] = strtok(NULL, " \t\n"));
         *nwords = *nwords + 1)
        ; /* empty body */
    return;
}

int isPosstiveNumber(char *word)
{
    while (*word != '\0')
    {
        if (!isdigit(*word))
        {
            return 0;
        }
        word++;
    }
    return 1;
}

int checkLimit(int row, int col)
{
    int l = 4;
    // corner
    if (
        (row == 0 && col == 0) ||
        (row == 0 && col == gameWidth - 1) ||
        (row == gameHeight - 1 && col == 0) ||
        (row == gameHeight - 1 && col == gameWidth - 1))
    {
        l = 2;
    }
    // side
    else if (row == 0 || row == gameHeight - 1 || col == 0 || col == gameWidth - 1)
    {
        l = 3;
    }
    return l;
}

void freeGameBoardMemory()
{
    if (gameBoardPtr == NULL)
    {
        return;
    }
    // free memory
    for (int i = 0; i < gameHeight; i++)
    {
        for (int j = 0; j < gameWidth; j++)
        {
            free(gameBoardPtr[i][j]);
        }
        free(gameBoardPtr[i]);
    }
    free(gameBoardPtr);
}

void freeMemory()
{
    freeGameBoardMemory();

    if (game != NULL)
    {
        move_t *m = game->moves;
        while (m != NULL)
        {
            move_t *n = m->next;
            free(m);
            m = n;
        }

        free(game);
    }

    if (loaded_file != NULL)
    {
        free(loaded_file->raw_move_data);
        free(loaded_file);
    }
}

void runCommandQuit()
{
    freeMemory();

    // quit the game
    exit(EXIT_SUCCESS);
}

void runCommandHelp()
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    printf("\n");
    printf("HELP displays this help message\n");
    printf("QUIT quits the current game\n");
    printf("\n");
    printf("DISPLAY draws the game board in terminal\n");
    printf("START <number of players> <width> <height> starts the game\n");
    printf("PLACE <x> <y> places an atom in a grid space\n");
    printf("UNDO undoes the last move made\n");
    printf("STAT displays game statistics\n");
    printf("\n");
    printf("SAVE <filename> saves the state of the game\n");
    printf("LOAD <filename> loads a save file\n");
    printf("PLAYFROM <turn> plays from n steps into the game\n");
}

void runCommandStat()
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    if (isGameStarted == 0)
    {
        printf("Game Not In Progress\n");
        return;
    }

    for (int i = 0; i < gamePlayer; i++)
    {
        printf("Player %s:\n", players[i].colour);

        if (isPlayerStarted[i] == 1 && players[i].grids_owned == 0)
        {
            printf("Lost\n");
        }
        else
        {
            printf("Grid Count: %d\n", players[i].grids_owned);
        }

        if (i < gamePlayer - 1)
        {
            printf("\n");
        }
    }
}
void initBoard()
{
    int width = gameWidth;
    int height = gameHeight;
    gameBoardPtr = malloc(height * sizeof(grid_t **));
    for (int i = 0; i < height; i++)
    {
        gameBoardPtr[i] = malloc(width * sizeof(grid_t *));
        for (int j = 0; j < width; j++)
        {
            gameBoardPtr[i][j] = malloc(sizeof(grid_t));
            gameBoardPtr[i][j]->owner = NULL;
            gameBoardPtr[i][j]->atom_count = 0;
        }
    }
}

void runCommandStart(char **words, int nwords)
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    // validation
    if (isGameStarted == 1)
    {
        printf("Invalid Command\n");
        return;
    }
    if (nwords < 4)
    {
        printf("Missing Argument\n");
        return;
    }
    if (nwords > 4)
    {
        printf("Too Many Arguments\n");
        return;
    }

    // check all params is number
    for (int i = 1; i < 4; i++)
    {
        if (!isPosstiveNumber(words[i]))
        {
            printf("Invalid command arguments\n");
            return;
        }
    }

    // extract var
    int n = atoi(words[1]),
        width = atoi(words[2]),
        height = atoi(words[3]);

    // validate
    if (width * height < n || n < MIN_PLAYERS || n > MAX_PLAYERS || width < MIN_WIDTH || width > MAX_WIDTH || height < MIN_HEIGHT || height > MAX_HEIGHT)
    {
        printf("Cannot Start Game\n");
        return;
    }

    // initilize game
    gameWidth = width;
    gameHeight = height;
    gamePlayer = n;

    // initlize board
    initBoard();
    isGameStarted = 1;

    printf("Game Ready\n");
    nextTurn();
}

void runCommandDisplay()
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    printf("\n");
    // print header
    printf("+");
    repeatPrint('-', 3 * gameWidth - 1);
    printf("+\n");

    for (int row = 0; row < gameHeight; row++)
    {
        printf("|");
        for (int col = 0; col < gameWidth; col++)
        {
            printGrid(gameBoardPtr[row][col]);
            printf("|");
        }
        printf("\n");
    }

    // print footer
    printf("+");
    repeatPrint('-', 3 * gameWidth - 1);
    printf("+\n");
}

void _nextTurn()
{
    int thisTurnPlayer = currentPlayer;
    while (1)
    {
        currentPlayer = (currentPlayer + 1) % gamePlayer;

        // check whether this player is alive
        if (isPlayerStarted[currentPlayer] == 1 && players[currentPlayer].grids_owned == 0)
        {
            // when he dead
        }
        else if (thisTurnPlayer == currentPlayer)
        {
            // when only one player left, he wins
            printf("%s Wins!\n", players[currentPlayer].colour);
            // quit
            runCommandQuit();
            break;
        }
        else
        {
            break;
        }
    }
}

void nextTurn()
{
    _nextTurn();
    printf("%s's Turn\n", players[currentPlayer].colour);
}

void expand(int row, int col)
{
    if (gameBoardPtr[row][col]->atom_count < checkLimit(row, col))
    {
        return;
    }

    // reset current grid
    gameBoardPtr[row][col]->owner->grids_owned--;
    gameBoardPtr[row][col]->owner = NULL;
    gameBoardPtr[row][col]->atom_count = 0;

    // expand clockwise
    place(row - 1, col);
    place(row, col + 1);
    place(row + 1, col);
    place(row, col - 1);
}

void place(int row, int col)
{
    // basic validation
    if (row < 0 || row > gameHeight - 1 || col < 0 || col > gameWidth - 1)
    {
        return;
    }

    player_t *old_owner = gameBoardPtr[row][col]->owner;
    gameBoardPtr[row][col]->owner = &players[currentPlayer];
    gameBoardPtr[row][col]->atom_count++;

    if (old_owner != NULL)
    {
        // when take over other's grid
        if (old_owner != &players[currentPlayer])
        {
            old_owner->grids_owned--;
            players[currentPlayer].grids_owned++;
        }

        // calculate expension
        expand(row, col);
    }
    else
    {
        // place atom of current player
        players[currentPlayer].grids_owned++;
    }

    isPlayerStarted[currentPlayer] = 1;
}

move_t *createMove(int row, int col)
{
    move_t *m = malloc(sizeof(move_t));
    m->x = col;
    m->y = row;
    m->prev = NULL;
    m->next = NULL;
    m->old_owner = gameBoardPtr[row][col]->owner;
    return m;
}

void placeMove(move_t *m)
{
    int row = m->y;
    int col = m->x;

    // basic validation
    if (row < 0 || row > gameHeight - 1 || col < 0 || col > gameWidth - 1)
    {
        return;
    }

    gameBoardPtr[row][col]->owner = &players[currentPlayer];
    gameBoardPtr[row][col]->atom_count++;

    if (m->old_owner != NULL)
    {
        // when take over other's grid
        if (m->old_owner != &players[currentPlayer])
        {
            m->old_owner->grids_owned--;
            players[currentPlayer].grids_owned++;
        }

        // calculate expension
        expand(row, col);
    }
    else
    {
        // place atom of current player
        players[currentPlayer].grids_owned++;
    }

    isPlayerStarted[currentPlayer] = 1;
}

void runCommandPlace(char **words, int nwords)
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    //validate
    if (nwords < 3)
    {
        printf("Invalid Coordinates\n");
        return;
    }
    if (nwords > 3)
    {
        printf("Invalid Coordinates\n");
        return;
    }

    //check param are nums
    for (int i = 1; i < 3; i++)
    {
        if (!isPosstiveNumber(words[i]))
        {
            printf("Invalid Coordinates\n");
            return;
        }
    }

    //extract position var
    int col = atoi(words[1]),
        row = atoi(words[2]);

    //validate
    if (col > gameWidth - 1 || col < 0 || row > gameHeight - 1 || row < 0)
    {
        printf("Invalid Coordinates\n");
        return;
    }

    // when not owned by player or is not unoccupied
    // need to add another condition where owner of cell is not current owner
    if (gameBoardPtr[row][col]->owner != NULL && &players[currentPlayer] != gameBoardPtr[row][col]->owner)
    {
        printf("Cannot Place Atom Here\n");
        return;
    }

    // making a move
    move_t *m = createMove(row, col);

    // call recurive method
    // place(row, col);
    placeMove(m);
    // save the moves globally
    addMove(m);
    nextTurn();
}

void resetGame()
{
    currentPlayer = 0;
    for (int i = 0; i < 6; i++)
    {
        players[i].grids_owned = 0;
        isPlayerStarted[i] = 0;
    }

    freeGameBoardMemory();
    initBoard();
}

void reloadMoves()
{
    move_t *m = game->moves;
    if (m == NULL)
    {
        return;
    }

    while (m != NULL)
    {
        placeMove(m);
        _nextTurn();
        m = m->next;
    }
}

void runCommandUndo()
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    if (game->moves == NULL)
    {
        printf("Cannot Undo\n");
        return;
    }

    removeLastMove();
    resetGame();
    reloadMoves();

    printf("%s's Turn\n", players[currentPlayer].colour);
}

void runCommandSave(char **words, int nwords)
{
    if (isGameLoaded == 1 && isPlayFromRun == 0)
    {
        printf("Invalid Command\n");
        return;
    }

    // validation
    if (nwords < 2)
    {
        printf("Missing Argument\n");
        return;
    }

    if (nwords > 2)
    {
        printf("Too Many Arguments\n");
        return;
    }

    if (isGameStarted == 0)
    {
        printf("Game Not In Progress\n");
        return;
    }

    FILE *file;
    if ((file = fopen(words[1], "r")))
    {
        fclose(file);
        // file already exists;
        printf("File Already Exists\n");
        return;
    }

    // create a new file
    file = fopen(words[1], "wb");

    save_file_t save_file;
    save_file.width = gameWidth;
    save_file.height = gameHeight;
    save_file.no_players = gamePlayer;

    fwrite(&save_file.width, sizeof(save_file.width), 1, file);
    fwrite(&save_file.height, sizeof(save_file.height), 1, file);
    fwrite(&save_file.no_players, sizeof(save_file.no_players), 1, file);

    // loop game moves, and save into files

    move_t *m = game->moves;
    while (m != NULL)
    {
        // // X Y 0 0
        // uint32_t moveData = m->x << 24 | m->y << 16;
        // fwrite(&moveData, sizeof(uint32_t), 1, file);

        // 0 0 Y X
        uint32_t moveData = m->y << 8 | m->x;
        fwrite(&moveData, sizeof(uint32_t), 1, file);

        // 0000 0101
        // 0000 1010

        // 0000 1111 bitwise or
        // 0000 0000 bitwise and

        // x = 00000000 00000000 00000000 00000101
        // y = 00000000 00000000 00000000 00001101

        // after x << 24
        // x = 00000101 00000000 00000000 00000000

        // after y << 16
        // y = 00000000 00001101 00000000 0000000

        // after x << 24 | y << 16
        // 00000101 00001101 00000000 0000000

        m = m->next;
    }

    fclose(file);
    printf("Game Saved\n");
}

/**
 * check the file size
 */
int fsize(char *file)
{
    int size;
    FILE *fh;

    fh = fopen(file, "rb"); //binary mode
    if (fh != NULL)
    {
        if (fseek(fh, 0, SEEK_END))
        {
            fclose(fh);
            return -1;
        }

        size = ftell(fh);
        fclose(fh);
        return size;
    }

    return -1; //error
}

void runCommandLoad(char **words, int nwords)
{
    // validation
    if (nwords < 2)
    {
        printf("Missing Argument\n");
        return;
    }

    if (nwords > 2)
    {
        printf("Too Many Arguments\n");
        return;
    }

    if (isGameStarted == 1 || isGameLoaded == 1)
    {
        printf("Restart Application To Load Save\n");
        return;
    }

    FILE *file;
    if ((file = fopen(words[1], "rb")))
    {
        // check file size
        int fileSize = fsize(words[1]);
        loadedMovesSize = (fileSize - 3) / 4;
        loaded_file = malloc(sizeof(save_file_t));
        loaded_file->raw_move_data = malloc(sizeof(uint32_t) * loadedMovesSize);

        uint8_t w, h, n;

        fread(&w, sizeof(w), 1, file);
        fread(&h, sizeof(h), 1, file);
        fread(&n, sizeof(n), 1, file);

        loaded_file->width = w;
        loaded_file->height = h;
        loaded_file->no_players = n;

        // load moves
        for (int i = 0; i < loadedMovesSize; i++)
        {
            fread(&loaded_file->raw_move_data[i],
                  sizeof(loaded_file->raw_move_data[i]), 1, file);
        }

        fclose(file);
    }
    else
    {
        printf("Cannot Load Save\n");
        return;
    }

    isGameLoaded = 1;
    printf("Game Loaded\n");
}

void runCommandPlayFrom(char **words, int nwords)
{
    if (nwords < 2)
    {
        printf("Missing Argument\n");
        return;
    }

    if (nwords > 2)
    {
        printf("Too Many Arguments\n");
        return;
    }

    if (loaded_file == NULL)
    {
        printf("Invalid Command\n");
        return;
    }

    int playFromTurn = 0;
    if (strcmp(words[1], "END") == 0)
    {
        // load till end;
        playFromTurn = loadedMovesSize;
    }
    else
    {
        // validate words[0]
        if (!isPosstiveNumber(words[1]))
        {
            printf("Invalid Turn Number\n");
            return;
        }
        playFromTurn = atoi(words[1]);

        // validate playFromTurn smaller than loaded moves size
        if (playFromTurn > loadedMovesSize)
        {
            printf("Invalid Turn Number\n");
            return;
        }
    }

    // restore moves

    // re-init game
    gameWidth = loaded_file->width;
    gameHeight = loaded_file->height;
    gamePlayer = loaded_file->no_players;
    initBoard();
    isGameStarted = 1;
    currentPlayer = 0;

    // convert encoded raw_move_data into moves
    for (int i = 0; i < playFromTurn; i++)
    {
        uint32_t raw_data = loaded_file->raw_move_data[i];

        // X Y 0 0
        // uint32_t x = raw_data >> 24;
        // uint32_t y = (raw_data << 8) >> 24;

        // 0 0 Y X
        uint32_t x = (raw_data << 24) >> 24;
        uint32_t y = raw_data >> 8;

        move_t *m = createMove(y, x);
        addMove(m);
    }

    reloadMoves();

    printf("Game Ready\n");
    printf("%s's Turn\n", players[currentPlayer].colour);
    isPlayFromRun = 1;
}

void runCommand(char **words, int nwords)
{
    if (strcmp(words[0], "QUIT") == 0)
    {
        printf("Bye!\n");
        runCommandQuit();
    }
    else if (strcmp(words[0], "HELP") == 0)
    {
        runCommandHelp();
    }
    else if (strcmp(words[0], "START") == 0)
    {
        runCommandStart(words, nwords);
    }
    else if (strcmp(words[0], "DISPLAY") == 0)
    {
        runCommandDisplay();
    }
    else if (strcmp(words[0], "PLACE") == 0)
    {
        runCommandPlace(words, nwords);
    }
    else if (strcmp(words[0], "STAT") == 0)
    {
        runCommandStat();
    }
    else if (strcmp(words[0], "UNDO") == 0)
    {
        runCommandUndo();
    }
    else if (strcmp(words[0], "SAVE") == 0)
    {
        runCommandSave(words, nwords);
    }
    else if (strcmp(words[0], "LOAD") == 0)
    {
        runCommandLoad(words, nwords);
    }
    else if (strcmp(words[0], "PLAYFROM") == 0)
    {
        runCommandPlayFrom(words, nwords);
    }
    else
    {
        printf("UNKNOWN COMMAND\n");
    }
    printf("\n");
}

void initGame()
{
    game = malloc(sizeof(game_t));
    game->moves = NULL;
}

void addMove(move_t *m)
{
    move_t *lastMove = game->moves;
    if (lastMove == NULL)
    {
        game->moves = m;
    }
    else
    {
        while (lastMove->next != NULL)
        {
            lastMove = lastMove->next;
        }
        lastMove->next = m;
        m->prev = lastMove;
    }
}

void removeLastMove()
{
    move_t *lastMove = game->moves;
    if (lastMove == NULL)
    {
        return;
    }

    while (lastMove->next != NULL)
    {
        lastMove = lastMove->next;
    }

    // when reach the header
    if (lastMove->prev == NULL)
    {
        game->moves = NULL;
    }
    else
    {
        lastMove->prev->next = NULL;
        lastMove->prev = NULL;
    }

    free(lastMove);
}

int main()
{
    initGame();

    char line[MAX_LINE], *words[MAX_WORDS];
    int nwords;

    while (1)
    {
        fgets(line, MAX_LINE, stdin);
        tokenize(line, words, &nwords);
        runCommand(words, nwords);
    }

    exit(EXIT_SUCCESS);
}
