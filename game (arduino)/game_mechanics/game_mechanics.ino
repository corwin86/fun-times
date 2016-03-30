#include <Adafruit_NeoPixel.h>

#define PIN 6

#define DEBUG 1

Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, PIN, NEO_GRB + NEO_KHZ800);

/*colours*/
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t magenta = strip.Color(255, 0, 255);
uint32_t cyan = strip.Color(0, 255, 255);
uint32_t yellow = strip.Color(255, 255, 0);

uint32_t gold = strip.Color(255, 215, 0);
uint32_t webworkGreen = strip.Color(118, 255, 122);
uint32_t white = strip.Color(255, 255, 255);
uint32_t off = strip.Color(0, 0, 0);

uint32_t victoryBlink;

// ==== Constants ====

/* ---- hardware dependent ---- */
const int MAX_BOARD_X = 8,
          MAX_BOARD_Y = 8,
          LED_BRIGHTNESS = 25;
/* -- end hardware dependent -- */

/* ---- game ids ---- */
const int CONNECT_FOUR = 0;
/* -- end game ids -- */

/* ---- game parameters ---- */
const int P1 = 1,          //
          P2 = 2,          // tokens used on board
          AI = 2,           //
          EMPTY_CELL = 0;  //
const int X_DIM = 8,
          Y_DIM = 8,
          SEQ_LENGTH = 5;
const int BAD_MOVE = -1000,
          GOOD_MOVE = 1000,
          IMPOSSIBLE_MOVE = -2000;
const int TOKEN_DROP_SPEED = 35; // millis delay for token dropping
/* -- end game parameters -- */

/* ---- error states ---- */
const int FN_SUCCESS = 1,
          ERR_GENERIC_FAIL = -1,
          ERR_INVALID_GAMEID = -2;
/* -- end error states -- */

/* ---- directions ---- */
const int UP = 1,
          LEFT = 2,
          UP_LEFT = 3,
          DOWN_LEFT = 4;
/* -- end directions -- */

// == End Constants ==


// ==== Global Variables ====

/* ---- player info ---- */
uint32_t p1_colour = blue,
         p2_colour = green;
int turns;
/* -- end player info -- */

/* ---- gane parameters ---- */
int   isSinglePlayer = 1,
      difficulty = 2;         //AI recursion depth
/* -- end gane parameters -- */

// == End Global Variables ==

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(4));

  //buffer for parsing
  char appBuffer[128] = {0};

  //initialize LED array
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show();

#if !DEBUG
  startupLEDSequence();
#endif
}

void loop() {
  int gameId = 0; //!!! if time allows, input from bluetooth app
  int e = runGame(gameId);

  // error handle
  switch (e) {
    case FN_SUCCESS : {
        break;
      }
    case ERR_INVALID_GAMEID : {
        Serial.print("** ERR - INVALID GAME ID in loop(): "); Serial.println(gameId);
        break;
      }
    case ERR_GENERIC_FAIL : {
        Serial.println("** ERR - GENERIC FAIL: runGame(gameId) failed");
        break;
      }
  }

  //!!! need loop/replay handling? -> TBD
}

int runGame(int gameId) {
  switch (gameId) {
    case CONNECT_FOUR : {
        int e = playConnectFour();

        // error handle
        if (e == 1) {
          return FN_SUCCESS;
        } else {
          return ERR_GENERIC_FAIL;
        }
        break;
      }
    //!!! add cases per new game
    default : {
        // gameId did not match any library games
        return ERR_INVALID_GAMEID;
      }
  }
}

