#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   8
#define TFT_DC   9
#define TFT_RST  10
#define TFT_MOSI 35
#define TFT_SCLK 36

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Buttons
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_OK    4
#define BTN_BACK  5

//wifi
#include <RF24.h>
#define WIFI_CSN   48 //attivo se low 
#define WIFI_CE    4
RF24 nrf24(WIFI_CE, WIFI_CSN);
const byte address[6] = "00001";

// ---------------- MENU STRUCT ----------------
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
  {"Wifi Jammer",  8, 0},

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

// ---------------- NAV ----------------
int parentStack[10];
int selectedStack[10];
int level = 0;
int currentParent = -1;
int selected = 0;

int visibleItems[20];
int visibleCount = 0;

// ---------------- DRAW MENU ----------------
void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);

  visibleCount = 0;
  int y = 20;

  for (int i = 0; i < menuSize; i++) {

    if (menu[i].parent == currentParent) {

      visibleItems[visibleCount] = i;
      visibleCount++;

      if (visibleCount - 1 == selected) {
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

// ---------------- SETUP ----------------
void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  SPI.begin(36, 37, 35);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  drawMenu();
}

// ---------------- LOOP ----------------
void loop() {

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
    currentParent = index;   // 👈 FIX IMPORTANTE
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