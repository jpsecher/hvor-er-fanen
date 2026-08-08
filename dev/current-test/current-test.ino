#include <ESP8266WiFi.h>
#include <FastLED.h>

constexpr uint8_t data_pin = D2;
constexpr uint16_t n_leds = 64;
constexpr uint8_t supply_volts = 5;
constexpr uint32_t hold_ms = 10000;

// The power limit must be off to measure what the panel really draws.  With it
// enforced, FastLED scales brightness down to stay inside the budget and the
// meter reads the budget instead of the LEDs.
enum class PowerLimit { off, enforced };
constexpr PowerLimit power_limit = PowerLimit::off;
constexpr uint32_t max_milliamps = 350;

const uint8_t brightness_steps[] = { 0, 8, 16, 32, 64, 128, 255 };
constexpr uint8_t n_steps = sizeof(brightness_steps) / sizeof(brightness_steps[0]);

CRGB leds[n_leds];
uint8_t step_index = 0;

static uint32_t estimated_milliamps(uint8_t brightness_level) {
  const uint32_t unscaled_mw = calculate_unscaled_power_mW(leds, n_leds);
  const uint32_t scaled_mw = unscaled_mw * brightness_level / 255;
  return scaled_mw / supply_volts;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  FastLED.addLeds<WS2812B, data_pin, GRB>(leds, n_leds);
  if (power_limit == PowerLimit::enforced) {
    FastLED.setMaxPowerInVoltsAndMilliamps(supply_volts, max_milliamps);
  }
  fill_solid(leds, n_leds, CRGB::White);
  Serial.println();
  Serial.printf("current-test: %u pixels white, %u steps, %u ms each\r\n", n_leds, n_steps, hold_ms);
  Serial.printf("power limit %s\r\n", power_limit == PowerLimit::enforced ? "enforced" : "off");
}

void loop() {
  const uint8_t brightness_level = brightness_steps[step_index];
  FastLED.setBrightness(brightness_level);
  FastLED.show();
  Serial.printf("brightness %u of 255, estimated %u mA\r\n", brightness_level, estimated_milliamps(brightness_level));
  delay(hold_ms);
  step_index = (step_index + 1) % n_steps;
  if (step_index == 0) {
    Serial.println("cycle complete");
  }
}
