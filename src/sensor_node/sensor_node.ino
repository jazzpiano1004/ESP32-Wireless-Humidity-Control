/*
    Wireless Humidity Control Project
    Sensor node
    Author by Nattapong Wattanasiri (nattapong.w1004@gmail.com) 
*/
#include <Arduino.h>

// Include for RH sensor SHT31D
#include <Wire.h>
#include <Adafruit_SHT31.h>

// Include for TFT LCD Display
#include "tft_lcd.h"
#include "Free_Fonts.h"

// Include for BLE

#include "bluetooth.h"

// Define for RTOS's core selection
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif



// RTOS tasks
void task_readSensor( void *pvParameters );
void task_display( void *pvParameters );



/*
 *  Global variables for RH sensor SHT31D
 */
#define GPIO_SENSOR_ERROR_LED          27
Adafruit_SHT31 sht31 = Adafruit_SHT31();
float temperatureValue;
float RH_value;
int8_t sht31_disconnected = 0;
uint8_t firstReadFlagCompleted = 0;

/*
 *  Global variables for TFT LCD Display
 */
#define RH_THRESHOLD_VALUE_1           55
#define RH_THRESHOLD_VALUE_2           60
#define RH_THRESHOLD_VALUE_3           70
#define RH_THRESHOLD_VALUE_4           80
#define UI_BACKGROUND_PAGE_FILENAME_1  "/humidity_55.jpg"
#define UI_BACKGROUND_PAGE_FILENAME_2  "/humidity_55-60.jpg"
#define UI_BACKGROUND_PAGE_FILENAME_3  "/humidity_61-70.jpg"  
#define UI_BACKGROUND_PAGE_FILENAME_4  "/humidity_71-80.jpg"
#define UI_BACKGROUND_PAGE_FILENAME_5  "/humidity_80.jpg"
#define UI_BACKGROUND_PAGE_1           1
#define UI_BACKGROUND_PAGE_2           2
#define UI_BACKGROUND_PAGE_3           3
#define UI_BACKGROUND_PAGE_4           4
#define UI_BACKGROUND_PAGE_5           5
//font color piker URL : http://www.barth-dev.de/online/rgb565-color-picker/
#define BG_COLOR_CODE_PAGE_1           0x055F
#define BG_COLOR_CODE_PAGE_2           0x0759
#define BG_COLOR_CODE_PAGE_3           0xF640 
#define BG_COLOR_CODE_PAGE_4           0xFBC0
#define BG_COLOR_CODE_PAGE_5           TFT_RED
#define TEMPERATURE_BOX_START_X      170
#define TEMPERATURE_BOX_START_Y      150
#define TEMPERATURE_BOX_W            150
#define TEMPERATURE_BOX_H            80
extern TFT_eSPI tft;
TFT_eSprite sprite = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object
                                         // the pointer is used by pushSprite() to push it onto the TFT
uint16_t textBgColorCode = TFT_BLACK;
uint16_t textColorCode = TFT_WHITE;
uint16_t buff_textBoxBg[TEMPERATURE_BOX_W * TEMPERATURE_BOX_H];




/*
 * Global variables for BLE communication
 */
bool BLE_connectionState = false;            



