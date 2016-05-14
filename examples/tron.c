/**
 * Define global constants.
 */
uint16 true  = 1;
uint16 false = 0;

uint16 MIN_PLAYERS = 2;
uint16 MAX_PLAYERS = 8;

uint16 SCREEN_WIDTH  = 256;
uint16 SCREEN_HEIGHT = 192;

uint16 DIR_UP    = 0;
uint16 DIR_DOWN  = 1;
uint16 DIR_LEFT  = 2;
uint16 DIR_RIGHT = 3;

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

/**
 * Global variables.
 */
uint16 nplayers = MIN_PLAYERS;
uint16[MAX_PLAYERS] player_alive;
uint16[MAX_PLAYERS] player_x;
uint16[MAX_PLAYERS] player_y;
uint16[MAX_PLAYERS] player_dir;
uint16[MAX_PLAYERS] player_color = {
  0b00000011,
  0b11100000,
  0b00011100,
  0b11111100,
  0b00011111,
  0b11100011,
  0b10010011,
  0b11110010
};
uint16[SCREEN_WIDTH*SCREEN_HEIGHT/4] arena;

// DRAWING FUNCTIONS

/**
 * Sets the color register to the closest displayable color based on the given
 * RGB values.
 */
void set_color(uint16 r, uint16 g, uint16 b) {
  COLOR((r & 0b11100000) | ((g >> 3) & 0b00011100) | ((b >> 6) & 0b00000011));
}

/**
 * Fills a rectangle with the given x, y, width, and height, with the color
 * already set in the color register.
 */
void draw_rect(uint16 x, uint16 y, uint16 width, uint16 height) {
  uint16 i;
  uint16 j;
  uint16 xmax = x + width;
  uint16 ymax = y + height;
  for (j = y; j < ymax; j = j + 1) {
    for (i = x; i < xmax; i = i + 1) {
      PIXEL(i, j);
    }
  }
}

/**
 * Paints the whole screen black (not a quick operation).
 */
void clear_screen() {
  COLOR(0);
  draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

/**
 * Draws a 3x5 bitmap stored in the lower 15 bits of bitmap, starting
 * at the given (x, y) and with the given scale factor.
 */
void draw_bitmap_3x5(uint16 bitmap, uint16 x, uint16 y, uint16 scale) {
  uint16 i;
  uint16 j;
  uint16 xmax = x + (3 * scale);
  uint16 ymax = y + (5 * scale);
  for (j = y; j < ymax; j = j + scale) {
    for (i = x; i < xmax; i = i + scale) {
      bitmap = bitmap << 1;
      if (bitmap & 0x8000) {
        draw_rect(i, j, scale, scale);
      }
    }
  }
}

/**
 * Draws the given digit d at the (x, y) coords with the given scale
 * factor. Uses the current color register.
 */
void draw_digit(uint16 d, uint16 x, uint16 y, uint16 scale) {
  draw_bitmap_3x5(DIGIT_BITMAPS[d], x, y, scale);
}

// ARRAY FUNCTIONS

/**
 * Fills an array with the given value.
 */
void array_fill(uint16 arr, uint16 val, uint16 len) {
  uint16 i;
  for (i = 0; i < len; i = i + 1) {
    arr[i] = val;
  }
}

/**
 * Copy an array "src" of length "len" into the array "dest".
 */
void array_copy(uint16 src, uint16 dest, uint16 len) {
  uint16 i;
  for (i = 0; i < len; i = i + 1) {
    dest[i] = src[i];
  }
}

/**
 * Populates an array with the state of a controller. Controllers have 12
 * buttons, so "state" must be an array of at least length 12.
 */
void get_controller_state(uint16 controller_id, uint16 state) {
  uint16 i;
  for (i = 0; i < 12; i = i + 1) {
    // Multiply controller_id by 12
    // c*12 = (c*4)+(c*8) = (c<<log2(4))+(c<<log2(8)) = (c<<2)+(c<<3)
    state[i] = INPUT(46 + (controller_id << 2) + (controller_id << 3) + i);
  }
}

// GAME-SPECIFIC LOGIC

/**
 * Draws TRON in big letters on top of the screen
 */
void draw_tron() {
  uint16[4] bitmaps = {
    0b111010010010010, // T
    0b111101111110101, // R
    0b111101101101111, // O
    0b111101101101101  // N
  };
  uint16 i;
  uint16 j;
  // Draw drop shadow/offset thing for 3D effect
  set_color(0, 50, 100);
  for (j = 0; j < 4; j = j + 1) {
    for (i = 0; i < 4; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], 75 + (25 * i) - j, 60 - j, 6);
    }
  }
  // Draw top layer in brighter color
  set_color(0, 100, 200);
  for (i = 0; i < 4; i = i + 1) {
    draw_bitmap_3x5(bitmaps[i], 75 + (25 * i), 60, 6);
  }
}

/**
 * Erases and redraws the number of players when choosing before
 * starting the game.
 */
