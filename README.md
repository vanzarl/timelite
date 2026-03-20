
# Timelite

A simple console utility for displaying a clock, timer, and stopwatch.

## Build

### For Linux

```console
cc main.c -o timelite
./timelite
```

## Usage

`./timelite [OPTION]`

### Options

- `-c`: clock mode
- `-t [HH:MM:SS|MM:SS|SS]`: timer (descending) mode
- `-s`: stopwatch (ascending) mode
- `-n`: show name before clock/timer/stopwatch
- `-h`: help

### Key bindings

| KEY | Description |
| --- | --- |
| <kbd>q</kbd> | Exit |

### Examples

- `./timelite -t :69`: timer for 69 seconds *(69 seconds = 1 minute and 9 second)*;
- `./timelite -t 50:`: timer for 50 minutes *(50: means 50:00)*;
- `./timelite -t 25: -n "coding"`: pomodoro timer with name `coding` for 25 minutes *(25: means 25:00)*;

## FPS

You can adjust the FPS of the application by changing the macros in the source code.
Simply modify the value to increase or decrease the update speed.

Availible options:

- `FPS`: for main refresh rate (now it is rate for reading user's keys)
- `CLOCK_FPS`: for updating clock

**`CLOCK_FPS` have to be smaller than `FPS`**
