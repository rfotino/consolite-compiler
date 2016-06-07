/**
 * Tetris in Consolite C.
 * Uses on-board buttons or 1st SNES controller as input.
 *
 * @author Robert Fotino, 2016
 */

/**
 * CONSTANTS (these should be #defines, but there is no preprocessor yet)
 */
uint16[2] KEY_SPACE = { 0, 46 };
uint16[2] KEY_UP    = { 1, 54 };
uint16[2] KEY_LEFT  = { 2, 52 };
uint16[2] KEY_DOWN  = { 3, 51 };
uint16[2] KEY_RIGHT = { 4, 53 };

uint16 NULL_PIECE = 0;
uint16 I_PIECE    = 1;
uint16 O_PIECE    = 2;
uint16 T_PIECE    = 3;
uint16 J_PIECE    = 4;
uint16 L_PIECE    = 5;
uint16 S_PIECE    = 6;
uint16 Z_PIECE    = 7;

uint16 true = 1;
uint16 false = 0;

uint16[8] PIECE_COLORS = { 0b00000000,   // black
                           0b00011111,   // cyan
                           0b11111100,   // yellow
                           0b11100011,   // magenta
                           0b00000011,   // blue
                           0b11110000,   // orange
                           0b00011100,   // green
                           0b11100000 }; // red

/**
 * The pieces, stored as (x0, y0, x1, y1, x2, y2, x3, y3, width, height)
 * tuples each corresponding to a piece type.
 */
uint16[80] PIECES = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // NULL_PIECE
                      0, 0, 0, 1, 0, 2, 0, 3, 1, 4,   // I_PIECE
                      0, 0, 0, 1, 1, 0, 1, 1, 2, 2,   // O_PIECE
                      1, 0, 0, 1, 1, 1, 2, 1, 3, 2,   // T_PIECE
                      1, 0, 1, 1, 1, 2, 0, 2, 2, 3,   // J_PIECE
                      0, 0, 0, 1, 0, 2, 1, 2, 2, 3,   // L_PIECE
                      1, 0, 2, 0, 0, 1, 1, 1, 3, 2,   // S_PIECE
                      0, 0, 1, 0, 1, 1, 2, 1, 3, 2 }; // Z_PIECE

/**
 * Bitmaps for digits 0-9 in 3x5 format.
 * For instance 0x7b6f == 0b111101101101111 ==
 * 111
 * 101
 * 101
 * 101
 * 111
 */
uint16[10] DIGIT_BITMAPS = { 0x7b6f,
                             0x2c97,
                             0x73e7,
                             0x72cf,
                             0x5bc9,
                             0x79cf,
                             0x79ef,
                             0x7249,
                             0x7bef,
                             0x7bcf };
uint16 DIGIT_WIDTH = 3;
uint16 DIGIT_HEIGHT = 5;
uint16 DIGIT_SCALE = 2;
uint16 DIGIT_PADDING = 1;
uint16 DIGITS_IN_SCORE = 5;

/**
 * The screen width and height in pixels.
 */
uint16 SCREEN_WIDTH  = 256;
uint16 SCREEN_HEIGHT = 192;

/**
 * Some score box constants, in pixels.
 */
uint16 SCORE_WIDTH =
  ((DIGIT_WIDTH * DIGIT_SCALE) + DIGIT_PADDING) * DIGITS_IN_SCORE;
uint16 SCORE_HEIGHT = DIGIT_HEIGHT * DIGIT_SCALE;
uint16 SCORE_X = (SCREEN_WIDTH - SCORE_WIDTH) / 2;
uint16 SCORE_Y = DIGIT_PADDING;
uint16 ADJ_SCREEN_HEIGHT = SCREEN_HEIGHT - SCORE_HEIGHT - (DIGIT_PADDING * 2);

/**
 * The board width and height in blocks.
 */
uint16 BOARD_WIDTH  = 10;
uint16 BOARD_HEIGHT = 22;

/**
 * The block width and height in pixels.
 */
uint16 BLOCK_SIZE = (SCREEN_HEIGHT - SCORE_HEIGHT) / BOARD_HEIGHT;

/**
 * The x and y coordinates of the board's top left corner, in pixels.
 */
uint16 BOARD_X = (SCREEN_WIDTH - (BLOCK_SIZE * BOARD_WIDTH)) / 2;
uint16 BOARD_Y =
  SCORE_HEIGHT + (DIGIT_PADDING * 2) +
  ((ADJ_SCREEN_HEIGHT - (BLOCK_SIZE * BOARD_HEIGHT)) / 2);

