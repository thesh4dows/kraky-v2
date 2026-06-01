#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <RadioLib.h>

// ---------------- DISPLAY ----------------
#define TFT_CS   8
#define TFT_DC   9
#define TFT_RST  10

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTONS ----------------
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_OK    4
#define BTN_BACK  5

// ---------------- CC1101 ----------------
#define CC1101_CS   6
#define CC1101_GDO0 7

Module mod(CC1101_CS, CC1101_GDO0, RADIOLIB_NC, RADIOLIB_NC);
CC1101 radio(&mod);

// ---------------- STATE ----------------
bool inSubGHz = false;
int  mode     = 0;   // 0=scan, 1=rx, 2=tx
float freq    = 433.0;

// ---------------- MENU ----------------
// Ogni voce: { etichetta, id univoco, id del parent (-1 = root) }
struct Item {
  const char* name;
  int id;
  int parent;
};

Item menu[] = {
  // Root
  { "Sub-GHz", 10, -1 },
  { "Wifi",    11, -1 },
  { "NFC",     12, -1 },
  { "RFID",    13, -1 },

  // Figli di Sub-GHz (parent = 10)
  { "Scan", 20, 10 },
  { "RX",   21, 10 },
  { "TX",   22, 10 },
};

const int menuSize = sizeof(menu) / sizeof(menu[0]);

// ---------------- NAVIGAZIONE ----------------
int currentParentId = -1;   // id del nodo corrente (-1 = root)
int selected        = 0;

int visible[20];
int vCount = 0;

// Stack per BACK
int stackParentId[10];
int stackSelect[10];
int level = 0;

// ---------------- DRAW MENU ----------------
void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);

  vCount = 0;
  int y  = 20;

  for (int i = 0; i < menuSize; i++) {
    if (menu[i].parent == currentParentId) {
      visible[vCount] = i;

      if (vCount == selected) {
        tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      } else {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      }

      tft.setCursor(10, y);
      tft.print(menu[i].name);

      y += 15;
      vCount++;
    }
  }
}

// ---------------- NAVIGAZIONE LISTA ----------------
int nextItem(int s, int d) {
  s += d;
  if (s < 0)       s = vCount - 1;
  if (s >= vCount) s = 0;
  return s;
}

// ---------------- SUBGHZ: avvio modalità ----------------
void startScan() {
  inSubGHz = true;
  mode     = 0;
  freq     = 433.0;

  // La CC1101 deve essere in RX continuo per leggere l'RSSI
  radio.standby();
  radio.startReceive();
}

void startRX() {
  inSubGHz = true;
  mode     = 1;

  radio.standby();
  radio.startReceive();
}

void startTX() {
  inSubGHz = true;
  mode     = 2;

  radio.standby();
}

// ---------------- SUBGHZ LOOP ----------------
void updateSubGHz() {

  // BACK — torna al menu
  if (digitalRead(BTN_BACK) == LOW) {
    inSubGHz = false;
    radio.standby();
    delay(200);
    drawMenu();
    return;
  }

  // ---- SCAN ----
  if (mode == 0) {
    radio.setFrequency(freq);

    // Piccolo delay per stabilizzare il sintetizzatore
    delay(10);
    int rssi = radio.getRSSI();

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

    tft.setCursor(10, 10);
    tft.print("[ SCAN ]");

    tft.setCursor(10, 30);
    tft.print(freq, 2);
    tft.print(" MHz");

    tft.setCursor(10, 50);
    tft.print("RSSI: ");
    tft.print(rssi);
    tft.print(" dBm");

    freq += 0.2;
    if (freq > 435.0) freq = 433.0;

    delay(100);
  }

  // ---- RX ----
  if (mode == 1) {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

    if (radio.available()) {
      String str;
      int state = radio.readData(str);

      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(10, 10);
      tft.print("[ RX ]");

      tft.setCursor(10, 30);
      if (state == RADIOLIB_ERR_NONE) {
        tft.print(str);
      } else {
        tft.print("ERR ");
        tft.print(state);
      }

      // Riavvia la ricezione dopo aver letto il pacchetto
      radio.startReceive();
    }

    delay(50);
  }

  // ---- TX ----
  if (mode == 2) {
    const char* msg = "HELLO";

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.print("[ TX ]");

    tft.setCursor(10, 30);
    tft.print("Sending...");

    int state = radio.transmit(msg);

    tft.setCursor(10, 50);
    if (state == RADIOLIB_ERR_NONE) {
      tft.print("OK");
    } else {
      tft.print("ERR ");
      tft.print(state);
    }

    // Attende prima del prossimo TX
    delay(1500);
  }
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK,   INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  SPI.begin(35, 37, 36);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // Inizializza CC1101
  int state = radio.begin(433.92);
  if (state != RADIOLIB_ERR_NONE) {
    tft.setCursor(10, 10);
    tft.print("CC1101 ERR:");
    tft.setCursor(10, 30);
    tft.print(state);
    while (true);
  }

  drawMenu();
}

// ---------------- LOOP ----------------
void loop() {

  // Se siamo in modalità Sub-GHz, delega tutto a updateSubGHz
  if (inSubGHz) {
    updateSubGHz();
    return;
  }

  // ---- Navigazione menu ----
  if (digitalRead(BTN_UP) == LOW) {
    selected = nextItem(selected, -1);
    drawMenu();
    delay(150);
    return;
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selected = nextItem(selected, +1);
    drawMenu();
    delay(150);
    return;
  }

  if (digitalRead(BTN_OK) == LOW) {
    int idx = visible[selected];   // indice nell'array menu[]
    int id  = menu[idx].id;        // id univoco della voce

    // Azioni foglia Sub-GHz
    if (id == 20) { startScan(); delay(200); return; }
    if (id == 21) { startRX();   delay(200); return; }
    if (id == 22) { startTX();   delay(200); return; }

    // Verifica se esistono figli (è un nodo navigabile)
    bool hasChildren = false;
    for (int i = 0; i < menuSize; i++) {
      if (menu[i].parent == id) { hasChildren = true; break; }
    }

    if (hasChildren) {
      // Entra nel sottomenu
      stackParentId[level] = currentParentId;
      stackSelect[level]   = selected;
      level++;

      currentParentId = id;
      selected        = 0;
      drawMenu();
    }

    delay(200);
    return;
  }

  if (digitalRead(BTN_BACK) == LOW) {
    if (level > 0) {
      level--;
      currentParentId = stackParentId[level];
      selected        = stackSelect[level];
    } else {
      // Già al root, niente da fare
      currentParentId = -1;
      selected        = 0;
    }

    drawMenu();
    delay(200);
  }
}