int playConnectFour() {
  // Create board
  int **board = createBoard();

  // initialize board as empty (no tokens until users start dropping)
  int x, y;
  for (y = 0; y < Y_DIM; y++) {
    for (x = 0; x < X_DIM; x++) {
      board[y][x] = EMPTY_CELL;
    }
  }

  printBoard(board);

  //---- gameplay ----
  int cur_player = P1; //cur_player is one of {P1, P2}
  turns = 0;
  while (!gameOver_connect4(board)) {
    //!!! game logic not done
    int col;
    do {
      //!!! -- take input --
      //!!! use serial for debug

      //      !!! AI vs AI
      //      if(cur_player == P1) {
      //        col = aiNextMove(board, P1, P2);
      //      } else {
      //        col = aiNextMove(board, P2, P1);
      //      }

      if (isSinglePlayer && cur_player == P2) {
        col = aiNextMove(board, AI, P1);
      } else {
        //#if !DEBUG
        while (!Serial.available()); //wait for serial data before parsing
        col = Serial.parseInt();
        Serial.println(col);

        // clamp inputs to valid range
        col = col < 0 ? 0 : col >= X_DIM ? X_DIM - 1 : col;
      }
    } while (!dropToken_connect4(board, col, cur_player, 1));

    printBoard(board);

    cur_player = cur_player == P1 ? P2 : P1; //change turns
    turns++;
  }
  //-- end gameplay --

  //!!! perhaps delay this call for user to confirm end game
  connect4Cascade(board); //GUI feature indicates game end, ***CLEARS BOARD***

  // free malloc'd memory
  freeBoard(board);

  return FN_SUCCESS; //game completed successfully
}

/*
    Place a player token on board in column x !!!untested
*/
int dropToken_connect4(int **board, int col, int token, int animated) {
  int y = -1;
  while (board[y + 1][col] == EMPTY_CELL && y + 1 < Y_DIM) {
    y++;
  }

  if (y == -1) { //no empty cells in column col
    return 0;
  }

  if (animated) {
    uint32_t colour = getPlayerColour(token);

    int i = -2;
    while (++i < y - 1) {
      delay(TOKEN_DROP_SPEED);
      printCell(col, i, off, 0);
      printCell(col, i + 1, colour, 0);
      strip.show();
    }
  }

  board[y][col] = token; //place token

  return 1; //successfully dropped token
}

/*
    Return true if a player has won, or a draw is reached (board is full)
*/
int gameOver_connect4(int **board) {
  return winningPlayer_connect4(board, 1) > 0 || boardFull();
}

/*
    Graphical sequence replicating pulling bottom out of connect 4 board
    Modifies board: replaces all user tokens with EMPTY_CELL
    !!!untested!!!
*/
void connect4Cascade(int **board) {
  Serial.print("Cascade\n");
  int x, y, count;

  for (count = 0; count < Y_DIM; count++) {
    for (y = Y_DIM - 1; y >= count; y--) {
      for (x = 0; x < X_DIM; x++) {
        if (y == 0) {
          board[y][x] = EMPTY_CELL;
        } else {
          board[y][x] = board[y - 1][x];
        }
      }
    }

    printBoard(board);
    delay(TOKEN_DROP_SPEED);
  }
}

