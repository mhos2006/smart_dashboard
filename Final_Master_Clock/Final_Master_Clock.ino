#include <WiFi.h>
#include <FS.h>          
#include <WebServer.h>   
#include <HTTPClient.h>
#include "time.h"
#include <SPI.h>
#include <TFT_eSPI.h>   
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI(); 
WebServer server(80);

const char* ssid     = "VM8820505";
const char* password = "Ly7ybbrhrHwf";
String apiKey = "2322ce436bbb0116f77457d5e3dcab2c"; 
String city = "Manchester,UK";

const char* tz = "GMT0BST,M3.5.0/1,M10.5.0";
const char* ntpServer = "pool.ntp.org";

// Expanded Telemetry Variables 
unsigned long lastWeatherTime = 0;
const unsigned long weatherInterval = 900000; 
String currentTemp = "--.- C";
String weatherState = "WAITING";
String humidityStr = "--%";
String windStr = "--.- mph";
String sunriseStr = "--:--";
String sunsetStr = "--:--";
int last_sec = -1;

// Dual-Mode Timer Variables 
enum DisplayMode { MODE_CLOCK, MODE_STOPWATCH, MODE_TIMER };
DisplayMode currentMode = MODE_CLOCK;
bool timerRunning = false;
int timeValue = 0; // Tracks elapsed seconds (stopwatch) or remaining seconds (timer)

