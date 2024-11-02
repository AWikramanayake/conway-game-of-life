/*
 -------------------------------------
 File:    life_functions.c
 Project: conway-game-of-life
 Functions for the game board
 -------------------------------------
 Author:  Akshath Wikramanayake
 Email:   akshath.wikramanayake@gmail.com
 Version  0.0.1
 -------------------------------------
 */

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <ncurses/ncurses.h>
#include <string.h>

#include "life_functions.h"


void init_game(game_state* G, size_t rows, size_t cols, size_t maxiters, bool toroidal) {
    G->mtx = PTHREAD_MUTEX_INITIALIZER;
    G->draw_inp = PTHREAD_COND_INITIALIZER;
    G->draw_upd = PTHREAD_COND_INITIALIZER;
    G->upd = PTHREAD_COND_INITIALIZER;

    G->update_rate = 10;
    G->n_rows = rows;
    G->n_cols = cols;
    G->maxiters = maxiters;
    G->toroidal = toroidal;

    G->xpos = 0;
    G->ypos = 0;
    G->last_command = 0;
    G->starty = 0;

    G->spawns = 0;
    G->updates = 0;
    G->drawn = 0;

    G->inp_over = false;
    G->finished = false;

    G->Mv = create_matrix(rows, cols);
    bool** M = G->Mv;                                               // initialise matrix data to 0s
    memset((void*)M[0], 0, sizeof(bool) * rows * cols);             //  M[0], the first row pointer, points to the start of thecontiguous block of matrix data
}


void destroy_game(game_state* G) {
    bool** ptr = G->Mv;
    free((void*)ptr[0]);                                            // free matrix data block
    free(G->Mv);                                                    // free array of row pointers
}


void* create_matrix(size_t rows, size_t cols) {
    void* RPv = malloc(rows * sizeof(bool*));                       // allocate memory for array of row pointers
    bool** row_ptrs = (bool**)RPv;
    row_ptrs[0] = malloc(rows * cols * sizeof(bool));               // allocate memory for matrix data

    for (int i = 1; i < rows; i++) {
        row_ptrs[i] = row_ptrs[0] + i * cols;                       // make row pointers point to start of each row
    }

    return RPv;         // return void* to START OF ROW POINTER ARRAY. Start of data block can be accessed using (bool**)RPv[0].
}


void init_win_params(WIN * p_win, size_t n_rows, size_t n_cols, size_t starty, size_t startx) {
    p_win->height = n_rows + 1;
    p_win->width = (2 * n_cols) + 2;
    p_win->starty = starty;
    p_win->startx = startx;

    p_win->border.ls = '|';
    p_win->border.rs = '|';
    p_win->border.ts = '-';
    p_win->border.bs = '-';
    p_win->border.tl = '+';
    p_win->border.tr = '+';
    p_win->border.bl = '+';
    p_win->border.br = '+';
}


void create_box(WIN * p_win, bool flag) {
    int i, j;
    int x, y, w, h;

    x = p_win->startx;
    y = p_win->starty;
    w = p_win->width;
    h = p_win->height;

    if (flag == TRUE) {
        mvaddch(y, x, p_win->border.tl);
        mvaddch(y, x + w, p_win->border.tr);
        mvaddch(y + h, x, p_win->border.bl);
        mvaddch(y + h, x + w, p_win->border.br);
        mvhline(y, x + 1, p_win->border.ts, w - 1);
        mvhline(y + h, x + 1, p_win->border.bs, w - 1);
        mvvline(y + 1, x, p_win->border.ls, h - 1);
        mvvline(y + 1, x + w, p_win->border.rs, h - 1);
    }
    refresh();
}


