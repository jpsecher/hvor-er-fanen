#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint8_t matrix_width = 8;
constexpr uint8_t matrix_height = 8;
constexpr uint16_t n_leds = matrix_width * matrix_height;
constexpr uint8_t brightness_level = 32;
constexpr uint32_t hold_ms = 1000;

// Most 8x8 panels are wired serpentine, with every second row running right to
// left.  The colour walk in dev/rgb-matrix tells which this one is: a serpentine
// panel reverses direction at the end of each row, a progressive panel jumps
// back to the left edge.  Every sprite comes out mirrored on alternate rows if
// this is wrong.
enum class PanelLayout { serpentine, progressive };
constexpr PanelLayout panel_layout = PanelLayout::serpentine;

struct Shape {
  uint8_t body[matrix_height];
  uint8_t eyes[matrix_height];
};

const Shape pacman_open = {
  { 0b00111100, 0b01111110, 0b11111100, 0b11111000, 0b11111000, 0b11111100, 0b01111110, 0b00111100 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
};

const Shape pacman_closed = {
  { 0b00111100, 0b01111110, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b01111110, 0b00111100 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
};

const Shape ghost = {
  { 0b00111100, 0b01111110, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11011011 },
  { 0, 0, 0b00100100, 0b00100100, 0, 0, 0, 0 },
};

struct Frame {
  const Shape *shape;
  CRGB colour;
  const char *name;
};

const Frame frames[] = {
  { &pacman_open, CRGB(255, 255, 0), "pacman open" },
  { &pacman_closed, CRGB(255, 255, 0), "pacman closed" },
  { &ghost, CRGB(255, 0, 0), "blinky" },
  { &ghost, CRGB(255, 105, 180), "pinky" },
  { &ghost, CRGB(0, 255, 255), "inky" },
  { &ghost, CRGB(255, 165, 0), "clyde" },
  { &ghost, CRGB(33, 33, 255), "frightened" },
};
constexpr uint8_t n_frames = sizeof(frames) / sizeof(frames[0]);

const CRGB eye_colour = CRGB(255, 255, 255);

CRGB leds[n_leds];
uint8_t frame_index = 0;

static uint16_t xy_to_index(uint8_t x, uint8_t y) {
  const bool row_reversed = panel_layout == PanelLayout::serpentine && (y % 2) == 1;
  return y * matrix_width + (row_reversed ? matrix_width - 1 - x : x);
}

static void draw(const Frame &frame) {
  fill_solid(leds, n_leds, CRGB::Black);
  for (uint8_t y = 0; y < matrix_height; y++) {
    for (uint8_t x = 0; x < matrix_width; x++) {
      const uint8_t mask = 0x80 >> x;
      if (frame.shape->body[y] & mask) {
        leds[xy_to_index(x, y)] = frame.colour;
      }
      if (frame.shape->eyes[y] & mask) {
        leds[xy_to_index(x, y)] = eye_colour;
      }
    }
  }
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  FastLED.addLeds<WS2812B, data_pin, GRB>(leds, n_leds);
  FastLED.setBrightness(brightness_level);
  FastLED.clear(true);
  Serial.println();
  Serial.printf("sprites: %ux%u, %u frames\r\n", matrix_width, matrix_height, n_frames);
}

void loop() {
  draw(frames[frame_index]);
  Serial.printf("%s\r\n", frames[frame_index].name);
  frame_index = (frame_index + 1) % n_frames;
  delay(hold_ms);
}
