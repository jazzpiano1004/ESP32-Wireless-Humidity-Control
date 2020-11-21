// This sketch if for an ESP32, it draws Jpeg images pulled from an SD Card
// onto the TFT.

// As well as the TFT_eSPI library you will need the JPEG Decoder library.
// A copy can be downloaded here, it is based on the library by Makoto Kurauchi.
// https://github.com/Bodmer/JPEGDecoder

// Images on SD Card must be put in the root folder (top level) to be found
// Use the SD library examples to verify your SD Card interface works!

// The example images used to test this sketch can be found in the library
// JPEGDecoder/extras folder
//----------------------------------------------------------------------------------------------------

#include "tft_lcd.h"

extern TFT_eSPI tft;

//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);

  // Set all chip selects high to avoid bus contention during initialisation of each peripheral
  digitalWrite(22, HIGH); // Touch controller chip select (if used)
  digitalWrite(15, HIGH); // TFT screen chip select
  digitalWrite( 5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)

  tft.begin();

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.println("initialisation done.");

  tft.setRotation(1);  // landscape
}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {

  File file;
  File root = SD.open("/");
  if(!root){
      Serial.println("Failed to open root directory");
      return;
  }
  file = root.openNextFile();  // Opens next file in root
  while(file)
  {
    if(!file.isDirectory())
    {
      drawSdJpeg(file.name(), 0, 0);     // This draws a jpeg pulled off the SD Card
      delay(4000);
    }
    file = root.openNextFile();  // Opens next file in root
  }
  root.close();
}
