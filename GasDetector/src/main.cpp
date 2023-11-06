#include <Arduino.h>
#include <ESP8266WiFi.h> // WiFi 
#include <WiFiClientSecure.h>
// #include <math.h>

#define CO A0   // Pin for the sensor
#define LED 16  // LED Pin

const int ppm = 400; // Gas treshold

const char* host = "script.google.com"; // Host
const int httpsPort = 443; // httpsPort

const char* ssid = "router_"; // wifi name or SSID.
const char* password = "martin420"; // wifi password

WiFiClientSecure client; // WiFiClientSecure object

// Apps Script Deployment ID
String GScriptID = "AKfycbzXLmbOBJVvaBzZIHXtk8HFcCcx2kDibFZvlCwFZMoFegwwZndMCPJU7HjmKKRtgP_iTg";

float gasData = 0.0; // Gas Data
String locData = "L1";


unsigned long prevSendMillis = 0; // Store last time
const long sendInterval = 2000; // Interval at which to send data
/*
unsigned long prevWifiMillis = 0;
const long wifiInterval = 1000;
*/
unsigned long prevGas1Millis = 0;
const long gas1Interval = 500;

void sendData(String, float);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  WiFi.begin(ssid, password); // Connect to WiFi 
  // unsigned long startMillis = millis(); // Start time

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { // Waiting connection
    /*
    if (startMillis - prevWifiMillis >= wifiInterval) {
      prevWifiMillis = startMillis;
      Serial.print(".");
      // continue;
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
  unsigned long currentMillis = millis(); // Current time
    
  gasData = analogRead(CO); // Read gas value
  // gasData = NAN; // NaN for Debugging
  if (isnan(gasData)) { // Check if any reads failed
    /*
    Serial.println("Failed to read!");
    delay(500);
    return;*/
    if (currentMillis - prevGas1Millis >= gas1Interval) {
      prevGas1Millis = currentMillis;
      Serial.println("Failed to read!");
      return;
    }
  }

  if (gasData > ppm) {
    digitalWrite(LED, LOW); // Turn on LED
  } else digitalWrite (LED, HIGH);
    
  String Loc = "Location : " + locData;
  String Gas = "Gas : " + String(gasData);
  Serial.println(Loc); 
  Serial.println(Gas);

  if (currentMillis - prevSendMillis >= sendInterval) {
    prevSendMillis = currentMillis;
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