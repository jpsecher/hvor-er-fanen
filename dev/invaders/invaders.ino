#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint8_t matrix_width = 8;
constexpr uint8_t matrix_height = 8;
constexpr uint16_t n_leds = matrix_width * matrix_height;
constexpr uint8_t brightness_level = 32;
constexpr uint32_t beat_ms = 400;
constexpr uint8_t beats_per_invader = 6;

// This panel is progressive: every row runs left to right.  Serpentine panels
// reverse alternate rows, and which rows depends on the corner the data enters.
enum class PanelLayout { progressive, serpentine_odd, serpentine_even };
constexpr PanelLayout panel_layout = PanelLayout::progressive;

const char *squid_a[matrix_height] = {
  "...##...", "..####..", ".######.", "##.##.##", "########", "..#..#..", ".#.##.#.", "#.#..#.#",
};
const char *squid_b[matrix_height] = {
  "...##...", "..####..", ".######.", "##.##.##", "########", "..#..#..", "#.#..#.#", ".#....#.",
};
const char *crab_a[matrix_height] = {
  "..#..#..", "...##...", ".######.", "##.##.##", "########", "#.####.#", "#.#..#.#", "..#..#..",
};
const char *crab_b[matrix_height] = {
  "..#..#..", "#..##..#", ".######.", "##.##.##", "########", ".######.", "..#..#..", ".#....#.",
};
const char *octopus_a[matrix_height] = {
  "..####..", ".######.", "########", "##.##.##", "########", "..#..#..", ".##..##.", "##....##",
};
const char *octopus_b[matrix_height] = {
  "..####..", ".######.", "########", "##.##.##", "########", "..#..#..", "..#..#..", ".#....#.",
};
const char *ship[matrix_height] = {
  "........", "........", "...##...", "...##...", ".######.", "########", "########", "########",
};

struct Invader {
  const char *const *frame_a;
  const char *const *frame_b;
  CRGB colour;
  const char *name;
};

const Invader invaders[] = {
  { squid_a, squid_b, CRGB(200, 200, 255), "squid" },
  { crab_a, crab_b, CRGB(0, 255, 255), "crab" },
  { octopus_a, octopus_b, CRGB(255, 0, 255), "octopus" },
  { ship, ship, CRGB(0, 255, 0), "ship" },
};
constexpr uint8_t n_invaders = sizeof(invaders) / sizeof(invaders[0]);

CRGB leds[n_leds];
uint8_t invader_index = 0;
uint8_t beat = 0;

static uint16_t xy_to_index(uint8_t x, uint8_t y) {
  const bool odd_row = (y % 2) == 1;
  const bool row_reversed = (panel_layout == PanelLayout::serpentine_odd && odd_row) ||
                            (panel_layout == PanelLayout::serpentine_even && !odd_row);
  return y * matrix_width + (row_reversed ? matrix_width - 1 - x : x);
}

static void draw(const char *const *sprite, CRGB colour) {
  fill_solid(leds, n_leds, CRGB::Black);
  for (uint8_t y = 0; y < matrix_height; y++) {
    for (uint8_t x = 0; x < matrix_width; x++) {
      if (sprite[y][x] == '#') {
        leds[xy_to_index(x, y)] = colour;
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
  Serial.printf("invaders: %u sprites on %ux%u\r\n", n_invaders, matrix_width, matrix_height);
  Serial.printf("%s\r\n", invaders[invader_index].name);
}

void loop() {
  const Invader &invader = invaders[invader_index];
  draw((beat % 2) == 0 ? invader.frame_a : invader.frame_b, invader.colour);
  beat++;
  if (beat >= beats_per_invader) {
    beat = 0;
    invader_index = (invader_index + 1) % n_invaders;
    Serial.printf("%s\r\n", invaders[invader_index].name);
  }
  delay(beat_ms);
}
