# Atoms
This game revolves around placing atoms on a gridded game board. Each grid space has a limit of how many atoms it can contain.

Once the limit is reached, the atoms will expand to the adjacent spaces.

This creates an interesting property in the game where chains can be triggered by a single placement.

The game will be accompanied by a number of utility functions for players to utilise such as saving, game statistics and loading. You will also need to implement a set of commands that will allow the players to interact with the game.

# Prerequisites
Compiler for C

# Commands
- HELP displays this help message
- QUIT quits the current game
- DISPLAY draws the game board in terminal
- START <width> <height> <number of players> starts the game
- PLACE <x> <y> places an atom in a grid space
- UNDO undoes the last move made
- STAT displays game statistics
- SAVE <filename> saves the state of the game
- LOAD <filename> loads a save file
- PLAYFROM <turn> plays from n steps into the game