/*
    Return player ID of winning player, 0 if none

    Modifies board: if winning player found, flashes winning
      tiles to webwork green 3 times

    !!!!!!NOTE: MUST SPECIFY WIN DATA!!!!!!!
*/
int winningPlayer_connect4(int **board, int flash) {
  int winData[4] = {0};
  int flashes = 3 * 2;

  detectDiagLine(P1, SEQ_LENGTH, board, winData);
  if (winData[0] == -1) {
    detectHorizLine(P1, SEQ_LENGTH, board, winData);
    if (winData[0] == -1) {
      detectVertLine(P1, SEQ_LENGTH, board, winData);
      if (winData[0] == -1) {
        detectDiagLine(P2, SEQ_LENGTH, board, winData);
        if (winData[0] == -1) {
          detectHorizLine(P2, SEQ_LENGTH, board, winData);
          if (winData[0] == -1) {
            detectVertLine(P2, SEQ_LENGTH, board, winData);
            if (winData[0] == -1) {
              return 0;
            }
          }
        }
      }
    }
  }

  //flashes 3 times
  if (flash) {
    delay(500);
    int x = winData[0], y = winData[1];
    if (p1_colour != green) {
      if (p2_colour != green) {
        victoryBlink = green;  //!green vs !green
      } else {
        if (p1_colour != red) {
          victoryBlink = red;  //!red vs green
        } else {
          victoryBlink = blue; //red vs green
        }
      }
    } else {
      if (p2_colour != red) {
        victoryBlink = red;  //green vs !red
      } else {
        victoryBlink = blue; //green vs red
      }
    }

    while (flashes-- > 0) {
      int i = -1;
      while (i++ < SEQ_LENGTH - 1) {
        uint32_t colour = flashes % 2 ? victoryBlink : getPlayerColour(winData[3]);
        switch (winData[2]) {
          case UP:
            printCell(x    , y + i, colour, 0); break;
          case LEFT:
            printCell(x + i, y    , colour, 0); break;
          case UP_LEFT:
            printCell(x + i, y + i, colour, 0); break;
          case DOWN_LEFT:
            printCell(x + i, y - i, colour, 0); break;
          default:
            Serial.print("ERROR: check line detecting logic: Win Data is "); Serial.println(winData[2]); break;
        }
      }
      strip.show();
      delay(500);
    }
  }

  return winData[3];
}

/*
   Detects if there is a vertical sequence of one player's tokens of length len.
   If one is found, it returns the coordinates of the first token, as well as the direction
   of the sequence, in the form:

   -> returnArray[] = {x, y, dir, player}

   If no such sequence is found, returns NULL
*/
void detectVertLine(int player, int len, int** board, int *coordAndDir) {
  int x, y, count;
  for (y = 0; y <= Y_DIM - len; y++) {                          //iterate through bottom half of grid
    for (x = 0; x < X_DIM; x++) {
      for (count = 0; count < len; count++) {                   //count that you have a sequence of length len
        if (board[y + count][x] != player)
          break;     //you're not on the right track, try a different sequence
      }
      if (count == len) {
        //we found a sequence of length len, so we return coords/direction
        coordAndDir[0] = x; coordAndDir[1] = y; coordAndDir[2] = UP; coordAndDir[3] = player;
        Serial.print("x: "); Serial.print(x); Serial.print(" y: "); Serial.println(y);
        return;
      }
    }
  }
  coordAndDir[0] = -1; coordAndDir[1] = -1; coordAndDir[2] = -1; coordAndDir[3] = -1;
}

/*
   Detects if there is a horizontal sequence of one player's tokens of length len.
   If one is found, it returns the coordinates of the first token, as well as the direction
   of the sequence, in the form:

   -> returnArray[] = {x, y, dir, player}

   If no such sequence is found, returns NULL
*/
void detectHorizLine(int player, int len, int** board, int *coordAndDir) {
  int x, y, count;

  for (x = 0; x <= X_DIM - len; x++) {                          //iterate through left half of grid
    for (y = 0; y < Y_DIM; y++) {
      for (count = 0; count < len; count++) {                   //count that you have a sequence of length len
        if (board[y][x + count] != player)
          break;     //you're not on the right track, try a different sequence
      }
      if (count == len) {
        //we found a sequence of length len, so we return coords/direction
        coordAndDir[0] = x; coordAndDir[1] = y; coordAndDir[2] = LEFT; coordAndDir[3] = player;
        return;
      }
    }
  }
  coordAndDir[0] = -1; coordAndDir[1] = -1; coordAndDir[2] = -1; coordAndDir[3] = -1;
}

