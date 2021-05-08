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
#define RH_THRESHOLD_HEATER           (float)53
#define RH_THRESHOLD_FAN              (float)48
#define HEATER_CTRL_STATE_HIGH_RH     1
#define HEATER_CTRL_STATE_LOW_RH      2
#define HEATER_CTRL_STATE_IDLE        0
float RH_value;
float temperatureValue;
int8_t sht31_disconnected;
int8_t heaterCtrlState = HEATER_CTRL_STATE_IDLE;

/*
 * BLE Define, Class & Global variable
 */
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SERVICE_UUID        "2c0c9b4a-2a57-11eb-adc1-0242ac120002"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"   // pair 1
//#define CHARACTERISTIC_UUID "b816f6fe-4ff4-11eb-ae93-0242ac130002"   // pair 2
//#define CHARACTERISTIC_UUID "d4582f1c-4ff5-11eb-ae93-0242ac130002"   // pair 3

#define BLE_CONNECTTIMEOUT      10
#define LED_IO_BLE_CONNECTED    25
String   BLE_message = "";
uint32_t timeoutCnt = BLE_CONNECTTIMEOUT;
bool     timeoutFlag = true;
int8_t ble_message_ready_flag = 0;


// Callback function class for BLE
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      timeoutCnt = BLE_CONNECTTIMEOUT;
      timeoutFlag = false;
      
      std::string value = pCharacteristic->getValue();
      BLE_message = value.c_str();
      Serial.println(BLE_message);

      ble_message_ready_flag = 1;
    }
};



void setup() {
  Serial.begin(115200);
  
  Serial.println("Initialize GPIO...");
  pinMode(CONTROL_IO_HEATER, OUTPUT);
  pinMode(CONTROL_IO_FAN, OUTPUT);
  pinMode(LED_IO_HEATER, OUTPUT);
  pinMode(LED_IO_FAN, OUTPUT);
  pinMode(LED_IO_BLE_CONNECTED, OUTPUT);
  digitalWrite(LED_IO_HEATER, HIGH);
  digitalWrite(LED_IO_FAN, HIGH);
  digitalWrite(LED_IO_BLE_CONNECTED, HIGH); 

  /*
  Serial.println("Starting BLE...");
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
  */
  
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
                    task_ble,         /* Task function. */
                    "BLE connection", /* String with name of task. */
                    20000,             /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */
  
  //xTaskCreate(
  //                  task_decodeBluetoothMessage,          /* Task function. */
  //                  "BLE timeout checking",        /* String with name of task. */
  //                  1024,             /* Stack size in bytes. */
  //                  NULL,             /* Parameter passed as input of the task */
  //                  1,                /* Priority of the task. */
  //                  NULL);
  
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
       digitalWrite(LED_IO_BLE_CONNECTED, LOW);
    }
    else{
       digitalWrite(LED_IO_BLE_CONNECTED, HIGH);
    }
    if(ble_message_ready_flag != 0){
       Serial.print("temp=");
       Serial.print(temperatureValue);
       Serial.print("\tRH=");
       Serial.print(RH_value);
       Serial.print("\terror=");
       Serial.print(sht31_disconnected);
       Serial.print("\tstate");
       Serial.println(heaterCtrlState);
    }
    else{
       Serial.println("Wait for the first BLE message...");
    }
    
    // task sleep
    vTaskDelay(1500);
  }
}



void task_ble(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  
  Serial.println("Starting BLE...");
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
  
  for (;;) // A Task shall never return or exit.
  {
    if(timeoutCnt > 0){
      timeoutCnt--;
      if(timeoutCnt == 0){
         timeoutFlag = true;
      }
    }
    else{
      // deinit BLE
      BLEDevice::deinit(false);

      // after this, pServer should be deleted to prevent memory leak.
      delete pServer;
      
      // reinit BLE
      Serial.println("Starting BLE...");
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
    }
    // task sleep
    vTaskDelay(1000);
  }
}



void task_heaterControl(void *pvParameters)  // This is a task.
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
  
  for (;;) // A Task shall never return or exit.
  { 
    /*
     * BLE message decoding. Message is CSV format -> temperature, RH_value, sht31_disconnected
     *                                                    field1,   field2,  field3
     */
    if(ble_message_ready_flag != 0)
    {  
      // Reset flag as consuming the BLE data
      ble_message_ready_flag = 0;
      
      /* 
       * Decode BLE message from sensor node 
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
        if(field1[0] != 'N' && field2[0] != 'N')
        {
           temperatureValue = atof(field1);
           RH_value = atof(field2); 
        }
        else{
           temperatureValue = 0;
           RH_value = 0;
        }
        sht31_disconnected = atoi(field3);
      }
      
      /*
       * Heater Control, Depends on sensor information 
       * When there is connection and no error from sensor node
       *
       */
      if(sht31_disconnected == 0){
        // Heater control
        if(RH_value > RH_THRESHOLD_HEATER){
          Serial.println("Turn on heater");
          // turn on fan and heater
          digitalWrite(CONTROL_IO_HEATER, HIGH);
          digitalWrite(LED_IO_HEATER, LOW);
        }
        else{
          Serial.println("Turn off heater");
          // turn on fan and heater
          digitalWrite(CONTROL_IO_HEATER, LOW);
          digitalWrite(LED_IO_HEATER, HIGH);
        }
  
        // Fan control
        if(RH_value > RH_THRESHOLD_FAN){
          Serial.println("Turn off fan");
          // turn on fan and heater
          digitalWrite(CONTROL_IO_FAN, LOW);
          digitalWrite(LED_IO_FAN, HIGH);
        }
        else{
          Serial.println("Turn on fan");
          // turn on fan and heater
          digitalWrite(CONTROL_IO_FAN, HIGH);
          digitalWrite(LED_IO_FAN, LOW);
        }
      }
    }

    // timeout auto turn-off
    if(timeoutFlag || (sht31_disconnected != 0)){
      /*
       * When there is no connection OR get error from sensor node
       * -> Turn off heater and fan
       */
       Serial.println("Turn off heater, go to IDLE");
       Serial.print("timeoutFlag");
       Serial.print(timeoutFlag);
       Serial.print("sht31_disconnected");
       Serial.println(sht31_disconnected);
        
       digitalWrite(CONTROL_IO_HEATER, LOW);
       digitalWrite(LED_IO_HEATER, HIGH);
       digitalWrite(CONTROL_IO_FAN, LOW);  
       digitalWrite(LED_IO_FAN, HIGH);
    }
    
    // task sleep
    vTaskDelay(1000);
  }
}
