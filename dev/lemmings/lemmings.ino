#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint8_t matrix_width = 8;
constexpr uint8_t matrix_height = 8;
constexpr uint16_t n_leds = matrix_width * matrix_height;
constexpr uint8_t brightness_level = 32;
constexpr uint32_t step_interval_ms = 220;

constexpr uint8_t sprite_width = 4;
constexpr uint8_t sprite_height = 8;

// This panel is progressive: every row runs left to right.  Serpentine panels
// reverse alternate rows, and which rows depends on the corner the data enters,
// so both phases are here.  A wrong setting tears the sprite across every
// second row.
enum class PanelLayout { progressive, serpentine_odd, serpentine_even };
constexpr PanelLayout panel_layout = PanelLayout::progressive;

const CRGB hair_colour = CRGB(0, 220, 0);
const CRGB skin_colour = CRGB(255, 170, 120);
const CRGB body_colour = CRGB(50, 50, 230);

// g hair, s skin, b body, dot transparent.
const char *walk_stride[sprite_height] = {
  ".gg.",
  "gggg",
  "gssg",
  ".ss.",
  "bbbb",
  "bbbb",
  ".bb.",
  "b..b",
};

const char *walk_together[sprite_height] = {
  ".gg.",
  "gggg",
  "gssg",
  ".ss.",
  "bbbb",
  "bbbb",
  ".bb.",
  ".bb.",
};

const char **const frames[] = { walk_stride, walk_together };
constexpr uint8_t n_frames = sizeof(frames) / sizeof(frames[0]);

CRGB leds[n_leds];
int8_t x_offset = -sprite_width;
uint8_t frame_index = 0;

static uint16_t xy_to_index(uint8_t x, uint8_t y) {
  const bool odd_row = (y % 2) == 1;
  const bool row_reversed = (panel_layout == PanelLayout::serpentine_odd && odd_row) ||
                            (panel_layout == PanelLayout::serpentine_even && !odd_row);
  return y * matrix_width + (row_reversed ? matrix_width - 1 - x : x);
}

static CRGB colour_for(char key) {
  switch (key) {
    case 'g': return hair_colour;
    case 's': return skin_colour;
    case 'b': return body_colour;
  }
  return CRGB::Black;
}

static void draw(const char **sprite, int8_t offset) {
  fill_solid(leds, n_leds, CRGB::Black);
  for (uint8_t y = 0; y < sprite_height; y++) {
    for (uint8_t sx = 0; sx < sprite_width; sx++) {
      const char key = sprite[y][sx];
      if (key == '.') {
        continue;
      }
      const int16_t x = offset + sx;
      if (x < 0 || x >= matrix_width) {
        continue;
      }
      leds[xy_to_index(x, y)] = colour_for(key);
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
  Serial.printf("lemmings: %ux%u sprite on %ux%u panel\r\n", sprite_width, sprite_height, matrix_width, matrix_height);
}

void loop() {
  draw(frames[frame_index], x_offset);
  frame_index = (frame_index + 1) % n_frames;
  x_offset++;
  if (x_offset >= static_cast<int8_t>(matrix_width)) {
    x_offset = -sprite_width;
    Serial.println("lap done");
  }
  delay(step_interval_ms);
}
