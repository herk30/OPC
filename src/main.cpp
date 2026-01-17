#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <WiFiMulti.h> 
#include <qrcode.h>
#include <ArduinoOTA.h>

WiFiMulti wifiMulti;

const char* status_url = "https://render-deploy-django-2nl1.onrender.com/api/locker/1/status/"; 
const char* confirm_url = "https://render-deploy-django-2nl1.onrender.com/api/locker/1/confirm/"; 

const char* url_pickup = "https://vercel-deploy-front-end-delta.vercel.app"; 
const char* url_store  = "https://vercel-deploy-front-end-delta.vercel.app/post?locker=1"; 

#define TFT_CS    17
#define TFT_RESET 16
#define TFT_AO    4  
#define TFT_SDA   2  
#define TFT_SCK   15

#define DOOR_SENSOR_PIN 27
#define Relay_pin 32

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RESET);
QRCode qrcode;

unsigned long lastTime = 0;
unsigned long timerDelay = 1000; 
bool lastIsOccupied = false; 
bool isFirstRun = true;

void drawQrWithStatus(bool isOccupied) {
  tft.fillScreen(ST7735_BLACK);
  
  const char* targetUrl = isOccupied ? url_pickup : url_store;

  uint8_t qrcodeData[qrcode_getBufferSize(6)]; 
  qrcode_initText(&qrcode, qrcodeData, 6, 0, targetUrl); 
  
  int scale = 2; 
  
  int startY = 22; 
  int startX = (tft.width() - (qrcode.size * scale)) / 2;

  tft.fillRect(startX - 2, startY - 2, (qrcode.size * scale) + 4, (qrcode.size * scale) + 4, ST7735_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ST7735_BLACK);
      }
    }
  }

  tft.setCursor(0, 5); 
  tft.setTextSize(1);  
  
  if (isOccupied) {
    tft.setTextSize(2);
    tft.setCursor(15, 2); 
    tft.setTextColor(ST7735_RED); 
    tft.println("OCCUPIED"); 
  } else {
    tft.setTextSize(2);
    tft.setCursor(10, 2); 
    tft.setTextColor(ST7735_GREEN); 
    tft.println("AVAILABLE");
  }

  int textBottomY = startY + (qrcode.size * scale) + 8;
  
  tft.setCursor(2, textBottomY); 
  tft.setTextSize(1); 
  tft.setTextColor(ST7735_WHITE);
  
  if (isOccupied) {
    tft.println("Locker is full.");
    tft.setCursor(2, textBottomY + 10); 
    tft.println("Scan to PICK UP");
  } else {
    tft.println("Locker is empty.");
    tft.setCursor(2, textBottomY + 10); 
    tft.println("Scan to STORE");
  }
}

void confirmUnlockDone() {
  if (wifiMulti.run() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;
    http.begin(client, confirm_url);
    int code = http.POST(""); 
    Serial.print("Confirm sent! Code: "); Serial.println(code);
    http.end();
  }
}

void openDoor(bool currentStatus)
{
  Serial.println(">>> HANH DONG: MO CUA");
  tft.fillScreen(ST7735_GREEN);
  tft.setTextColor(ST7735_BLACK);

  if (currentStatus) {
    tft.setTextSize(2);
    tft.setCursor(22, 50); 
    tft.println("DEPOSIT"); 
    
    tft.setTextSize(1);
    tft.setCursor(34, 75); 
    tft.println("SUCCESSFUL");
  } else {
    tft.setTextSize(2);
    tft.setCursor(15, 60);
    tft.println("UNLOCKED");
  }

  digitalWrite(Relay_pin, HIGH); 
  delay(1000); 
  digitalWrite(Relay_pin, LOW);
  confirmUnlockDone();
  delay(2000); 
  drawQrWithStatus(currentStatus);
}

void setup() 
{
  Serial.begin(115200); 
  
  wifiMulti.addAP("Ti Li","tianhtiem2730");
  wifiMulti.addAP("ACLAB","ACLAB2023");
  wifiMulti.addAP("HCMUT01","khoi.lenguyen3010");
  wifiMulti.addAP("TriaCafe","Triacafe");

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);  
  
  tft.setCursor(0,0);
  tft.setTextColor(ST7735_WHITE);
  tft.println("Connecting Wifi...");

  Serial.println("Scanning and Connecting...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected.");
  Serial.print("SSID: "); Serial.println(WiFi.SSID()); 
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  pinMode(Relay_pin, OUTPUT);
  digitalWrite(Relay_pin, LOW); 
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  
  drawQrWithStatus(false);
}

void loop() 
{
  if ((millis() - lastTime) > timerDelay)
  {
    if (wifiMulti.run() == WL_CONNECTED){
      
      WiFiClientSecure client;
      client.setInsecure(); 
      HTTPClient http;
      http.begin(client, status_url); 
      
      int httpRespCode = http.GET();
      if (httpRespCode > 0) {
        String payload = http.getString();
        
        bool unlockCommand = (payload.indexOf("\"unlock\":true") >= 0);
        bool isOccupied = (payload.indexOf("\"is_occupied\":true") >= 0);

        if (unlockCommand) {
             openDoor(isOccupied);
             lastIsOccupied = isOccupied; 
             isFirstRun = false;
        }
        else if (isOccupied != lastIsOccupied || isFirstRun) {
             Serial.println("Update QR Status");
             drawQrWithStatus(isOccupied);
             lastIsOccupied = isOccupied;
             isFirstRun = false;
        }
      }
      http.end();
    }
    lastTime = millis();
  }
}