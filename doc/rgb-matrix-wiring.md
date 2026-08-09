# RGB Matrix wiring

How to connect four 8x8 WS2812B panels to the NodeMCU as one 256-pixel chain.  There are two builds here: a bench build powered from USB for testing now, and a permanent build with an external supply.  Start with the bench build, because it needs no extra parts and it proves the firmware before any soldering is hard to undo.

## The panel

64 WS2812B pixels in an 8x8 grid, with three pads: 5V, GND and DIN.  The local datasheet is [ws2812b.pdf](./ws2812b.pdf), and the [original is from Worldsemi](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf).

Four of these panels are chained DOUT to DIN into one run of 256 pixels.  The measurements below were taken on a single panel, so multiply them by four wherever the whole display is meant.

Three figures from it govern everything below.  Supply voltage VDD is +3.5 to +5.3 V.  The electrical characteristics are only specified over the narrower range of 4.5 to 5.5 V.  A logic high on DIN needs 0.7 × VDD.

Per-pixel current is not in the datasheet.  The usual figures quoted for the part are about 1 mA per pixel with its LEDs off, because the controller is always running, and about 60 mA at full white.  This panel measured far below the second of those, at about 16.6 mA per pixel white, so the numbers below use the measured value where it matters.  See [what this panel actually draws](#what-this-panel-actually-draws).

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

USB gives 500 mA, and the NodeMCU takes 80 to 100 mA of it, leaving roughly 350 mA.  The idle panel takes about 56 mA of that before anything is lit.  At the measured 16.6 mA per white pixel, what remains is around 17 pixels at full white, and the whole panel at full white needs about 1.1 A, which is three times the USB budget.

Let the library enforce this rather than relying on care in each sketch:

    FastLED.setMaxPowerInVoltsAndMilliamps(5, 350);

FastLED then scales brightness down to stay inside the budget whatever the sketch asks for.

### Expect brownout resets, not crashes

The ESP8266 draws 300 to 400 mA in bursts when the WiFi radio transmits.  With the LEDs drawing at the same time, the 5 V rail sags, the onboard regulator's 3.3 V output dips, and the chip resets.  This looks exactly like a firmware crash, and it usually appears only once WiFi connects.  Keep the radio off for the first tests so it is not a variable, and add the power limit above before turning it on.

### What the bench build does not prove

It exercises the data path, the library, the pin choice and the firmware.  It says nothing about behaviour at full brightness, colour balance at high drive, or how voltage holds up across the panel.  A good result here means the pipeline works, not that the design is finished.

## Permanent build, external supply

This panel measured 1116 mA with all 64 pixels white, which is three times what USB can give, so the panels are fed from the supply directly and the NodeMCU only shares its ground.  Never power a panel through the NodeMCU.

Size the supply for the whole display, not one panel.  Four panels all white is about **4.5 A**, so a 2 A supply is enough for one panel but not for four; allow 6 A to keep margin.  Feed 5 V and GND to every panel from the supply rather than passing power along the panel-to-panel jumpers, which are sized for one panel's share and not for four.

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

## Frame rate

The protocol fixes the cost at **30 µs per pixel**: 800 kbps gives 1.25 µs per bit, and each pixel takes 24 bits.  Frame time therefore scales with the length of the chain and nothing else.

| Chain length | Frame time | Maximum rate |
| --- | --- | --- |
| 64, one panel | 1.92 ms | ~520 fps |
| 256, four panels | 7.68 ms | ~130 fps |
| 1024 | 30.7 ms | ~32.6 fps |

The datasheet line "when the refresh rate is 30fps, cascade number are not less than 1024 points" is easy to misread as a limit.  It is a statement about how long a chain can be at that rate, and it agrees with the 1024 row above.  It says nothing about a small panel.

A single chain of 256 runs at about 130 fps, which is far more than the display needs, so the four panels are wired as one chain.

### Why not parallel output

FastLED can drive several chains at once on the ESP8266, through `InlineBlockClocklessController` in `platforms/esp/8266/clockless_block_esp8266.h`, used as `FastLED.addLeds<WS2811_PORTA, LANES>`.  It transposes the pixel data and toggles several GPIOs on one port together, so the lanes genuinely cost the same time as one.

It is not needed here, and it is awkward.  The lane pins are fixed by `PORT_MASK` and `FIX_BITS` to GPIO 12, 13, 14 and 15, then GPIO 4 and 5, with six lanes the maximum and every lane the same length.  Four lanes therefore forces D8 (GPIO15), the boot strap pin this document already says to avoid.  Two lanes on D6 and D7 is the only arrangement that avoids that pin.  The reason to use it would be shortening the `show` window against WiFi interrupts, not frame rate.

Do not expect four ordinary `addLeds` calls on four pins to help.  `CFastLED::show` walks its controllers as a linked list and drives them one after another, so the total time is unchanged.

## The capacitor

A bulk capacitor across the panel's supply absorbs the switch-on inrush and the fast current steps as pixels change, both of which otherwise appear as a voltage spike at the first pixel.

The 22 µF tantalum is acceptable for the bench build, where the current is small, but check its voltage rating first.  Tantalums are derated to half their rating in normal practice, so a 5 V rail wants a 10 V part and preferably a 16 V one.  A 6.3 V tantalum does not belong on this rail.

Do not carry it over to the external-supply build.  Tantalums fail short rather than open, sometimes with flame, and they specifically dislike the inrush from a stiff low-impedance supply, which is exactly what a 2 A bench PSU is.  Use a 1000 µF electrolytic there instead.

## Parts

- 470 Ω resistor in series with DIN, protecting the first pixel from edge reflections
- 22 µF tantalum rated 10 V or more for the bench build, replaced by a 1000 µF electrolytic for the permanent build
- 5 V supply of 6 A or more for the permanent build, covering all four panels white
- 74AHCT125, or a 3 A silicon rectifier such as a 1N5401, for the data level

## Pixel order

This panel is wired **progressive**: pixel 0 is the first pixel of the top row, every row runs left to right, and index 8 starts the next row back at the left edge.  It is not serpentine, which is what most 8x8 panels use and what the sketches assumed at first.

Anything that draws by coordinate needs this, so the sprite sketches carry a `panel_layout` constant with three settings.  Progressive reverses no rows.  The two serpentine settings reverse alternate rows, and there are two of them because which rows are reversed depends on the corner the data enters.

A wrong setting is easy to recognise once you know the shape of it.  Every second row is mirrored, so a moving sprite looks torn along horizontal lines and the tearing shifts as it moves.  A symmetric sprite hides the fault completely, which is why the ghost looked plausible while the walking lemming did not.  Test with something asymmetric and moving.

### Panel order and rotation

With four panels in one chain there are two more facts to establish, and neither can be seen by looking at the assembled board: which panel comes first in the chain, and how each one is rotated.  Both are decided by how the panels were placed and jumpered, and the drawing code needs them.

Do not rely on notes taken during assembly.  Read both facts off the display instead, using the walk in `dev/rgb-matrix` described under [bring-up order](#bring-up-order).  The walk steps through the raw chain index, so:

- the quadrant that lights during each block of 64 gives the panel's place in the chain
- the corner holding the first pixel of a panel, which stays lit longest, gives that panel's origin
- the direction the following pixels sweep gives the rotation
- where pixel 8 lands relative to pixel 0 confirms progressive against serpentine, for that panel on its own

Record the result here once it is known, as a table of panel position against chain order and rotation.  That table is what the 16x16 coordinate mapping is built from.

## Measuring the current

The sketch in `dev/current-test` fills all 64 pixels white and steps brightness through 0, 8, 16, 32, 64, 128 and 255, holding each for six seconds so there is time to read a meter.  Build it with `just sketch=current-test build`, which leaves the display sketch untouched.

It runs with the power limit off, set by the `power_limit` constant.  That is deliberate and it is the whole point: with the limit enforced, FastLED scales brightness down to stay inside the budget, so the meter reads the budget rather than the panel.

Put the meter in series with the panel's 5 V wire, not across it, and use the 10 A jack.  The milliamp jack on most meters is fused at 200 to 500 mA and this test goes well past that.  Measuring the panel wire rather than the USB input keeps the NodeMCU's own draw out of the reading.

The first step sits at brightness 0, which measures the floor: 64 controllers that are always running, with every LED off.  From there the draw climbs close to linearly with brightness.

On USB the board resets partway up the list.  That is the supply collapsing, not a fault in the sketch, and where it happens is itself a useful number.

### What this panel actually draws

All 64 pixels white at full brightness on the external supply measured **1116 mA** at the panel.  Taking the idle floor out of that leaves about 16.6 mA per white pixel, or 5.5 mA per channel.

That is far below the figure the common WS2812B numbers give.  Those assume roughly 20 mA per channel and predict about 3.9 A for the same test, so FastLED's own estimate, which uses them, was more than three times high.  Treat 1116 mA as the number for this panel and the 3.9 A figure as not applicable to it.

A second measurement confirms that this is the panel and not the supply.  A ghost sprite at brightness 32, which lights 52 pixels in one colour and 4 in white, measured about 100 mA.  Standard current figures predict roughly 220 mA for the same picture, and at that level nothing could have been limiting a 2 A supply, so the standard figures are wrong for this panel rather than the earlier reading being clamped.

Solving the two measurements together gives the panel's characteristics:

| Quantity | This panel | Standard WS2812B figures |
| --- | --- | --- |
| Current per channel at full | ~5.5 mA | ~20 mA |
| One white pixel at full | ~16.6 mA | ~60 mA |
| All 64 white at full | 1116 mA measured | ~3.9 A |
| Idle floor, all LEDs off | ~56 mA | ~64 mA |

The derived idle floor agrees with the roughly 64 mA expected from 64 always-running controllers, which is a useful check that the model fits.  The result is also insensitive to the exact ghost reading: anywhere from 90 to 110 mA gives between 5.5 and 5.6 mA per channel.

The practical consequence is that the panel is undemanding.  One panel fully lit and white draws a little over 1 A, so the earlier concern about needing nearly 4 A for it does not apply to this hardware.  USB still cannot drive a whole panel white, but it can drive far more of it than the standard figures suggest.  Across four panels the same figure comes to about 4.5 A, which is what the supply must be sized for.

To confirm the idle floor directly rather than by inference, run `dev/current-test` and read the meter during its brightness 0 step.

## Bring-up order

Work up in steps, so a failure points at one thing.  The sketch in `dev/rgb-matrix` turns the radio off, which keeps step 5 out of the way until you want it.

1. Flash with WiFi off, the power limit set, and brightness low
2. Light one pixel red, which works even when the supply is marginal
3. Light one pixel green, then blue, which is where a low supply shows itself
4. Light a few pixels white, and watch for the board resetting
5. Enable WiFi last, and watch for resets again

The colour order matters for reading the result.  If the red pass looks right and the green or blue pass is dim or missing, the supply is too low for those dies and the data line is fine.  If pixels light in the wrong colour or at random positions in every pass, the problem is the data line rather than the supply.

The sketch walks a single lit pixel through all 256 positions in one colour, then moves to the next, covering red, green, blue, yellow, cyan, magenta and white.  The three primaries test each die on its own, the three secondaries test each pair, and white tests all three together.  Serial names each colour as its pass begins, so a fault can be tied to a colour without watching the panel continuously.

It doubles as the assembly check, which is why it steps through the raw chain index instead of a coordinate.  Serial announces each panel as the walk enters it, at indices 0, 64, 128 and 192, and that first pixel is held for `panel_start_ms` rather than `step_interval_ms` so there is time to see which corner it is in.  Every step also reports its chain index, panel, panel-relative pixel and the (x, y) the pixel would have if the panel is progressive — a mismatch against what the panel shows is the result being measured, not a fault.  See [panel order and rotation](#panel-order-and-rotation) for how to read it.

`n_panels` is the only constant that needs changing.  Set it to 1 to walk a single panel, which is how a suspect panel is tested on its own before it goes into the chain.

One pixel at a time means the lit draw stays near the idle floor whatever the colour, and the sketch runs at full brightness with no power limit.  The floor itself has grown with the chain, though: 256 always-running controllers take about 224 mA at the measured 0.875 mA each, and with the NodeMCU's own 80 to 100 mA that is already at the edge of the USB budget before anything is lit.  **Run the 256-pixel walk on the external supply.**  A power limit does not rescue it, because FastLED can only scale the lit pixels and not the controller idle current.  Lighting the whole panel is the job of `dev/current-test` instead.