void setup() {
  Serial.begin(115200);

  /*
   * SHT31 Initialize
   */
  pinMode(GPIO_SENSOR_ERROR_LED, OUTPUT);
  digitalWrite(GPIO_SENSOR_ERROR_LED, LOW);
  
  /*
   * TFT LCD Initialize
   */
  // Set all chip selects high to avoid bus contention during initialisation of each peripheral
  digitalWrite(22, HIGH); // Touch controller chip select (if used)
  digitalWrite(15, HIGH); // TFT screen chip select
  digitalWrite( 5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)
  tft.begin();
  tft.setRotation(1);  // landscape

  /*
   * BLE Initialize
   */
  BLEDevice::init("");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  
  xTaskCreate(
                    task_readSensor,          /* Task function. */
                    "read RH sensor task",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */
 
  xTaskCreate(
                    task_display,          /* Task function. */
                    "TFT LCD display task",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    NULL);            /* Task handle. */
  xTaskCreate(
                    task_bluetooth,          /* Task function. */
                    "BLE communication task",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */

}

void loop() {
  // put your main code here, to run repeatedly:

}



/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/
void task_readSensor(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
      sht31_disconnected = 1; // Set disconnect flag of sht31 sensor
      // Turn on sensor problem LED
      digitalWrite(GPIO_SENSOR_ERROR_LED, HIGH);
      Serial.println("Couldn't find SHT31");
    }
    else{
      sht31_disconnected = 0;
      digitalWrite(GPIO_SENSOR_ERROR_LED, LOW);
      temperatureValue = sht31.readTemperature();
      RH_value = sht31.readHumidity();
      if(firstReadFlagCompleted == 0) firstReadFlagCompleted = 1;
    }

    // task sleep
    vTaskDelay(3000);
  }
}

