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
[GifDecoder](https://github.com/pixelmatix/GifDecoder) (depends on [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF))
* [FilenameFunctions](https://github.com/pixelmatix/GifDecoder/tree/master/examples/SmartMatrixGifPlayer) from GifDecoder example sketches
Teensy built-in libraries, notably [USBHost_t36](https://github.com/PaulStoffregen/USBHost_t36)

## Known Limitations

### GIF Playback
* Not all files will successfully load, notably GIFs with interleaved frames (per [GifDecoder](https://github.com/pixelmatix/GifDecoder) docs). Files made with [gifski](gif.ski)'s default settings are recommended and confirmed working.
* Transparent GIFs are not handled correctly; work around this by adding a black background to the file.
* GIFs larger than the screen resolution are not scaled to fit. Very high-res files may cause crashes.