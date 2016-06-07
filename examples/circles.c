/**
 * Test circle drawing program in Consolite C.
 * Draws circles of random size, color, and position as fast as the
 * processor will allow. Takes no input.
 *
 * @author Robert Fotino, 2016
 */

void draw_circle(uint16 cx, uint16 cy, uint16 r) {
  uint16 rSq = r*r;
  uint16 x;
  uint16 y;
  uint16 a;
  uint16 b;
  uint16 c;
  uint16 d;
  uint16 ySq;
  for (y = 0; y < r; y = y + 1) {
    c = cy + y;
    d = cy - y;
    ySq = y*y;
    for (x = 0; x < r; x = x + 1) {
      a = cx + x;
      b = cx - x;
      if (((x*x)+ySq) <= rSq) {
        PIXEL(a, c);
        PIXEL(a, d);
        PIXEL(b, c);
        PIXEL(b, d);
      }
    }
  }
}

void main() {
  while (1) {
    COLOR(RND());
    draw_circle(RND() & 0xff, RND() & 0xff, (RND() & 0x1f) + 16);
  }
}
