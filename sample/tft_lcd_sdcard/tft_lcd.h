


#ifndef tft_lcd_h
#define tft_lcd_h



#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>


extern TFT_eSPI tft;


void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void jpegInfo();

#endif
