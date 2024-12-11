#include <ArduinoWebsockets.h>


#include <WiFiClient.h>
#include <ESP8266WiFi.h>

#include <WebSocketClient.h>
#include <Arduino_JSON.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

/*
 *  Created by TheCircuit
*/

#define SS_PIN 15       //D8
#define RST_PIN 0       //D3
int DoorLed = 16;       // D0
int doorMotor = 2;      // D4
const int pingPin = 4;  //D2 Trigger Pin of Ultrasonic Sensor
const int echoPin = 5;  //D1 Echo Pin of Ultrasonic Sensor
long duration, cm;
bool doorStateServer = false;
// bool doorStateServer = false;
const char* ssid = "Fest";
const char* password = "12345678";

Servo servo;
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
// WebSocketClient ws(false);
using namespace websockets;

WebsocketsClient client;



void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  SPI.begin();         // Initiate SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522

  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  bool connected = client.connect("wss://autochalitbackend.onrender.com");

  while (!connected) {
    client.connect("wss://autochalitbackend.onrender.com");
    Serial.println("connecting to websocket");
    delay(500);
  }
  pinMode(DoorLed, OUTPUT);
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(500);
  digitalWrite(DoorLed, HIGH);
  delay(200);
  digitalWrite(DoorLed, LOW);
  delay(200);
  digitalWrite(DoorLed, HIGH);
  delay(200);
  digitalWrite(DoorLed, LOW);
  servo.attach(doorMotor);
  Serial.println("Setup is completed");
  servo.write(180);
}

void loop() {
  cm = calculateDistance();

  // connectWebSocket();

  // if (ws.available()) {
  //   String msg;
  //   if (ws.isConnected()) {

  //     if (ws.getMessage(msg)) {  // loop stuck here
  //       Serial.println("Received message: " + msg);
  //       processWebSocketMessage(msg);
  //     } else {
  //       Serial.println("No message");
  //     }
  //   }
  // }
  if (client.available()) {
    client.poll();
  }

  client.onMessage(onMessageCallback);

  if (doorStateServer) {
    if (cm < 25) {
      servo.write(0);
    } else {
      doorStateServer = false;
      delay(1000);
      servo.write(180);
      sendWebSocketMessage("off", false);
    }
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {


    // Serial.println("hello opwejeufhighiuy");
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show UID on serial monitor
  Serial.println();
  Serial.print(" UID tag :");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println();
  Serial.print("Tag: ");
  Serial.println(content);

  if (content.substring(1) == "C3 4B BB 08" || content.substring(1) == "03 98 61 0D") {  // Change UID of the card that you want to give access to
    Serial.println(" Access Granted ");
    Serial.println(" Welcome Memby ");
    digitalWrite(DoorLed, HIGH);
    servo.write(0);

    doorStateServer = true;
    sendWebSocketMessage("on", true);
    delay(600);
    digitalWrite(DoorLed, LOW);
    Serial.println(" Have FUN ");
    Serial.println();
    Serial.print("cm: ");
    Serial.println(cm);
  } else {
    if (!doorStateServer) {
      Serial.println(" Access Denied ");
      servo.write(180);
      delay(600);
      doorStateServer = false;
      digitalWrite(DoorLed, HIGH);
      digitalWrite(DoorLed, HIGH);
      delay(200);
      digitalWrite(DoorLed, LOW);
      delay(200);
      digitalWrite(DoorLed, HIGH);
      delay(200);
      digitalWrite(DoorLed, LOW);
      delay(200);
      digitalWrite(DoorLed, HIGH);
      delay(200);
      digitalWrite(DoorLed, LOW);
      sendWebSocketMessage("off", false);
      digitalWrite(DoorLed, LOW);
      delay(200);
      digitalWrite(DoorLed, HIGH);
      delay(200);
      digitalWrite(DoorLed, LOW);
    }
  }

  // Halt PICC
  mfrc522.PICC_HaltA();

  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

long calculateDistance() {
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  return microsecondsToCentimeters(duration);
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

// void connectWebSocket() {
//   while (!ws.isConnected()) {
//     Serial.println("Connecting to WebSocket...");
//     ws.connect("autochalitbackend.onrender.com", "/", 80);
//     delay(500);
//   }
//   if (ws.isConnected()) {
//     Serial.println("Socket Connected");
//   }
// }


void onMessageCallback(WebsocketsMessage message) {
  Serial.println("Message received:");
  Serial.println(message.data());
  processWebSocketMessage(message.data());
}

void processWebSocketMessage(String message) {
  JSONVar responseObject = JSON.parse(message);
  if (JSON.typeof(responseObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  } else {
    String name = String(responseObject["name"]);
    if (name == "door") {
      bool doorState = (bool)responseObject["state"];
      servo.write(doorState ? 0 : 180);
      Serial.print("door state set to: ");
      Serial.println(doorState ? "ON" : "OFF");
      doorStateServer = doorState;
    }
  }
}


void sendWebSocketMessage(String message, bool success) {
  JSONVar msg;
  msg["message"] = message;
  msg["success"] = success;
  msg["name"] = "door";
  String jsonString = JSON.stringify(msg);
  client.send(jsonString);
  Serial.print("Sent message: ");
  Serial.println(jsonString);
}
