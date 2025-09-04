/*
 * Copyright (c) 2025, Film Oddments
 * All rights reserved.
 *
 * This code is for controlling the Remote Mini, a DIY Bluetooth Shutter Release.
 *
 * [Intellectual Property]
 * The design, schematics, and this code are the intellectual property of Film Oddments.
 *
 * [License]
 * This source code is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 
 * International License (CC BY-NC-ND 4.0). You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at: http://creativecommons.org/licenses/by-nc-nd/4.0/
 *
 * Under this license, you are free to share (copy and redistribute) the material in any medium or format
 * for non-commercial purposes, but you are NOT allowed to remix, transform, build upon, or distribute
 * modified versions of the material.
 *
 * Unauthorized distribution, reproduction, modification, or commercial use of this code is strictly
 * prohibited and constitutes a violation of the license agreement and copyright law.
 *
 * [Disclaimer of Warranty & Limitation of Liability]
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL FILM ODDMENTS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * [Contact]
 * For any inquiries related to the code, please contact: daenamuro0726@gmail.com
 * For hardware-related inquiries, please contact: yjkim9625@gmail.com
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ESP32Servo.h>
#include <Preferences.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- Servo & Pin Configuration ---
Servo myservo;
int servoPin = 4;
int ledPin = 2; 
int pos = 0;    
bool isBulbMode = false; 

#define SERVO_OFF_POS 0   
int servoOnAngle = 0;       

BLECharacteristic *pCharacteristic;
String receivedMessage = "";
Preferences preferences;

void calculateAndSetOnAngle(int userValue) {
  servoOnAngle = 130 * (180 - userValue) / 100;
  Serial.print("User value received: ");
  Serial.print(userValue);
  Serial.print(" -> Calculated ON Angle: ");
  Serial.println(servoOnAngle);

  preferences.begin("servoConfig", false);
  preferences.putInt("userValue", userValue);
  preferences.end();
  Serial.println("New setting saved.");
}

void loadSettingFromNVS() {
  preferences.begin("servoConfig", true); 
  int userValue = preferences.getInt("userValue", 130); 
  preferences.end();
  
  Serial.print("Loaded userValue from NVS: ");
  Serial.println(userValue);
  
  calculateAndSetOnAngle(userValue);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE and Servo setup!");

  loadSettingFromNVS(); 

  BLEDevice::init("BT Shutter_Film oddments");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setValue("");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  ESP32PWM::allocateTimer(0);
  myservo.setPeriodHertz(50);   
  myservo.attach(servoPin, 500, 2500);
  myservo.write(SERVO_OFF_POS); 

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  if (pCharacteristic->getValue().length() > 0) {
    receivedMessage = pCharacteristic->getValue().c_str();
    Serial.print("Received BLE message: ");
    Serial.println(receivedMessage);

    handleCommand(receivedMessage);  
    pCharacteristic->setValue("");     
  }
}

void handleCommand(String command) {
  int commaIndex = command.indexOf(',');
  if (commaIndex != -1) {
    String mode = command.substring(0, commaIndex);
    mode.trim();
    String valueStr = command.substring(commaIndex + 1);
    valueStr.trim();
    int userValue = valueStr.toInt();

    calculateAndSetOnAngle(userValue);

  } else if (command.equalsIgnoreCase("on")) {
    triggerServo();    
  } else if (command.equalsIgnoreCase("off")) {
    resetServo();      
  } else if (command.equalsIgnoreCase("bulb")) {
    toggleBulbMode();  
  } else if (command.equalsIgnoreCase("blink")) {
    blinkLED();        
  } else {
    Serial.println("Unknown command received.");
  }
}

void triggerServo() {
  Serial.print("Triggering Servo ON... Moving to ");
  Serial.print(servoOnAngle);
  Serial.println(" degrees.");
  
  for (pos = SERVO_OFF_POS; pos <= servoOnAngle; pos += 5) { 
    myservo.write(pos);
    delay(20);
  }
  myservo.write(servoOnAngle);
  
  Serial.print("Servo is ON at ");
  Serial.print(servoOnAngle);
  Serial.println(" degrees.");
}

void resetServo() {
  Serial.println("Triggering Servo OFF... Moving to 0 degrees.");

  for (pos = servoOnAngle; pos >= SERVO_OFF_POS; pos -= 5) {
    myservo.write(pos);
    delay(20);
  }
  myservo.write(SERVO_OFF_POS);
  
  Serial.println("Servo is OFF at 0 degrees.");
}

void toggleBulbMode() {
  if (isBulbMode) {
    Serial.println("Exiting BULB mode...");
    resetServo();
  } else {
    Serial.println("Entering BULB mode...");
    triggerServo();
  }
  isBulbMode = !isBulbMode;
}

void blinkLED() {
  Serial.println("Blinking LED 5 times...");
  for (int i = 0; i < 5; i++) {
    digitalWrite(ledPin, HIGH);
    delay(300);
    digitalWrite(ledPin, LOW);
    delay(300);
  }
  Serial.println("LED blinking complete.");
}
