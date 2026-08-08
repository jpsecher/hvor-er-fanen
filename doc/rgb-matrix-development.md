# RGB Matrix development

How to build, flash and monitor the ESP8266 firmware in `dev/`.  This covers the tooling only, not what the firmware does.

## Two environments

Work is split across two shells, and the split is not a preference.  The dev container has no USB access, because `claude-contained` creates it without `--device` and the container lacks `CAP_SYS_ADMIN`.  So the container compiles and the host talks to the board.

| Where | How to enter | Provides | Used for |
| --- | --- | --- | --- |
| Host, outer shell | `nix develop` in the repo root | container scripts, esptool, picocom | flashing, serial monitor, container control |
| Container, inner shell | `dev-shell` from the outer shell | just, arduino-cli, python3 | compiling |

Claude Code sessions started with `claude` run inside the container, so they can build but cannot flash.

## First time

From the outer shell, start the container and enter it:

    start-dev
    dev-shell

Then install the Arduino toolchain from the inner shell:

    just setup

`just setup` downloads the ESP8266 core and the libraries into `dev/.arduino`, which is about 670 MB and git-ignored.  Keeping them in the project rather than `$HOME` means the whole toolchain is removable with one directory, and nothing leaks between projects.

## Building

From the inner shell, in `dev/`:

    just build

The result is `dev/build/rgb-matrix.ino.bin`, also git-ignored.  Other recipes are `just boards` to list the board options, `just artifacts` to show what was built, and `just clean` to remove `dev/build`.  Run `just` alone to list them.

The sketch directory is `dev/rgb-matrix`, and the sketch file must keep the same name as its directory, which is an Arduino requirement.

## Flashing

Flashing runs on the host, in the outer shell, from the repo root.  Confirm the board answers before writing anything to it:

    esptool --port /dev/cu.usbserial-210 flash-id

Then write the image:

    esptool --port /dev/cu.usbserial-210 --baud 460800 write-flash 0x0 dev/build/rgb-matrix.ino.bin

The port name is an example.  On macOS it is `/dev/cu.usbserial-*` or `/dev/cu.wchusbserial*`, and on Linux `/dev/ttyUSB0` or `/dev/ttyACM0`, where the account also has to be in the `dialout` group.  If the write fails with checksum or timeout errors, use `--baud 115200`, because CP210x bridges are not always reliable at the higher rate.

## Reading serial output

Also on the host:

    picocom -b 115200 /dev/cu.usbserial-210

Ctrl-A Ctrl-X exits.  picocom holds the port open, so close it before flashing again or esptool cannot claim the device.

## Pinned versions

- ESP8266 core 3.1.2, board `esp8266:esp8266:nodemcuv2`
- FastLED 3.10.5
- ESP Async WebServer 3.12.0 with ESP Async TCP 2.0.0
- arduino-cli 1.5.1, esptool 5.3.1

The hardware reports an ESP8266EX with 4 MB of flash and auto-resets over RTS, which matches the `nodemcuv2` default flash layout of `eesz=4M2M`.  Build for a different board with `just fqbn=esp8266:esp8266:d1_mini build`, or change the default in `dev/justfile`.

## Things that look broken but are not

The first line after a reset is unreadable, because the ESP8266 boot ROM prints its banner at 74880 baud before the sketch starts at 115200.

Each line starts at the column where the previous line ended, when a line ends with `\n` alone.  Arduino `println` sends `\r\n`, but `printf` sends exactly what the format string contains, so use `\r\n` there.  As a terminal-side workaround, `picocom --imap lfcrlf` converts incoming line endings.

## Troubleshooting

No `/dev/cu.*` entry appears when the board is plugged in.  macOS has no udev and needs no rule or group, so there is nothing to configure.  Check the cable first, because charge-only USB cables have no data lines and are the most common cause.  Then run `system_profiler SPUSBDataType` to see whether the bridge chip enumerates at all: if it is missing the problem is the cable, port or board, and if it is present the problem is the driver.

A build fails with a missing `ESPAsyncTCP.h`.  The ESP8266 build of ESP Async WebServer needs ESP Async TCP, which arduino-cli does not install automatically, so `just setup` installs it explicitly.

New tools added to `dev/flake.nix` are not found in a Claude Code session.  The environment is captured once when the session starts, so exit `claude` and start it again.

## Two package choices worth knowing

arduino-cli comes from `arduino-cli.pureGoPkg` rather than the default `arduino-cli`.  The default is wrapped in bubblewrap to provide an FHS layout, which needs a `CAP_SYS_ADMIN` the container does not have, and the Debian-based container already provides the real FHS paths the Arduino toolchain expects.

python3 is declared explicitly in `dev/flake.nix` even though nothing in the project calls it directly.  The ESP8266 core runs `elf2bin.py` and `signing.py` on every build, and python3 would otherwise be present only as a side effect of another package's closure.
