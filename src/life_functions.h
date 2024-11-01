/*
 -------------------------------------
 File:    life_functions.h
 Project: conway-game-of-life
 Header for the game board functions
 -------------------------------------
 Author:  Akshath Wikramanayake
 Email:   akshath.wikramanayake@gmail.com
 Version  0.0.1
 -------------------------------------
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <ncurses/ncurses.h>
#include <stdatomic.h>

enum input_enum {
    EMPTY = 0,
    GO_LEFT,
    GO_RIGHT,
    GO_UP,
    GO_DOWN,
    RESET_POS,
    INC_FRAMES,
    DEC_FRAMES,
    START_GAME,
    SPAWN_LIFE,
    TOGGLE_TOROIDAL,
};


typedef struct _win_border_struct {
    chtype ls, rs, ts, bs, tl, tr, bl, br;
} WIN_BORDER;


typedef struct _WIN_struct {
    int startx, starty;
    int height, width;
    WIN_BORDER border;
} WIN;


typedef struct _game_state {
    // thread-related variables
    pthread_mutex_t mtx;        // mutex that protects the game board 
    pthread_cond_t draw_inp;    // cond to draw after user input
    pthread_cond_t draw_upd;    // cond to draw after game state update
    pthread_cond_t upd;         // cond to update the game board

    // game parameters
    size_t update_rate;                 // stores the desired game updates per second
    size_t n_rows;              // # of rows (board height) 
    size_t n_cols;              // # of columns (board width)
    size_t maxiters;            // maximum # of iterations before game ends
    bool toroidal;              // true: toroidal/donut shaped geometry; false: walled

    // dynamic game state variables
    size_t xpos;                // stores the cursor x-coordinate
    size_t ypos;                // stores the cursor y-coordinate
    size_t last_command;        // stores the most recent user input
    size_t starty;              // game board y-coordinate offset

    // iterators/counters
    size_t spawns;              // # of times user spawned/despawned life
    size_t updates;             // # of times the board has been updated
    size_t drawn;               // # of times the board has been drawn

    
    // game state control variables
    atomic_bool inp_over;       // set to true when user input phase is completed 
    atomic_bool finished;       // set to true when the game is over
    void* Mv;                   // void pointer to the game board matrix

} game_state;


/*  
    Initialises the game_state struct and allocates memory for the game board
    Params:
    G: pointer to game_state object 
    rows: # of rows
    cols: # of columns
    iters: # of iterations before game ends
    toroidal: bool, set to true for toroidal/doughnut geometry, false for walled
*/
void init_game(game_state* G, size_t rows, size_t cols, size_t iters, bool toroidal);


/*
    Deallocates the memory allocated for the game board
*/
void destroy_game(game_state*);


/*  
    Allocates memory for a matrix of boolean values and returns a null pointer to
    an array of pointers to the rows of the matrix.
    Cast to bool** to access the matrix. M[i][j] notation works and data is stored contiguously
    Params:
    rows: # of rows
    cols: # of cols
    returns: void pointer to the start of the array of row pointers to the matrix rows 
*/ 
void* create_matrix(size_t rows, size_t cols);


/*
    Initialises the parameters for the board border
    p_win: window struct that contains the params
    n_rows: board height
    n_cols: board width
    starty: y-offset
    startx: x-offset
*/
void init_win_params(WIN * p_win, size_t n_rows, size_t n_cols, size_t starty, size_t startx);


/*
    Creates the border of the game board for display
    NOTE: calls refresh()
    win: window struct with position params
    flag: toggle whether or not to add the border characters
*/
void create_box(WIN * win, bool flag);


/*
    Updates the game board state by one iteration (spawning/killing based on the rules)
    returns: number of updates completed
*/
int game_update(game_state*);


/*
    Calls pthread_cond_timedwait with a wait time of 1 second
    returns: return value of pthread_cond_timedwait function call
*/
int timed_cond_wait(pthread_cond_t*, pthread_mutex_t*);


/*
    Draws the game board on the ncurses window (note: refresh() must still be called)
*/
void draw_board(game_state*);


/*
    Draws or erases the cursor at the given position on the ncurses window
    ypos: y-coordinate of the cursor
    xpos: x-coordinate of the cursor
    starty: y-offset of the game board
    erase: true: erases the cursor from the given position | false: draws cursor at the given position
*/
void draw_cursor(size_t ypos, size_t xpos, size_t starty, bool erase);


/*
    Prints the last user input directly below the game board (note: refresh() must still be called)
    com: input command ID (see the enum)
    n_cols: height of the game board, used to calculate offset
*/
void print_lastcom(int com, size_t n_cols);


/*
    Prints the current geometry status of the game board (note: refresh() must still be called)
    toroidal: true: board is toroidal | false: board is flat/walled
    n_cols: height of the game board, used to calculate offset
*/
void print_gameParams(bool toroidal, size_t upd_rate, size_t maxiters, size_t n_rows);


/*
    Checks if a tile will have life on the next cycle based on current life status & # of living neighbours
    current: current life status of the tile
    nbrs: # of living neighbours
    return: true: will have life | false: won't have life
*/
static inline
bool test_life(bool current, size_t nbrs);

