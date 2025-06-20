// include/User_Setup.h
// Configurazione per TTGO T-Display (ST7789 1.14” 135×240)

#define ST7789_DRIVER

// Display size
#define TFT_WIDTH  135
#define TFT_HEIGHT 240

// Pin definition
#define TFT_MISO -1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  23
#define TFT_BL   4

// SPI speed
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

#define SUPPORT_TRANSACTIONS