void task_display(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  int8_t backgroundPage_old = -1;
  int8_t backgroundPage_new;
  String tmp_string;
  int16_t RH_value_rounded[2] = {0, 0};
  
  // SD card open
  File file;
  File root = SD.open("/");
  file = root.openNextFile();  // Opens next file in roo
  
  for (;;) // A Task shall never return or exit.
  {
    /*
     * Display assert image from SD card
     */
    if (!SD.begin()) {
      Serial.println("Card Mount Failed");
    }
    else{
      if(firstReadFlagCompleted != 0){
        if(RH_value > RH_THRESHOLD_VALUE_4){
          backgroundPage_new = UI_BACKGROUND_PAGE_5;
          // Check if page will use the same background as the previous one
          if(backgroundPage_new != backgroundPage_old){
             drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_5, 0, 0);     // This draws a jpeg pulled off the SD Card
             backgroundPage_old = backgroundPage_new;
             tft.readRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
             vTaskDelay(100);
          }
        }
        else if(RH_value > RH_THRESHOLD_VALUE_3){
           backgroundPage_new = UI_BACKGROUND_PAGE_4;
           // Check if page will use the same background as the previous one
           if(backgroundPage_new != backgroundPage_old){
              drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_4, 0, 0);     // This draws a jpeg pulled off the SD Card
              backgroundPage_old = backgroundPage_new;
              tft.readRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
              vTaskDelay(100);
           }
        }
        else if(RH_value > RH_THRESHOLD_VALUE_2){
           backgroundPage_new = UI_BACKGROUND_PAGE_3;
           if(backgroundPage_new != backgroundPage_old){
              drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_3, 0, 0);     // This draws a jpeg pulled off the SD Card
              backgroundPage_old = backgroundPage_new;
              tft.readRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
              vTaskDelay(100);
           }
        }
        else if(RH_value > RH_THRESHOLD_VALUE_1){
           backgroundPage_new = UI_BACKGROUND_PAGE_2;
           // Check if page will use the same background as the previous one
           if(backgroundPage_new != backgroundPage_old){
              drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_2, 0, 0);     // This draws a jpeg pulled off the SD Card
              backgroundPage_old = backgroundPage_new;
              tft.readRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
              vTaskDelay(100);
           }  
        }
        else{
           backgroundPage_new = UI_BACKGROUND_PAGE_1;
           // Check if page will use the same background as the previous one
           if(backgroundPage_new != backgroundPage_old){
              drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_1, 0, 0);     // This draws a jpeg pulled off the SD Card
              backgroundPage_old = backgroundPage_new;
              tft.readRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
              vTaskDelay(100);
           }
        } 
      }
    }

    /*
     * Write BLE connectivity to LCD display
     */
    /*
    if(backgroundPage_new == UI_BACKGROUND_PAGE_1)       textBgColorCode = BG_COLOR_CODE_PAGE_1;
    else if(backgroundPage_new == UI_BACKGROUND_PAGE_2)  textBgColorCode = BG_COLOR_CODE_PAGE_2;
    else if(backgroundPage_new == UI_BACKGROUND_PAGE_3)  textBgColorCode = BG_COLOR_CODE_PAGE_3;
    else if(backgroundPage_new == UI_BACKGROUND_PAGE_4)  textBgColorCode = BG_COLOR_CODE_PAGE_4;
    else if(backgroundPage_new == UI_BACKGROUND_PAGE_5)  textBgColorCode = BG_COLOR_CODE_PAGE_5;
    */

    if(BLE_connectionState == 0){
       tmp_string = "No Connect...";
    }
    else{
       tmp_string = "Connected!";
    }
    //tft.fillRect(180, 5, 120, 20, textBgColorCode);  
    //tft.drawString(tmp_string, 180, 5, GFXFF);
    //Serial.println(BLE_connectionState);

    /*
     * Write tempurature & RH value to LCD display
     */
    if(!isnan(RH_value) && firstReadFlagCompleted != 0){  // check if 'is not a number'

      // Convert to integer for temperature displaying
      RH_value_rounded[0] = (int) RH_value;

      Serial.print(RH_value_rounded[0]);
      Serial.print(",");
      Serial.println(RH_value_rounded[1]);
      
      if(RH_value_rounded[0] != RH_value_rounded[1]){
        // refresh background at textbox area only
        tft.pushRect(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TEMPERATURE_BOX_W, TEMPERATURE_BOX_H, buff_textBoxBg);
        
        //create sprite with box size of 140 x 80 for text area
        //8-bit color-level
        sprite.setColorDepth(8);
        sprite.createSprite(TEMPERATURE_BOX_W, TEMPERATURE_BOX_H);
  
        // Fill Sprite with a "transparent" colour
        // TFT_TRANSPARENT is already defined for convenience
        // We could also fill with any colour as "transparent" and later specify that
        // same colour when we push the Sprite onto the screen.
        sprite.fillSprite(TFT_BLACK);
  
        sprite.setTextColor(textColorCode, TFT_BLACK);  // White text, no background colour 
        sprite.setFreeFont(FF24);
        sprite.setTextSize(2);
        tmp_string = String(RH_value_rounded[0]);
        sprite.drawString(tmp_string, 0, 0, GFXFF);
        sprite.setFreeFont(FF21);
        sprite.drawString("%", 110, 40, GFXFF);
  
        // Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
        // Specify what colour is to be treated as transparent.
        sprite.pushSprite(TEMPERATURE_BOX_START_X, TEMPERATURE_BOX_START_Y, TFT_BLACK);
  
        // Delete it to free memory
        sprite.deleteSprite();

        // Update rounded RH_value
        RH_value_rounded[1] = RH_value_rounded[0];
      }
    }
    else{ 
      Serial.println("Failed to read temperature");
    }

    // task sleep
    vTaskDelay(1000);
  }
}

void task_bluetooth(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {   
      // If the flag "doConnect" is true then we have scanned for and found the desired
      // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
      // connected we set the connected flag to be true
      if(doConnect == true){
        if(connectToServer()){
          Serial.println("We are now connected to the BLE Server.");
        } 
        else{
          Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
      }
    
      // If we are connected to a peer BLE Server, update the characteristic each time we are reached
      // with the current time since boot.
      if(connected){
        BLE_connectionState = true;
        String newValue = String(temperatureValue, 2) + "," + String(RH_value, 2) + "," + String(sht31_disconnected);
        //String newValue = String(RH_value, 2);
        Serial.println("Setting new characteristic value to \"" + newValue + "\"");
        
        // Set the characteristic's value to be the array of bytes that is actually a string.
        pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
      }
      else if(doScan){
        BLE_connectionState = false;
        BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
      }
      
      // task sleep
      vTaskDelay(3000);
  }
}
