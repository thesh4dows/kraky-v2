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
  // NFC submenu
  {"NFC Read",     2, 0},
  {"NFC Write",    3, 0},
  {"NFC Info",     4, 0},
  {"NFC Dump",     5, 0},
  // Settings
  {"Display",      6, 1},
  {"System",       7, 1},
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
String dataToWrite = "Ciao NFC!";

// ---------------- DRAW MENU ----------------
void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  visibleCount = 0;
  int y = 20;

  for (int i = 0; i < menuSize; i++) {
    if (menu[i].parent == currentParent) {
      visibleItems[visibleCount] = i;
      visibleCount++;

      if (visibleCount - 1 == selected) {
        tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      } else {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
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
  nfcModeActive = true;
  
  switch(id) {
    case 2: // NFC Read
      nfcReadTag();
      break;
    case 3: // NFC Write
      nfcWriteTag();
      break;
    case 4: // NFC Info
      nfcShowInfo();
      break;
    case 5: // NFC Dump
      nfcDumpMemory();
      break;
    default:
      nfcModeActive = false;
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(10, 50);
      tft.println("Funzione in");
      tft.println("sviluppo...");
      delay(1500);
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
  tft.setCursor(5, 20);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 140);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(300);
      break;
    }
    
    // Cerca tag NFC
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(5, 5);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      // Mostra UID
      tft.setCursor(5, 25);
      tft.print("UID (");
      tft.print(uidLength);
      tft.print("b): ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
        if (i < uidLength - 1) tft.print(":");
      }
      
      // Leggi e mostra dati da tutte le pagine
      uint8_t data[16];
      int yPos = 45;
      
      tft.setCursor(5, yPos);
      tft.println("DATI LETTI:");
      yPos += 12;
      
      // Leggi pagine (NTAG o Mifare)
      for (int page = 0; page < 16 && yPos < 135; page++) {
        bool readSuccess = false;
        
        // Prova lettura pagina NTAG
        readSuccess = nfc.ntag2xx_ReadPage(page, data);
        
        // Se fallisce, prova Mifare Classic
        if (!readSuccess) {
          uint8_t authKey[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
          readSuccess = nfc.mifareclassic_AuthenticateBlock(nfcUid, uidLength, page * 4, 0, authKey);
          if (readSuccess) {
            readSuccess = nfc.mifareclassic_ReadDataBlock(page, data);
          }
        }
        
        if (readSuccess) {
          // Numero pagina
          tft.setCursor(0, yPos);
          tft.setTextColor(ST77XX_CYAN);
          tft.print("P");
          if (page < 10) tft.print(" ");
          tft.print(page);
          tft.print(":");
          
          // Dati in HEX
          tft.setTextColor(ST77XX_WHITE);
          tft.setCursor(28, yPos);
          for (int i = 0; i < 4; i++) {
            if (data[i] < 0x10) tft.print("0");
            tft.print(data[i], HEX);
            tft.print(" ");
          }
          
          // Dati come testo
          tft.setCursor(68, yPos);
          for (int i = 0; i < 4; i++) {
            if (data[i] >= 32 && data[i] <= 126) {
              tft.print((char)data[i]);
            } else {
              tft.print(".");
            }
          }
          
          yPos += 10;
          
          // Stampa seriale
          Serial.print("Pagina ");
          Serial.print(page);
          Serial.print(": ");
          for (int i = 0; i < 4; i++) {
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();
        }
      }
      
      delay(3000);
      
      // Torna alla schermata di attesa
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.println("NFC READ MODE");
      tft.setCursor(5, 20);
      tft.println("Avvicina un tag...");
      tft.setCursor(5, 140);
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("BACK per uscire");
    }
    
    delay(100);
  }
}

void nfcWriteTag() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC WRITE MODE");
  tft.setCursor(5, 20);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 60);
  tft.print("Dati: ");
  tft.println(dataToWrite);
  tft.setCursor(5, 140);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(300);
      break;
    }
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      // Mostra UID
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Prepara dati (max 4 byte per pagina)
      uint8_t writeData[4] = {0};
      int dataLen = dataToWrite.length();
      
      for (int i = 0; i < 4 && i < dataLen; i++) {
        writeData[i] = dataToWrite[i];
      }
      
      tft.setCursor(5, 45);
      tft.println("Scrittura...");
      
      // Prova scrittura su pagina 4
      bool writeSuccess = nfc.ntag2xx_WritePage(4, writeData);
      
      if (!writeSuccess) {
        // Prova altre pagine
        for (int page = 5; page < 12; page++) {
          writeSuccess = nfc.ntag2xx_WritePage(page, writeData);
          if (writeSuccess) break;
        }
      }
      
      tft.setCursor(5, 65);
      if (writeSuccess) {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("SCRITTURA OK!");
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(5, 85);
        tft.print("Scritto: ");
        
        for (int i = 0; i < 4; i++) {
          if (writeData[i] >= 32 && writeData[i] <= 126) {
            tft.print((char)writeData[i]);
          } else {
            tft.print(".");
          }
        }
        
        // Verifica
        delay(100);
        uint8_t verifyData[16];
        if (nfc.ntag2xx_ReadPage(4, verifyData)) {
          tft.setCursor(5, 105);
          tft.print("Verifica: ");
          for (int i = 0; i < 4; i++) {
            if (verifyData[i] >= 32 && verifyData[i] <= 126) {
              tft.print((char)verifyData[i]);
            } else {
              tft.print(".");
            }
          }
        }
        
        Serial.println("Scrittura completata!");
      } else {
        tft.setTextColor(ST77XX_RED);
        tft.println("ERRORE SCRITTURA!");
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(5, 85);
        tft.println("Tag non scrivibile");
        tft.println("o protetto");
        
        Serial.println("Errore scrittura!");
      }
      
      delay(3000);
      
      // Reset schermata
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.println("NFC WRITE MODE");
      tft.setCursor(5, 20);
      tft.println("Avvicina un tag...");
      tft.setCursor(5, 60);
      tft.print("Dati: ");
      tft.println(dataToWrite);
      tft.setCursor(5, 140);
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("BACK per uscire");
    }
    
    delay(100);
  }
}