// The HTML Dashboard UI 
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Project 28</title>
  <style>
    body { font-family: Arial; text-align: center; background-color: #121212; color: white; margin-top: 20px; }
    .box { background-color: #1e1e1e; margin: 10px auto; padding: 20px; border-radius: 10px; width: 90%; max-width: 400px; }
    button { padding: 15px 20px; font-size: 18px; margin: 5px; border: none; border-radius: 8px; cursor: pointer; color: white; width: 100%; }
    .start { background-color: #4CAF50; } .pause { background-color: #FF9800; } .reset { background-color: #F44336; } .clock { background-color: #2196F3; }
    input { padding: 10px; font-size: 18px; width: 80%; margin-bottom: 10px; text-align: center; }
  </style>
</head>
<body>
  <h1>Project 28 Hub </h1>
  
  <div class="box">
    <h3> Stopwatch Mode</h3>
    <button class="start" onclick="fetch('/sw/start')">START SW</button>
    <button class="pause" onclick="fetch('/sw/pause')">PAUSE SW</button>
    <button class="reset" onclick="fetch('/sw/reset')">RESET SW</button>
  </div>

  <div class="box">
    <h3> Countdown Timer</h3>
    <input type="number" id="mins" placeholder="Enter Minutes" value="25">
    <button class="start" onclick="startTimer()">START TIMER</button>
    <button class="pause" onclick="fetch('/tm/pause')">PAUSE TIMER</button>
    <button class="reset" onclick="fetch('/tm/reset')">RESET TIMER</button>
  </div>
  
  <div class="box">
    <button class="clock" onclick="fetch('/clock')"> RETURN TO CLOCK</button>
  </div>

  <script>
    function startTimer() {
      let m = document.getElementById("mins").value;
      fetch('/tm/start?m=' + m);
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Booting Network... ");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  

  Serial.println("");
  Serial.print("Connected to WiFi! IP Address: ");
  Serial.println(WiFi.localIP());
  
  configTzTime(tz, ntpServer);
  
// Set up the Web Server Endpoints 
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  
  // Stopwatch Endpoints
  server.on("/sw/start", []() { 
    tft.fillRect(0, 70, 320, 60, TFT_BLACK); 
    currentMode = MODE_STOPWATCH; 
    timerRunning = true; 
    server.send(200); 
  });
  server.on("/sw/pause", []() { timerRunning = false; server.send(200); });
  server.on("/sw/reset", []() { timerRunning = false; timeValue = 0; server.send(200); });
  
  // Timer Endpoints (Also with anti-ghosting! )
  server.on("/tm/start", []() { 
    if (server.hasArg("m")) { 
      tft.fillRect(0, 70, 320, 60, TFT_BLACK); 
      timeValue = server.arg("m").toInt() * 60; 
      currentMode = MODE_TIMER; 
      timerRunning = true; 
    } 
    server.send(200); 
  });
  server.on("/tm/pause", []() { timerRunning = false; server.send(200); });
  server.on("/tm/reset", []() { timerRunning = false; timeValue = 0; server.send(200); });
  
  // Return to Clock Endpoint
  server.on("/clock", []() { 
    tft.fillRect(0, 70, 320, 60, TFT_BLACK); 
    currentMode = MODE_CLOCK; 
    timerRunning = false; 
    server.send(200); 
  });
  
  server.begin();
  
  tft.fillScreen(TFT_BLACK);
  tft.drawLine(10, 155, 310, 155, TFT_CYAN); 
  fetchWeather(); 
}

void loop() {
  server.handleClient(); // Listen for phone taps 

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return; 

  int s = timeinfo.tm_sec;
  int m = timeinfo.tm_min;
  int h = timeinfo.tm_hour;

  // Screen update loop 
  if (s != last_sec) {
    last_sec = s;
    
    // 1. DATE STRING 
    char dateStringBuff[50];
    strftime(dateStringBuff, sizeof(dateStringBuff), "%A, %d %B %Y", &timeinfo);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 20);
    tft.print(dateStringBuff);

    
    tft.setTextSize(5);
    
    if (currentMode == MODE_CLOCK) {
      // Normal Atomic Time 
      tft.setTextColor(TFT_YELLOW, TFT_BLACK); 
      char timeStringBuff[50];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
      tft.setCursor(35, 80); 
      tft.print(timeStringBuff);
    } 
    else {
      // Stopwatch / Timer Display 
      if (timerRunning) {
        if (currentMode == MODE_STOPWATCH) timeValue++;
        else if (currentMode == MODE_TIMER) {
          if (timeValue > 0) timeValue--;
          else timerRunning = false; 
        }
      }
      
      int d_min = timeValue / 60;
      int d_sec = timeValue % 60;
      
      // Clear the previous text
      tft.setCursor(35, 80); 
      tft.setTextColor(currentMode == MODE_STOPWATCH ? TFT_CYAN : TFT_RED, TFT_BLACK);
      tft.printf(" %02d:%02d ", d_min, d_sec);
    }

    //  Extra Info Section
    tft.setTextSize(2);
    tft.setCursor(10, 165);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.print("LOC: Manchester, UK      ");
    
    tft.setCursor(10, 185);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.printf("%s | %s | %s      ", currentTemp.c_str(), weatherState.c_str(), humidityStr.c_str()); 
    
    tft.setCursor(10, 205);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.printf("Wind: %s      ", windStr.c_str());
    
    tft.setCursor(10, 225);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.printf("Up: %s | Dn: %s      ", sunriseStr.c_str(), sunsetStr.c_str());
  }

  // Refresh weather every 15 minutes 
  if (millis() - lastWeatherTime >= weatherInterval) {
    fetchWeather();
    lastWeatherTime = millis();
  }
}

void fetchWeather() {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=metric&appid=" + apiKey;
    http.begin(url);
    if (http.GET() > 0) {
      JsonDocument doc; 
      if (!deserializeJson(doc, http.getString())) {
        currentTemp = String((float)doc["main"]["temp"], 1) + " C";
        weatherState = String((const char*)doc["weather"][0]["main"]);
        humidityStr = String((int)doc["main"]["humidity"]) + "%";
        windStr = String((float)doc["wind"]["speed"] * 2.23694, 1) + " mph";
        
        time_t riseTs = doc["sys"]["sunrise"], setTs = doc["sys"]["sunset"];
        struct tm *ti; char buf[10];
        ti = localtime(&riseTs); strftime(buf, sizeof(buf), "%H:%M", ti); sunriseStr = String(buf);
        ti = localtime(&setTs); strftime(buf, sizeof(buf), "%H:%M", ti); sunsetStr = String(buf);
      }
    }
    http.end();
  }
}