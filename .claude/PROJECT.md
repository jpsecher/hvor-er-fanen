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

An 8x8 WS2812B panel of 64 pixels on a NodeMCU 1.0, with data on D2 (GPIO4).  See [RGB Matrix wiring](../doc/rgb-matrix-wiring.md) for the two builds, the logic level trade and the power budget.

## Toolchain

The container compiles and the host flashes, because the dev container has no USB access.  See [RGB Matrix development](../doc/rgb-matrix-development.md) for the commands, the pinned versions and the problems that have already been solved.

Sketch source is `dev/rgb-matrix`, build output is `dev/build`, and the Arduino cores and libraries are in `dev/.arduino`.  The last two are git-ignored.

## Code style

- Numeric variables have unit suffixes (`delay_ns`) or count prefix (`n_members`)
- Function arguments should never be booleans, use Enums instead
- Serial output ends lines with `\r\n`, which is what Arduino `println` sends; a bare `\n` in `printf` makes each terminal line start where the previous one ended
