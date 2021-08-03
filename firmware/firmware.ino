/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define NAME                "ESP32 BLE Controller"
#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"
#define MAX_DELAY           500
#define MOTORS              2
#define CHANNELS            (MOTORS * 2)

// pins
// every motor needs two pins and two channels.
// The ESP32 has up to 16 PWM channels, so can support up to 8 motors
// Two pins / channels per motor connect to C1 and C2 (forward / backward)
const int mPins[] =  {16, 17, 18, 19};
const int mChannels[] = {0, 1, 2, 3};
// PWM properties
const int freq = 5000;
const int resolution = 8;

// BLE objects
BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLEAdvertising *pAdvertising = NULL;
bool debug = false;
String readString;

// remember last time the channels have been updated, when no new input arrives within MAX_DELAY, set output to 0
unsigned int lastUpdate = 0;

class ServerState : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("onConnect");
    }

    void onDisconnect(BLEServer* pServer) {
      Serial.println("onDisconnect");
      Serial.println("restart advertising...");
      BLEDevice::startAdvertising();
    }
};

void setMotors(byte motors[MOTORS]) {
  for (byte i = 0; i < MOTORS; i++) {
    signed char val = motors[i];
    if(debug) {
      Serial.println(String("Set motor ") + i + " val:" + val);
    }
    if (val > 0) {
      // forwards
      ledcWrite(mChannels[2*i], val * 2);
      ledcWrite(mChannels[2*i+1], 0);
    } else {
      // backwards
      ledcWrite(mChannels[2*i], 0);
      ledcWrite(mChannels[2*i+1], -val * 2);
    }
  }
  lastUpdate = millis();
}

class CharacteristicState : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      if(debug) {
        Serial.println("RX characteristic");
      }
      byte data[MOTORS] = {0};
      if(pCharacteristic->getLength() <= MOTORS) {
        memcpy(data, pCharacteristic->getData(), pCharacteristic->getLength());
        // for(byte i = 0; i < pCharacteristic->getLength(); i++) {
        //   data[i] = pCharacteristic->getData()[i];
        // }
        setMotors(data);
      } else {
        if(debug) {
          Serial.println("Received characteristic is too long: "+pCharacteristic->getLength());
        }
      }
      //Serial.write(pCharacteristic->getData(), pCharacteristic->getLength());
    }
    
    void onRead(BLECharacteristic* pCharacteristic) {
      // somebody did read the Characteristic.
    }

};

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("");
  Serial.print("Booting ");
  Serial.print(NAME);
  Serial.println("...");
  Serial.setTimeout(100);
  readString.reserve(128);

  for(byte i = 0; i < (sizeof(mPins)/sizeof(mPins[0])); i++) {
    ledcSetup(mChannels[i], freq, resolution);
    ledcAttachPin(mPins[i], mChannels[i]);
  }

  Serial.println("Starting BLE...");
  BLEDevice::init(NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerState());
  pService = pServer->createService(SERVICE_UUID);
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      // BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new CharacteristicState());
  Serial.print("Characteristic UUID: ");
  Serial.println(CHARACTERISTIC_UUID);

  pCharacteristic->setValue("");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();


  Serial.println(String("debug = ") + debug);
  Serial.println("Boot done.");
}

void loop() {
  if (lastUpdate && millis() > lastUpdate + MAX_DELAY) {
    byte zeros[MOTORS] = {0};
    setMotors(zeros);
    // when channels are zero anyway, no need to check for update until after real input
    lastUpdate = 0;
  }
  if (Serial.available()) {
    readString = Serial.readStringUntil('\n');
  }
  if (readString.endsWith("debug")) { // strstr(readString.c_str(), "debug")
    debug = !debug;
    Serial.println(String("debug = ") + debug);
    readString = "";
  }
  delay(50);
  /*static std::string lastCharacteristic = "";
    std::string currentCharacteristic = pCharacteristic->getValue();

    if (lastCharacteristic.compare(currentCharacteristic) != 0) {
    lastCharacteristic = currentCharacteristic;

    Serial.println(currentCharacteristic.c_str());
    //Serial.println(pCharacteristic->getData().c_str());
    }
  */
  /*
    Serial.write(pCharacteristic->getData(), pCharacteristic->getLength());
    Serial.println("");
  */
}