/*
   Detects if there is a diagonal sequence of one player's tokens of length len.
   If one is found, it returns the coordinates of the first token, as well as the direction
   of the sequence, in the form:

   -> returnArray[] = {y, x, dir, player}

   If no such sequence is found, returns NULL
*/
void detectDiagLine(int player, int len, int** board, int *coordAndDir) {
  int x, y, count;

  for (y = 0; y <= Y_DIM - len; y++) {                          //iterate through all of bottom left corner of grid
    for (x = 0; x <= X_DIM - len; x++) {
      for (count = 0; count < len; count++) {                   //count that you have a sequence of length len
        if (board[y + count][x + count] != player)
          break;     //you're not on the right track, try a different sequence
      }

      if (count == len) {
        //we found a sequence of length len, so we return coords/direction
        coordAndDir[0] = x; coordAndDir[1] = y; coordAndDir[2] = UP_LEFT; coordAndDir[3] = player;
        return;
      }
    }
  }
  for (y = len - 1; y < Y_DIM; y++) {                           //iterate through all of top left corner of grid
    for (x = 0; x <= X_DIM - len; x++) {
      for (count = 0; count < len; count++) {                   //count that you have a sequence of length len
        if (board[y - count][x + count] != player)
          break;     //you're not on the right track, try a different sequence
      }
      if (count == len) {
        //we found a sequence of length len, so we return coords/direction
        coordAndDir[0] = x; coordAndDir[1] = y; coordAndDir[2] = DOWN_LEFT; coordAndDir[3] = player;
        return;
      }
    }
  }
  coordAndDir[0] = -1; coordAndDir[1] = -1; coordAndDir[2] = -1; coordAndDir[3] = -1;
}

/*
    Returns 0 if board contains 1+ empty cells
*/
int boardFull() {
  return turns >= X_DIM * Y_DIM;
}

/*
   Returns column number for AI's move
*/
int aiNextMove(int **board, int aiToken, int otherToken) {
  int ai_col, p_col;
  int next_move = -1;
  int col_points[X_DIM] = {0};

  for (ai_col = 0; ai_col < X_DIM; ai_col++) {
    if (board[0][ai_col] != EMPTY_CELL) {
      col_points[ai_col] = IMPOSSIBLE_MOVE;
      continue;
    }
    dropToken_connect4(board, ai_col, aiToken, 0);

    if (winningPlayer_connect4(board, 0) == aiToken) {
      return ai_col; //win if you can
    } else {
      for (p_col = 0; p_col < X_DIM; p_col++) {
        if (board[0][p_col] != EMPTY_CELL) {
          continue;
        }
        int p_break = 0;
        dropToken_connect4(board, p_col, otherToken, 0);

        if (winningPlayer_connect4(board, 0) == otherToken) {
          col_points[ai_col] = BAD_MOVE;
          p_break = 1;
        } else {
          //!!! expand recursively, time permitting
        }
        popToken(board, p_col); //remove board modification

        if (p_break) {
          break;
        }
      }
    }
    popToken(board, ai_col); //remove board modifications

    if (next_move != -1) {
      return next_move;
    }
  }

  return selectNextMove(col_points); //random column
}

/*
    Example of call to aiRecursiveSearch
*/
int aiExampleCall(int **board) {
  int col_points[X_DIM] = {0};
  aiRecursiveSearch(board, 1, col_points);
  return selectNextMove(col_points);
}

/*
    Searches through possible moves recursively by checking a number of
    moves in the future. !!!!!!!!!!!!!!!!! 1st attempt !!!!!!!!!!!!!!!!!

    level - how many turns in advance the function is checking
    board - the board

    Modifies: col_points - the current number of points for each column, changes through recursive calls
*/
void aiRecursiveSearch(int **board, int level, int *col_points) {
  int ai_col, p_col;

  //note: num points assigned equal to 1000/level.
  //ex. if it can win 3 moves in advance but can lose in 2 moves, assign 333 - 500 = -167 points.

  for (ai_col = 0; ai_col < X_DIM; ai_col++) {
    if (board[0][ai_col] != EMPTY_CELL) {
      col_points[ai_col] = IMPOSSIBLE_MOVE;
      continue;
    }
    dropToken_connect4(board, ai_col, P2, 0);

    if (winningPlayer_connect4(board, 0) == AI) {
      col_points[ai_col] += GOOD_MOVE / level;        //future win with this move

    } else {
      for (p_col = 0; p_col < X_DIM; p_col++) {
        if (board[0][p_col] != EMPTY_CELL) {
          continue;
        }
        int p_break = 0;
        dropToken_connect4(board, p_col, P1, 0);

        if (winningPlayer_connect4(board, 0) == P1) {
          col_points[ai_col] += BAD_MOVE / (level + 1); //future loss with this move, but one level deeper than ai move !!!!!!!could be problematic, overlap with next level!!!!!!
          p_break = 1;                                  //!!!!!still necessary with recursive search????????
        } else if (level++ < difficulty) {                     //decrement level and call again, otherwise start popping tokens and returning
          aiRecursiveSearch(board, level, col_points);
        }
        popToken(board, p_col); //remove board modification

        if (p_break) {
          break;        //!!!!!still necessary with recursive search????????
        }
      }
    }
    popToken(board, ai_col); //remove board modifications
  }
}


