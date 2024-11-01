/*
 -------------------------------------
 File:    game.c
 Project: conway-game-of-life
 Main game file
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
#include <windows.h>

#include "life_functions.h"

/*
    Flips alive/dead state of the selected tile
*/
static inline
void life_spawn(game_state* L) {
    bool** M = L->Mv;
    M[L->ypos][L->xpos] = !(M[L->ypos][L->xpos]);
}


/*
    Moves cursor by given offset (t0: x-offset, t0: y-offset)
*/
static inline
void move_cursor(game_state* L, signed t0, signed t1) {     
  if (t0) {
    L->xpos = (L->xpos + L->n_cols + t0) % L->n_cols;
  }
  if (t1) {
    L->ypos = (L->ypos + L->n_rows + t1) % L->n_rows;
  }
}

static void* input_thread(void*);
static void* draw_input_thread(void*);
static void* game_update_thread(void*);
static void* draw_game_thread(void* Lv);

int main (int argc, char* argv[argc+1]) {
    size_t rows = 15;
    size_t cols = 50;
    size_t maxiters = 250;

    if (argc > 1) rows = strtoull(argv[1], 0, 0);
    if (argc > 2) cols = strtoull(argv[2], 0, 0);
    if (argc > 3) maxiters = strtoull(argv[3], 0, 0);

    game_state G;
    init_game(&G, rows, cols, maxiters, true);

    initscr();
    cbreak();
    noecho();

    pthread_t in_thrd[2];                                       // Threads for the input/setup portion of the game
    pthread_create(&in_thrd[0], 0, input_thread, &G);
    pthread_create(&in_thrd[1], 0, draw_input_thread, &G);

    pthread_join(in_thrd[0], 0);
    pthread_join(in_thrd[1], 0);


    pthread_t game_thrd[2];                                     // Threads for the actual game
    pthread_create(&game_thrd[0], 0, game_update_thread, &G);
    pthread_create(&game_thrd[1], 0, draw_game_thread, &G);

    pthread_join(game_thrd[0], 0);
    pthread_join(game_thrd[1], 0);

    getch();                                                    // Wait for user input to end
    endwin();

    destroy_game(&G);                                           // Free memory
}

static
void* draw_input_thread(void* Gv) {
    game_state*restrict G = Gv;
    size_t xpos = 0; 
    size_t ypos = 0;
    size_t spawns = 0;
    size_t update_rate = 0;
    bool toroidal = G->toroidal;

    xpos = G->xpos;
    ypos = G->ypos;

    WIN win;
    init_win_params(&win, G->n_rows, G->n_cols, 2, 0);

    mvprintw(0, 0, "Move: WASD | adjust update rate: q/e or +/- | spawn: f or SPACE\n");
    mvprintw(1,0, "Change Geometry: T | Start game: ENTER or ESCAPE");
    G->starty = 3;
    create_box(&win, TRUE);
    refresh();
    draw_board(G);
    draw_cursor(ypos, xpos, 3, false);
    print_gameParams(G->toroidal, G->update_rate, G->maxiters, G->n_rows);
    print_lastcom(G->last_command, G->n_rows);
    refresh();
   
    while (!G->inp_over) {
        pthread_mutex_lock(&G->mtx);                            // WAIT IF:
        while (!G->inp_over                                     // - input phase not over
            && (xpos == G->xpos)                                // - current displayed cursor position not outdated
            && (ypos == G->ypos)
            && (update_rate = G->update_rate)                   // - correct update rate is displayed
            && (spawns = G->spawns)) { // "... y ...."          // - all user-added spawns/kills drawn
            timed_cond_wait(&G->draw_inp, &G->mtx);
        }

        // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
        draw_board(G);
        spawns = G->spawns;                                     // ensure thread knows if displayed spawns/kills are up to date
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        pthread_mutex_unlock(&G->mtx);

        mvprintw(G->n_rows + 5, 13, "%3d", G->update_rate);
        draw_cursor(ypos, xpos, 3, true);
        xpos = G->xpos;
        ypos = G->ypos;
        update_rate = G->update_rate;
        draw_cursor(ypos, xpos, 3, false);
        if (toroidal != G->toroidal) {
            print_gameParams(G->toroidal, G->update_rate, G->maxiters, G->n_rows);
            toroidal = G->toroidal;
        }
        print_lastcom(G->last_command, G->n_rows);
        refresh();

        Sleep(1000 / 120);                                      // cap refresh rate to prevent unnecessary draw calls (eg: if a move key is held down)
    }
    draw_cursor(ypos, xpos, 3, true);                           // erase cursor from final position before moving on
    return 0;
}

