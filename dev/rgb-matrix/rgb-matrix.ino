#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint8_t n_panels = 4;
constexpr uint8_t panel_width = 8;
constexpr uint8_t panel_height = 8;
constexpr uint16_t n_panel_pixels = panel_width * panel_height;
constexpr uint16_t n_leds = n_panels * n_panel_pixels;
constexpr uint8_t brightness_level = 255;
constexpr uint32_t step_interval_ms = 100;
constexpr uint32_t panel_start_ms = 1500;

struct NamedColour {
  CRGB colour;
  const char *name;
};

const NamedColour palette[] = {
  { CRGB(255, 0, 0), "red" },
  { CRGB(0, 255, 0), "green" },
  { CRGB(0, 0, 255), "blue" },
  { CRGB(255, 255, 0), "yellow" },
  { CRGB(0, 255, 255), "cyan" },
  { CRGB(255, 0, 255), "magenta" },
  { CRGB(255, 255, 255), "white" },
};
constexpr uint8_t n_colours = sizeof(palette) / sizeof(palette[0]);

CRGB leds[n_leds];
uint16_t lit_index = 0;
uint8_t colour_index = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  FastLED.addLeds<WS2812B, data_pin, GRB>(leds, n_leds);
  FastLED.setBrightness(brightness_level);
  FastLED.clear(true);
  Serial.println();
  Serial.printf("rgb-matrix: %u pixels in %u panels of %u, %u colours, one lit at a time\r\n",
                n_leds, n_panels, n_panel_pixels, n_colours);
  Serial.printf("walking %s\r\n", palette[colour_index].name);
}

// The walk steps through the raw chain index rather than a coordinate, because
// which panel comes first in the chain and how each one is rotated is exactly
// what this sketch exists to reveal.  The reported (x, y) is what the pixel
// would be if the panel is progressive, so a mismatch against the panel is the
// result, not a fault.
void loop() {
  const uint8_t panel = lit_index / n_panel_pixels;
  const uint16_t panel_pixel = lit_index % n_panel_pixels;
  if (panel_pixel == 0) {
    Serial.printf("panel %u begins at index %u\r\n", panel, lit_index);
  }
  fill_solid(leds, n_leds, CRGB::Black);
  leds[lit_index] = palette[colour_index].colour;
  FastLED.show();
  Serial.printf("index %3u  panel %u  pixel %2u  expected (%u, %u)\r\n",
                lit_index, panel, panel_pixel,
                panel_pixel % panel_width, panel_pixel / panel_width);
  delay(panel_pixel == 0 ? panel_start_ms : step_interval_ms);
  lit_index++;
  if (lit_index == n_leds) {
    lit_index = 0;
    colour_index = (colour_index + 1) % n_colours;
    Serial.printf("walking %s\r\n", palette[colour_index].name);
  }
}
