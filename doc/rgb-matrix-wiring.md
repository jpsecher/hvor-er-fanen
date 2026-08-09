# RGB Matrix wiring

How to connect four 8x8 WS2812B panels to the NodeMCU as one 256-pixel chain.  There are three builds here: a bench build powered from USB for testing now, a permanent build with an external 5 V supply, and the intended [battery build on a single 18650 cell](#battery-build-single-18650-cell).  Start with the bench build, because it needs no extra parts and it proves the firmware before any soldering is hard to undo.

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

All of this applies to a 5 V build.  A cell sits between 3.5 and 4.2 V, which puts the threshold at 2.45 to 2.94 V and leaves margin everywhere, so the [battery build](#battery-build-single-18650-cell) does not have this problem and needs no level shifter.  Skip the rest of this trade if that is the build you are making.

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

This is a property of USB through a thin cable rather than of the firmware.  A cell has an internal resistance near 30 mΩ, so the same 400 mA burst sags it about 12 mV, and the [battery build](#battery-build-single-18650-cell) should largely be free of this.  Still enable the radio as the last bring-up step, so that if resets do appear the cause is not in doubt.

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

## Battery build, single 18650 cell

The panels run straight from the cell with no converter of any kind, and the ESP8266 gets its own 3.3 V regulator.  This is the intended build.

The panel measurements in this document are taken from this hardware and can be trusted.  Everything about the battery is calculated from them together with standard 18650 and ESP8266 figures, and none of it has been checked on a bench yet.  Treat the runtimes as estimates until they are measured.

### Why the cell drives the panel directly

A cell runs 4.2 V charged to about 3.0 V empty, and the WS2812B wants 3.5 V absolute minimum.  That looks like it needs a boost converter to 5 V, but it does not, for a reason that is easy to miss: **the WS2812B drives its LEDs with constant-current sinks.**  Channel current does not follow VDD as long as there is headroom above the LED forward voltage.  The panel therefore draws the same milliamps at 3.7 V as it does at 5 V, which is the same current at a lower voltage and so less power.  Boosting to 5 V would raise the voltage without reducing the current, and add converter losses as well.

There are two further gains.  Direct drive removes the converter, its cost and its inrush, and it removes the level shifting problem this document spends most of its length on: at 3.5 to 4.2 V the DIN threshold is 2.45 to 2.94 V against a 3.3 V drive, so there is margin across the whole usable range.

    cell + ──┬── panel 5V pads
             ├── 1000uF +
             └── LDO in ── LDO out ── NodeMCU 3V3
    ESP D2 ──[470R]── panel DIN
    cell - ──┬── panel GND
             ├── 1000uF -
             └── NodeMCU GND

What it costs is the bottom of the discharge.  Below roughly 3.6 V the green and blue dies, whose forward voltages sit near 3.0 to 3.2 V, run out of headroom and white shifts towards orange.  A cell holds above 3.5 V for about 85 to 90% of its capacity, so this affects the last part of the discharge rather than all of it, and the colour shift is a usable warning that the cell is nearly empty.  Where it actually happens on these panels is not known.  The standard current figures were wrong for this hardware by more than three times, as [what this panel actually draws](#what-this-panel-actually-draws) records, so the standard voltage figures deserve the same suspicion.  Measure it: put one panel on a bench supply, show white at a moderate brightness, and walk the voltage down from 4.2 V in 0.1 V steps until the colour breaks.

### Runtime

| Display content | Cell current | Direct from cell | Through a 5 V boost |
| --- | --- | --- | --- |
| Blank, panel idle floor only | 244 to 294 mA | ~9 to 11 h | ~6.4 h |
| Sprite at brightness 32 | 420 to 470 mA | ~5.7 to 6.4 h | ~4 h |
| All 256 lit, one colour, brightness 128 | ~1030 mA | ~2.6 h | ~1.9 h |
| All white, full brightness | ~4550 mA | ~0.6 h | ~0.43 h |

Assumes 2700 mAh usable down to a 3.6 V cutoff, the 224 mA panel idle floor from 256 always-running controllers, and the ESP8266 between 20 mA with the radio off and 70 mA receiving.  The ranges in the first two rows are that difference.  Boost figures assume 90% efficiency and 3.6 V average, and get the full 3000 mAh because they can run the cell lower.

Two things follow.  Direct drive gives roughly 40% more runtime even after giving up the last 10% of the cell.  And the 224 mA idle floor cannot be reduced in software, because FastLED only scales the lit pixels, so **a blank display still empties the cell in about half a day**.  If the display should survive being left alone, it needs a MOSFET load switch that actually disconnects the panels, not a black frame.

### Powering the ESP8266

An input that starts above 3.3 V and ends below it looks like a job for a buck-boost, and it is not.  A modern LDO with 250 mV dropout stops regulating at 3.55 V, and the panel loses its colours at about the same place, so a buck-boost would buy capacity the display cannot use.  Efficiency is Vout/Vin, which at 3.3 V from 3.7 V is 89% — within a few points of a switcher, with no inductor and no switching noise beside a data line that has limited margin.

Do not use the AMS1117 on the NodeMCU.  Its 1.1 V dropout needs 4.4 V in, which is why VIN cannot be fed from a cell at all.

The part fitted here is an **AP130-33**: 3.3 V fixed, 300 mA maximum, dropout 0.4 to 0.5 V at 300 mA, quiescent current 100 µA, input up to 5.5 V, in a SOT-89-3 package with no enable pin.  See the [Diodes AP130 datasheet](https://www.diodes.com/assets/Datasheets/AP130.pdf).

Those two headline figures look poor beside the 600 mA parts below, and both are quoted at full load, which is not where this runs.  Dropout falls with load current, so at the loads that actually occur:

| ESP8266 state | Load | Dropout | Regulation stops at |
| --- | --- | --- | --- |
| Radio off, as the firmware runs today | ~20 mA | ~30 to 50 mV | ~3.35 V |
| Receiving continuously | ~70 mA | ~120 mV | ~3.42 V |
| Transmitting | 300 to 400 mA | over the 300 mA limit | — |

The dropout figures for 20 and 70 mA are scaled from the 300 mA datasheet number rather than read from a curve, so treat them as estimates.  They are far enough from the limit that the imprecision does not change anything: both sit below the roughly 3.6 V where the panel loses blue and green, so **the regulator outlives the display** and the argument above holds with margin.  Quiescent current of 100 µA is irrelevant beside a panel idle floor of 224 mA.

The real limit is the 300 mA ceiling, which the radio exceeds when it transmits.  Exceeding it makes the regulator current-limit, the rail sag and the chip reset, which looks exactly like the USB brownouts described earlier and is just as easy to mistake for a firmware fault.

Two things make transmitting workable on a 300 mA part.

Put **1000 µF on the 3.3 V rail** beside the ESP.  A capacitor cannot supply a whole burst — at the full 400 mA, holding the rail within 100 mV lasts about 55 µs against an 802.11b frame of one to two milliseconds.  But it does not have to, because the regulator supplies 300 mA of that and the capacitor only covers the difference: 0.1 A × 2 ms / 300 mV is about 670 µF, so 1000 µF carries the burst with margin.  Sizing the capacitor against the deficit rather than the whole burst is what makes the smaller regulator viable.

Reduce the transmit power with `WiFi.setOutputPower`.  The radio defaults to 20.5 dBm, and a display talking to an access point in the same room does not need it; running 802.11n nearer 14 dBm cuts the peak substantially.

SOT-89-3 has only input, output and ground, so there is no way to shut the regulator down from firmware.  If the ESP should ever be switched off rather than slept, it needs its own load switch, in the same way the panels do.

If the 300 mA ceiling later becomes a real constraint, these are the parts to fit instead.  Note that all of them are SOT-23-5 or SOT-25 and so need a different footprint from the AP130-33.

| Part | Package | Current | Dropout | Notes |
| --- | --- | --- | --- | --- |
| AP2112K-3.3 | SOT-23-5 | 600 mA | 250 mV at full load | Used on several ESP dev boards, easy to get and to solder |
| XC6220B331 | SOT-25 | 1 A | 200 mV | Most headroom for transmit bursts |
| ME6211C33 | SOT-23-5 | 500 mA | ~250 mV | Cheapest, common on LCSC |
| TLV75533P | SOT-23-5 | 500 mA | ~200 mV | Best datasheet if the curves need checking |

### NodeMCU or a bare module

Feeding regulated 3.3 V into the NodeMCU's 3V3 pin bypasses the AMS1117 and works, and flashing and serial keep working exactly as they do now.  The one rule is to **never have USB connected while the board is on battery**, because the LDO and the AMS1117 then drive the same rail against each other.  Add a Schottky or a jumper if that will happen by accident.

A bare ESP-12F on the board drops the AMS1117 and the USB serial chip, saves space and cuts idle draw, at the cost of providing a programming path: either a USB serial chip, or a header for an external adapter along with the boot strapping of GPIO0 and GPIO15 low and EN and GPIO2 high.  The existing `esptool` flow works unchanged with an adapter.

Keep the NodeMCU for now.  The panel mapping and the colour cutoff are both still unknown, and flashing that works without thought is worth more at this stage than the board space.  A NodeMCU footprint on the PCB leaves the choice open.

### WiFi receive

Receiving does not avoid transmitting.  An associated station acknowledges every unicast frame it receives, within 10 µs and in MAC hardware, whether the sketch sends anything or not, and association, DHCP and ARP are transmissions too.  **An application that only receives still needs a regulator sized for the transmit peak.**

Two modes genuinely never transmit, if that is what is wanted.  ESP-NOW broadcast receive needs no association and broadcast frames are not acknowledged.  Promiscuous mode only listens.  Neither works with `ESPAsyncWebServer`, which needs a normal association.

| ESP8266 state | Current at 3.3 V |
| --- | --- |
| Radio off | ~20 mA |
| Modem-sleep, waking on DTIM beacons | ~15 to 25 mA |
| Receiving continuously | ~56 to 70 mA |

WiFi is not what limits this build.  Against the 224 mA panel idle floor, continuous receive costs about 10% of runtime, so it is not worth contorting the design to save.  If it is wanted cheaply, modem-sleep powers the radio down between DTIM beacons and wakes it to listen, typically every 100 to 300 ms: everything still arrives, delayed by up to one beacon interval, at roughly a third of the current.

### Charging and protection

Charging is future work, but two decisions are worth knowing before the board is laid out.

A TP4056 is cheap and available everywhere, but it has no power path: the load hangs on the cell terminals, which disturbs charge termination and means the cell never sees a clean end of charge.  A BQ24074 costs more and comes in a QFN, but it runs the system from USB and charges the cell at the same time.  Choose it if the display should keep working while plugged in.

Protection is a separate circuit from charging, and here it needs care: **with the panel wired straight to the cell, the protection FETs carry panel current, not just the ESP's.**  The DW01A and FS8205 pairing found on most TP4056 modules trips somewhere around 2 to 3 A.  Moderate brightness is nowhere near that, but a full white frame would trip it.  Either note the limit as the real brightness ceiling, or choose protection sized for the panel.

### Board layout

**Star ground at the cell terminals.**  The panel and the ESP must return to the cell separately rather than sharing a trace.  Panel current swings by amps as pixels change, and if the ESP sits downstream of that, its ground reference moves with the display — and the DIN threshold is measured against exactly that reference.  Getting this wrong builds a board that works on the bench and glitches whenever the picture is busy.

Pour thick copper for the cell to panel path only.  The regulator branch is a few hundred milliamps and needs nothing special.  The 1000 µF electrolytic goes at the panel input, and the 470 Ω resistor stays in series with DIN, close to the panel.

Lay out a SOT-89-3 footprint for the regulator, and place the 1000 µF rail capacitor beside the ESP even if it is left unpopulated while the radio is off.  Enabling WiFi later then costs one component rather than a new board.

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

Common to every build:

- 470 Ω resistor in series with DIN, protecting the first pixel from edge reflections
- 1000 µF electrolytic at the panel input, or a 22 µF tantalum rated 10 V or more for the bench build only

For the permanent 5 V build:

- 5 V supply of 6 A or more, covering all four panels white
- 74AHCT125, or a 3 A silicon rectifier such as a 1N5401, for the data level

For the battery build, which needs neither of those two:

- 18650 cell, with protection sized for the panel current rather than a 2 to 3 A module
- AP130-33 low-dropout regulator for the ESP8266, in SOT-89-3, or a 600 mA part such as the AP2112K-3.3 if WiFi transmit needs more headroom
- 1000 µF on the 3.3 V rail, beside the ESP8266
- MOSFET load switch to disconnect the panels, without which the idle floor drains the cell
- TP4056, or a BQ24074 if the display should run while charging

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
