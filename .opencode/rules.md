# Amiga Demo 1

An Amiga OS demo written in C, implementing effects like text scrolling with starfield/planet, logo display, rotating cube, and more. Uses `intuition.library` for screen setup and `graphics.library` for blitting operations.

## Build

On macOS, always use the container build script:

```bash
./apple-container-build.sh
```

This handles the m68k-amigaos-gcc toolchain via Docker container.

## Project Structure

- `src/main.c` - Main state machine orchestrating demo effects
- `src/effects/` - Demo effects (textscroller, showlogo, rotatingcube)
- `src/gfx/` - Graphics utilities (stars, rotation, zoom, movement controllers)
- `src/utils/` - Utility functions (timer, logging)
