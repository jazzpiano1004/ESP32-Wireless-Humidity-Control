#include "bluetooth.h"



// The remote service we wish to connect to.
BLEUUID serviceUUID("2c0c9b4a-2a57-11eb-adc1-0242ac120002");

#define CHARACTERISTIC_UUID_1  "beb5483e-36e1-4688-b7f5-ea07361b26a1"
#define CHARACTERISTIC_UUID_2  "beb5483e-36e1-4688-b7f5-ea07361b26a2"
#define CHARACTERISTIC_UUID_3  "beb5483e-36e1-4688-b7f5-ea07361b26a3"
#define CHARACTERISTIC_UUID_4  "beb5483e-36e1-4688-b7f5-ea07361b26a4"
#define CHARACTERISTIC_UUID_5  "beb5483e-36e1-4688-b7f5-ea07361b26a5"
#define CHARACTERISTIC_UUID_6  "beb5483e-36e1-4688-b7f5-ea07361b26a6"
#define CHARACTERISTIC_UUID_7  "beb5483e-36e1-4688-b7f5-ea07361b26a7"
#define CHARACTERISTIC_UUID_8  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_9  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHARACTERISTIC_UUID_10 "beb5483e-36e1-4688-b7f5-ea07361b26a0"

#define CHARACTERISTIC_UUID CHARACTERISTIC_UUID_5
// The characteristic of the remote service we are interested in.
BLEUUID    charUUID(CHARACTERISTIC_UUID);


boolean doConnect = false;
boolean connected = false;
boolean doScan = true;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
