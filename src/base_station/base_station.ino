/*
    Wireless Humidity Control Project
    Base station
    Author by Nattapong Wattanasiri (nattapong.w1004@gmail.com) 
*/
#include <Arduino.h>

// Include for BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Define for RTOS's core selection
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// RTOS tasks
void task_connectTimeout(void *pvParameters);
void task_displayStatus(void *pvParameters);
void task_heater_ctrl(void *pvParameters);

/*
 * IO for Heater&Fan Control
 */
#define CONTROL_IO_HEATER   26
#define CONTROL_IO_FAN      27
#define LED_IO_HEATER       33
#define LED_IO_FAN          32
#define RH_THRESHOLD_HIGH             (float)55.10
#define RH_THRESHOLD_LOW              (float)54.90
#define HEATER_CTRL_STATE_HIGH_RH     1
#define HEATER_CTRL_STATE_LOW_RH      2
#define HEATER_CTRL_STATE_IDLE        0
float RH_value=50;
float temperatureValue;
int8_t sht31_disconnected = 0;
int8_t heaterCtrlState = HEATER_CTRL_STATE_IDLE;

/*
 * BLE Define, Class & Global variable
 */
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SERVICE_UUID        "2c0c9b4a-2a57-11eb-adc1-0242ac120002"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_CONNECTTIMEOUT      10
#define LED_IO_BLE_CONNECTED    25
String   BLE_message = "";
uint32_t timeoutCnt = BLE_CONNECTTIMEOUT;
bool     timeoutFlag = true;

// Callback function class for BLE
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      timeoutCnt = BLE_CONNECTTIMEOUT;
      timeoutFlag = false;
      std::string value = pCharacteristic->getValue();

      //BLE_message = value;
      BLE_message = value.c_str();
      //Serial.println(BLE_message);
    }
};



void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  pinMode(CONTROL_IO_HEATER, OUTPUT);
  pinMode(CONTROL_IO_FAN, OUTPUT);
  pinMode(LED_IO_HEATER, OUTPUT);
  pinMode(LED_IO_FAN, OUTPUT);
  pinMode(LED_IO_BLE_CONNECTED, OUTPUT);
  
  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World says Neil");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE Characteristic defined! Now you can read it in your phone!");

  /*
   * Create RTOS task
   */
  xTaskCreate(
                    task_displayStatus,            /* Task function. */
                    "Status display with LED and serial print",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    NULL);            /* Task handle. */
  xTaskCreate(
                    task_heaterControl,          /* Task function. */
                    "Heater and Fan Control",        /* String with name of task. */
                    2048,             /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */
  xTaskCreate(
                    task_connectTimeout,          /* Task function. */
                    "BLE timeout checking",        /* String with name of task. */
                    1024,             /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */
  xTaskCreate(
                    task_decodeBluetoothMessage,          /* Task function. */
                    "BLE timeout checking",        /* String with name of task. */
                    1024,             /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}



/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/
void task_displayStatus(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    // Display BLE connection status
    if(!timeoutFlag){
       digitalWrite(LED_IO_BLE_CONNECTED, HIGH);
    }
    else{
       digitalWrite(LED_IO_BLE_CONNECTED, LOW);
    }
    Serial.print("temp=");
    Serial.print(temperatureValue);
    Serial.print("\tRH=");
    Serial.print(RH_value);
    Serial.print("\terror=");
    Serial.println(sht31_disconnected);
    
    // task sleep
    vTaskDelay(1500);
  }
}



void task_decodeBluetoothMessage(void *pvParameters)
{
  (void) pvParameters;

  /* for message CSV format decoding */
  String tmp_msg;
  char field1[8] = "";
  char field2[8] = "";
  char field3[8] = "";
  int8_t msg_len;
  int8_t pos;
  int8_t previous_comma_pos;
  int8_t field_num;
  int8_t i;
  
  for (;;)
  { 
    /*
     * BLE message decoding. Message is CSV format -> temperature, RH_value, sht31_disconnected
     *                                                    field1,   field2,  field3
     */
    tmp_msg = BLE_message;   // load current BLE message to temporary variable
    msg_len = tmp_msg.length();
    if(msg_len > 0){
      pos=0;
      field_num = 1;
      previous_comma_pos = -1;
      while(pos < msg_len){
        if(tmp_msg[pos] == ','){
          for(i=0; i<pos-previous_comma_pos-1; i++){
            if(field_num == 1)       field1[i] = tmp_msg[previous_comma_pos + 1 + i];
            else if(field_num == 2)  field2[i] = tmp_msg[previous_comma_pos + 1 + i];
          }
          i=0;
          previous_comma_pos = pos;
          field_num++;
        }
        else if(field_num == 3){
          field3[i] = tmp_msg[pos];
          i++;
        }
        pos++;
      }

      // Update temperature, RH value, error status with decoded message
      temperatureValue = atof(field1);
      RH_value = atof(field2);
      sht31_disconnected = atoi(field3);
    }
    
    // task sleep
    vTaskDelay(250);
  }
}



void task_connectTimeout(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    if(timeoutCnt > 0){
      timeoutCnt--;
      if(timeoutCnt == 0){
         timeoutFlag = true;
      }
    }
    // task sleep
    vTaskDelay(1000);
  }
}



void task_heaterControl(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  { 
    if(timeoutFlag || (sht31_disconnected != 0)){
      /*
       * When there is no connection OR get error from sensor node
       * -> Turn off heater and fan
       */
       if(heaterCtrlState != HEATER_CTRL_STATE_IDLE){
          Serial.println("Turn off heater, go to IDLE");
          digitalWrite(CONTROL_IO_HEATER, LOW);
          digitalWrite(LED_IO_HEATER, LOW);
          vTaskDelay(60000);
          digitalWrite(CONTROL_IO_FAN, LOW);  
          digitalWrite(LED_IO_FAN, LOW);
          heaterCtrlState = HEATER_CTRL_STATE_IDLE;
       }
    }
    else{
      /*
       * When there is connection and no error from sensor node
       * -> Activate heater control
       */
      if(RH_value > RH_THRESHOLD_HIGH){
        if(heaterCtrlState != HEATER_CTRL_STATE_HIGH_RH){
           /*
            * RH value rise from LOW_RH threshold to HIGH_RH threshold
            */
           Serial.println("Turn on heater");
           // turn on fan and heater
           digitalWrite(CONTROL_IO_HEATER, HIGH);
           digitalWrite(LED_IO_HEATER, HIGH);
           vTaskDelay(10000);
           digitalWrite(CONTROL_IO_FAN, HIGH);   
           digitalWrite(LED_IO_FAN, HIGH);
           heaterCtrlState = HEATER_CTRL_STATE_HIGH_RH;
        }
      }
      else if(RH_value < RH_THRESHOLD_LOW){
        if(heaterCtrlState != HEATER_CTRL_STATE_LOW_RH){
           /*
            * RH value drop from HIGH_RH threshold to LOW_RH threshold
            */
           // turn off fan and heater
           Serial.println("Turn off heater, go to IDLE");
           digitalWrite(CONTROL_IO_HEATER, LOW);
           digitalWrite(LED_IO_HEATER, LOW);   
           vTaskDelay(60000);   
           digitalWrite(CONTROL_IO_FAN, LOW);  
           digitalWrite(LED_IO_FAN, LOW);
           heaterCtrlState = HEATER_CTRL_STATE_LOW_RH;
        }
      }
    }  
    
    // task sleep
    vTaskDelay(500);
  }
}