void draw_nplayers() {
  uint16 scale = 4;
  uint16 x = 60;
  uint16 y = 106;
  // Erase
  set_color(0, 0, 0);
  draw_rect(x, y, 3 * scale, 5 * scale);
  // Redraw
  set_color(255, 255, 255);
  draw_digit(nplayers, x, y, scale);
}

/**
 * Draws the string "PLAYERS" after the number of players when
 * choosing before the game starts.
 */
void draw_nplayers_str() {
  uint16 i;
  uint16[7] bitmaps = {
    0b111101111100100, // P
    0b100100100100111, // L
    0b111101111101101, // A
    0b101101111010010, // Y
    0b111100110100111, // E
    0b111101111110101, // R
    0b111100111001111  // S
  };
  for (i = 0; i < 7; i = i + 1) {
    draw_bitmap_3x5(bitmaps[i], 90 + (15 * i), 106, 4);
  }
}

/**
 * Present a prompt for the user to select the number of players
 * (between 2 and 8). The controller in the player 1 position must
 * be used to select.
 */
void select_nplayers() {
  // Whether the underline is currently blinking or not
  uint16 blink_on = false;
  // Store the button state of 1st player's controller in an array.
  // Have current and previous states to prevent double presses.
  uint16[12] prev_state;
  uint16[12] btn_state;
  array_fill(prev_state, 1, 12);
  array_fill(btn_state, 0, 12);
  clear_screen();
  // Draw the number of players UI
  draw_nplayers();
  draw_nplayers_str();
  draw_tron();
  // Listen for input from the user
  while (true) {
    get_controller_state(0, btn_state);
    // B pressed, return so we can start the game
    if (btn_state[0] && !prev_state[0]) {
      return;
    }
    // Up pressed, increment the number of players
    if (btn_state[4] && !prev_state[4]) {
      if (nplayers < MAX_PLAYERS) {
        nplayers = nplayers + 1;
        draw_nplayers();
      }
    }
    // Down pressed, decrement the number of players
    if (btn_state[5] && !prev_state[5]) {
      if (MIN_PLAYERS < nplayers) {
        nplayers = nplayers - 1;
        draw_nplayers();
      }
    }
    // Check for blink
    if (500 < TIME()) {
      TIMERST();
      blink_on = !blink_on;
      if (blink_on) {
        COLOR(255);
      } else {
        COLOR(0);
      }
      draw_rect(60, 130, 12, 2);
    }
    // Copy current button state to previous button state
    array_copy(btn_state, prev_state, 12);
  }
}

/**
 * Returns true if only one player is remaining (meaning the game
 * is over). Returns false otherwise.
 */
uint16 one_player_remaining() {
  uint16 n = 0;
  uint16 i;
  for (i = 0; i < MAX_PLAYERS; i = i + 1) {
    if (player_alive[i]) {
      n = n + 1;
    }
  }
  return n <= 1;
}

/**
 * Returns the color of the arena pixel at the given (x, y) location.
 * Used for collision and redrawing.
 */
uint16 get_color_arena_pixel(uint16 x, uint16 y) {
  uint16 lower_bits = x & 0b11;
  uint16 arena_index = (y << 6) + (x >> 2);
  uint16 val = arena[arena_index];
  uint16 packed_id = (val >> (lower_bits << 2)) & 0xf;
  if (packed_id) {
    return player_color[packed_id - 1];
  } else {
    return 0;
  }
}

/**
 * Draws a pixel to the frame buffer and updates the arena array
 * for collision purposes.
 */
void color_arena_pixel(uint16 player_id, uint16 x, uint16 y) {
  uint16 lower_bits;
  uint16 arena_index;
  uint16 prev_val;
  uint16 packed_id = (player_id + 1) & 0x000f;
  // Draw the pixel to the frame buffer
  COLOR(player_color[player_id]);
  PIXEL(x, y);
  // Update the arena. The arena is an array of 16-bit values, but
  // we only want to update 4 bits. Use masking depending on the LSBs
  // of x
  lower_bits = x & 0b11;
  arena_index = (y << 6) + (x >> 2);
  prev_val = arena[arena_index];
  if (0 == lower_bits) {
    arena[arena_index] = (prev_val & 0xfff0) | packed_id;
  } else if (1 == lower_bits) {
    arena[arena_index] = (prev_val & 0xff0f) | (packed_id << 4);
  } else if (2 == lower_bits) {
    arena[arena_index] = (prev_val & 0xf0ff) | (packed_id << 8);
  } else if (3 == lower_bits) {
    arena[arena_index] = (prev_val & 0x0fff) | (packed_id << 12);
  }
}

/**
 * The main game loop, handles user input and the drawing/updating of
 * the game.
 */
