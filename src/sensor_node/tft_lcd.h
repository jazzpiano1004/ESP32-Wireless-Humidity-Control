


#ifndef tft_lcd_h
#define tft_lcd_h



#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

/* This should be setup at TFT_eSPI library folder
 *  
 */
//#define TFT_MISO 19
//#define TFT_MOSI 23
//#define TFT_SCLK 18
//#define TFT_CS   15  // Chip select control pin
//#define TFT_DC   26  // Data Command control pin
//#define TFT_RST  25  // Reset pin (could connect to RST pin)

extern TFT_eSPI tft;


void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void jpegInfo();

#endif
