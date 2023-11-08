#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>  
#include <WiFiClientSecure.h>
// #include <math.h>

#define CO A0     // Pin for the sensor
#define BUZZER 16 // BUZZER Pin GPIO 16
#define SCREEN_WIDTH 128 // Display width
#define SCREEN_HEIGHT 32 // Display height 

// Declaration SSD1306 connect to I2C
#define OLED_RESET -1   // Sharing Arduino reset pin
#define SCREEN_ADDRESS 0x3C // 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);

const int gas1Treshold = 400; // Gas treshold

const char* host = "script.google.com"; // Host
const int httpsPort = 443; // httpsPort

const char* ssid = "router_"; // wifi name or SSID.
const char* password = "martin420"; // wifi password

WiFiClientSecure client; // WiFiClientSecure object
// Apps Script Deployment ID
String GScriptID = "AKfycbzXLmbOBJVvaBzZIHXtk8HFcCcx2kDibFZvlCwFZMoFegwwZndMCPJU7HjmKKRtgP_iTg";

float gasData = 0.0; // Gas Data
String locData = "L1";

unsigned long preSendTime = 0; // Store last time
const long sendPeriod = 2000; // Interval at which to send data

unsigned long preDisplayTime = 0;
const long displayPeriod = 2000;

unsigned long preGas1Time = 0;
const long gas1Period = 500;

void sendData(String, float);
void displayData();

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 failed"));
    for(;;); // Don't proceed, loop forever
  };
  display.display();
  delay(2000);
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
  gasData = analogRead(CO); // Read gas value
  // gasData = NAN; // NaN for Debugging
  if (isnan(gasData)) { // Check if any reads failed
    if (curTime - preGas1Time >= gas1Period) {
      preGas1Time = curTime;
      Serial.println("Failed to read!");
      return;
    }
  }
  if (curTime - preDisplayTime >= displayPeriod){
    preDisplayTime = curTime;
    displayData();
    display.clearDisplay();
  }
  // delay(2000);
  if (gasData > gas1Treshold) {
    digitalWrite(BUZZER, LOW); // Turn on BUZZER
  } else digitalWrite (BUZZER, HIGH);
    
  String Loc = "Location : " + locData;
  String Gas = "Gas : " + String(gasData);
  Serial.println(Loc); 
  Serial.println(Gas);

  if (curTime - preSendTime >= sendPeriod) {
    preSendTime = curTime;
    sendData(locData, gasData);
  }
}

void sendData(String loct, float gas) {
  Serial.println("==========");
  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  // Processing and sending data
  String string_gas = String(gas);
  String url = "/macros/s/" + GScriptID 
   + "/exec?locData=" + loct
   + "&gasData=" + string_gas; 
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

void displayData(){
  display.setCursor(5, 10); // (x, y)
  display.setTextSize(1);
  display.print("CO:");
  display.println(gasData);
  display.display();
}