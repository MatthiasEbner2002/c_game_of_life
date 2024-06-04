# Game of Life

![GameOfLife](/images/gol.png)

## Use

```bash
make
./main
```

## usage

This only affects the starting settings and can be change by pressing keys.

```bash
Usage: ./main [-2] [-nc] [-nh] [-ni]
Options:
  -2 : Display two cells per block
  -nc: No colors will be used
  -nh: Do not show history
  -ni: Do not show info at start
```

## key bindings

- **q** = quit
- **i** = info
- **c** = color
- **h** = history
- **r** = reload
- **p** = pause
- **2** = mode

## color cells meaning

| alive for | color |
| --- | --- |
| < 1 | RED |
| < 10 | GREEN |
| < 30 | BLUE |
| >= 30 | YELLOW |

## Resize

New cells will have a 50/50 change to be alive.
