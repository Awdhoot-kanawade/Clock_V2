#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <DS3231_Simple.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// WiFi credentials
/*const char* ssid = "AirFiber-8bLTLv5.0";
const char* password = "Pass@123";*/

const char* primarySSID = "TP-Link_35E2";
const char* primaryPassword = "80067450";
const char* secondarySSID = "AirFiber-8bLTLv5.0";
const char* secondaryPassword = "Pass@123";

// Web server running on port 80
ESP8266WebServer server(80);

// NTP Client settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org",19800, 60000); // Update every 60 seconds

DS3231_Simple Clock;
DateTime MyDateAndTime;

#define LEDCLOCK_PIN    D6 // GPIO12
#define LEDDOWNLIGHT_PIN    D5 // GPIO14
#define LED_PER_SEG 7

#define LEDCLOCK_COUNT (23*LED_PER_SEG)
#define LEDDOWNLIGHT_COUNT 12

uint32_t clockHourColour = 0x008000;   // Initial pure green
uint32_t clockMinuteColour = 0x800080; // Initial purple 
uint32_t downlightColour = 0xFFFFFF;   // Initial white

int clockFaceBrightness = 250;
int downlightBrightness = 150;

Adafruit_NeoPixel stripClock(LEDCLOCK_COUNT, LEDCLOCK_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDownlighter(LEDDOWNLIGHT_COUNT, LEDDOWNLIGHT_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  
  /*// Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP address: ");
   Serial.println(WiFi.localIP());*/
  
  WiFi.begin(primarySSID, primaryPassword);
  Serial.print("Connecting to primary network");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  // If connection to primary network fails, try the secondary network
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nPrimary network failed. Trying secondary network...");
    WiFi.begin(secondarySSID, secondaryPassword);
    startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }
  }

  // Check if connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to both networks");
    return;
  }
  

  // Start the NTP client
  timeClient.begin();
  
  Clock.begin();
  
  stripClock.begin();
  stripClock.setBrightness(clockFaceBrightness);
  stripClock.show();
  
  stripDownlighter.begin();
  stripDownlighter.setBrightness(downlightBrightness);
  stripDownlighter.show();

  setupWebServer();



  ArduinoOTA.setHostname("Wall Clock");
  ArduinoOTA.setPassword((const char *)"123456789");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());




}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  readTheTime();
  displayTheTime();
  nightDim();
  stripClock.show();
  
  stripDownlighter.fill(downlightColour, 0, LEDDOWNLIGHT_COUNT);
  stripDownlighter.show();
  delay(500);
}

void nightDim() {
  if ((MyDateAndTime.Hour >= 0) && (MyDateAndTime.Hour < 6)) {  // night time dimming
    stripClock.setBrightness(10);
    stripDownlighter.setBrightness(10);
  } else { 
    stripClock.setBrightness(clockFaceBrightness);
    stripDownlighter.setBrightness(downlightBrightness);
  }
}

void readTheTime() {
  MyDateAndTime = Clock.read();
  Serial.println("");
  Serial.print("Time is: ");   Serial.print(MyDateAndTime.Hour);
  Serial.print(":"); Serial.print(MyDateAndTime.Minute);
  Serial.print(":"); Serial.println(MyDateAndTime.Second);
  Serial.print("Date is: 20");   Serial.print(MyDateAndTime.Year);
  Serial.print(":");  Serial.print(MyDateAndTime.Month);
  Serial.print(":");    Serial.println(MyDateAndTime.Day);
}

void displayTheTime() {
  stripClock.clear();
  
  int firstMinuteDigit = MyDateAndTime.Minute % 10;
  displayNumber(firstMinuteDigit, 0, clockMinuteColour);

  int secondMinuteDigit = floor(MyDateAndTime.Minute / 10);
  displayNumber(secondMinuteDigit, (7*LED_PER_SEG), clockMinuteColour);


  int firstHourDigit = MyDateAndTime.Hour; //work out the value for the third digit and then display it
  if (firstHourDigit > 12){
    firstHourDigit = firstHourDigit - 12;
  }
 
 // Comment out the following three lines if you want midnight to be shown as 12:00 instead of 0:00
  if (firstHourDigit == 0){
    firstHourDigit = 12;
   }
 
  firstHourDigit = firstHourDigit % 10;
  displayNumber(firstHourDigit, (14*LED_PER_SEG), clockHourColour);


  int secondHourDigit = MyDateAndTime.Hour; //work out the value for the fourth digit and then display it

// Comment out the following three lines if you want midnight to be shwon as 12:00 instead of 0:00
  if (secondHourDigit == 0){
    secondHourDigit = 12;
  }
 
 if (secondHourDigit > 12){
    secondHourDigit = secondHourDigit - 12;
  }
    if (secondHourDigit > 9){
      stripClock.fill(clockHourColour,(21*LED_PER_SEG), 14); 
    }

  

  
}

