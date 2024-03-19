#include <MatrixHardware_Teensy4_ShieldV5.h>  // SmartLED Shield for Teensy 4 (V5)
#include <SmartMatrix.h>

#define MAX_SCREEN_WIDTH 192
#define MAX_SCREEN_HEIGHT 128
#define CHAR_WIDTH 4
#define CHAR_HEIGHT 6
#define DEFAULT_FONT font3x5

#define COLOR_DEPTH 24  // Color depth (bits-per-pixel) of gfx layers
const uint16_t kMatrixWidth = 192;
const uint16_t kMatrixHeight = 128;
const uint8_t kRefreshDepth = 36;                               // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 4;                               // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;  // 64x64 "HUB75E" 5-address panels
// see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_C_SHAPE_STACKING);  // aka "serpentine" - flip alternate rows to save cable length
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

uint8_t matrixBrightness = 255;
uint16_t termCursorX = 0;
uint16_t termCursorY = 0;

uint16_t termLastLineCurX = 0;
uint16_t termLastLineCurY = 0;

const rgb24 colorWhite = { 255, 255, 255 };
const rgb24 colorBlack = { 0, 0, 0 };
const rgb24 colorRed = { 255, 0, 0 };
const rgb24 colorGreen = { 0, 255, 0 };
const rgb24 colorBlue = { 0, 0, 255 };

rgb24 termBgColor = colorBlack;
rgb24 termInputColor = colorGreen;
rgb24 termResponseColor = colorWhite;
rgb24 termErrorColor = colorRed;