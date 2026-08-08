#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint16_t n_leds = 64;
constexpr uint8_t supply_volts = 5;
constexpr uint32_t max_milliamps = 350;
constexpr uint8_t brightness_level = 32;
constexpr uint32_t step_interval_ms = 100;
constexpr uint16_t snake_length = 1;

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
uint16_t head_index = 0;
uint8_t colour_index = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  FastLED.addLeds<WS2812B, data_pin, GRB>(leds, n_leds);
  FastLED.setMaxPowerInVoltsAndMilliamps(supply_volts, max_milliamps);
  FastLED.setBrightness(brightness_level);
  FastLED.clear(true);
  Serial.println();
  Serial.printf("rgb-matrix: %u pixels, %u colours, %u mA cap\r\n", n_leds, n_colours, max_milliamps);
  Serial.printf("walking %s\r\n", palette[colour_index].name);
}

void loop() {
  leds[head_index] = palette[colour_index].colour;
  const uint16_t tail_index = (head_index + n_leds - snake_length) % n_leds;
  leds[tail_index] = CRGB::Black;
  FastLED.show();
  head_index = (head_index + 1) % n_leds;
  if (head_index == 0) {
    colour_index = (colour_index + 1) % n_colours;
    Serial.printf("walking %s\r\n", palette[colour_index].name);
  }
  delay(step_interval_ms);
}