int game_update(game_state* G) {
    size_t count;                                                   // stores # of tiles whose life/death status has changed

    bool** tempgrid =  create_matrix(G->n_rows+2, G->n_cols+2);     // create placeholder matrix to hold old board + padding
    bool** M = G->Mv;

    // set the corners/edges to padding values
    if (G->toroidal) {                                              // toroidal: copy values from opposite point on the board
        // horizontal edges minus corners
        memcpy(tempgrid[0] + 1, M[G->n_rows - 1], G->n_cols * sizeof(bool));
        memcpy(tempgrid[G->n_rows + 1] + 1, M[0], G->n_cols * sizeof(bool));

        // verical edges minus corners
        for (int i = 1; i < G->n_rows + 1; i++) {
            tempgrid[i][0] = M[i - 1][G->n_cols - 1];
            tempgrid[i][G->n_cols + 1] = M[i - 1][0];
        }

        // corners
        tempgrid[0][0] = M[G->n_rows - 1][G->n_cols - 1];
        tempgrid[G->n_rows + 1][0] = M[0][G->n_cols - 1];
        tempgrid[0][G->n_cols + 1] = M[G->n_rows - 1][0];
        tempgrid[G->n_rows + 1][G->n_cols + 1] = M[0][0];

    } else {                                                        // not toroidal => walls are padded with 0s
        for (int i = 0; i < G->n_rows + 2; i++) {
            tempgrid[i][0] = false;                                 // padding left and right columns including corners
            tempgrid[i][G->n_cols + 1] = false;
        }
        for (int j = 1; j < G->n_cols + 1; j++) {                   // padding top and bottom row, excluding corners (doesn't matter if you overwrite corners)
            tempgrid[0][j] = false;
            tempgrid[G->n_rows + 1][j] = false;
        }
    }

    // copy old board to interior of placeholder matrix
    for (int i = 0; i < G->n_rows; i++) {                                   // tempgrid[0][j] is the top row used for padding -> avoid! (same for bottom row)
        memcpy(&tempgrid[i+1][1], M[i], G->n_cols * sizeof(bool));          // tempgrid[i][0] is the left column, also for padding -> avoid! (same for right column)
    }

    // Reset Lv->M to an empty board after copying
    memset(M[0], 0, G->n_rows * G->n_cols * sizeof(bool));

    // temporary var to store number of neighbours
    size_t nbrs = 0;

    // fill in M with updated values
    for (size_t i = 1; i < G->n_rows + 1; i++) {
        for (size_t j = 1; j < G->n_cols + 1; j++) {
            nbrs = 0;
            nbrs = tempgrid[i-1][j-1] + tempgrid[i-1][j] + tempgrid[i-1][j+1] + tempgrid[i][j-1] + tempgrid[i][j+1] + tempgrid[i+1][j-1] + tempgrid[i+1][j] + tempgrid[i+1][j+1];
            bool temp = test_life(tempgrid[i][j], nbrs);            // check if tile in the placehoolder matrix will have life next cycle
            count += (temp != tempgrid[i][j]);                      // if life/death status next cycle is different, increment count
            M[i-1][j-1] = temp;                                     // set new life/death status to the corresponding tile in the original matrix
        }
    }
    free((void*)tempgrid[0]);                                       // free the placeholder matrix
    free((void*)tempgrid);
    return count;                                                   // if no tiles have changed, early stopping will be triggered
}


int timed_cond_wait(pthread_cond_t* cnd, pthread_mutex_t* mtx) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    now.tv_sec += 1;
    return pthread_cond_timedwait(cnd, mtx, &now);    
}


void draw_board(game_state* G) {
    bool** M = G->Mv;
    
    for (size_t i = 0; i < G->n_rows; i ++) {
        for (size_t j = 0; j < G->n_cols; j ++) {
            if (M[i][j]) {
                mvaddch(i + G->starty, (j * 2) + 2, 'O');
            } else {
                mvaddch(i + G->starty, (j * 2) + 2, ' ');
            }
        }
    }
}


void draw_cursor(size_t ypos, size_t xpos, size_t starty, bool erase) {
   if (!erase) {
      mvaddch(ypos + starty, (xpos * 2) + 1, '[');
      mvaddch(ypos + starty, (xpos * 2) + 3, ']');
   } else {
      mvaddch(ypos + starty, (xpos * 2) + 1, ' ');                          // if we are in erase mode, just overwrite with a space character
      mvaddch(ypos + starty, (xpos * 2) + 3, ' ');      
   }
}


void print_lastcom(int com, size_t n_rows) {
    int ypos = n_rows + 4;
    switch(com) {
        case EMPTY: mvprintw(ypos, 0, "Last command: EMPTY"); clrtoeol(); break;
        case GO_LEFT: mvprintw(ypos, 0, "Last command: MOVE LEFT"); clrtoeol(); break;
        case GO_RIGHT: mvprintw(ypos, 0, "Last command: MOVE RIGHT"); clrtoeol(); break;
        case GO_UP: mvprintw(ypos, 0, "Last command: MOVE UP"); clrtoeol(); break;
        case GO_DOWN: mvprintw(ypos, 0, "Last command: MOVE DOWN"); clrtoeol(); break;
        case RESET_POS: mvprintw(ypos, 0, "Last command: RESET POS"); clrtoeol(); break;
        case INC_FRAMES: mvprintw(ypos, 0, "Last command: INC UPDATE RATE"); clrtoeol(); break;
        case DEC_FRAMES: mvprintw(ypos, 0, "Last command: DEC UPDATE RATE"); clrtoeol(); break;
        case START_GAME: mvprintw(ypos, 0, "Last command: START GAME"); clrtoeol(); break;
        case SPAWN_LIFE: mvprintw(ypos, 0, "Last command: SPAWN/KILL"); clrtoeol(); break;
        case TOGGLE_TOROIDAL: mvprintw(ypos, 0, "Last command: CHANGE GEOMETRY"); clrtoeol(); break;
   }
}


void print_gameParams(bool toroidal, size_t upd_rate, size_t maxiters, size_t n_rows) {
    int ypos = n_rows + 5;
    if (toroidal) {
        mvprintw(ypos, 0, "Update rate:%3d /s | maxiters: %4d | Board geometry: TOROIDAL", upd_rate, maxiters); clrtoeol();
    } else {
        mvprintw(ypos, 0, "Update rate:%3d /s | maxiters: %4d | Board geometry: FLAT/WALLED", upd_rate, maxiters); clrtoeol();
    }
}


static inline
bool test_life(bool current, size_t nbrs) {
    return (nbrs == 3 || (current && nbrs == 2));           // 3 living neighbours always gives life. 2 living neighbours also does IFF tile is currently alive.
}                                                           