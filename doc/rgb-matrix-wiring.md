# RGB Matrix wiring

How to connect four 8x8 WS2812B panels to a NodeMCU as one 256-pixel chain, powered from a single 18650 cell.  The panels run straight from the cell with no converter, and the ESP8266 has its own 3.3 V regulator.  Other ways of doing it are under [alternatives](#alternatives).

Panel figures here are measured on this hardware.  Battery figures are calculated from them together with standard 18650 and ESP8266 characteristics, and are unverified, so every runtime is an estimate.

## The panel

64 WS2812B pixels in an 8x8 grid, with three pads: 5V, GND and DIN.  The local datasheet is [ws2812b.pdf](./ws2812b.pdf), and the [original is from Worldsemi](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf).

Four of these are chained DOUT to DIN into one run of 256 pixels.  Measurements quoted for one panel need multiplying by four wherever the whole display is meant.

Three figures from the datasheet govern everything below.  Supply voltage VDD is +3.5 to +5.3 V.  The electrical characteristics are only specified over the narrower range of 4.5 to 5.5 V.  A logic high on DIN needs 0.7 × VDD.

Per-pixel current is not in the datasheet.  This panel draws about 16.6 mA per white pixel and about 0.875 mA per pixel with its LEDs off, the second because the controller is always running.  Both are measured, and both are well below the figures usually quoted for the part.  See [what this panel actually draws](#what-this-panel-actually-draws).

## Pixel order

This panel is wired **progressive**: pixel 0 is the first pixel of the top row, every row runs left to right, and index 8 starts the next row back at the left edge.  Most 8x8 panels are serpentine instead.

Anything that draws by coordinate needs this, so the sprite sketches carry a `panel_layout` constant with three settings.  Progressive reverses no rows.  The two serpentine settings reverse alternate rows, and there are two of them because which rows are reversed depends on the corner the data enters.

A wrong setting mirrors every second row, so a moving sprite looks torn along horizontal lines and the tearing shifts as it moves.  A symmetric sprite hides the fault completely, so test with something asymmetric and moving.

### Panel order and rotation

With four panels in one chain there are two more facts to establish, and neither can be seen by looking at the assembled board: which panel comes first in the chain, and how each one is rotated.  Both are decided by how the panels were placed and jumpered, and the drawing code needs them.

Do not rely on notes taken during assembly.  Read both facts off the display instead, using the walk in `dev/rgb-matrix` described under [bring-up order](#bring-up-order).  The walk steps through the raw chain index, so:

- the quadrant that lights during each block of 64 gives the panel's place in the chain
- the corner holding the first pixel of a panel, which stays lit longest, gives that panel's origin
- the direction the following pixels sweep gives the rotation
- where pixel 8 lands relative to pixel 0 confirms progressive against serpentine, for that panel on its own

Record the result here once it is known, as a table of panel position against chain order and rotation.  That table is what the 16x16 coordinate mapping is built from.

## The data line

Use D2 (GPIO4) for data, with a 470 Ω resistor in series close to the panel, protecting the first pixel from edge reflections.  Avoid D3, D4 and D8 (GPIO0, GPIO2, GPIO15), which set the boot mode and can stop the board starting when a panel is attached.  Avoid D0 (GPIO16) as well, because FastLED cannot bit-bang it on the ESP8266.

The ESP8266 drives 3.3 V and the panel needs 0.7 × VDD, so the panel's supply voltage decides whether the data line works at all:

| Panel VDD | Threshold (0.7 VDD) | Margin against 3.3 V |
| --- | --- | --- |
| 5.0 V | 3.50 V | -0.20 V, needs a level shifter |
| 4.2 V, cell charged | 2.94 V | +0.36 V |
| 3.5 V, cell nearly empty | 2.45 V | +0.85 V |

Running the panel from a cell therefore has margin across the whole usable range, and needs no level shifter.

## Power

The cell feeds the panels directly, over a range of 4.2 V charged to about 3.0 V empty against the WS2812B minimum of 3.5 V.

No boost converter is needed, because the WS2812B drives its LEDs with constant-current sinks.  Channel current does not follow VDD as long as there is headroom above the LED forward voltage, so the panel draws the same current at 3.7 V as at 5 V.  That is the same current at a lower voltage, and so less power.  Boosting raises the voltage without reducing the current, and adds converter losses on top.

    cell + ──┬── panel 5V pads
             ├── 1000uF +
             └── LDO in ── LDO out ── NodeMCU 3V3
    ESP D2 ──[470R]── panel DIN
    cell - ──┬── panel GND
             ├── 1000uF -
             └── NodeMCU GND

The 1000 µF electrolytic across the panel supply absorbs the switch-on inrush and the fast current steps as pixels change, both of which otherwise appear as a voltage spike at the first pixel.

When the cell is below 3.6 V the blue LED dies, because its forward voltage sit near 3.2 V.  When it runs out of headroom, then white shifts towards orange.  A cell holds above 3.5 V for about 90% of its capacity, so this affects only the last part of the discharge, and the colour shift serves as a warning that the cell is nearly empty.

The exact voltage for these panels is unknown, and the standard figures are not a safe guide, since the standard current figures are wrong for this hardware by more than three times.  Measure it: put one panel on a bench supply, show white at a moderate brightness, and step the voltage down from 4.2 V in 0.1 V steps until the colour breaks.

### Runtime

| Display content | Cell current | Runtime |
| --- | --- | --- |
| Blank, panel idle floor only | 244 to 294 mA | ~9 to 11 h |
| **Map mode, about ten pixels lit** | **274 to 374 mA** | **~7 to 10 h** |
| Sprite at brightness 32 | 420 to 470 mA | ~5.7 to 6.4 h |
| All 256 lit, one colour, brightness 128 | ~1030 mA | ~2.6 h |
| All white, full brightness | ~4550 mA | ~0.6 h |

Assumes 2700 mAh usable down to a 3.6 V cutoff, the 224 mA panel idle floor from 256 always-running controllers, and the ESP8266 between 20 mA with the radio off and 70 mA receiving.  The ranges are mostly that difference.  Map mode is the normal operating point; its ten lit pixels add only 30 to 80 mA depending on colour and brightness, which is why it sits so close to the blank row.

**The idle floor dominates every realistic mode**, and it cannot be reduced in software, because FastLED only scales the lit pixels.  A blank frame still empties the cell in about half a day.  That is why the switch removes power in its off position rather than displaying black.

### Powering the ESP8266

An LDO is sufficient, despite the input starting above 3.3 V and ending below it.  One with 250 mV dropout stops regulating at 3.55 V, which is about where the panel loses its colours, so a buck-boost would only buy capacity the display cannot use.  LDO efficiency is Vout/Vin, which at 3.3 V from 3.7 V is 89% — within a few points of a switcher, with no inductor and no switching noise beside a data line that has limited margin.

The part fitted is an **AP130-33**: 3.3 V fixed, 300 mA guaranteed, input 2.7 to 5.5 V, quiescent current 50 µA, with current limit, thermal shutdown and short-circuit fold-back, in a SOT-89-3 package with no enable pin.  The datasheet is [AP130.pdf](./AP130.pdf).

The pass element is a PMOS with **RON of 1.5 Ω**, so dropout is resistive and follows the load:

| ESP8266 state | Load | Dropout | Regulation stops at |
| --- | --- | --- | --- |
| Radio off, as the firmware runs today | ~20 mA | 30 mV | ~3.33 V |
| Receiving continuously | ~70 mA | 105 mV | ~3.41 V |
| Transmitting | 300 to 400 mA | 450 mV at 300 mA | inside the current limit |

Both of the first two sit below the roughly 3.6 V where the panel loses blue and green, so the regulator outlives the display.  Quiescent current of 50 µA is irrelevant beside a panel idle floor of 224 mA.

The transmit row is the constraint.  Current limit is 350 mA minimum and 450 mA typical, so a 400 mA burst lands inside the limit band rather than safely under it, and the part may or may not carry it depending on the individual device.  In limit the rail sags and the chip resets, so the symptom is a reset that appears only once the radio is enabled, rather than a fault in the sketch.  Two things make transmitting workable.

Put **1000 µF on the 3.3 V rail** beside the ESP.  The regulator supplies at least 350 mA of a 400 mA burst, so the capacitor covers a 50 to 100 mA difference: at the worst of that, 0.1 A × 2 ms / 300 mV is about 670 µF.

Fit at least **10 µF from OUT to ground**, which the datasheet requires for stability whatever else is on the rail, and 1 µF at IN.

Reduce the transmit power with `WiFi.setOutputPower`.  The radio defaults to 20.5 dBm, and a display talking to an access point in the same room does not need it; running 802.11n nearer 14 dBm cuts the peak substantially.

SOT-89-3 has only input, output and ground, so there is no way to shut the regulator down from firmware.  If the ESP should ever be switched off rather than slept, it needs its own load switch, in the same way the panels do.

### WiFi

The application only receives, but **that does not mean the radio never transmits.**  An associated station acknowledges every unicast frame it receives, within 10 µs and in MAC hardware, whether the sketch sends anything or not; association, DHCP and ARP are transmissions; and an HTTP server sends a response to every request.  This is why the regulator above is sized for the transmit peak rather than for the receive current.

| ESP8266 state | Current at 3.3 V |
| --- | --- |
| Radio off | ~20 mA |
| Modem-sleep, waking on DTIM beacons | ~15 to 25 mA |
| Receiving continuously | ~56 to 70 mA |

WiFi is not what limits this build.  Against the 224 mA panel idle floor, continuous receive costs about 10% of runtime, so it is not worth contorting the design to save.  If it is wanted cheaply, modem-sleep powers the radio down between DTIM beacons and wakes it to listen, typically every 100 to 300 ms: everything still arrives, delayed by up to one beacon interval, at roughly a third of the current.

### Charging and protection

The charger is an **MCP73831T-2ACI/OT**, a linear constant-current then constant-voltage single-cell controller in SOT-23-5, identified by its `KD` marking code.  The datasheet is [MCP73831-Family-Data-Sheet-DS20001984H.pdf](./MCP73831-Family-Data-Sheet-DS20001984H.pdf).

The suffixes matter, because the family covers cells this one is not:

- **-2** is 4.20 V regulation, which is the correct one for a standard cell.  The -3, -4 and -5 variants regulate to 4.35, 4.40 and 4.50 V
- **AC** sets preconditioning to 10% of the programmed current below 2.79 V, charge termination to 7.5% of it, and automatic recharge below 4.05 V
- **/OT** is the 5-lead SOT-23

It takes 3.75 to 6 V in, with UVLO at about 3.45 V rising and 3.38 V falling, folds charge current back on die temperature, and shuts down at 150 °C.

**Charge current** is set by a resistor from PROG to ground, as **I = 1000 V / R_PROG**, valid from 2 kΩ giving 500 mA down to 67 kΩ giving 15 mA.

Use **3.3 kΩ, for about 300 mA**.  The full 500 mA is not useful here: from a 5 V input into a 3.7 V cell it puts 0.65 W into a SOT-23-5, which drives the part into thermal regulation and folds the current back anyway.  At 300 mA a 3000 mAh cell takes roughly 10 to 12 hours.

Fit 4.7 µF at the input and at VBAT, and 470 Ω in series with an LED on STAT if a charge indicator is wanted.

**The display must be off while charging.**  This charger has five pins — VDD, VSS, VBAT, STAT and PROG — so there is no system output and the load necessarily sits on the cell terminals.

**Charge termination is 7.5% of the programmed current, which is 22.5 mA at 300 mA.**  A blank display already draws 244 to 294 mA and a sprite 420 to 470 mA from the same node, so the current never falls to the termination threshold and the charge cycle never completes.  At sprite brightness the load exceeds the charge current outright and the cell discharges while nominally charging.

Blanking the panels during charging is therefore required, not optional, and the part gives two ways to arrange it:

- **STAT** is a tri-state output on the MCP73831, open-drain on the MCP73832, so firmware can tell when a charger is connected.  D1, D5, D6 and D7 are all free
- **PROG above about 200 kΩ shuts the charger down**, the datasheet giving 70 to 200 kΩ as the minimum shutdown impedance.  A MOSFET switching the programming resistor therefore lets firmware stop and start charging deliberately

**Protection** is a separate circuit from charging, and the MCP73831 provides none: its reverse-discharge protection stops the cell draining back into VDD when the input is removed, which is not over-discharge protection.

**With the panel wired straight to the cell, the protection FETs carry panel current, not just the ESP's.**  The DW01A and FS8205 pairing found on most charger modules trips somewhere around 2 to 3 A.  Moderate brightness is nowhere near that, but a full white frame would trip it.  Either note the limit as the real brightness ceiling, or choose protection sized for the panel.

### Board layout

Pins, with D0, D3, D4 and D8 excluded for the reasons under [the data line](#the-data-line):

| Signal | Pin | Notes |
| --- | --- | --- |
| Panel data | D2 (GPIO4) | through the 470 Ω resistor |
| Mode select | D1 (GPIO5) | the switch's signal pole, with a pull-up |
| Charger STAT | D6 (GPIO12) | so the panels can be blanked while charging |
| Ambient light | A0 | the only analog input, so nothing else may claim it |
| Spare | D5, D7 | |

**The mode switch carries the cell current.**  It is a 2-pole 3-position ON-OFF-ON toggle: the outer positions of the power pole are commoned so both live modes pass current and the centre breaks it, and the signal pole grounds D1 in one of them.  Size it for the worst frame the firmware can produce — a 6 A part covers a full white panel at 4.5 A, and a 3 A part means capping brightness with `setMaxPowerInVoltsAndMilliamps`, remembering that FastLED over-predicts this panel threefold so the figure passed must be about three times the real limit wanted.

**Star ground at the cell terminals.**  The panel and the ESP must return to the cell separately rather than sharing a trace.  Panel current swings by amps as pixels change, and if the ESP sits downstream of that, its ground reference moves with the display — and the DIN threshold is measured against exactly that reference.  Getting this wrong builds a board that works on the bench and glitches whenever the picture is busy.

Pour thick copper for the cell to panel path only.  The regulator branch is a few hundred milliamps and needs nothing special.  The 1000 µF electrolytic goes at the panel input, and the 470 Ω resistor stays in series with DIN, close to the panel.

Lay out a SOT-89-3 footprint for the regulator, and place the 1000 µF rail capacitor beside the ESP even if it is left unpopulated while the radio is off.  Enabling WiFi later then costs one component rather than a new board.

**SOT-89-3 pin order is not standard between parts, so check it before substituting a regulator.**  Diodes ships the AP130 in two SOT-89 variants with the pins in different places, and the identification code in the top marking is what tells them apart:

| Part | Package | Marking | Pin 1 | Pin 2 | Pin 3 |
| --- | --- | --- | --- | --- | --- |
| AP130-33YR | SOT89R-3L | **H9** | GND | IN | OUT |
| AP130-33Y | SOT89-3L | CS | OUT | GND | IN |
| HT7833 | SOT89 | — | GND | VIN | VOUT |

The part in hand is marked **H9**, so it is the YR variant and the footprint is GND, IN, OUT.  The HT7833 has the same order, so it drops into that footprint without change.  A part marked CS does not.

## Frame rate

The protocol fixes the cost at **30 µs per pixel**: 800 kbps gives 1.25 µs per bit, and each pixel takes 24 bits.  Frame time therefore scales with the length of the chain and nothing else.

| Chain length | Frame time | Maximum rate |
| --- | --- | --- |
| 64, one panel | 1.92 ms | ~520 fps |
| 256, four panels | 7.68 ms | ~130 fps |
| 1024 | 30.7 ms | ~32.6 fps |

The datasheet line "when the refresh rate is 30fps, cascade number are not less than 1024 points" is easy to misread as a limit.  It is a statement about how long a chain can be at that rate, and it agrees with the 1024 row above.  It says nothing about a small panel.

A single chain of 256 runs at about 130 fps, which is far more than the display needs, so the four panels are wired as one chain.

## Parts

- 470 Ω resistor in series with DIN, protecting the first pixel from edge reflections
- 1000 µF electrolytic at the panel input
- 18650 cell, with protection sized for the panel current rather than a 2 to 3 A module
- AP130-33YR low-dropout regulator for the ESP8266, in SOT-89-3, marked H9
- 10 µF at the regulator output, required for stability, and 1 µF at its input
- 1000 µF on the 3.3 V rail, beside the ESP8266
- 2-pole 3-position ON-OFF-ON toggle, off in the centre, rated for the worst frame the firmware can produce
- MCP73831T-2ACI/OT charge controller, in SOT-23-5, marked KD
- 3.3 kΩ from PROG to ground, setting the charge current to about 300 mA
- 4.7 µF at the charger input and at VBAT
- 470 Ω and an LED on STAT, if a charge indicator is wanted

## Bring-up order

Work up in steps, so a failure points at one thing.  The sketch in `dev/rgb-matrix` turns the radio off, which keeps step 5 out of the way until you want it.

1. Flash with WiFi off and brightness low
2. Light one pixel red, which works even when the supply is marginal
3. Light one pixel green, then blue, which is where a low supply shows itself
4. Light a few pixels white, and watch for the board resetting
5. Enable WiFi last, and watch for resets again

The colour order matters for reading the result.  If the red pass looks right and the green or blue pass is dim or missing, the supply is too low for those dies and the data line is fine.  If pixels light in the wrong colour or at random positions in every pass, the problem is the data line rather than the supply.

Step 5 is where a brownout would appear, because a transmit burst of 300 to 400 mA is the largest step the regulator ever sees.  A cell is a stiff source — around 30 mΩ, so the same burst sags it about 12 mV — so resets here point at the regulator or the rail capacitor rather than at the cell.

The sketch walks a single lit pixel through all 256 positions in one colour, then moves to the next, covering red, green, blue, yellow, cyan, magenta and white.  The three primaries test each die on its own, the three secondaries test each pair, and white tests all three together.  Serial names each colour as its pass begins, so a fault can be tied to a colour without watching the panel continuously.

It doubles as the assembly check, which is why it steps through the raw chain index instead of a coordinate.  Serial announces each panel as the walk enters it, at indices 0, 64, 128 and 192, and that first pixel is held for `panel_start_ms` rather than `step_interval_ms` so there is time to see which corner it is in.  Every step also reports its chain index, panel, panel-relative pixel and the (x, y) the pixel would have if the panel is progressive — a mismatch against what the panel shows is the result being measured, not a fault.  See [panel order and rotation](#panel-order-and-rotation) for how to read it.

`n_panels` is the only constant that needs changing.  Set it to 1 to walk a single panel, which is how a suspect panel is tested on its own before it goes into the chain.

One pixel at a time means the lit draw stays near the idle floor whatever the colour, so the sketch runs at full brightness with no power limit.  The floor itself has grown with the chain, though: 256 always-running controllers take about 224 mA at the measured 0.875 mA each, which is more than USB has left after the NodeMCU takes its share.  **Run the 256-pixel walk from the cell or a bench supply, not from USB.**  A power limit does not rescue it, because FastLED can only scale the lit pixels and not the controller idle current.  Lighting the whole panel is the job of `dev/current-test` instead.

## Measuring the current

The sketch in `dev/current-test` fills all 64 pixels white and steps brightness through 0, 8, 16, 32, 64, 128 and 255, holding each for six seconds so there is time to read a meter.  Build it with `just sketch=current-test build`, which leaves the display sketch untouched.

The `power_limit` constant turns the power limit off, which the test requires: with the limit enforced, FastLED scales brightness down to stay inside the budget, so the meter reads the budget rather than the panel.

Put the meter in series with the panel's supply wire, not across it, and use the 10 A jack.  The milliamp jack on most meters is fused at 200 to 500 mA and this test goes well past that.  Measuring the panel wire rather than the supply input keeps the NodeMCU's own draw out of the reading.

The first step sits at brightness 0, which measures the floor: 64 controllers that are always running, with every LED off.  From there the draw climbs close to linearly with brightness.

If the board resets partway up the list, that is the supply collapsing rather than a fault in the sketch, and where it happens is itself a useful number.

### What this panel actually draws

| Quantity | This panel | Standard WS2812B figures |
| --- | --- | --- |
| Current per channel at full | ~5.5 mA | ~20 mA |
| One white pixel at full | ~16.6 mA | ~60 mA |
| All 64 white at full | 1116 mA measured | ~3.9 A |
| Idle floor, all LEDs off | ~56 mA | ~64 mA |

Two measurements give these, and they agree: all 64 pixels white at full brightness draws **1116 mA** at the panel, and a ghost sprite at brightness 32 lighting 52 pixels in one colour and 4 in white draws about 100 mA.  The result is insensitive to the second reading, since anywhere from 90 to 110 mA gives between 5.5 and 5.6 mA per channel, and the derived idle floor matches the roughly 64 mA expected from 64 always-running controllers.

**FastLED's own power estimate uses the standard figures and so over-predicts this hardware by more than three times.**  Any budget passed to `setMaxPowerInVoltsAndMilliamps` must allow for that, because the library will clamp at roughly a third of the current actually drawn.

One panel fully lit and white draws a little over 1 A, and four come to about 4.5 A, which is the figure that sizes the protection circuit.

To confirm the idle floor directly rather than by inference, run `dev/current-test` and read the meter during its brightness 0 step.

## Future extensions

Planned but not built.  Both affect the board layout, so leave room for them.

**Charge from USB.**  The part is chosen and specified under [charging and protection](#charging-and-protection).  Two things remain.  The connector: decide whether the NodeMCU USB socket is used only for flashing or whether a separate charging input is fitted, and keep them separate if possible, because the rule that USB and the battery must never drive the 3.3 V rail together is easier to keep when the charging port is not the flashing port.  And the firmware: the panels have to be blanked while charging, for the reason given in that section, which means reading STAT on a spare GPIO.

**Ambient light sensor for night dimming.**  In map mode this is a comfort feature rather than a power saving: ten lit pixels contribute 30 to 80 mA against a 224 mA floor, so dimming them recovers perhaps 20 to 40 mA out of 300 and the [runtime table](#runtime) barely moves.  The power argument only holds in sprite mode, where far more of the panel is lit — brightness 128 across the whole panel is 2.6 h against 5.7 to 6.4 h for a sprite at brightness 32.  `FastLED.setBrightness` sets it, and because the sinks are constant-current, brightness scales current close to linearly.

Two ways to sense it, and the pin budget decides.  D2 (GPIO4) is taken by the LED data, leaving D1, D5, D6, D7 and A0 usable, with D0, D3, D4 and D8 to avoid.

- an LDR in a divider into **A0**, which is the ESP8266's only analog input and is currently free.  Cheapest, needs two components, and gives an uncalibrated reading that is fine for a day or night decision
- a **BH1750** or TSL2561 over I2C, giving calibrated lux at the cost of two GPIOs.  `Wire.begin` takes the pins as arguments, so any free pair such as D1 and D5 works

Start with the LDR on A0.  Nothing here needs true lux, and it leaves every GPIO free.

## Alternatives

**Powered from USB.**  Runs the panel from NodeMCU VIN at 4.7 V, putting the threshold at 3.29 V against a 3.3 V drive, so it works with no margin at all.  USB leaves roughly 350 mA after the NodeMCU takes its share, which covers about 17 white pixels of one panel, and `FastLED.setMaxPowerInVoltsAndMilliamps(5, 350)` keeps a sketch inside it.  Enough for one panel, not for 256.

**External 5 V supply.**  Feeds the panels directly while the NodeMCU shares only ground, with no limit on brightness.  Four panels all white is about 4.5 A, so it needs a 6 A supply, and it ties the display to a wall socket.

**Level shifting for a 5 V panel.**  A 74AHCT125 converts the 3.3 V data to a clean 5 V signal and keeps the panel inside its characterised range.  A 3 A silicon rectifier such as a 1N5401 in series with the panel supply is the cheaper answer, dropping about 0.7 V to put the panel near 4.3 V and the threshold near 3.0 V.  A Schottky is not a substitute, because its smaller drop leaves the threshold marginal again.  Neither is needed when the panel runs from a cell.

**A boost converter to 5 V.**  Would let the cell run to 3.0 V rather than 3.6 V, but costs roughly 40% of the runtime, because the panel draws the same current at 5 V as at 3.7 V and the converter adds losses on top.

**A buck-boost for the ESP8266.**  Would hold 3.3 V across the whole cell range instead of giving up near 3.4 V.  Not worth an inductor and a QFN package, because the panel loses its colours before the LDO gives up.

**A larger regulator in the same footprint.**  The **HT7833** gives 500 mA in SOT-89-3, which clears the 300 to 400 mA transmit peak outright and makes the rail capacitor margin rather than a requirement.  Its pin order is GND, VIN, VOUT, matching the AP130-33YR fitted here, so it is a drop-in.  Quiescent current is 4 µA typical against 50 µA, it has thermal shutdown and current limiting, and it is about $0.35 as LCSC C50936.  Dropout is 800 mV typical at 500 mA, which is around 110 mV at the 70 mA this actually draws, so no worse in use; the widely quoted 360 mV figure belongs to the 5.0 V member of the series, not the 3.3 V one.  At 4.2 V in and 500 mA out it dissipates about 450 mW, which is more than SOT-89 wants continuously but harmless for millisecond bursts.

**A larger regulator in a different footprint.**  AP2112K-3.3 at 600 mA, XC6220B331 at 1 A, or ME6211C33 and TLV75533P at 500 mA.  All are SOT-23-5 or SOT-25, and the first three add an enable pin, which the SOT-89-3 parts lack.

**Parallel output.**  FastLED can drive several chains at once through `InlineBlockClocklessController` in `platforms/esp/8266/clockless_block_esp8266.h`, used as `FastLED.addLeds<WS2811_PORTA, LANES>`.  The lane pins are fixed by `PORT_MASK` and `FIX_BITS` to GPIO 12, 13, 14 and 15, then GPIO 4 and 5, so four lanes forces the D8 boot strap pin and only two lanes on D6 and D7 avoid it.  A single chain already runs at about 130 fps, so there is nothing to gain.  Four ordinary `addLeds` calls on four pins are not parallel either: `CFastLED::show` walks its controllers as a linked list and drives them one after another.

**Never transmitting.**  ESP-NOW broadcast receive needs no association and broadcast frames are not acknowledged, and promiscuous mode only listens, so neither ever transmits and both would remove the 300 mA ceiling as a concern.  Both also give up normal IP networking, and so the HTTP fetch that map mode is built on.

**A TP4056 charger.**  Charges at up to 1 A against the MCP73831's 500 mA, and is sold everywhere as a finished module.  It has no power path either, so it shares the termination problem, and the DW01A and FS8205 protection bundled onto most modules trips at 2 to 3 A, below what a bright panel draws.

**A BQ24074 charger.**  Has a proper power path, running the system from USB while charging the cell, which is the only way to keep the display working while plugged in.  It costs more and comes in a QFN.

**A bare ESP-12F instead of the NodeMCU.**  Drops the AMS1117 and the USB serial chip, saving board space and idle current, at the cost of providing a programming path and the boot strapping of GPIO0 and GPIO15 low with EN and GPIO2 high.  Worth revisiting once the rest of the board is settled; a NodeMCU footprint leaves the choice open.

**A 22 µF tantalum at the panel.**  Adequate only at bench currents, and it must be rated 10 V or more, because tantalums are derated to half their rating.  They fail short rather than open, sometimes with flame, and dislike the inrush from a stiff low-impedance supply, so a 1000 µF electrolytic is used instead.