void displayNumber(int digitToDisplay, int offsetBy, uint32_t colourToUse) {
  switch (digitToDisplay) {  case 0: digitZero(offsetBy, colourToUse); break;
    case 1: digitOne(offsetBy, colourToUse); break;
    case 2: digitTwo(offsetBy, colourToUse); break;
    case 3: digitThree(offsetBy, colourToUse); break;
    case 4: digitFour(offsetBy, colourToUse); break;
    case 5: digitFive(offsetBy, colourToUse); break;
    case 6: digitSix(offsetBy, colourToUse); break;
    case 7: digitSeven(offsetBy, colourToUse); break;
    case 8: digitEight(offsetBy, colourToUse); break;
    case 9: digitNine(offsetBy, colourToUse); break;
  }
}

  void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    String html = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; max-width: 600px; margin-left: auto; margin-right: auto; padding: 10px; box-sizing: border-box; }";
    html += "h1 { color: #333; font-size: 24px; text-align: center; }";
    html += "input[type=range] { width: 100%; margin: 10px 0; }";
    html += "input[type=color] { width: 100%; height: 40px; border: none; margin: 10px 0; }";
    html += "label { display: block; margin-top: 15px; font-size: 16px; }";
    html += "button { width: 100%; padding: 15px; font-size: 18px; color: #fff; background-color: #007BFF; border: none; border-radius: 5px; cursor: pointer; margin-top: 20px; }";
    html += "button:hover { background-color: #0056b3; }";
    html += "</style></head><body>";
    html += "<h1>Clock Control</h1>";
    html += "<button onclick=\"fetch('/syncTime')\">Sync Time</button><br><br>";
    
    // Brightness sliders
    html += "<label for=\"clockBrightness\">Clock Brightness: <input type=\"range\" min=\"0\" max=\"255\" value=\"" + String(clockFaceBrightness) + "\" id=\"clockBrightness\" onchange=\"updateBrightness()\"></label>";
    html += "<label for=\"downlightBrightness\">Downlight Brightness: <input type=\"range\" min=\"0\" max=\"255\" value=\"" + String(downlightBrightness) + "\" id=\"downlightBrightness\" onchange=\"updateBrightness()\"></label><br><br>";
    
    // Color pickers
    html += "<label for=\"hourColor\">Hour Color: <input type=\"color\" id=\"hourColor\" value=\"#008000\" onchange=\"updateColors()\"></label>";
    html += "<label for=\"minuteColor\">Minute Color: <input type=\"color\" id=\"minuteColor\" value=\"#800080\" onchange=\"updateColors()\"></label>";
    html += "<label for=\"downlightColor\">Downlight Color: <input type=\"color\" id=\"downlightColor\" value=\"#FFFFFF\" onchange=\"updateColors()\"></label>";
    
    html += "<script>";
    html += "function updateBrightness() {";
    html += "  var clockBrightness = document.getElementById('clockBrightness').value;";
    html += "  var downlightBrightness = document.getElementById('downlightBrightness').value;";
    html += "  fetch('/updateBrightness?clock=' + clockBrightness + '&downlight=' + downlightBrightness);";
    html += "}";
    html += "function updateColors() {";
    html += "  var hourColor = document.getElementById('hourColor').value.substring(1);";
    html += "  var minuteColor = document.getElementById('minuteColor').value.substring(1);";
    html += "  var downlightColor = document.getElementById('downlightColor').value.substring(1);";
    html += "  fetch('/updateColors?hour=' + hourColor + '&minute=' + minuteColor + '&downlight=' + downlightColor);";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/syncTime", HTTP_GET, []() {
    timeClient.update();
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        DateTime MyTimestamp;
        MyTimestamp.Hour = hours;
        MyTimestamp.Minute = minutes;
        MyTimestamp.Second = 0;
        Clock.write(MyTimestamp);
        fillAnimation(CRGB::Red, 50);
  });

  server.on("/updateBrightness", HTTP_GET, []() {
    clockFaceBrightness = server.arg("clock").toInt();
    downlightBrightness = server.arg("downlight").toInt();
    stripClock.setBrightness(clockFaceBrightness);
    stripDownlighter.setBrightness(downlightBrightness);
    server.send(200, "text/plain", "Brightness updated");
    Serial.println("Brightness set to :");
    Serial.println(clockFaceBrightness);
    
  });

  server.on("/updateColors", HTTP_GET, []() {
    clockHourColour = strtol(server.arg("hour").c_str(), NULL, 16);
    clockMinuteColour = strtol(server.arg("minute").c_str(), NULL, 16);
    downlightColour = strtol(server.arg("downlight").c_str(), NULL, 16);
    server.send(200, "text/plain", "Colors updated");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void fillAnimation(CRGB color, uint8_t wait) {
  for(int i = 0; i < LEDCLOCK_COUNT; i++) {
    stripClock.setPixelColor(i, color);
    stripClock.show();
    delay(wait);
  }
}



