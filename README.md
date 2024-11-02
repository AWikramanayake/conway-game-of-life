# conway-game-of-life
An interactive implementation of Conway's Game of Life in C
<br>

<p align="center">
<img src="https://github.com/AWikramanayake/conway-game-of-life/blob/main/misc/Simkin%20glider%20gun.gif" width="720"/>
</p>
<p align="center">
Example 1: Simkin's Glider Gun</br>

## Background: Conway's Game of Life
From Wikipedia: <br><br>
*The Game of Life, also known as Conway's Game of Life or simply Life, is a cellular automaton devised by the British mathematician John Horton Conway in 1970. It is a zero-player game, meaning that its evolution is determined by its initial state, requiring no further input. One interacts with the Game of Life by creating an initial configuration and observing how it evolves. It is Turing complete and can simulate a universal constructor or any other Turing machine.* <br><be>

*Theoretically, the Game of Life has the power of a universal Turing machine: anything that can be computed algorithmically can be computed within the Game of Life.* <br><br>

### Game rules:
The evolution of the game board from one cycle to the next is governed by a set of simple rules:<br>
- Any live cell with fewer than two live neighbours dies, as if by underpopulation.
- Any live cell with two or three live neighbours lives on to the next generation.
- Any live cell with more than three live neighbours dies, as if by overpopulation.
- Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
<br>

## This project
This project is a lightweight (semi-)interactive implementation of Conway's game of life written in C that (currently) runs in the terminal. The game consists of an input phase during which the user can arrange the initial state of the board and adjust game parameters (board size, update rate, board geometry, maximum iterations) followed by the fully automated game phase.

Both game phases use 2 threads: one to handle output and one to handle other game tasks (reading and implementing user input in the first phase, and updating the game board in the second phase). Updates are currently done using a naive algorithm (simply counting living neighbours for each tile). A simple early stopping mechanism also detects when the board has reached a steady state.

<br>
<p align="center">
<img src="https://github.com/AWikramanayake/conway-game-of-life/blob/main/misc/Gosper%20glider%20gun.gif" width="720"/>
</p>
<p align="center">
Example 2: Gosper's Glider Gun</br>
<br>
<p align="center">
<img src="https://github.com/AWikramanayake/conway-game-of-life/blob/main/misc/Gosper%20glider%20gun%20toroidal.gif" width="720"/>
</p>
<p align="center">
Blooper: Gosper's Glider Gun with toroidal board geometry</p></br><br>

Simkin's glider gun (example 1) and Gosper's glider gun (example 2) are implementations of periodic signal emitters that can be created using the game's simple rules. These can be combined with signal eaters, negators, reflectors, etc to create logic gates, hence why the game is Turing complete.


### Next steps:
- Add linux support (the project currently uses Sleep() from windows.h to cap output refresh rate) and create a makefile
- Allow the user to pause the game during the second phase and return to the input phase to change board state/game parameters
- Use a hash table to compare the board state to recent previous states to detect when the board has entered a loop state.
- Implement a more efficient update algorithms that uses a bitmap and bitshift operations to operate on multiple tiles directly.
