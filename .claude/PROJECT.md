# RGB Matrix

This part of the project only concerns itself with `dev/`.  Ignore everything else.

## Specification

See `.claude/<topic>.allium` for the Allium behavioural spec.

## Behaviour

A mechanical switch with three positions selects what the display does.

- **Off** removes power from the whole device, rather than showing a black frame.  This is the only way to stop the panels drawing, because 256 always-running controllers take about 224 mA whatever is displayed
- **Sprites** cycles through retro game sprites, which is what the sketches in `dev/` already draw
- **Map** fetches data from a backend API over plain HTTP and plots it on a fixed spatial map.  The map is mostly unlit, with around ten pixels lit at once

The switch passes cell current on one pole and gives the MCU a single input on the other, saying which of the two live modes it is in.  Off needs no input, because there is no power to read it with.

## Technology

- Arduino Core
- arduino-cli
- WS2812B matrix
- ESP8266
- FastLED
- ESP8266WiFi & ESP8266HTTPClient, over plain HTTP so that TLS needs no heap

## Hardware

Four 8x8 WS2812B panels arranged 2x2 as a 16x16 display, chained into one run of 256 pixels on a NodeMCU 1.0.  Each panel is progressive.  The chain order and the rotation of each panel are read off the display with the walk in `dev/rgb-matrix`.  Powered from a single 18650 cell driving the panels directly, with an AP130-33 regulator for the ESP8266, an MCP73831 charger and an AP9101C protection circuit driving two AOB2146L MOSFETs.  See [RGB Matrix wiring](../doc/rgb-matrix-wiring.md) for the wiring, the pin allocation, the power budget, the frame rate and the options that were rejected.

## Toolchain

The container compiles and the host flashes, because the dev container has no USB access.  See [RGB Matrix development](../doc/rgb-matrix-development.md) for the commands, the pinned versions and the troubleshooting notes.

Sketch source is `dev/rgb-matrix`, build output is `dev/build`, and the Arduino cores and libraries are in `dev/.arduino`.

## Code style

- Numeric variables have unit suffixes (`delay_ns`) or count prefix (`n_members`)
- Function arguments should never be booleans, use Enums instead
- Serial output ends lines with `\r\n`, which is what Arduino `println` sends
