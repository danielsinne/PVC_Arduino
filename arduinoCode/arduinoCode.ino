#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Wifi
const char* ssid = "Oneplus_6";
const char* password = "12345678";

// Time
int timezone = 1;
int dst = 1;
struct tm * t;

// Data wire of temperature sensor is connected to the Arduino digital pin 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to Dallas Temperature sensor 
DallasTemperature temperatureSensor(&oneWire);

// Firebase server
String serverName = "http://34.90.125.247:8080/api/v1/temperatures";
// Currently connects every 10 minutes after started
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
//unsigned long timerDelay = 5000;

// When equal 6, 1 hour has passed duh
unsigned int counter10min = 0;
double tempCTotal = 0.0;

void setup(void)
{ 
  // Open serial monitor
  Serial.begin(9600);
  
  // Connect to wifi
  delay(5000);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.println(".");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //TIME
  configTime(timezone * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");

  // SENSORS
  // Temperature sensor
  temperatureSensor.begin();
}

void loop(void){ 
  for(int i = 0; i < 6; i++){
    // READ SENSORS
    // Call temperatureSensor.requestTemperatures() to issue a global temperature and Requests to all devices on the bus (multiple temperature sensors could be connected)
    temperatureSensor.requestTemperatures();
    double tempC = temperatureSensor.getTempCByIndex(0); //get temperature
    Serial.print("Temp before rounding: ");
    Serial.println(tempC);
    tempC = round(tempC*10)/10;
    Serial.print("Temp after rounding: ");
    Serial.println(tempC);
    tempCTotal += tempC;

    delay(5000);//600000
  }

  // SERVER
  // Send an HTTP POST request depending on timerDelay
    
  // Print temperature to serial monitor
  Serial.print("Celsius temperature: ");
  Serial.println(tempCTotal/6); //getTempFByIndex() fot fahrenheit

   //check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
  
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);

    // Calculate current time
    time_t now;
    time(&now);
    t = localtime(&now);
    char currentTime[sizeof "2011-10-08T07:07:09"];
    strftime (currentTime, sizeof currentTime,"%FT%TZ",t);
  

    // JSON document
    DynamicJsonDocument doc(200);
    doc["temperatureValue"] = tempCTotal/6;
    doc["localDateTime"] = currentTime;
    doc["inputSource"] = "ARDUINO";
    String json;
    serializeJson(doc, json);
    
    // If you need an HTTP request with a content type: application/json, use the following:
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(json);

    //Cleanup
    tempCTotal = 0.0;

    // Read response
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");

    Serial.println("Attempting to reconnect");
    //Try reconnecting
    WiFi.begin(ssid, password);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println(".");
    }
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}
