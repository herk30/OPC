#include <Arduino.h>
#include <iostream>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <Wire.h>
#include "qrcode.h"
#include <Adafruit_SSD1306.h>
#include <Ultrasonic.h>
#include <ESP32Servo.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif
#define BT_DISCOVER_TIME 10000

#define TFT_CS    2
#define TFT_RESET 4
#define TFT_AO    16  
#define TFT_SDA   17  
#define TFT_SCK   5   

#define DP_SCK    18  
#define DP_SDA    19  
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

#define Echo_PIN  12
#define Trig_PIN  13

#define SERVO_PIN 14

using namespace std;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RESET);
Ultrasonic ultrasonic(Trig_PIN, Echo_PIN);
BluetoothSerial SerialBT;
QRCode qrcode;
Servo myServo;

static bool btScanAsync = true;
static bool btScanSync = true;

int lastState = -1; 

void btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice) {
  Serial.printf("Found a device asynchronously: %s\n", pDevice->toString().c_str());
}

void drawQrToTFT(String text) {
  tft.fillScreen(ST7735_BLACK);
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, text.c_str()); 

  int scale = 4; 
  int startX = (128 - (qrcode.size * scale)) / 2;
  int startY = (128 - (qrcode.size * scale)) / 2; 

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ST7735_WHITE);
      }
    }
  }
}
void setup() 
{
  Serial.begin(115200); 

  SerialBT.begin("ESP32_Lele");  
  Serial.println("The device started, now you can pair it with bluetooth!");

  if (btScanAsync) {
    Serial.print("Starting asynchronous discovery... ");
    if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
      Serial.println("Findings will be reported in \"btAdvertisedDeviceFound\"");
      delay(10000);
      Serial.print("Stopping discoverAsync... ");
      SerialBT.discoverAsyncStop();
      Serial.println("stopped");
    } else {
      Serial.println("Error on discoverAsync f.e. not working after a \"connect\"");
    }
  }

  if (btScanSync) {
    Serial.println("Starting synchronous discovery... ");
    BTScanResults *pResults = SerialBT.discover(BT_DISCOVER_TIME);
    if (pResults) {
      pResults->dump(&Serial);
    } else {
      Serial.println("Error on BT Scan, no result!");
    }
  }

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);  
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
  tft.setTextSize(2); 
  
  myServo.setPeriodHertz(50);    
  myServo.attach(SERVO_PIN, 500, 2400); 
  myServo.write(0);

  Wire.begin(DP_SDA, DP_SCK); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    for(;;); 
  }
  
  display.clearDisplay();
  display.display();
}

void loop() {
  float distance = ultrasonic.read(); 
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (distance > 400 || distance <= 0) {
    display.println("---");
  } else {
    display.print((int)distance); 
    display.println(" cm");
  }
  display.display(); 

  int currentState = 0;
  if (distance <= 30 && distance > 0) {
    currentState = 1; 
  } else {
    currentState = 0; 
  }

  if (currentState != lastState) {
    tft.fillRect(0, 0, 128, 40, ST7735_BLACK); 

    tft.setCursor(0, 0);
    tft.setTextSize(2); 

    if (currentState == 1){
      myServo.write(0); 
      tft.fillScreen(ST7735_BLACK);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK); 
      tft.println("ITEM");
      tft.println("DETECTED!");
    }
    else{
      myServo.write(180);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK); 
      tft.println("ITEM NOT");
      tft.println("DETECTED!"); 
      drawQrToTFT("https://www.google.com");
    }
    lastState = currentState;
  }
  delay(200); 
}