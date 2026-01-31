/**
 * CYD (ESP32-2432S028R) pin definitions. Do not use generic defaults.
 */
#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// Display (ILI9341)
#define TFT_BL   21
#define TFT_CS   15
#define TFT_DC   2
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_RST  -1

// Touch (XPT2046) - separate SPI bus
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

#endif /* CONFIG_PINS_H */
