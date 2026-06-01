/*
 * KRAKY2
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>

// ================= PIN =================

// Pulsanti
#define BTN_UP     41
#define BTN_DOWN   38
#define BTN_OK     40
#define BTN_BACK   39


// SPI (VSPI)
#define SPI_SCK    21
#define SPI_MISO   17
#define SPI_MOSI   18

// PN532 E
#include <Adafruit_PN532.h>
#define NFC_SS     1
#define NFC_ANTSEL 5
Adafruit_PN532 nfc(NFC_SS);

// LoRa D
#include <RadioLib.h>
#define LORA_CSN   3 //attivo se low
#define LORA_DIO0  2
CC1101 cc1101 = new Module(LORA_CSN, LORA_DIO0, RADIOLIB_NC, RADIOLIB_NC);

//IR
#include <IRremote.hpp>
#define IR_LED     42
#define IR_REC     45

//DISPLAY A ST7789
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#define DS_CS      46
#define DS_DC      47
Adafruit_ST7789 tft = Adafruit_ST7789(DS_CS, DS_DC, -1);

//WIFI B
#include <RF24.h>
#define WIFI_CSN   48 //attivo se low 
#define WIFI_CE    4
#include <WiFi.h>
RF24 nrf24(WIFI_CE, WIFI_CSN);
const byte address[6] = "00001";

//RFID F
#include <MFRC522.h>
#define RFID_CSN   6
#define RFID_ANTSEL 7
MFRC522 rfid(RFID_CSN, -1);

//MICRO SD SLOT C
#include <SD.h>
#define MSD_CS     8
//Battery 
#define BAT_PIN 9
//BLUETOOTH
#include <NimBLEDevice.h>
// ================= OGGETTI =================



// ================= MENU =================

struct Item {
  const char* name;
  int id;
  int parent;
};

Item menu[] = {
  {"WiFi",     0, -1},
  {"NFC",      1, -1},
  {"RFID",     2, -1},
  {"Sub-GHz", 3, -1},
  {"Bluetooth", 4, -1},
  {"IR", 5,-1},
  {"Settings", 6, -1},
  // WiFi
  {"Wifi Scan",     7, 0},
  {"Wifi Monitor",  8, 0},

  // NFC
  {"NFC Read",     9, 1},
  {"NFC Write",    10, 1},

  // RFID
  {"RFID Dump",     11, 2},
  {"RFID Clone",    12, 2},

  // subghz
  {"Sub-GHz Scan", 13, 3},
  {"Sub-GHz Jammer",  14, 3},

  //bluetooth
  {"BLE Scan", 15, 4},
  {"BLE Jammer", 16, 4},

  //IR
  {"IR Scan", 17, 5},
  {"IR Write", 18, 5},

  //settings
  {"Display", 19, 6},
  {"System",  20, 6},
};

int menuSize = sizeof(menu) / sizeof(menu[0]);

// ---------------- NAV STACK ----------------
int parentStack[10];
int selectedStack[10];
int level = 0;
int currentParent = -1;
int selected = 0;
int visibleItems[20];
int visibleCount = 0;
// ---------------- DRAW ----------------
void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  visibleCount = 0;
  int y = 20;

  for (int i = 0; i < menuSize; i++) {

    if (menu[i].parent == currentParent) {
      visibleItems[visibleCount] = i;
      visibleCount++;
      if (i == selected) {
        tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      } else {
        tft.setTextColor(ST77XX_WHITE);
      }

      tft.setCursor(10, y);
      tft.print(menu[i].name);
      y += 15;
    }
  }
}

// ---------------- FIND NEXT ----------------
int findNext(int start, int dir) {
  int i = start;

  while (true) {
    i += dir;
    if (i < 0) i = visibleCount - 1;
    if (i >= visibleCount) i = 0;

    return i;
  }
}


// ================= SETUP =================

void setup(){
  SPI.begin();
  //display
  tft.init(172, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  //LoRa
  int state = cc1101.begin(868.0); // o 915.0
  //IR
  IrSender.begin(IR_LED); 
  IrReceiver.begin(IR_REC, ENABLE_LED_FEEDBACK);
  //WIFI
    //modalità trasmissione
  nrf24.begin();
  nrf24.openWritingPipe(address);
  nrf24.setPALevel(RF24_PA_LOW);
  nrf24.stopListening();
    //modalità ricezione
  nrf24.begin();
  nrf24.openReadingPipe(0, address);
  nrf24.setPALevel(RF24_PA_LOW);
  nrf24.startListening();
  //RFID
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  //NFC
  nfc.begin();
  nfc.SAMConfig(); //modalità ascolto
  //Menù
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  drawMenu();
}

// ================= LOOP =================

void loop() {
  //Battery
    int raw = analogRead(BAT_PIN);
    float voltage = ((float)raw / 4095.0) * 3.3 * 2.0;
  //Menù
    // UP
    if (digitalRead(BTN_UP) == LOW) {
      selected = findNext(selected, -1);
      drawMenu();
      delay(150);
    }

    // DOWN
    if (digitalRead(BTN_DOWN) == LOW) {
      selected = findNext(selected, +1);
      drawMenu();
      delay(150);
    }

    // OK → entra nel submenu corretto
    if (digitalRead(BTN_OK) == LOW) {

      parentStack[level] = currentParent;
      selectedStack[level] = selected;
      level++;

      int index = visibleItems[selected];
      currentParent = index;   
      selected = 0;

      drawMenu();
      delay(200);
    }

    // BACK → torna di 1 livello
    if (digitalRead(BTN_BACK) == LOW) {

      if (level > 0) {
        level--;
        if (level > 0) {
        level--;
        currentParent = parentStack[level];
        selected = selectedStack[level];
      } else {
        currentParent = -1;
        selected = 0;
      }
        selected = selectedStack[level];
      }

      drawMenu();
      delay(200);
    }
}