#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "prep.h"
#include "worm.h"
#include "worm_model.h"
#include "board_model.h"
#include "messages.h"

// ************************************
// Management of the game
// ************************************

// Initialize colors of the game
void initializeColors() {
    // Define colors of the game
    start_color();
    init_pair(COLP_USER_WORM, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLP_FREE_CELL, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLP_FOOD_1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLP_FOOD_2, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLP_FOOD_3, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLP_BARRIER, COLOR_RED, COLOR_BLACK);
}

void readUserInput(struct worm* aworm, enum GameStates* agame_state ) {
    int ch; // For storing the key codes

    if ((ch = getch()) > 0) {
        // Is there some user input?
        // Blocking or non-blocking depends of config of getch
        switch(ch) {
            case 'q' :    // User wants to end the show
                *agame_state = WORM_GAME_QUIT;
                break;
            case KEY_UP :// User wants up
                setWormHeading(aworm, WORM_UP);
                break;
            case KEY_DOWN :// User wants down
                setWormHeading(aworm, WORM_DOWN);
                break;
            case KEY_LEFT :// User wants left
                setWormHeading(aworm, WORM_LEFT);
                break;
            case KEY_RIGHT :// User wants right
                setWormHeading(aworm, WORM_RIGHT);
                break;
            case 's' : // User wants single step
                nodelay(stdscr, TRUE);  // We simply make getch blocking
                break;
            case ' ' : // Terminate single step; make getch non-blocking again
                nodelay(stdscr, FALSE);   // Make getch non-blocking again
                break;
            case 'g': //For development: Let worm grow
                growWorm(aworm, BONUS_3);
                break;
        }
    }
    return;
}

enum ResCodes doLevel() {
    struct worm userworm;       //current worm
    struct board theboard;      //The game board
    enum GameStates game_state; // The current game_state

    enum ResCodes res_code; // Result code from functions
    bool end_level_loop;    // Indicates whether we should leave the main loop

    struct pos bottomLeft;   // Start positions of the worm

    // At the beginnung of the level, we still have a chance to win
    game_state = WORM_GAME_ONGOING;

    //Setup the board
    res_code = initializeBoard(&theboard);
    if(res_code != RES_OK){
        return res_code;
    }

    //initialize the current level
    res_code = initializeLevel(&theboard);
    if(res_code != RES_OK){
        return res_code;
    }

    // There is always an initialized user worm.
    // Initialize the userworm with its size, position, heading.
    bottomLeft.y =  getLastRowOnBoard(&theboard);
    bottomLeft.x =  0;

    res_code = initializeWorm(&userworm, WORM_LENGTH, WORM_INITIAL_LENGTH, bottomLeft, WORM_RIGHT, COLP_USER_WORM);
    if ( res_code != RES_OK) {
        return res_code;
    }
    
    // Show worm at its initial position
    showWorm(&theboard, &userworm);

    // Display all what we have set up until now
    refresh();

    // Start the loop for this level
    end_level_loop = false; // Flag for controlling the main loop
    while(end_level_loop == false) {
        // Process optional user input
        readUserInput(&userworm, &game_state); 
        if ( game_state == WORM_GAME_QUIT ) {
            end_level_loop = true;
            continue; // Go to beginning of the loop's block and check loop condition
        }

        // Process userworm
        // Now move the worm for one step
        cleanWormTail(&theboard, &userworm);
        moveWorm(&theboard, &userworm, &game_state);
        // Bail out of the loop if something bad happened
        if ( game_state != WORM_GAME_ONGOING ) {
            end_level_loop = true;
            continue; // Go to beginning of the loop's block and check loop condition
        }
        // Show the worm at its new position
        showWorm(&theboard, &userworm);
        // END process userworm

        showStatus(&theboard, &userworm);

        // Sleep a bit before we show the updated window
        napms(NAP_TIME);

        // Display all the updates
        refresh();

        //are we done with that level?
        if(getNumberOfFoodItems(&theboard) == 0){
            end_level_loop = true;
        }
        // Start next iteration
    }

    // Preset res_code for rest of the function
    res_code = RES_OK;

    // For some reason we left the control loop of the current level.
    // Check why
    switch(game_state){
        case WORM_GAME_ONGOING:
            if(getNumberOfFoodItems(&theboard) == 0){
                showDialog("WINNNNNNNNNNNER", "Bitte Taste druecken");
            }
            else{
                showDialog("Interner Fehler!", "Bitte Taste druecken");
            }
            break;

        case WORM_GAME_QUIT:
            showDialog("...", "Bitte Taste druecken");
            break;

        case WORM_CRASH:
            showDialog("Hinderniss gerammt", "Bitte Taste drücken");
            break;
        
        case WORM_OUT_OF_BOUNDS:
            showDialog("Noob", "Bitte Taste druecken");
            break;

        case WORM_CROSSING:
            showDialog("NOOOOOOOOOOOB", "Bitte Taste druecken");
            break;

        default:
            showDialog("", "Bitte Taste druecken");
            res_code = RES_INTERNAL_ERROR;
    }
    return res_code;
}

// ********************************************************************************************
// MAIN
// ********************************************************************************************

int main(void) {

    enum ResCodes res_code;         // Result code from functions

    // Here we start
    initializeCursesApplication();  // Init various settings of our application
    initializeColors();             // Init colors used in the game

    // Maximal LINES and COLS are set by curses for the current window size.
    // Note: we do not cope with resizing in this simple examples!

    // Check if the window is large enough to display messages in the message area
    // a has space for at least one line for the worm
    if ( LINES < MIN_NUMBER_OF_ROWS + ROWS_RESERVED || COLS < MIN_NUMBER_OF_COLS ) {
        // Since we not even have the space for displaying messages
        // we print a conventional error message via printf after
        // the call of cleanupCursesApp()
        cleanupCursesApp();
        printf("Das Fenster ist zu klein: wir brauchen mindestens %dx%d\n",
                MIN_NUMBER_OF_COLS, MIN_NUMBER_OF_ROWS + ROWS_RESERVED);
        res_code = RES_FAILED;
    } 
    else {
        res_code = doLevel();
        cleanupCursesApp();
    }

    return res_code;
}
