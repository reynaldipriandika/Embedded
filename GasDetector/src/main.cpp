#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>  
#include <WiFiClientSecure.h>
// #include <math.h>

#define CO_PIN  14  // GPIO14/D5 for sensor MQ7
#define CO2_PIN 12  // GPIO12/D6 for sensor MQ135
#define ANALOG  A0  // Analog Pin
#define BUZZER  16  // BUZZER Pin GPIO 16
#define SCREEN_WIDTH 128 // Display width
#define SCREEN_HEIGHT 32 // Display height 

// Declaration SSD1306 connect to I2C
#define OLED_RESET -1   // Sharing Arduino reset pin
#define SCREEN_ADDRESS 0x3C // 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);

const int treshold = 400; // Gas treshold

const char* host = "script.google.com"; // Host
const int httpsPort = 443; // httpsPort

const char* ssid = "router_"; // wifi name or SSID.
const char* password = "martin420"; // wifi password

WiFiClientSecure client; // WiFiClientSecure object
// Apps Script Deployment ID
String GScriptID = "AKfycbzXLmbOBJVvaBzZIHXtk8HFcCcx2kDibFZvlCwFZMoFegwwZndMCPJU7HjmKKRtgP_iTg";

float coData  = 0.0;     // CO Data
float co2Data = 0.0;    // CO2 Data
String locData = "L1";  // Location

unsigned long sendTime = 0; // Store last time
const long sendPeriod = 2000; // Interval at which to send data

unsigned long displayTime = 0;
const long displayPeriod = 2000;

unsigned long coETime = 0;
unsigned long co2ETime= 0;
unsigned long coTime  = 0;
unsigned long co2Time = 0;

const long coEPeriod  = 500;
const long co2EPeriod = 500;
const long coPeriod   = 200;
const long co2Period  = 200;

void sendData(String, float, float);
void displayGas();
float coRead();
float co2Read();

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(CO_PIN, OUTPUT);
  pinMode(CO2_PIN, OUTPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 failed"));
    for(;;); // Don't proceed, loop forever
  };
  display.display();
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);

  WiFi.begin(ssid, password); // Connect to WiFi 
  // unsigned long startMillis = millis(); // Start time

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { // Waiting connection
    /*if (startMillis - prevWifiMillis >= wifiInterval) {
      prevWifiMillis = startMillis;
      Serial.print(".");
    }*/
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.println();

  client.setInsecure();
}

void loop() {
  unsigned long curTime = millis(); // Current time
  if (curTime - coTime >= coPeriod) {
    coTime = curTime;
    coData = coRead(); // Read CO value
  }
  // coData = NAN; // NaN for Debugging
  if (isnan(coData)) { // Check if any reads failed
    if (curTime - coETime >= coEPeriod) {
      coETime = curTime;
      Serial.println("Failed read CO");
      return;
    }
  }
  if (curTime - co2Time >= co2Period) {
    co2Time = curTime;
    co2Data = co2Read(); // Read CO2
  }
  if (isnan(co2Data)) { // Failed to read
    if (curTime - co2ETime >= co2EPeriod) {
      co2ETime = curTime;
      Serial.println("Failed read CO2");
      return;
    }
  }
  if (curTime - displayTime >= displayPeriod){
    displayTime = curTime;
    displayGas();
    display.clearDisplay();
  }
  if ((coData > treshold) | (co2Data > treshold)) {
    digitalWrite(BUZZER, LOW); // Turn on BUZZER
  } else digitalWrite (BUZZER, HIGH);
    
  String locStr = "Location: " + locData;
  String coStr = "CO: "   + String(coData);
  String co2Str = "CO2: " + String(co2Data);
  Serial.println(locStr); 
  Serial.println(coStr);
  Serial.println(co2Str);

  if (curTime - sendTime >= sendPeriod) {
    sendTime = curTime;
    sendData(locData, coData, co2Data);
  }
}

void sendData(String loct, float co, float co2) {
  Serial.println("==========");
  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  // Processing and sending data
  String string_co = String(co);
  String string_co2 = String(co2);
  String url = "/macros/s/" + GScriptID 
   + "/exec?locData=" + loct
   + "&coData="  + string_co    // Send CO data 
   + "&co2Data=" + string_co2;  // Send CO2 data
  Serial.print("requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n"
   + "Host: " + host + "\r\n"
   + "User-Agent: BuildFailureDetectorESP8266\r\n"
   + "Connection: close\r\n\r\n"
  );
  Serial.println("request sent");

  // Checking the data was sent or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") Serial.println("headers received");
    break;
  }
  
  String line = client.readStringUntil('\n');
  if(line.startsWith("{\"state\":\"success\"")){
    Serial.println("esp8266 CI successfull!");
  } else Serial.println("esp8266 CI has failed");
  
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println();
}

void displayGas(){
  display.setCursor(5, 10); // (x, y)
  display.setTextSize(1);
  display.print("CO:");
  display.println(coData);
  
  display.setCursor(5, 20); // (x, y)
  display.print("CO2:");
  display.println(co2Data);
  display.display();
}

float coRead() {
  digitalWrite(CO_PIN, HIGH); // MQ7 Turn ON
  digitalWrite(CO2_PIN, LOW); // MQ135 Turn OFF
  return analogRead(ANALOG);
}
float co2Read() {
  digitalWrite(CO_PIN, LOW); // MQ7 Turn OFF
  digitalWrite(CO2_PIN, HIGH); // MQ135 Turn ON
  return analogRead(ANALOG);
}