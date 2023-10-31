#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <FirebaseArduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
const uint8_t fingerprint[20] = { 0x5A, 0x12, 0xCA, 0xB5, 0x35, 0x69, 0x04, 0x81, 0xE6, 0x1F, 0x8A, 0x3D, 0xBA, 0xF1, 0x87, 0x1A, 0x24, 0xA5, 0x40, 0x64 };
ESP8266WiFiMulti WiFiMulti;

int lightMode = 0; // 0=iseneselik, 1=nupuvajutuse peale, 2=vilkuv kollane
int lightLength = 60000; //sekundites kogu tsükli pikkus

const int pedButton = D2;
const int pedRed = D0;
const int pedGreen = D1;
const int pedButtonLight = D3;
const int carRed = D5;
const int carYellow = D6;
const int carGreen = D7;

unsigned long startTime, endTime, currentTime, relativeTime;
bool pedButtonState;

void getLightInfo() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setFingerprint(fingerprint);
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");



    if (https.begin(*client, "https://jl-arduino-default-rtdb.europe-west1.firebasedatabase.app/1110.json")) {  // HTTPS

      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET();
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();        
        lightMode = payload.substring(1, 2).toInt();
        lightLength = payload.substring(3, payload.length() - 1).toInt() * 1000;
        Serial.println(lightMode + "," + lightLength);
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

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

void turnAllCarOff() {
  digitalWrite(carRed, LOW);
  digitalWrite(carYellow, LOW);
  digitalWrite(carGreen, LOW);
}

void turnAllPedOff() {
  digitalWrite(pedRed, LOW);
  digitalWrite(pedGreen, LOW);
}

// 5000 ms
void setCarRed() {
  turnAllCarOff();

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
  turnAllCarOff();

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
  turnAllPedOff();

  for (int i = 0; i < 10; i++) {
    digitalWrite(pedGreen, i % 2 == 0 ? HIGH : LOW);
    delay(500);
  }

  digitalWrite(pedGreen, LOW);
  digitalWrite(pedRed, HIGH);
}

// 2000 ms
void setPedGreen() {
  turnAllPedOff();

  digitalWrite(pedGreen, HIGH);
  delay(2000);
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

  startTime = millis();
  
  turnAllCarOff();
  turnAllPedOff();
  digitalWrite(carRed, HIGH);
  digitalWrite(pedRed, HIGH);
}

void customWaitUntil(unsigned long until) {
  unsigned long offset = millis() - startTime;

  if (until > offset) {
    delay(until - offset);
  }
  else {
    // no time
  }
}

void customWaitUntilWithFunnyTwist(unsigned long until) {  
  while (until > millis() - startTime) {
    updatePedLight();
    delay(50);
  }
}

void loop() {    
  getLightInfo();
  unsigned long freeLength = lightLength - 2000 - 5000 - 2000 - 5000;
  unsigned long carLength = freeLength * 0.7;
  unsigned long pedLength = freeLength * 0.3;
  
  if (lightMode == 0) {
    pedButtonState = false;
    digitalWrite(pedRed, HIGH);
    
    setCarGreen();
    customWaitUntilWithFunnyTwist(2000 + carLength);
    setCarRed();
    customWaitUntil(2000 + carLength + 5000);    
    setPedGreen();
    resetPedLight();
    customWaitUntil(2000 + carLength + 5000 + 2000 + pedLength);    
    setPedRed();
    customWaitUntil(2000 + carLength + 5000 + 2000 + pedLength + 5000);
    
    startTime = startTime + lightLength;
  }
  else if (lightMode == 1) {
    digitalWrite(pedRed, HIGH);
    setCarGreen();
    
    do {
      for (int i = 0; i < 5; i++) {
        updatePedLight();
        delay(1000);
      }

      getLightInfo();
    }
    while (lightMode == 1 && !pedButtonState);

    if (lightMode == 1) {
      // ega me teda ometigi kohe üle tee luba
      delay(1000);
      
      startTime = millis();
      
      setCarRed();
      customWaitUntil(5000);
      
      resetPedLight();
      
      setPedGreen();
      customWaitUntil(5000 + 2000 + pedLength);
      setPedRed();
      customWaitUntil(5000 + 2000 + pedLength + 5000);
    }
    
    startTime = millis();
  }
  else {
    turnAllCarOff();
    turnAllPedOff();
    
    for (int i = 0; i < 5; i++) {
      digitalWrite(carYellow, HIGH);
      delay(1000);
      digitalWrite(carYellow, LOW);
      delay(1000);
    }
    startTime = millis();
  }  
}
