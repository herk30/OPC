#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include "qrcode.h"

const char* ssid = "Tria VP Tang 1";      
const char* password = "Triacafe";     

const char* status_url = "https://render-deploy-django-2nl1.onrender.com/api/locker/1/status/"; 
const char* confirm_url = "https://render-deploy-django-2nl1.onrender.com/api/locker/1/confirm/"; 
const char* webUrl = "https://vercel-deploy-front-end-delta.vercel.app"; 

#define TFT_CS    2
#define TFT_RESET 4
#define TFT_AO    16  
#define TFT_SDA   17  
#define TFT_SCK   5

#define DOOR_SENSOR_PIN 27
#define Relay_pin 22

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RESET);
QRCode qrcode;

unsigned long lastTime = 0;
unsigned long timerDelay = 1000; 
bool lastIsOccupied = false; 
bool isFirstRun = true;

void drawQrWithStatus(String url, bool isOccupied) {
  tft.fillScreen(ST7735_BLACK);
  
  uint8_t qrcodeData[qrcode_getBufferSize(6)]; 
  qrcode_initText(&qrcode, qrcodeData, 6, 0, url.c_str()); 
  
  int scale = 3; 
  int startX = (128 - (qrcode.size * scale)) / 2;
  int startY = 5; 

  tft.fillRect(startX - 2, startY - 2, (qrcode.size * scale) + 4, (qrcode.size * scale) + 4, ST7735_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ST7735_BLACK);
      } else {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ST7735_WHITE);
      }
    }
  }

  tft.setCursor(0, 135); 
  tft.setTextSize(1);
  
  if (isOccupied) {
    tft.setTextColor(ST7735_RED); 
    tft.println("   [ CO DO ]");
    tft.setTextColor(ST7735_WHITE);
    tft.println(" Quet de LAY do");
  } else {
    tft.setTextColor(ST7735_GREEN); 
    tft.println("   [ TU TRONG ]");
    tft.setTextColor(ST7735_WHITE);
    tft.println(" Quet de GUI do");
  }
}

void confirmUnlockDone() {
  if (WiFi.status() == WL_CONNECTED) {
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
  tft.setCursor(15, 60);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(2);
  tft.println("MO KHOA!");
  digitalWrite(Relay_pin, HIGH); 
  delay(1000); 
  digitalWrite(Relay_pin, LOW);
  confirmUnlockDone();
  delay(2000); 
  drawQrWithStatus(webUrl, currentStatus);
}

void setup() 
{
  Serial.begin(115200); 
  
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);  
  
  tft.setCursor(0,0);
  tft.setTextColor(ST7735_WHITE);
  tft.println("Connecting Wifi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");
  pinMode(Relay_pin, OUTPUT);
  digitalWrite(Relay_pin, LOW); 
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  drawQrWithStatus(webUrl, false);
}

void loop() 
{
  if ((millis() - lastTime) > timerDelay)
  {
    if (WiFi.status() == WL_CONNECTED){
      
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
             drawQrWithStatus(webUrl, isOccupied);
             lastIsOccupied = isOccupied;
             isFirstRun = false;
        }
      }
      http.end();
    }
    lastTime = millis();
  }
}