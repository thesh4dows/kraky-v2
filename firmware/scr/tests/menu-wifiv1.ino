#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>

// ---------------- TFT ----------------
#define TFT_CS   8
#define TFT_DC   9
#define TFT_RST  10

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTONS ----------------
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_OK    4
#define BTN_BACK  5

// ---------------- MENU ----------------
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

// ---------------- NAV ----------------
int parentStack[10];
int selectedStack[10];
int level = 0;

int currentParent = -1;
int selected = 0;

int visibleItems[30];
int visibleCount = 0;

// ---------------- WIFI STATE ----------------
bool wifiRunning = false;
bool wifiMonitorMode = false;
unsigned long wifiTimer = 0;

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

// ---------------- NEXT ----------------
int findNext(int start, int dir) {
  int i = start;

  i += dir;
  if (i < 0) i = visibleCount - 1;
  if (i >= visibleCount) i = 0;

  return i;
}

// ---------------- WIFI SCAN (ONESHOT) ----------------
void startWifiScan() {
  wifiRunning = true;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  int n = WiFi.scanNetworks();

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.print("WiFi Scan");

  for (int i = 0; i < n && i < 6; i++) {
    tft.setCursor(10, 30 + i * 15);
    tft.print(WiFi.SSID(i));
  }

  wifiTimer = millis();
}

// ---------------- WIFI MONITOR (NON BLOCCANTE) ----------------
void startWifiMonitor() {
  wifiRunning = true;
  wifiMonitorMode = true;
  wifiTimer = millis();
}

// ---------------- UPDATE WIFI ----------------
void updateWifi() {

  // BACK sempre attivo
  if (digitalRead(BTN_BACK) == LOW) {
    wifiRunning = false;
    wifiMonitorMode = false;
    drawMenu();
    delay(200);
    return;
  }

  // AUTO EXIT dopo 20 secondi
  if (millis() - wifiTimer > 20000) {
    wifiRunning = false;
    wifiMonitorMode = false;
    drawMenu();
    return;
  }

  if (!wifiMonitorMode) return;

  // MONITOR UPDATE
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int n = WiFi.scanNetworks();

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.print("WiFi Monitor");

  for (int i = 0; i < n && i < 5; i++) {
    tft.setCursor(10, 30 + i * 15);
    tft.print(WiFi.SSID(i));
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

  // ---------------- WIFI MODE ----------------
  if (wifiRunning) {
    updateWifi();
    delay(200);
    return;
  }

  // ---------------- MENU NAV ----------------

  if (digitalRead(BTN_UP) == LOW) {
    selected = findNext(selected, -1);
    drawMenu();
    delay(150);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selected = findNext(selected, +1);
    drawMenu();
    delay(150);
  }

  if (digitalRead(BTN_OK) == LOW) {

    int index = visibleItems[selected];
    int id = menu[index].id;

    // WIFI ACTIONS
    if (id == 7) {
      startWifiScan();
      return;
    }

    if (id == 8) {
      startWifiMonitor();
      return;
    }

    // ENTER SUBMENU
    parentStack[level] = currentParent;
    selectedStack[level] = selected;
    level++;

    currentParent = index;
    selected = 0;

    drawMenu();
    delay(200);
  }

  if (digitalRead(BTN_BACK) == LOW) {

    if (level > 0) {
      level--;
      currentParent = parentStack[level];
      selected = selectedStack[level];
    } else {
      currentParent = -1;
      selected = 0;
    }

    drawMenu();
    delay(200);
  }
}