int selectNextMove(int *col_points) {
  int i;
  int next_move = 0, ties = 0;
  for (i = 1; i < X_DIM; i++) {
    if (col_points[i] != IMPOSSIBLE_MOVE && col_points[i] > col_points[next_move]) {
      next_move = i;
      ties = 0;
    } else if (col_points[i] == col_points[next_move]) {
      ties++;
    }
  }

  ties = random(0, ties);

  Serial.println(ties);

  i = 0;
  while (ties > 0) {
    if (col_points[i] == col_points[next_move]) {
      ties--;
      next_move = i;
    }
    if (i == X_DIM) {
      i = 0;
    }
    i++;
  }

  return next_move;
}

void popToken(int **board, int col) {
  int y = 0;
  while (board[y][col] == EMPTY_CELL && y < Y_DIM) {
    y++;
  }

  board[y][col] = EMPTY_CELL;
}

/*
    Allocate a x_dim by y_dim board to be used in a game
*/
int ** createBoard() {
  int **board = (int **)malloc(Y_DIM * sizeof(int *));
  int x, y; // loop indeces
  for (y = 0; y < Y_DIM; y++) {
    board[y] = (int *)malloc(X_DIM * sizeof(int));
  }

  return board;
}

/*
    Free an x_dim by (irrelevant) board's allocated memory
*/
void freeBoard(int **board) {
  int y;
  for (y = 0; y < Y_DIM; y++) {
    free(board[y]);
  }
  free(board);
}

/*
    Output current state of a board
*/
void printBoard(int **board) {
  //!!! to be changed to print to LEDs
  //    uses serial for dev
  int x, y;
  for (y = 0; y < Y_DIM; y++) {
    for (x = 0; x < X_DIM; x++) {
      //Serial.print(board[y][x]); Serial.print("\t");
      printCell(x, y, getPlayerColour(board[y][x]), 0);
    }
    //Serial.println();
  }
  //Serial.println();

  strip.show();
}

/*
   given a row,column and color light up specified led
*/
void printCell(int x, int y, uint32_t color, int show) {
  int led = X_DIM * y + x;
  strip.setPixelColor(led, color); //sets colour of first pixel
  if (show) {
    strip.show(); //'pushes' colour data to the strip
  }
}

