#ifndef ATOMS_H
#define ATOMS_H

#include <stdint.h>

#define MAX_LINE 255
#define MIN_WIDTH 2
#define MIN_HEIGHT 2
#define MAX_WIDTH 255
#define MAX_HEIGHT 255
#define MIN_PLAYERS 2
#define MAX_PLAYERS 6

typedef struct move_t move_t;
typedef struct grid_t grid_t;
typedef struct game_t game_t;
typedef struct save_t save_t;
typedef struct player_t player_t;
typedef struct save_file_t save_file_t;

struct move_t
{
    int x;
    int y;
    move_t *parent;
    move_t *extra;

    move_t *prev;
    move_t *next;

    player_t *old_owner;
};

struct game_t
{
    move_t *moves;
};

struct grid_t
{
    player_t *owner;
    int atom_count;
};

struct player_t
{
    char *colour;
    int grids_owned;
};

struct save_t
{
    char *filename;
    save_file_t *data;
};

struct save_file_t
{
    uint8_t width;
    uint8_t height;
    uint8_t no_players;
    uint32_t *raw_move_data;
};

#endif