/**
 * GLOBAL VARIABLES
 */

/**
 * The current piece type.
 */
uint16 cur_piece;

/**
 * A map of fallen pieces.
 */
uint16[BOARD_WIDTH*BOARD_HEIGHT] fallen_pieces;

/**
 * The score.
 */
uint16 score;

/**
 * A flag for the endgame condition.
 */
uint16 game_over;

/**
 * The number of milliseconds the timer waits before making the piece
 * fall.
 */
uint16 speed;

/**
 * The current piece's position and orientation.
 */
uint16 cur_pos_x;
uint16 cur_pos_y;
uint16 cur_rot;

/**
 * Sets the paint color to the given RGB value.
 */
void color_rgb(uint16 r, uint16 g, uint16 b) {
  // Colors are packed into an 8-bit value, where the first three bits
  // are for red, the middle three bits are for green, and the last two bits
  // are for blue.
  COLOR((r & 0xe0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6));
}

/**
 * Paints a rectangle from (x, y) to (x + width, y + height) with
 * the given RGB value.
 */
void paint_rectangle(uint16 x, uint16 y,
                     uint16 width, uint16 height) {
  uint16 i;
  uint16 j;
  for (i = x; i < x + width; i = i + 1) {
    for (j = y; j < y + height; j = j + 1) {
      PIXEL(i, j);
    }
  }
}

/**
 * Paint a piece at the given x and y coordinate, with the color
 * of the given piece type.
 */
void paint_piece_at(uint16 x, uint16 y, uint16 piece_type) {
  // If x or y are out of bounds, or the piece type is invalid,
  // do nothing.
  if (BOARD_WIDTH <= x || BOARD_HEIGHT <= y || 8 <= piece_type) {
    return;
  }
  // Paint the piece with the appropriate color.
  COLOR(PIECE_COLORS[piece_type]);
  paint_rectangle(BOARD_X + (x * BLOCK_SIZE),
                  BOARD_Y + (y * BLOCK_SIZE),
                  BLOCK_SIZE, BLOCK_SIZE);
}

/**
 * Paints the background gray.
 */
