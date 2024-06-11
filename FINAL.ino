#include <BLEDevice.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker details
const char* mqtt_server = "";
const int mqtt_port = ;
const char* mqtt_user = "";
const char* mqtt_password = "";

// UUIDs for the remote service and characteristics
static BLEUUID serviceUUID("");
static BLEUUID rxCharUUID("");
static BLEUUID txCharUUID("");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic *pTxCharacteristic;
static BLEAdvertisedDevice *myDevice;

WiFiClient espClient;
PubSubClient client(espClient);

void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
    client.publish("BLEdata", (char*)pData, length);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient *pclient) {}
    void onDisconnect(BLEClient *pclient) {
        connected = false;
    }
};

bool connectToServer() {
    BLEClient *pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        pClient->disconnect();
        return false;
    }

    pTxCharacteristic = pRemoteService->getCharacteristic(txCharUUID);
    if (pTxCharacteristic == nullptr) {
        pClient->disconnect();
        return false;
    }

    if (pTxCharacteristic->canNotify()) {
        pTxCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    BLEDevice::init("");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    client.setServer(mqtt_server, mqtt_port);

    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("ArduinoClient", mqtt_user, mqtt_password)) {
            break;
        } else {
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (doConnect) {
        if (connectToServer()) {
            Serial.println("Connected to BLE Server.");
        } else {
            Serial.println("Failed to connect to BLE Server.");
        }
        doConnect = false;
    }

    if (connected) {
        String inputValue = "";
        pTxCharacteristic->writeValue(inputValue.c_str(), inputValue.length());
        delay(5000); 
    } else {
        BLEDevice::getScan()->start(0);
    }

    delay(1000);
}
