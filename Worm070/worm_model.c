#include <curses.h>
#include "worm.h"
#include "board_model.h"
#include "worm_model.h"

extern enum ResCodes initializeWorm(struct worm* aworm, int len_max, int len_cur, struct pos headpos, enum WormHeading dir, enum ColorPairs color) {
    // Initialize last usable index to len_max -1
    // theworm_headindex
    aworm->maxindex = len_max -1;

    //Current last usable index
    aworm->cur_lastindex = len_cur -1;

    // Initialize headindex
    // theworm_headindex
    aworm->headindex = 0;

    // Mark all elements as unused in the arrays of positions
    // theworm_wormpos_y[] and theworm_wormpos_x[]
    // with code UNUSED_POS_ELEM
    for(int i = 1; i < WORM_LENGTH; i++){
        aworm->wormpos[i].y = UNUSED_POS_ELEM;
        aworm->wormpos[i].x = UNUSED_POS_ELEM;
    }
    
    // Initialize position of worms head
    aworm->wormpos[aworm->headindex] = headpos;

    // Initialize the heading of the worm
    setWormHeading(aworm, dir);

    // Initialze color of the worm
    aworm->wcolor = color;

    return RES_OK;
}

// Show the worms's elements on the display
// Simple version
extern void showWorm(struct board* aboard, struct worm* aworm) {
    int i = aworm->headindex;
    int tailindex = (aworm -> headindex + 1) % (aworm -> cur_lastindex + 1);

    do {
      if (i == aworm->headindex) {
          placeItem(aboard, 
            aworm->wormpos[i].y,
            aworm->wormpos[i].x, 
            BC_USED_BY_WORM,
            SYMBOL_WORM_HEAD_ELEMENT, 
            aworm->wcolor);
      }
      else if (i == tailindex) {
          placeItem(aboard, 
            aworm->wormpos[i].y, 
            aworm->wormpos[i].x,
            BC_USED_BY_WORM, 
            SYMBOL_WORM_TAIL_ELEMENT, 
            aworm->wcolor);
      } 
      else {
          placeItem(aboard, 
            aworm->wormpos[i].y, 
            aworm->wormpos[i].x,
            BC_USED_BY_WORM, 
            SYMBOL_WORM_INNER_ELEMENT, 
            aworm->wcolor);
      }
      i = (i + aworm -> cur_lastindex) % (aworm -> cur_lastindex + 1);
    } while (i != aworm->headindex && aworm->wormpos[i].y != UNUSED_POS_ELEM);
}

extern void cleanWormTail(struct board* aboard, struct worm* aworm){
    int tailIndex;
    tailIndex = (aworm->headindex +1) % (aworm->cur_lastindex +1);
    if(aworm->wormpos[tailIndex].y != UNUSED_POS_ELEM){
        placeItem(
            aboard,
            aworm->wormpos[tailIndex].y,
            aworm->wormpos[tailIndex].x,
            BC_FREE_CELL,
            SYMBOL_FREE_CELL,
            COLP_FREE_CELL
        );
    }
}

extern void moveWorm(struct board* aboard, struct worm* aworm, enum GameStates* agame_state) {
    struct pos headpos;
    // Compute and store new head position according to current heading.
    headpos = aworm->wormpos[aworm->headindex];
    headpos.y += aworm->dy;
    headpos.x += aworm->dx;

    // Check if we would leave the display if we move the worm's head according
    // to worm's last direction.
    // We are not allowed to leave the display's window.
    if (headpos.x < 0) {
        *agame_state = WORM_OUT_OF_BOUNDS;
    } 
    else if (headpos.x > getLastColOnBoard(aboard)) { 
        *agame_state = WORM_OUT_OF_BOUNDS;
    } 
    else if (headpos.y < 0) {  
        *agame_state = WORM_OUT_OF_BOUNDS;
	} 
    else if (headpos.y > getLastRowOnBoard(aboard)) {
        *agame_state = WORM_OUT_OF_BOUNDS;
    } 
    else {
        // We will stay within bounds.
        //Check if worm is hitting any (bad) objects
        switch(getContentAt(aboard, headpos)){
            case BC_FOOD_1:
                *agame_state = WORM_GAME_ONGOING;
                //Grow worm according to food
                growWorm(aworm, BONUS_1);
                decrementNumberOfFoodItems(aboard);
                break;
            
            case BC_FOOD_2:
                *agame_state = WORM_GAME_ONGOING;
                //Grow according to food
                growWorm(aworm, BONUS_2);
                decrementNumberOfFoodItems(aboard);
                break;

            case BC_FOOD_3:
                *agame_state = WORM_GAME_ONGOING;
                //Grow according to food
                growWorm(aworm, BONUS_3);
                decrementNumberOfFoodItems(aboard);
                break;

            case BC_BARRIER:
                *agame_state = WORM_CRASH;
                break;
            
            case BC_USED_BY_WORM:
                *agame_state = WORM_CROSSING;
                break;

            default:
                {;}     //do nothing
        }
    }

    if(*agame_state == WORM_GAME_ONGOING){
        //update headindex
        aworm->headindex++;

        if(aworm->headindex > aworm->cur_lastindex){
            aworm->headindex = 0;
        }
        aworm->wormpos[aworm->headindex] = headpos;
    }
}