/*
    Displays a welcome/startup pattern on the LEDs
*/
void startupLEDSequence() {
  int i = 0;

  uint32_t square1 = blue;
  uint32_t square2 = cyan;
  uint32_t square3 = green;
  uint32_t square4 = yellow;

  //uint32_t[] colors = red, magenta, green, blue;

  for (i = 0; i < 3 * 4; i++) { //while(i == 0){
    //printsquare2
    printCell(3, 3, square1 , 1);
    printCell(4, 3, square1 , 1);
    printCell(3, 4, square1 , 1);
    printCell(4, 4, square1 , 1);

    //print square 2
    printCell(2, 2, square2 , 1);
    printCell(2, 3, square2 , 1);
    printCell(2, 4, square2 , 1);
    printCell(2, 5, square2 , 1);

    printCell(3, 2, square2 , 1);
    printCell(4, 2, square2 , 1);
    printCell(3, 5, square2 , 1);
    printCell(4, 5, square2 , 1);

    printCell(5, 2, square2 , 1);
    printCell(5, 3, square2 , 1);
    printCell(5, 4, square2 , 1);
    printCell(5, 5, square2 , 1);

    //print square 3
    printCell(1, 1, square3 , 1);
    printCell(1, 2, square3 , 1);
    printCell(1, 3, square3 , 1);
    printCell(1, 4, square3 , 1);
    printCell(1, 5, square3 , 1);
    printCell(1, 6, square3 , 1);

    printCell(2, 1, square3 , 1);
    printCell(3, 1, square3 , 1);
    printCell(4, 1, square3 , 1);
    printCell(5, 1, square3 , 1);
    printCell(2, 6, square3 , 1);
    printCell(3, 6, square3 , 1);
    printCell(4, 6, square3 , 1);
    printCell(5, 6, square3 , 1);

    printCell(6, 1, square3 , 1);
    printCell(6, 2, square3 , 1);
    printCell(6, 3, square3 , 1);
    printCell(6, 4, square3 , 1);
    printCell(6, 5, square3 , 1);
    printCell(6, 6, square3 , 1);

    //print square 4
    printCell(0, 0, square4 , 1);
    printCell(0, 1, square4 , 1);
    printCell(0, 2, square4 , 1);
    printCell(0, 3, square4 , 1);
    printCell(0, 4, square4 , 1);
    printCell(0, 5, square4 , 1);
    printCell(0, 6, square4 , 1);
    printCell(0, 7, square4 , 1);

    printCell(1, 0, square4 , 1);
    printCell(2, 0, square4 , 1);
    printCell(3, 0, square4 , 1);
    printCell(4, 0, square4 , 1);
    printCell(5, 0, square4 , 1);
    printCell(6, 0, square4 , 1);
    printCell(1, 7, square4 , 1);
    printCell(2, 7, square4 , 1);
    printCell(3, 7, square4 , 1);
    printCell(4, 7, square4 , 1);
    printCell(5, 7, square4 , 1);
    printCell(6, 7, square4 , 1);

    printCell(7, 0, square4 , 1);
    printCell(7, 1, square4 , 1);
    printCell(7, 2, square4 , 1);
    printCell(7, 3, square4 , 1);
    printCell(7, 4, square4 , 1);
    printCell(7, 5, square4 , 1);
    printCell(7, 6, square4 , 1);
    printCell(7, 7, square4 , 1);

    //switch colors
    uint32_t temp = square1;
    square1 = square4;
    square4 = square3;
    square3 = square2;
    square2 = temp;

    delay(250);
  }

  for (i = 0; i < 64; i++) {
    strip.setPixelColor(i + 3, yellow);
    strip.setPixelColor(i + 2, green);
    strip.setPixelColor(i + 1, cyan);
    strip.setPixelColor(i    , blue);
    delay(10);
    strip.show();
  }

  for (i = 0; i < 64; i++) {
    strip.setPixelColor(i + 3, yellow);
    strip.setPixelColor(i + 2, green);
    strip.setPixelColor(i + 1, cyan);
    if ((i > 9 && i < 14) || i == 18 || (i > 25 && i < 29) || i == 37 || i == 45 || (i > 49 && i < 53)) {
      strip.setPixelColor(i, blue);
    } else {
      strip.setPixelColor(i, off);
    }

    delay(10);
    strip.show();
  }

  delay(5000);

  for (int j = 0; j < X_DIM * Y_DIM; j++) {
    strip.setPixelColor(j, off);
    strip.show();
  }
}

/*
   Returns the colour of the player
*/
uint32_t getPlayerColour(int player) {
  if (player == P1)
    return p1_colour;
  else if (player == P2)
    return p2_colour;
  else
    return off;
}

int calculateLedPosition(int x, int y) {
  int led = 8 * y + x;
  return led;
}
