```
                          __                __             ______    ______  
                         |  \              |  \           /      \  /      \ 
 ______ ____    ______  _| $$_     ______   \$$ __    __ |  $$$$$$\|  $$$$$$\
|      \    \  |      \|   $$ \   /      \ |  \|  \  /  \| $$  | $$| $$___\$$
| $$$$$$\$$$$\  \$$$$$$\\$$$$$$  |  $$$$$$\| $$ \$$\/  $$| $$  | $$ \$$    \ 
| $$ | $$ | $$ /      $$ | $$ __ | $$   \$$| $$  >$$  $$ | $$  | $$ _\$$$$$$\
| $$ | $$ | $$|  $$$$$$$ | $$|  \| $$      | $$ /  $$$$\ | $$__/ $$|  \__| $$
| $$ | $$ | $$ \$$    $$  \$$  $$| $$      | $$|  $$ \$$\ \$$    $$ \$$    $$
\ $$  \$$  \$$  \$$$$$$$   \$$$$  \$$       \$$ \$$   \$$  \$$$$$$   \$$$$$$ 
```
by [me](https://github.com/slaugaus), [@cbialorucki](https://github.com/cbialorucki), and [@Jakedez](https://github.com/Jakedez)

OS-like thing for HUB75 LED matrix panels (Teensy 4 + SmartLED Shield)

## Library Credits
[SmartMatrix](https://github.com/pixelmatix/SmartMatrix)
[PNGdec](https://github.com/bitbank2/PNGdec/) and [JPEGDEC](https://github.com/bitbank2/JPEGDEC/)
[GifDecoder](https://github.com/pixelmatix/GifDecoder) (depends on [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF))
* [FilenameFunctions](https://github.com/pixelmatix/GifDecoder/tree/master/examples/SmartMatrixGifPlayer), with some modifications, from GifDecoder example sketches
Teensy built-in libraries, notably [USBHost_t36](https://github.com/PaulStoffregen/USBHost_t36)

## Known Limitations

### CLI
* Backspace is funny
* No scrolling or paging; text simply goes offscreen. Use `cls` or Ctrl+L to clear the screen.

### Image Viewers
* PNGs get squashed to 16-bit color (RGB565) despite the format and libraries supporting better depths.
* JPEG files must have the extension `.jpg` (for now).
* Images larger than the screen resolution are not scaled to fit and may crash the system.

### GIF Playback
* Not all files will successfully load, notably GIFs with interleaved frames (per [GifDecoder](https://github.com/pixelmatix/GifDecoder) docs). Files made with [gifski](gif.ski)'s default settings are recommended and confirmed working.
* Transparent GIFs are not handled correctly; work around this by adding a black background to the file.
* GIFs larger than the screen resolution are not scaled to fit. Very high-res files may cause crashes.