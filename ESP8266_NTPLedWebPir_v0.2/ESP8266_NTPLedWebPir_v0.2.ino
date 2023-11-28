//Please install these libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ArduinoOTA.h>

const char* ssid = "AP1_AdministratorOnly"; //your WiFi name
const char* password = "Depressed0822"; //your WiFi password
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

const char* ntpServer = "asia.pool.ntp.org"; //your NTP Server
const long timeZoneOffset = 25200; //To define GMT+7 timezone
//to define your time offset: timezone*3600

WiFiClient wifiClient;
WiFiUDP udpClient;

NTPClient timeClient(udpClient, ntpServer, timeZoneOffset);

bool autoSwitch = true;
bool manualSwitch = false;

const int lampPin = 14;
const int wifiLight = 2;
const int pirPin1 = 5;
const int pirPin2 = 4;
const int pirPinInd1 = 12;
const int pirPinInd2 = 13;
uint8_t retries = 0;

AsyncWebServer server(80);

#define RAW_LIT(x) (x)

void initWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting...");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
void onWiFiConnect(const WiFiEventStationModeGotIP& event){
  Serial.println("Connected to WiFi.");
  Serial.println(WiFi.localIP());
  digitalWrite(wifiLight, HIGH);
}
void onWiFiDisconnect(const WiFiEventStationModeDisconnected& event){
  Serial.println("WiFi Disconnect");
  WiFi.disconnect();
  WiFi.begin(ssid,password);
  digitalWrite(wifiLight, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(lampPin, OUTPUT);
  pinMode(pirPin1, INPUT);
  pinMode(pirPin2, INPUT);
  pinMode(pirPinInd1, OUTPUT);
  pinMode(pirPinInd2, OUTPUT);
  pinMode(wifiLight, OUTPUT);
////////////////////// WIFI CONFIGURATION //////////////////////
  wifiConnectHandler = WiFi.onStationModeGotIP(onWiFiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWiFiDisconnect);

  initWiFi();
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
 
////////////////////////////////////////////////////////////////
////////////////////// OTA CONFIGURATION //////////////////////
ArduinoOTA.onStart([](){
    Serial.println("Start...");
  });
  ArduinoOTA.onEnd([](){
    Serial.println("\nEnd.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error){
    Serial.printf("Error[%u]: ", error);
    if(error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if(error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if(error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if(error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if(error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.println(WiFi.localIP());
//////////////////////////////////////////////////////////////////
////////////////////// NTP CONFIGURATION //////////////////////
  timeClient.begin();
  timeClient.setTimeOffset(timeZoneOffset);
  timeClient.update();
//////////////////////////////////////////////////////////////////
////////////////////// WEB SERVER CONFIGURATION //////////////////
  //start the web server
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
   String html = RAW_LIT(R"html(
    <html lang="en-US">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>HDLabs_Light1</title>

        <style type="text/css">
          body{
            font-family: arial;
          }
          small{
            color: red;
          }
        </style>

        <script>
          function sendRequest(url) {
            var xhttp = new XMLHttpRequest();
            xhttp.open("GET", url, true);
            xhttp.send();
            xhttp.onreadystatechange = function() {
              if (xhttp.readyState == 4 && xhttp.status == 200) {
                document.getElementById("status").innerHTML = xhttp.responseText;
              }
            };
          }
        </script>
      </head>
      <body>
        <h2>Switch</h2>
        <span><p>Switch status: </p><p id="status"></p></span>
        <p>Mode: %a</p>
        <a href="/auto"><button>Auto</button></a>
        <a href="/on"><button>[Manual] Turn On</button></a>
        <a href="/off"><button>[Manual] Turn Off</button></a>
        <br>
        <small>HDLabs_[Warning] This project is under development</small>
      </body>
    </html>
  )html");

  request->send(200, "text/html", html);
});
server.on("/auto", HTTP_GET, [](AsyncWebServerRequest *request){
  autoSwitch = true;
  request->send(200, "text/plain", "Currently running in automatic mode");
});
server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
  autoSwitch = false;
  manualSwitch = true;
  request->send(200, "text/plain", "[Manual] Switch on");
});
server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
  autoSwitch = false;
  manualSwitch = false;
  request->send(200, "text/plain", "[Manual] Switch off");
});

  server.begin();
  Serial.println("Web Server strated.");
}

void loop() {
  ArduinoOTA.handle();
  timeClient.update();
  // server.handleClient();

  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  int pirState1 = digitalRead(pirPin1);
  int pirState2 = digitalRead(pirPin2);

  if (autoSwitch == true) {
    if ((hours >= 18 && hours < 23) && (pirState1 == HIGH || pirState2 == HIGH)) {
      Serial.println(" [Auto]Lamp On | 6p.m - 11p.m");
      digitalWrite(lampPin, LOW);
    } else {
      Serial.println(" [Auto]Lamp Off");
      digitalWrite(lampPin, HIGH);
    }
  } else if (autoSwitch == false) {
    if (manualSwitch == true) {
      Serial.println(" [Manual]Lamp On");
      digitalWrite(lampPin, LOW);
    } else if (manualSwitch == false) {
      Serial.println(" [Manual]Lamp Off");
      digitalWrite(lampPin, HIGH);
    }
  }

  if (hours >= 18 || hours < 23) {
    digitalWrite(wifiLight, LOW);
  } else {
    digitalWrite(wifiLight, HIGH);
  }

  if (pirState1 == HIGH) {
    digitalWrite(pirPinInd1, HIGH);
  } else {
    digitalWrite(pirPinInd1, LOW);
  }

  if (pirState2 == HIGH) {
    digitalWrite(pirPinInd2, HIGH);
  } else {
    digitalWrite(pirPinInd2, LOW);
  }

  Serial.println("PIR1: ");
  Serial.print(pirState1);
  Serial.println("PIR2: ");
  Serial.print(pirState2);
  Serial.println();
  Serial.println("Time: ");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print(seconds);

  delay(1000);
}
