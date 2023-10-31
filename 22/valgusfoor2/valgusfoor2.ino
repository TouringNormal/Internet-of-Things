#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <FirebaseArduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
const uint8_t fingerprint[20] = { 0x5A, 0x12, 0xCA, 0xB5, 0x35, 0x69, 0x04, 0x81, 0xE6, 0x1F, 0x8A, 0x3D, 0xBA, 0xF1, 0x87, 0x1A, 0x24, 0xA5, 0x40, 0x64 };
ESP8266WiFiMulti WiFiMulti;

const int pedButton = D2;
const int pedRed = D0;
const int pedGreen = D1;
const int pedButtonLight = D3;
const int carRed = D5;
const int carYellow = D6;
const int carGreen = D7;

int cycleMode = 2;
int cycleLength = 60000;
int cycleOffset = 30;

unsigned long globalStartTime = 0;
unsigned long startTime, endTime, currentTime, relativeTime;

bool pedButtonState;

void updatePedLight() {
  // if pedestrian presses button, we do thing
  if (!digitalRead(pedButton)) {
    pedButtonState = true;
  }

  if (pedButtonState) {
    digitalWrite(pedButtonLight, HIGH);
  }
  else {
    digitalWrite(pedButtonLight, LOW);
  }
}

void resetPedLight() {
  pedButtonState = false;
  digitalWrite(pedButtonLight, LOW);
}

void setCarOff() {
  digitalWrite(carRed, LOW);
  digitalWrite(carYellow, LOW);
  digitalWrite(carGreen, LOW);
}

void setPedOff() {
  digitalWrite(pedRed, LOW);
  digitalWrite(pedGreen, LOW);
}

void setAllLights(
  int pedRedState = LOW,
  int pedGreenState = LOW,
  int carRedState = LOW,
  int carYellowState = LOW,
  int carGreenState = LOW,
  int pedButtonLightState = LOW) {
  if (pedRedState != -1) digitalWrite(pedRed, pedRedState);
  if (pedGreenState != -1) digitalWrite(pedGreen, pedGreenState);
  if (carRedState != -1) digitalWrite(carRed, carRedState);
  if (carYellowState != -1) digitalWrite(carYellow, carYellowState);
  if (carGreenState != -1) digitalWrite(carGreen, carGreenState);
  if (pedButtonLightState != -1) digitalWrite(pedButtonLight, pedButtonLightState);
}

// 5000 ms
void setCarRed() {
  setCarOff();

  // vilkuv roheline
  for (int i = 0; i < 6; i++) {
    digitalWrite(carGreen, i % 2 == 0 ? HIGH : LOW);
    delay(500);
  }

  // veidi kollast
  digitalWrite(carGreen, LOW);
  digitalWrite(carYellow, HIGH);
  delay(2000);

  // läheb punaseks
  digitalWrite(carYellow, LOW);
  digitalWrite(carRed, HIGH);
}

// 2000 ms
void setCarGreen() {
  setCarOff();

  // veidi kollast ja punast
  digitalWrite(carRed, HIGH);
  digitalWrite(carYellow, HIGH);
  delay(2000);

  // läheb roheliseks
  digitalWrite(carRed, LOW);
  digitalWrite(carYellow, LOW);
  digitalWrite(carGreen, HIGH);
}

// 5000 ms
void setPedRed() {  
  setPedOff();

  for (int i = 0; i < 10; i++) {
    digitalWrite(pedGreen, i % 2 == 0 ? HIGH : LOW);
    delay(500);
  }

  digitalWrite(pedGreen, LOW);
  digitalWrite(pedRed, HIGH);
}

// 2000 ms
void setPedGreen() {
  setPedOff();

  digitalWrite(pedGreen, HIGH);
  delay(2000);
}

void customWaitUntil(unsigned long until) {
  unsigned long offset = millis() - (startTime + cycleOffset);

  if (until > offset) {
    delay(until - offset);
  }
  else {
    // no time
  }
}

void customWaitUntilWithFunnyTwist(unsigned long until) {  
  while (until > millis() - (startTime + cycleOffset)) {
    updatePedLight();
    delay(50);
  }
}