void nfcShowInfo() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC INFO");
  tft.setCursor(5, 20);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 140);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(300);
      break;
    }
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG INFO:");
      tft.setTextColor(ST77XX_WHITE);
      
      // UID
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Tipo tag
      tft.setCursor(5, 45);
      tft.print("Tipo: ");
      if (uidLength == 4) {
        tft.println("Mifare Classic");
        tft.setCursor(5, 60);
        tft.println("1K o 4K");
      } else if (uidLength == 7) {
        tft.println("NTAG21x");
        tft.setCursor(5, 60);
        tft.println("144-888 bytes");
      } else {
        tft.println("Altro standard");
      }
      
      // Lunghezza UID
      tft.setCursor(5, 80);
      tft.print("UID Length: ");
      tft.print(uidLength);
      tft.println(" bytes");
      
      // Numero pagine leggibili
      tft.setCursor(5, 100);
      tft.print("Pagine: ");
      uint8_t data[16];
      int readablePages = 0;
      for (int p = 0; p < 16; p++) {
        if (nfc.ntag2xx_ReadPage(p, data)) readablePages++;
      }
      tft.print(readablePages);
      
      Serial.println("Info tag:");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("UID Length: ");
      Serial.println(uidLength);
      Serial.print("Pagine leggibili: ");
      Serial.println(readablePages);
      
      delay(3000);
      
      // Reset schermata
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.println("NFC INFO");
      tft.setCursor(5, 20);
      tft.println("Avvicina un tag...");
      tft.setCursor(5, 140);
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("BACK per uscire");
    }
    
    delay(100);
  }
}

void nfcDumpMemory() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("NFC MEMORY DUMP");
  tft.setCursor(5, 20);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 140);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      nfcModeActive = false;
      delay(300);
      break;
    }
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      Serial.println("\n=== MEMORY DUMP ===");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println("\n");
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("DUMP IN CORSO...");
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 25);
      tft.println("Vedi Serial Monitor");
      
      uint8_t data[16];
      
      for (int page = 0; page < 16; page++) {
        if (nfc.ntag2xx_ReadPage(page, data)) {
          Serial.print("Pagina ");
          Serial.print(page);
          Serial.print(": ");
          
          for (int i = 0; i < 4; i++) {
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
            Serial.print(" ");
          }
          
          Serial.print(" | ");
          
          for (int i = 0; i < 4; i++) {
            if (data[i] >= 32 && data[i] <= 126) {
              Serial.print((char)data[i]);
            } else {
              Serial.print(".");
            }
          }
          
          Serial.println();
        }
      }
      
      Serial.println("=== FINE DUMP ===\n");
      
      delay(2000);
      
      // Reset schermata
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.println("NFC MEMORY DUMP");
      tft.setCursor(5, 20);
      tft.println("Avvicina un tag...");
      tft.setCursor(5, 140);
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("BACK per uscire");
    }
    
    delay(100);
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  SPI.begin(36, 37, 35);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  
  Serial.println("Display TFT inizializzato");

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
    
    if (menu[index].id >= 2) {
      executeAction(menu[index].id);
    } else {
      parentStack[level] = currentParent;
      selectedStack[level] = selected;
      level++;
      
      currentParent = index;
      selected = 0;
      
      drawMenu();
    }
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