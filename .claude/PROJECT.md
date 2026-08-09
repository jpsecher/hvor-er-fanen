# RGB Matrix

This part of the project only concerns itself with `dev/`.  Ignore everything else.

## Specification

See `.claude/<topic>.allium` for the Allium behavioural spec.

## Technology

- Arduino Core
- arduino-cli
- WS2812B matrix
- ESP8266
- FastLED
- ESP8266WiFi & ESPAsyncWebServer

## Hardware

Four 8x8 WS2812B panels chained into 256 pixels on a NodeMCU 1.0, with data on D2 (GPIO4).  Each panel is progressive.  The chain order and the rotation of each panel are read off the display with the walk in `dev/rgb-matrix`.  See [RGB Matrix wiring](../doc/rgb-matrix-wiring.md) for the two builds, the logic level trade, the frame rate and the power budget.

## Toolchain

The container compiles and the host flashes, because the dev container has no USB access.  See [RGB Matrix development](../doc/rgb-matrix-development.md) for the commands, the pinned versions and the problems that have already been solved.

Sketch source is `dev/rgb-matrix`, build output is `dev/build`, and the Arduino cores and libraries are in `dev/.arduino`.

## Code style

- Numeric variables have unit suffixes (`delay_ns`) or count prefix (`n_members`)
- Function arguments should never be booleans, use Enums instead
- Serial output ends lines with `\r\n`, which is what Arduino `println` sends
