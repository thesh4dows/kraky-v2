#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// ---------------- DISPLAY TFT ----------------
#define TFT_CS   8
#define TFT_DC   9
#define TFT_RST  10

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ---------------- BUTTONS ----------------
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_OK    4
#define BTN_BACK  5

// ---------------- PN532 NFC (SPI) ----------------
// Condivide SCK e MOSI con il display
// Pin separati per MISO, SS e IRQ
#define PN532_SS    6
#define PN532_IRQ   7

Adafruit_PN532 nfc(PN532_SS);

// Buffer per dati NFC
uint8_t nfcUid[7];
uint8_t uidLength;
bool nfcInitialized = false;

// ---------------- MENU STRUCT ----------------
struct Item {
  const char* name;
  int id;
  int parent;
};

Item menu[] = {
  {"NFC",        0, -1},
  {"Settings",   1, -1},
  // NFC
  {"NFC Read",     2, 0},
  {"NFC Write",    3, 0},
  {"NFC Info",     4, 0},
  // Settings
  {"Display",      5, 1},
  {"System",       6, 1},
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

// ---------------- NFC STATE ----------------
bool nfcModeActive = false;
int nfcMode = 0; // 0=Read, 1=Write, 2=Info
String dataToWrite = "Ciao NFC!";

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

// ---------------- EXECUTE ACTION ----------------
void executeAction(int id) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 30);
  tft.print("Avvio: ");
  tft.println(menu[id].name);
  delay(500);
  
  switch(id) {
    case 2: // NFC Read
      nfcModeActive = true;
      nfcMode = 0;
      nfcReadTag();
      break;
    case 3: // NFC Write
      nfcModeActive = true;
      nfcMode = 1;
      nfcWriteTag();
      break;
    case 4: // NFC Info
      nfcModeActive = true;
      nfcMode = 2;
      nfcShowInfo();
      break;
    case 5: // Display
    case 6: // System
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(10, 50);
      tft.println("Funzione in");
      tft.println("sviluppo...");
      delay(1500);
      break;
    default:
      break;
  }
  
  if (!nfcModeActive) {
    drawMenu();
  }
}

// ---------------- NFC FUNCTIONS ----------------
void nfcReadTag() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC READ MODE");
  tft.setCursor(5, 25);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 120);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    // Controlla pulsante BACK per uscire
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(200);
      break;
    }
    
    // Cerca tag NFC
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 100);
    
    if (success) {
      tft.fillRect(0, 40, 160, 80, ST77XX_BLACK);
      tft.setCursor(5, 45);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      // Mostra UID
      tft.setCursor(5, 65);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Leggi pagine dati
      uint8_t data[16];
      tft.setCursor(5, 85);
      tft.print("Dati: ");
      
      for (int page = 4; page < 8; page++) {
        if (nfc.ntag2xx_ReadPage(page, data)) {
          for (int i = 0; i < 4; i++) {
            if (data[i] >= 32 && data[i] <= 126) {
              tft.print((char)data[i]);
            }
          }
        }
      }
      
      Serial.print("NFC Read - UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      delay(2000);
    }
    
    delay(50);
  }
}

void nfcWriteTag() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC WRITE MODE");
  tft.setCursor(5, 25);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 120);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(200);
      break;
    }
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 100);
    
    if (success) {
      // Prepara dati da scrivere
      uint8_t writeData[16] = {0};
      int dataLen = dataToWrite.length();
      for (int i = 0; i < dataLen && i < 16; i++) {
        writeData[i] = dataToWrite[i];
      }
      
      bool writeSuccess = nfc.ntag2xx_WritePage(4, writeData);
      
      tft.fillRect(0, 40, 160, 80, ST77XX_BLACK);
      tft.setCursor(5, 45);
      if (writeSuccess) {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("SCRITTURA OK!");
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(5, 65);
        tft.print("Dati: ");
        tft.println(dataToWrite);
      } else {
        tft.setTextColor(ST77XX_RED);
        tft.println("ERRORE!");
        tft.setTextColor(ST77XX_WHITE);
      }
      
      delay(2000);
    }
    
    delay(50);
  }
}

void nfcShowInfo() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC INFO");
  tft.setCursor(5, 25);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 120);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(200);
      break;
    }
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 100);
    
    if (success) {
      tft.fillRect(0, 40, 160, 80, ST77XX_BLACK);
      tft.setCursor(5, 45);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG INFO:");
      tft.setTextColor(ST77XX_WHITE);
      
      // Tipo tag
      tft.setCursor(5, 65);
      tft.print("Tipo: ");
      if (uidLength == 4) tft.print("Classic/UL");
      else if (uidLength == 7) tft.print("NTAG21x");
      
      // UID completo
      tft.setCursor(5, 80);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Lunghezza
      tft.setCursor(5, 95);
      tft.print("Lunghezza: ");
      tft.print(uidLength);
      tft.println(" bytes");
      
      delay(2000);
    }
    
    delay(50);
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Inizializza pulsanti
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // Inizializza SPI con pin specifici
  // SCK = 36, MISO = 37, MOSI = 35
  SPI.begin(36, 37, 35);

  // Inizializza display TFT
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  
  Serial.println("Display TFT inizializzato");

  // Inizializza PN532 NFC (usa lo stesso SPI del display)
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  
  if (versiondata) {
    nfcInitialized = true;
    nfc.SAMConfig();
    Serial.print("PN532 NFC trovato! Versione: ");
    Serial.print((versiondata >> 16) & 0xFF, HEX);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF, HEX);
    
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.println("NFC: OK");
  } else {
    Serial.println("PN532 NFC non trovato!");
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_RED);
    tft.println("NFC: Non trovato");
  }
  
  delay(1000);
  drawMenu();
}

// ---------------- LOOP ----------------
void loop() {
  if (nfcModeActive) {
    delay(50);
    return;
  }
  
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

  // OK → entra nel submenu o esegui azione
  if (digitalRead(BTN_OK) == LOW) {
    int index = visibleItems[selected];
    
    // Controlla se è un'azione (ID >= 2) o un submenu
    if (menu[index].id >= 2) {
      // Esegui azione
      executeAction(menu[index].id);
    } else {
      // Entra nel submenu
      parentStack[level] = currentParent;
      selectedStack[level] = selected;
      level++;
      
      currentParent = index;
      selected = 0;
      
      drawMenu();
    }
    delay(200);
  }

  // BACK → torna di 1 livello
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