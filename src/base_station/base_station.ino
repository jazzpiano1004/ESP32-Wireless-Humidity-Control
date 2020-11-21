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
#define RH_THRESHOLD_HIGH             54
#define RH_THRESHOLD_LOW              48
#define HEATER_CTRL_STATE_HIGH_RH     1
#define HEATER_CTRL_STATE_LOW_RH      2
float RH_value=50;
float temperatureValue;
int8_t heaterCtrlState = -1;

/*
 * BLE Define, Class & Global variable
 */
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SERVICE_UUID        "2c0c9b4a-2a57-11eb-adc1-0242ac120002"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_CONNECTTIMEOUT      15
#define LED_IO_BLE_CONNECTED    25
String   BLE_message;
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
      RH_value = BLE_message.toFloat();
      Serial.println(BLE_message);
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
                    task_heater_ctrl,          /* Task function. */
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
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
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
    
    // task sleep
    vTaskDelay(1500);
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



void task_heater_ctrl(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    if(RH_value > RH_THRESHOLD_HIGH){
       if(heaterCtrlState != HEATER_CTRL_STATE_HIGH_RH){
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
           // turn off fan and heater
           digitalWrite(CONTROL_IO_HEATER, LOW);
           digitalWrite(LED_IO_HEATER, LOW);   
           vTaskDelay(60000);   
           digitalWrite(CONTROL_IO_FAN, LOW);  
           digitalWrite(LED_IO_FAN, LOW);

           heaterCtrlState = HEATER_CTRL_STATE_LOW_RH;
       }
    }  
    
    // task sleep
    vTaskDelay(100);
  }
}