static
void* input_thread(void* Gv) {
    game_state* G = Gv;
   
    do {
        int c = getch();
        switch(c) {
        case 'a' :
        case 'A' : G->last_command = GO_LEFT; move_cursor(G, -1, 0); break;
        case 'd' :
        case 'D' : G->last_command = GO_RIGHT; move_cursor(G, +1, 0); break;
        case 'w' :
        case 'W' : G->last_command = GO_UP; move_cursor(G, 0, -1); break;
        case 's' :
        case 'S' : G->last_command = GO_DOWN; move_cursor(G, 0, +1); break;
        //case 'r' : 
        //case 'R' : G->last_command = RESET_POS; break;
        case 't' : 
        case 'T' :
                pthread_mutex_lock(&G->mtx);
                G->last_command = TOGGLE_TOROIDAL; 
                // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
                G->toroidal = !G->toroidal;
                // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                pthread_cond_signal(&G->draw_inp);
                pthread_mutex_unlock(&G->mtx);
                break;
        case 'e' :
        case 'E' : 
        case '+' : G->last_command = INC_FRAMES; if (G->update_rate < 128) G->update_rate++; break;
        case 'q' :
        case 'Q' :        
        case '-' : G->last_command = DEC_FRAMES; if (G->update_rate > 1)   G->update_rate--; break;
        case ' ' :
        case 'f' :
        case 'F' :
                pthread_mutex_lock(&G->mtx);
                G->last_command = SPAWN_LIFE; 
                // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
                life_spawn(G);
                // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                pthread_cond_signal(&G->draw_inp);
                pthread_mutex_unlock(&G->mtx);
                break;
        case '\e':
        case '\r':  
        case  EOF : G->last_command = START_GAME; goto FINISH;
        }
        pthread_cond_signal(&G->draw_inp);
    } while (!(G->inp_over || feof(stdin)));
FINISH:
    G->inp_over = true;
    pthread_cond_signal(&G->draw_inp);
    return 0;
}

static 
void* draw_game_thread(void* Gv) {
    game_state*restrict G = Gv;
    print_lastcom(G->last_command, G->n_rows);
    mvprintw(0, 0, "ITERATION: %d", G->drawn);
    clrtoeol();
    refresh();
    while (!G->finished) {
        pthread_mutex_lock(&G->mtx);
        while (!G->finished && (G->drawn >= G->updates)) {
            pthread_cond_signal(&G->upd);
            timed_cond_wait(&G->draw_upd, &G->mtx);
        }

        // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
        draw_board(G);
        mvprintw(0, 0, "ITERATION: %d", G->drawn);
        G->drawn++;
        refresh();
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        pthread_cond_signal(&G->upd);
        pthread_mutex_unlock(&G->mtx);
    }
    return 0;
}

static void* game_update_thread(void* Gv) {
    game_state*restrict G = Gv;
    size_t changed = 1;
    while (!G->finished && changed) {
        pthread_mutex_lock(&G->mtx);
        while (!G->finished && (G->updates != G->drawn)) {
           timed_cond_wait(&G->upd, &G->mtx);
        }

        // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
        changed = game_update(G);
        G->updates++;
        if ((G->updates >= G->maxiters) || (changed == 0)) {
            G->finished = true;
        }
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        pthread_cond_signal(&G->draw_upd);
        pthread_mutex_unlock(&G->mtx);
        Sleep((1.0 / G->update_rate) * 1000);
    }

    if (changed == 0) {
        mvprintw(1, 0, "GAME OVER: steady state detected. Press any key to exit.");
        clrtoeol();
    } else {
        mvprintw(1, 0, "GAME OVER: Max iterations reached. Press any key to exit.");
        clrtoeol();
    }

    return 0;
}