void paint_background() {
  color_rgb(150, 150, 150);
  paint_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

/**
 * Paints the board black.
 */
void paint_board() {
  color_rgb(0, 0, 0);
  paint_rectangle(BOARD_X, BOARD_Y,
                  BOARD_WIDTH * BLOCK_SIZE,
                  BOARD_HEIGHT * BLOCK_SIZE);
}

/**
 * Paints a 6x10 pixel digit at the given x and y coordinates.
 */
void paint_digit(uint16 digit, uint16 x, uint16 y) {
  uint16 i;
  uint16 j;
  uint16 bitmap;
  if (10 <= digit) {
    return;
  }
  bitmap = DIGIT_BITMAPS[digit];
  for (j = 0; j < DIGIT_HEIGHT*DIGIT_SCALE; j = j + DIGIT_SCALE) {
    for (i = 0; i < DIGIT_WIDTH*DIGIT_SCALE; i = i + DIGIT_SCALE) {
      bitmap = bitmap << 1;
      if (bitmap & 0x8000) {
        paint_rectangle(x + i,
                        y + j,
                        DIGIT_SCALE, DIGIT_SCALE);
      }
    }
  }
}

/**
 * Covers up the old score and paints the new score.
 */
void paint_score() {
  uint16 score_reg = score;
  uint16 i;
  // Blank out the old score.
  color_rgb(150, 150, 150);
  paint_rectangle(SCORE_X, SCORE_Y, SCORE_WIDTH, SCORE_HEIGHT);
  color_rgb(0, 0, 0);
  for (i = 0; i < DIGITS_IN_SCORE; i = i + 1) {
    paint_digit(score_reg % 10,
                SCORE_X + (((DIGIT_WIDTH * DIGIT_SCALE) + DIGIT_PADDING) * (DIGITS_IN_SCORE - i - 1)),
                SCORE_Y);
    score_reg = score_reg / 10;
  }
}

/**
 * Sets the score to the new value and repaints the
 * score if necessary.
 */
void set_score(uint16 new_score) {
  score = new_score;
  paint_score();
}

/**
 * Resets global variables and repaints the screen.
 */
void reset_game() {
  uint16 i;
  // Set the fallen pieces to zero.
  for (i = 0; i < BOARD_WIDTH*BOARD_HEIGHT; i = i + 1) {
    fallen_pieces[i] = 0;
  }
  // Set the falling speed back to 1 per second.
  speed = 1000;
  // Paint the board black.
  paint_board();
  // Reset score to zero and repaint it.
  set_score(0);
  // Set game_over back to false.
  game_over = false;
}

/**
 * Gets the 4 block positions for the given tetromino. The positions
 * variable is a pointer to an array of 10 uint16s, and the positions
 * will be given as { x0, y0, x1, y1, x2, y2, x3, y3, width, height }
 * in that array. Notice width and height are included.
 */
void get_piece_positions(uint16 positions,
                         uint16 piece_type,
                         uint16 pos_x,
                         uint16 pos_y,
                         uint16 rot) {
  uint16 copy_positions = PIECES + (piece_type * 20);
  uint16 i;
  uint16 i2;
  uint16 i2p1;
  uint16 width = copy_positions[8];
  uint16 height = copy_positions[9];
  for (i = 0; i < 4; i = i + 1) {
    i2 = i * 2;
    i2p1 = i2 + 1;
    if (0 == rot) {
      positions[i2] = pos_x + copy_positions[i2];
      positions[i2p1] = pos_y + copy_positions[i2p1];
    } else if (1 == rot) {
      positions[i2] = pos_x + height - 1 - copy_positions[i2p1];
      positions[i2p1] = pos_y + copy_positions[i2];
    } else if (2 == rot) {
      positions[i2] = pos_x + width - 1 - copy_positions[i2];
      positions[i2p1] = pos_y + height - 1 - copy_positions[i2p1];
    } else if (3 == rot) {
      positions[i2] = pos_x + copy_positions[i2p1];
      positions[i2p1] = pos_y + width - 1 - copy_positions[i2];
    }
  }
  if (0 == rot || 2 == rot) {
    positions[8] = copy_positions[8];
    positions[9] = copy_positions[9];
  } else {
    positions[8] = copy_positions[9];
    positions[9] = copy_positions[8];
  }
}

/**
 * Paints the current piece at the current location. Optionally
 * blanks out the piece instead of painting it.
 */
void paint_cur_piece(uint16 blackout) {
  uint16[10] positions;
  uint16 i;
  uint16 piece_type = cur_piece;
  if (blackout) {
    piece_type = NULL_PIECE;
  }
  get_piece_positions(positions, cur_piece, cur_pos_x, cur_pos_y, cur_rot);
  for (i = 0; i < 4; i = i + 1) {
    paint_piece_at(positions[i*2], positions[(i*2)+1], piece_type);
  }
}

/**
 * Accepts an array of 10 uint16s and checks if any of them intersect
 * with the fallen pieces array.
 */
uint16 intersects_fallen_pieces(uint16 positions) {
  uint16 i;
  uint16 i2;
  uint16 i2p1;
  uint16 j;
  for (i = 0; i < 4; i = i + 1) {
    i2 = i*2;
    i2p1 = i2 + 1;
    if (BOARD_WIDTH <= positions[i2] ||
        BOARD_HEIGHT <= positions[i2p1]) {
      continue;
    }
    if (0 != fallen_pieces[(positions[i2p1]*BOARD_WIDTH)+positions[i2]]) {
      return true;
    }
  }
  return false;
}

/**
 * Sets the current piece to a new random piece and draws it
 * at the top of the board.
 */
void get_new_piece() {
  uint16[10] positions;
  cur_piece = (RND() % 7) + 1;
  cur_pos_x = (BOARD_WIDTH / 2) - 1;
  cur_pos_y = 0;
  cur_rot = 0;
  paint_cur_piece(false);
  get_piece_positions(positions, cur_piece, cur_pos_x, cur_pos_y, cur_rot);
  if (intersects_fallen_pieces(positions)) {
    game_over = true;
  }
  TIMERST();
}

/**
 * Returns true if the current piece can move left.
 */
uint16 can_move_left() {
  uint16[10] test_positions;
  if (0 == cur_pos_x) {
    return false;
  }
  get_piece_positions(test_positions, cur_piece,
                      cur_pos_x - 1, cur_pos_y, cur_rot);
  return !intersects_fallen_pieces(test_positions);
}

/**
 * Returns true if the current piece can move right.
 */
uint16 can_move_right() {
  uint16[10] test_positions;
  get_piece_positions(test_positions, cur_piece,
                      cur_pos_x + 1, cur_pos_y, cur_rot);
  if (BOARD_WIDTH <= cur_pos_x + test_positions[8]) {
    return false;
  }
  return !intersects_fallen_pieces(test_positions);
}

/**
 * Returns true if the current piece can rotate.
 */
uint16 can_rotate() {
  uint16[10] test_positions;
  get_piece_positions(test_positions, cur_piece,
                      cur_pos_x, cur_pos_y, (cur_rot + 1) % 4);
  if (BOARD_WIDTH < cur_pos_x + test_positions[8] ||
      BOARD_HEIGHT <= cur_pos_y - 1 + test_positions[9]) {
    return false;
  }
  return !intersects_fallen_pieces(test_positions);
}

/**
 * Returns true if the current piece can move down.
 */
uint16 can_move_down() {
  uint16[10] test_positions;
  get_piece_positions(test_positions, cur_piece,
                      cur_pos_x, cur_pos_y + 1, cur_rot);
  if (BOARD_HEIGHT <= cur_pos_y + test_positions[9]) {
    return false;
  }
  return !intersects_fallen_pieces(test_positions);
}

/**
 * Move the current piece down.
 */
void move_down() {
  // Black out the current piece, move it down, and repaint it.
  paint_cur_piece(true);
  cur_pos_y = cur_pos_y + 1;
  paint_cur_piece(false);
}

/**
 * Checks all lines for completeness and returns the number of lines
 * that were complete. Removes the complete rows and shifts them down.
 */
uint16 check_lines() {
  uint16 i;
  uint16 j;
  uint16 k;
  uint16 a;
  uint16 b;
  uint16 lines_complete = 0;
  uint16 complete;
  // Iterate over rows, starting at the bottom and moving toward the top.
  for (j = BOARD_HEIGHT - 1; -1 != j; j = j - 1) {
    // Iterate over all positions in the current row.
    complete = true;
    for (i = 0; i < BOARD_WIDTH; i = i + 1) {
      if (NULL_PIECE == fallen_pieces[(j*BOARD_WIDTH)+i]) {
        complete = false;
        break;
      }
    }
    if (complete) {
      // Shift the upper rows down.
      for (k = j; 0 < k; k = k - 1) {
        for (i = 0; i < BOARD_WIDTH; i = i + 1) {
          a = (k*BOARD_WIDTH)+i;
          b = ((k-1)*BOARD_WIDTH)+i;
          // Don't repaint unnecessarily.
          if (fallen_pieces[a] != fallen_pieces[b]) {
            fallen_pieces[a] = fallen_pieces[b];
            paint_piece_at(i, k, fallen_pieces[a]);
          }
        }
      }
      // Paint the upper row black.
      for (i = 0; i < BOARD_WIDTH; i = i + 1) {
        fallen_pieces[i] = NULL_PIECE;
        paint_piece_at(i, 0, NULL_PIECE);
      }
      // Check the current line again.
      j = j + 1;
      // Increment the return value.
      lines_complete = lines_complete + 1;
    }
  }
  return lines_complete;
}

/**
 * Adds the current block to the fallen pieces array.
 */
void add_to_fallen_pieces() {
  uint16[10] positions;
  uint16 i;
  uint16 i2;
  uint16 i2p1;
  uint16 lines_complete;
  get_piece_positions(positions, cur_piece, cur_pos_x, cur_pos_y, cur_rot);
  for (i = 0; i < 4; i = i + 1) {
    i2 = i * 2;
    i2p1 = i2 + 1;
    fallen_pieces[(positions[i2p1]*BOARD_WIDTH)+positions[i2]] = cur_piece;
  }
  // Check lines for completion.
  lines_complete = check_lines();
  // Add the correct value to the score.
  if (4 == lines_complete) {
    set_score(score + 1200);
  } else if (3 == lines_complete) {
    set_score(score + 300);
  } else if (2 == lines_complete) {
    set_score(score + 100);
  } else if (1 == lines_complete) {
    set_score(score + 40);
  }
  // Increase the speed by decreasing the number of milliseconds between
  // drops, up to a minimum of 300 milliseconds.
  if (300 < speed) {
    speed = speed - 10;
  }
  get_new_piece();
}

/**
 * Drop as far down as we can go and get a new block.
 */
void drop_down() {
  uint16 old_pos_y = cur_pos_y;
  uint16 new_pos_y;
  while (can_move_down()) {
    cur_pos_y = cur_pos_y + 1;
  }
  new_pos_y = cur_pos_y;
  // Black out the old position
  cur_pos_y = old_pos_y;
  paint_cur_piece(true);
  // Paint the new position
  cur_pos_y = new_pos_y;
  paint_cur_piece(false);
  
  add_to_fallen_pieces();
}

/**
 * The main game loop.
 */
void play_game() {
  uint16 up_pressed = false;
  uint16 up_pressed_prev = true;
  uint16 left_pressed = false;
  uint16 left_pressed_prev = true;
  uint16 right_pressed = false;
  uint16 right_pressed_prev = true;
  uint16 down_pressed = false;
  uint16 down_pressed_prev = true;
  uint16 space_pressed = false;
  uint16 space_pressed_prev = true;
  get_new_piece();
  while (1) {
    // Check for left key pressed, in which case we should try to
    // move the current piece left.
    left_pressed = INPUT(KEY_LEFT[0]) || INPUT(KEY_LEFT[1]);
    if (left_pressed && !left_pressed_prev) {
      // Check if we can go left, then go left.
      if (can_move_left()) {
        // Black out the current piece, move it left, and repaint it.
        paint_cur_piece(true);
        cur_pos_x = cur_pos_x - 1;
        paint_cur_piece(false);
      }
    }

    // Check for right key pressed, in which case we should try to
    // move the current piece right.
    right_pressed = INPUT(KEY_RIGHT[0]) || INPUT(KEY_RIGHT[1]);;
    if (right_pressed && !right_pressed_prev) {
      // Check if we can go right, then go right.
      if (can_move_right()) {
        // Black out the current piece, move it left, and repaint it.
        paint_cur_piece(true);
        cur_pos_x = cur_pos_x + 1;
        paint_cur_piece(false);
      }
    }

    // Check for up key pressed, in which case we should try to
    // rotate the current piece.
    up_pressed = INPUT(KEY_UP[0]) || INPUT(KEY_UP[1]);
    if (up_pressed && !up_pressed_prev) {
      // Check if we can rotate, then rotate.
      if (can_rotate()) {
        // Black out the current piece.
        paint_cur_piece(true);
        // Increment the rotation and repaint the piece.
        cur_rot = (cur_rot + 1) % 4;
        paint_cur_piece(false);
      }
    }

    // Check for down key press, in which case we should try to move
    // the current piece down one.
    down_pressed = INPUT(KEY_DOWN[0]) || INPUT(KEY_DOWN[1]);
    if (down_pressed && !down_pressed_prev) {
      // Check if we can move down, then move down.
      if (can_move_down()) {
        move_down();
      } else {
        add_to_fallen_pieces();
      }
    }

    // Check for the spacebar pressed, in which case we should drop the
    // piece down all the way.
    space_pressed = INPUT(KEY_SPACE[0]) || INPUT(KEY_SPACE[1]);
    if (space_pressed && !space_pressed_prev) {
      drop_down();
    }

    // Set previous key state to current key state.
    up_pressed_prev = up_pressed;
    left_pressed_prev = left_pressed;
    right_pressed_prev = right_pressed;
    down_pressed_prev = down_pressed;
    space_pressed_prev = space_pressed;

    // Check timer
    if (speed <= TIME()) {
      if (can_move_down()) {
        move_down();
      } else {
        add_to_fallen_pieces();
      }
      TIMERST();
    }

    if (game_over) {
      break;
    }
  }
}

/**
 * Do nothing while waiting for the key with the given input_id to be
 * pressed.
 */
void wait_for_key(uint16 input_ids, uint16 num_ids) {
  uint16 currently_down = false;
  uint16 previously_down = true;
  uint16 i;
  while (true) {
    currently_down = false;
    for (i = 0; i < num_ids; i = i + 1) {
      currently_down = currently_down || INPUT(input_ids[i]);
    }
    if (currently_down && !previously_down) {
      return;
    }
    previously_down = currently_down;
  }
}

/**
 * The entry point of the program, takes no arguments
 * and returns nothing.
 */
void main() {
  // Paint the background gray, then reset the game.
  paint_background();
  reset_game();
  // Play the game repeatedly.
  while (true) {
    // Start in the game over state, wait for a spacebar press
    wait_for_key(KEY_SPACE, 2);
    // Reset everything.
    reset_game();
    // Enter the game loop.
    play_game();
  }
}