void getInfo() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(fingerprint);
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, "https://jl-arduino-default-rtdb.europe-west1.firebasedatabase.app/1118/1.json")) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET();
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

        
        String payload = https.getString();

        String value;
        int startIndex = 0, endIndex = 0;



        startIndex = endIndex + 1;
        endIndex = payload.indexOf(',', startIndex);
        value = payload.substring(startIndex, endIndex);
        cycleMode = value.toInt();

        startIndex = endIndex + 1;
        endIndex = payload.indexOf(',', startIndex);
        value = payload.substring(startIndex, endIndex);
        cycleLength = value.toInt() * 1000;

        startIndex = endIndex + 1;
        endIndex = payload.indexOf(',', startIndex);
        value = payload.substring(startIndex, endIndex);
        cycleOffset = value.toInt() * 1000;



        Serial.print(cycleMode);
        Serial.print(" , ");
        Serial.print(cycleLength);
        Serial.print(" , ");
        Serial.print(cycleOffset);
        Serial.println();
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

void syncTime() {
  while(digitalRead(pedButton)) {
    delay(10);
  }

  digitalWrite(pedRed, HIGH);
  digitalWrite(pedGreen, HIGH);
  digitalWrite(carRed, HIGH);
  digitalWrite(carYellow, HIGH);
  digitalWrite(carGreen, HIGH);
  digitalWrite(pedButtonLight, HIGH);

  while(!digitalRead(pedButton)) {
    delay(10);
  };

  setCarOff();
  setPedOff();
  digitalWrite(pedButtonLight, LOW);
  
  globalStartTime = millis();
}

void setup() {
  pinMode(pedButton, INPUT); // jalakäija nupp
  pinMode(pedRed, OUTPUT); // jalakäija punane
  pinMode(pedGreen, OUTPUT); // jalakäija roheline
  pinMode(pedButtonLight, OUTPUT); // jalakäija nupu lamp
  pinMode(carRed, OUTPUT); //autod punane
  pinMode(carYellow, OUTPUT); //autod kollane
  pinMode(carGreen, OUTPUT); //autod roheline



  Serial.begin(115200);
    for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("TLU", "");


 
  syncTime();
  startTime = globalStartTime;
}

void loop() {
  getInfo();
  unsigned long freeLength = cycleLength - 2000 - 5000 - 2000 - 5000;
  unsigned long carLength = freeLength * 0.7;
  unsigned long pedLength = freeLength * 0.3;
  unsigned long nextStartTime = startTime + cycleLength;

  if (cycleMode == 0) {
    pedButtonState = false;
    setAllLights(HIGH, LOW, LOW, LOW, HIGH, LOW);
    
    customWaitUntilWithFunnyTwist(carLength);
    setCarRed();
    customWaitUntil(carLength + 5000);    
    setPedGreen();
    resetPedLight();
    customWaitUntil(carLength + 5000 + 2000 + pedLength);    
    setPedRed();
    customWaitUntil(carLength + 5000 + 2000 + pedLength + 5000);
    setCarGreen(); 
    customWaitUntil(carLength + 5000 + 2000 + pedLength + 5000 + 2000);   
  }
  else if (cycleMode == 1) {
    digitalWrite(pedRed, HIGH);
    setAllLights(HIGH, LOW, LOW, LOW, HIGH, -1);
  
    while (!pedButtonState && millis() + 1000 <= nextStartTime + cycleOffset) {
      updatePedLight();
      delay(1000);
    }

    if (pedButtonState) {
      unsigned long wait1 = 5000;
      unsigned long wait2 = 5000 + 2000 + pedLength;
      unsigned long wait3 = 5000 + 2000 + pedLength + 5000;

      if (millis() + wait3 <= nextStartTime + cycleOffset) {
        setCarRed();
        customWaitUntil(wait1);
        resetPedLight();
        setPedGreen();
        customWaitUntil(wait2);
        setPedRed();
        customWaitUntil(wait3);
      }
    }
  }
  else {
    setAllLights();

    while (millis() + 2000 <= nextStartTime + cycleOffset) {
      digitalWrite(carYellow, HIGH);
      delay(1000);
      digitalWrite(carYellow, LOW);
      delay(1000); 
    }    
  }

  startTime = nextStartTime;
}
