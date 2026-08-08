# RGB Matrix wiring

How to connect an 8x8 WS2812B panel to the NodeMCU.  There are two builds here: a bench build powered from USB for testing now, and a permanent build with an external supply.  Start with the bench build, because it needs no extra parts and it proves the firmware before any soldering is hard to undo.

## The panel

64 WS2812B pixels in an 8x8 grid, with three pads: 5V, GND and DIN.  The local datasheet is [ws2812b.pdf](./ws2812b.pdf), and the [original is from Worldsemi](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf).

Three figures from it govern everything below.  Supply voltage VDD is +3.5 to +5.3 V.  The electrical characteristics are only specified over the narrower range of 4.5 to 5.5 V.  A logic high on DIN needs 0.7 × VDD.

Per-pixel current is not in the datasheet.  In practice each pixel draws about 1 mA with its LEDs off, because the controller is always running, and about 60 mA at full white with all three channels at maximum.  A dark 64-pixel panel therefore still draws around 60 mA.

## Why the data line is the hard part

The ESP8266 drives 3.3 V, and the panel wants 0.7 × VDD.  Lowering the panel's supply lowers the threshold, which is the whole trade:

| Panel VDD | Threshold (0.7 VDD) | Margin against 3.3 V | Verdict |
| --- | --- | --- | --- |
| 5.0 V | 3.50 V | -0.20 V | below threshold, needs a level shifter |
| 4.7 V | 3.29 V | +0.01 V | measured VIN, works but has no margin |
| 4.3 V | 3.01 V | +0.29 V | good margin, just under the characterised floor |
| 3.3 V | 2.31 V | +0.99 V | below the supply minimum, colours break |

The bottom row is why a 3.3 V supply is not the easy answer it looks like.  The green and blue dies have forward voltages near 3.0 to 3.2 V, so at 3.3 V the internal current sink has no headroom left: red still lights, green and blue go dim or dark, and white turns orange.  The panel does not fail cleanly, it just looks wrong, and it varies with temperature and between production batches.

## Bench build, powered from USB

    NodeMCU VIN (4.7 V) ──┬── panel 5V
                          └── 22uF +
    NodeMCU GND ──────────┬── panel GND
                          └── 22uF -
    NodeMCU D2 (GPIO4) ──[470R]── panel DIN

VIN is USB 5 V behind the board's protection diode, measured here at 4.7 V.  That puts the threshold at 3.29 V against a 3.3 V drive, so it works, but with no margin to spare.  If pixels show random wrong colours, this is the cause and not the firmware.

Use D2 (GPIO4) for data.  Avoid D3, D4 and D8 (GPIO0, GPIO2, GPIO15), which set the boot mode and can stop the board starting when a panel is attached.  Avoid D0 (GPIO16) as well, because FastLED cannot bit-bang it on the ESP8266.

### Power budget

USB gives 500 mA, and the NodeMCU takes 80 to 100 mA of it, leaving roughly 350 mA.  The idle panel takes 60 mA of that before anything is lit.  What remains is about five pixels at full white, or a few dozen at low brightness.

Let the library enforce this rather than relying on care in each sketch:

    FastLED.setMaxPowerInVoltsAndMilliamps(5, 350);

FastLED then scales brightness down to stay inside the budget whatever the sketch asks for.

### Expect brownout resets, not crashes

The ESP8266 draws 300 to 400 mA in bursts when the WiFi radio transmits.  With the LEDs drawing at the same time, the 5 V rail sags, the onboard regulator's 3.3 V output dips, and the chip resets.  This looks exactly like a firmware crash, and it usually appears only once WiFi connects.  Keep the radio off for the first tests so it is not a variable, and add the power limit above before turning it on.

### What the bench build does not prove

It exercises the data path, the library, the pin choice and the firmware.  It says nothing about behaviour at full brightness, colour balance at high drive, or how voltage holds up across the panel.  A good result here means the pipeline works, not that the design is finished.

## Permanent build, external supply

64 pixels at full white need about 3.8 A at 5 V, so the panel is fed from the supply directly and the NodeMCU only shares its ground.  Never power the panel through the NodeMCU.

There are two ways to fix the data level, and they are a straight trade between part count and correctness.

A 74AHCT125 keeps the panel at a full 5 V, inside the characterised range, and converts the 3.3 V data to a clean 5 V signal.  This is the correct answer and the one to choose for a build that should not be revisited:

    PSU 5V ──┬── panel 5V
             └── 74AHCT125 Vcc
    ESP D2 ──> 74AHCT125 in ── out ──[470R]── panel DIN
    PSU GND ──┬── panel GND ── NodeMCU GND ── 74AHCT125 GND

A silicon diode in series with the panel supply is the alternative, dropping about 0.7 V to put the panel near 4.3 V and the threshold near 3.0 V:

    PSU 5V ──|>|── panel 5V (~4.3 V)
    ESP D2 ──[470R]── panel DIN
    PSU GND ──┬── panel GND
              └── NodeMCU GND

This needs no logic chip and gives a solid 0.29 V of margin, at the cost of running just below the 4.5 V characterisation floor and losing a little blue and green brightness.  Size the diode for the full panel current: a 1 A part such as a 1N4001 is not enough, so use a 3 A rectifier such as a 1N5401.  Do not substitute a Schottky, because its smaller drop leaves the panel near 4.6 V and the threshold back at 3.2 V, which is the marginal case again.

## The capacitor

A bulk capacitor across the panel's supply absorbs the switch-on inrush and the fast current steps as pixels change, both of which otherwise appear as a voltage spike at the first pixel.

The 22 µF tantalum is acceptable for the bench build, where the current is small, but check its voltage rating first.  Tantalums are derated to half their rating in normal practice, so a 5 V rail wants a 10 V part and preferably a 16 V one.  A 6.3 V tantalum does not belong on this rail.

Do not carry it over to the external-supply build.  Tantalums fail short rather than open, sometimes with flame, and they specifically dislike the inrush from a stiff low-impedance supply, which is exactly what a 2 A bench PSU is.  Use a 1000 µF electrolytic there instead.

## Parts

- 470 Ω resistor in series with DIN, protecting the first pixel from edge reflections
- 22 µF tantalum rated 10 V or more for the bench build, replaced by a 1000 µF electrolytic for the permanent build
- 5 V supply of 2 A or more for the permanent build
- 74AHCT125, or a 3 A silicon rectifier such as a 1N5401, for the data level

## Bring-up order

Work up in steps, so a failure points at one thing.  The sketch in `dev/rgb-matrix` turns the radio off, sets the power limit to 350 mA at 5 V and holds brightness at 32 of 255, which covers step 1 and keeps step 5 out of the way until you want it.

1. Flash with WiFi off, the power limit set, and brightness low
2. Light one pixel red, which works even when the supply is marginal
3. Light one pixel green, then blue, which is where a low supply shows itself
4. Light a few pixels white, and watch for the board resetting
5. Enable WiFi last, and watch for resets again

The colour order matters for reading the result.  If the red pass looks right and the green or blue pass is dim or missing, the supply is too low for those dies and the data line is fine.  If pixels light in the wrong colour or at random positions in every pass, the problem is the data line rather than the supply.

The sketch walks a single pixel through all 64 positions in one colour, then moves to the next, covering red, green, blue, yellow, cyan, magenta and white.  The three primaries test each die on its own, the three secondaries test each pair, and white tests all three together, which is also the highest current the pattern draws.  Serial names each colour as its pass begins, so a fault can be tied to a colour without watching the panel continuously.