void game_loop() {
  uint16 i;
  uint16 dir;
  uint16 x;
  uint16 y;
  uint16[12] btn_state;
  while (true) {
    TIMERST();
    // Move all live players in their current directions
    for (i = 0; i < MAX_PLAYERS; i = i + 1) {
      if (!player_alive[i]) {
        continue;
      }
      x = player_x[i];
      y = player_y[i];
      color_arena_pixel(i, x, y);
      // Change direction on player input
      get_controller_state(i, btn_state);
      dir = player_dir[i];
      if (btn_state[4] && DIR_DOWN != dir) {
        dir = DIR_UP;
      } else if (btn_state[5] && DIR_UP != dir) {
        dir = DIR_DOWN;
      } else if (btn_state[6] && DIR_RIGHT != dir) {
        dir = DIR_LEFT;
      } else if (btn_state[7] && DIR_LEFT != dir) {
        dir = DIR_RIGHT;
      }
      player_dir[i] = dir;
      // Move the player, wrapping around if necessary
      if (DIR_UP == dir) {
        if (0 == y) {
          player_y[i] = SCREEN_HEIGHT - 1;
        } else {
          player_y[i] = y - 1;
        }
      } else if (DIR_DOWN == dir) {
        if (SCREEN_HEIGHT - 1 == y) {
          player_y[i] = 0;
        } else {
          player_y[i] = y + 1;
        }
      } else if (DIR_LEFT == dir) {
        if (0 == x) {
          player_x[i] = SCREEN_WIDTH - 1;
        } else {
          player_x[i] = x - 1;
        }
      } else if (DIR_RIGHT == dir) {
        if (SCREEN_WIDTH - 1 == x) {
          player_x[i] = 0;
        } else {
          player_x[i] = x + 1;
        }
      }
      // If the player moved into a line, set them to dead
      if (get_color_arena_pixel(player_x[i], player_y[i])) {
        player_alive[i] = false;
      }
    }
    // If there is one player remaining, the game is over so we can exit
    // the game loop
    if (one_player_remaining()) {
      break;
    }
    // Wait until this frame is over (16 milliseconds)
    while (TIME() < 16);
  }
}

/**
 * Show the winning player's number on the screen. Return when 1st
 * player presses B.
 */
void show_winner() {
  uint16[12] prev_state;
  uint16[12] btn_state;
  uint16[19] bitmaps = {
    0b111010010010010, // T
    0b101101111101101, // H
    0b111100110100111, // E
    0b101101111101101, // H
    0b111101101101111, // O
    0b101101101101111, // U
    0b111100111001111, // S
    0b111100110100111, // E
    0b111101111100100, // P
    0b100100100100111, // L
    0b111101111101101, // A
    0b101101111010010, // Y
    0b111100110100111, // E
    0b111101111110101, // R
    0b101101111111101, // W
    0b111010010010111, // I
    0b111101101101101, // N
    0b111100111001111, // S
    0b010010010000010  // !
  };
  // Get the winner (the player left alive)
  uint16 alive = -1;
  uint16 i;
  for (i = 0; i < MAX_PLAYERS; i = i + 1) {
    if (player_alive[i]) {
      alive = i;
      break;
    }
  }
  COLOR(255);
  if (-1 == alive) {
    // Draw "THE"
    for (i = 0; i < 3; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], 27 + (i * 15), 86, 4);
    }
    // Draw "HOUSE"
    for (i = 3; i < 8; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], 37 + (i * 15), 86, 4);
    }
    // Draw "WINS!"
    for (i = 14; i < 19; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], -44 + (i * 15), 86, 4);
    }
  } else {
    // Draw "PLAYER"
    for (i = 8; i < 14; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], -90 + (i * 15), 86, 4);
    }
    // Draw the player's number
    draw_bitmap_3x5(DIGIT_BITMAPS[alive + 1], 132, 86, 4);
    // Draw "WINS!"
    for (i = 14; i < 19; i = i + 1) {
      draw_bitmap_3x5(bitmaps[i], -54 + (i * 15), 86, 4);
    }
  }
  // Wait until the player presses the continue button
  array_fill(prev_state, 1, 12);
  array_fill(btn_state, 0, 12);
  while (true) {
    get_controller_state(0, btn_state);
    if (btn_state[0] && !prev_state[0]) {
      break;
    }
    array_copy(btn_state, prev_state, 12);
  }
}

/**
 * Initializes variables to start the game with the selected number
 * of players.
 */
void init() {
  uint16 i;
  for (i = 0; i < MAX_PLAYERS; i = i + 1) {
    player_alive[i] = i < nplayers;
    player_x[i] = (i + 1) * SCREEN_WIDTH / (nplayers + 1);
    player_y[i] = RND() % 192;
    if (player_y[i] < 96) {
      player_dir[i] = DIR_DOWN;
    } else {
      player_dir[i] = DIR_UP;
    }
  }
  for (i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT/4; i = i + 1) {
    arena[i] = 0;
  }
}

/**
 * The entry point of the program. Have player 1 select the number
 * of players, then start the game loop, then show the winner, then
 * repeat indefinitely.
 */
void main() {
  while (true) {
    select_nplayers();
    init();
    clear_screen();
    game_loop();
    show_winner();
  }
}
