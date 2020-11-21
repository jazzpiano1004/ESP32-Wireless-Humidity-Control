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
Adafruit_SHT31 sht31 = Adafruit_SHT31();
float temperatureValue;
float RH_value;

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
extern TFT_eSPI tft;



/*
 * Global variables for BLE communication
 */
bool BLE_connectionState = false;            



void setup() {
  Serial.begin(115200);

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
      Serial.println("Couldn't find SHT31");
    }
    else{
      temperatureValue = sht31.readTemperature();
      RH_value = sht31.readHumidity();
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

  File file;
  File root = SD.open("/");
  file = root.openNextFile();  // Opens next file in root
  
  for (;;) // A Task shall never return or exit.
  {
    /*
     * Display assert image from SD card
     */
    if (!SD.begin()) {
      Serial.println("Card Mount Failed");
    }
    else{
      if(RH_value > RH_THRESHOLD_VALUE_4){
         backgroundPage_new = UI_BACKGROUND_PAGE_5;
         // Check if page will use the same background as the previous one
         if(backgroundPage_new != backgroundPage_old){
            drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_5, 0, 0);     // This draws a jpeg pulled off the SD Card
            backgroundPage_old = backgroundPage_new;
         }
      }
      else if(RH_value > RH_THRESHOLD_VALUE_3){
         backgroundPage_new = UI_BACKGROUND_PAGE_4;
         // Check if page will use the same background as the previous one
         if(backgroundPage_new != backgroundPage_old){
            drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_4, 0, 0);     // This draws a jpeg pulled off the SD Card
            backgroundPage_old = backgroundPage_new;
         }
      }
      else if(RH_value > RH_THRESHOLD_VALUE_2){
         backgroundPage_new = UI_BACKGROUND_PAGE_3;
         if(backgroundPage_new != backgroundPage_old){
            drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_3, 0, 0);     // This draws a jpeg pulled off the SD Card
            backgroundPage_old = backgroundPage_new;
         }
      }
      else if(RH_value > RH_THRESHOLD_VALUE_1){
         backgroundPage_new = UI_BACKGROUND_PAGE_2;
         // Check if page will use the same background as the previous one
         if(backgroundPage_new != backgroundPage_old){
            drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_2, 0, 0);     // This draws a jpeg pulled off the SD Card
            backgroundPage_old = backgroundPage_new;
         }
      }
      else{
         backgroundPage_new = UI_BACKGROUND_PAGE_1;
         // Check if page will use the same background as the previous one
         if(backgroundPage_new != backgroundPage_old){
            drawSdJpeg(UI_BACKGROUND_PAGE_FILENAME_1, 0, 0);     // This draws a jpeg pulled off the SD Card
            backgroundPage_old = backgroundPage_new;
         }
      }
    }

    /*
     * Write BLE connectivity to LCD display
     */
    tft.setCursor(130, 20, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    if(BLE_connectionState == 0){
       tmp_string = "No Connect...";
    }
    else{
       tmp_string = "Connected!";
    }
    tft.print(tmp_string);
    Serial.println(BLE_connectionState);

    /*
     * Write tempurature & RH value to LCD display
     */
    tft.fillRect(170, 140, 140, 60, TFT_BLACK);    
    if(! isnan(temperatureValue)){  // check if 'is not a number'
      /*
      Serial.print("Temp *C = "); 
      Serial.print(temperatureValue); 
      Serial.print("\t\t");
      */
      tft.setCursor(170, 140, 2);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      tmp_string = "Temp=" + String(temperatureValue, 1);
      tft.print(tmp_string);
    }
    else{ 
      Serial.println("Failed to read temperature");
    }

    if(! isnan(RH_value)){  // check if 'is not a number'
      /*
      Serial.print("RH = "); 
      Serial.print(RH_value); 
      Serial.print("\r\n");
      */
      tft.setCursor(170, 170, 2);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      tmp_string = "RH=" + String(RH_value, 2) + "%";
      tft.print(tmp_string);
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
        //String newValue = String(temperatureValue, 2) + "," + String(RH_value, 2);
        String newValue = String(RH_value, 2);
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