
# Timelite

A simple console utility for displaying a clock, timer, and stopwatch.

## Build

### General

```console
./build.sh
./timelite
```

## Usage

`./timelite [OPTION]`

### Options

- `-c`: clock mode
- `-t [HH:MM:SS|MM:SS|SS]`: timer (descending) mode
- `-s`: stopwatch (ascending) mode

### Key bindings

| KEY | Description |
| --- | --- |
| <kbd>q</kbd> | Exit |

## FPS

You can adjust the FPS of the application by changing the macros in the source code.
Simply modify the value to increase or decrease the update speed.

Availible options:

- `FPS`: for main refresh rate (now it is rate for reading user's keys)
- `CLOCK_FPS`: for updating clock

**don't set `FPS` > `CLOCK_FPS`**
**`CLOCK_FPS` have to be smaller than `FPS`**