extern void setWormHeading(struct worm* aworm, enum WormHeading dir) {
    switch(dir) {
        case WORM_UP :
            aworm->dx = 0;
            aworm->dy = -1;
            break;
        case WORM_DOWN :
            aworm->dx = 0;
            aworm->dy = 1;
            break;
        case WORM_LEFT :
            aworm->dx = -1;
            aworm->dy = 0;
            break;
        case WORM_RIGHT :
            aworm->dx = 1;
            aworm->dy = 0;
            break;
    }
}

extern struct pos getWormHeadPos(struct worm* aworm){
    return aworm->wormpos[aworm->headindex];
}

int getWormLength(struct worm* aworm){
    return aworm->cur_lastindex + 1;
}

void growWorm(struct worm* aworm, enum Boni growth){
    //case for cuurent_last_index to surpass maxindex
    /*int dif;
    int i = aworm->cur_lastindex;
    int latestTailIndex = (aworm->headindex +1) % (aworm->cur_lastindex +1); 
    int cmpy = (latestTailIndex == i) ? (aworm->wormpos[latestTailIndex].y - aworm->wormpos[0].y) : (aworm->wormpos[latestTailIndex].y - aworm->wormpos[latestTailIndex+1].y);
    int cmpx = (latestTailIndex == i) ? (aworm->wormpos[latestTailIndex].x - aworm->wormpos[0].x) : (aworm->wormpos[latestTailIndex].x - aworm->wormpos[latestTailIndex+1].x);
    */
    //case for curent_last_index to surpass maxindex
    if(aworm->cur_lastindex + growth <= aworm->maxindex){
        //dif = growth;
        aworm->cur_lastindex += growth;
    }
    else{
        //dif = aworm->maxindex - aworm->cur_lastindex;
        aworm->cur_lastindex = aworm->maxindex;
    }
    /*
    do{
        aworm->wormpos[i] = aworm->wormpos[i + dif];

        if(i != aworm->headindex){
            break;
        }
        i = (i + aworm->cur_lastindex) % (aworm->cur_lastindex +1);
    }while(1);

    aworm->headindex = aworm->headindex + dif;
    
    //Herausfinden der Auffüllrichtung
    switch(cmpy){
        case 1:
            switch(cmpx){
                case 1:
                //Fehler
                break;

                case 0:
                    for(int iterate = 1; iterate <= dif; iterate++){
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                    }
                    break;

                case -1:
                //Fehler
                break;

                default:
                //Kann nicht eintreten außer bei einem internen Fehler
                {;}
            }
        break;

        case 0:
            switch(cmpx){
                case 1:
                    for(int iterate = 1; iterate <= dif; iterate++){
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                    }                    
                break;

                case 0:
                //Fehler
                break;

                case -1:
                    for(int iterate = 1; iterate <= dif; iterate++){
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                    }                    
                break;

                default:
                 //Kann nicht eintreten außer bei einem internen Fehler
                {;}
            }
        break;

        case -1:
            switch(cmpx){
                case 1:
                //Fehler
                break;

                case 0:
                    for(int iterate = 1; iterate <= dif; iterate++){
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                        aworm->wormpos[latestTailIndex + iterate].y = aworm->wormpos[latestTailIndex].y + (cmpy*iterate);
                    }                    
                break;

                case -1:
                //Fehler
                break;

                default:
                //Kann nicht eintreten außer bei einem internen Fehler
                {;}
            }
        break;

        default:
        //Kann nicht eintreten außer bei einem internen Fehler
        {;}
    }*/
}
