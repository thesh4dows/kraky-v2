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
  {"Write Custom", 6, 0},
  // Settings
  {"Display",      7, 1},
  {"System",       8, 1},
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
String dataToWrite = "Hello NFC!";

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

// ---------------- AUTHENTICATE BLOCK ----------------
bool authenticateBlock(uint8_t block) {
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  bool result = nfc.mifareclassic_AuthenticateBlock(nfcUid, uidLength, block, 0, keya);
  
  if (result) {
    Serial.print("✓ Blocco ");
    Serial.print(block);
    Serial.println(" autenticato");
  } else {
    Serial.print("✗ Errore autenticazione blocco ");
    Serial.println(block);
  }
  
  return result;
}

// ---------------- EXECUTE ACTION ----------------
void executeAction(int id) {
  nfcModeActive = true;
  
  switch(id) {
    case 2:
      nfcReadTag();
      break;
    case 3:
      nfcWriteTag();
      break;
    case 4:
      nfcShowInfo();
      break;
    case 5:
      nfcDumpMemory();
      break;
    case 6:
      nfcWriteCustomData();
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

// ---------------- WRITE CUSTOM (Mifare Classic specific) ----------------
void nfcWriteCustomData() {
  Serial.println("\n=== WRITE CUSTOM DATA (Mifare Classic) ===");
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println("WRITE CUSTOM DATA");
  tft.setCursor(5, 20);
  tft.println("Avvicina un tag...");
  tft.setCursor(5, 60);
  tft.println("Scrive in blocco 4:");
  tft.println("'luisllamas.es'");
  tft.setCursor(5, 140);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("BACK per uscire");
  
  while (nfcModeActive) {
    if (digitalRead(BTN_BACK) == LOW) {
      Serial.println("Uscita da Write Custom");
      nfcModeActive = false;
      delay(300);
      break;
    }
    
    Serial.println("In attesa di un tag...");
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      Serial.println("✓ Tag rilevato!");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) Serial.print("0");
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("UID Length: ");
      Serial.println(uidLength);
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Solo per Mifare Classic (UID length = 4)
      if (uidLength == 4) {
        tft.setCursor(5, 45);
        tft.println("Autenticazione...");
        Serial.println("Autenticazione blocco 4...");
        
        if (authenticateBlock(4)) {
          tft.setCursor(5, 65);
          tft.println("Autenticato blocco 4");
          
          uint8_t data[16];
          memcpy(data, (const uint8_t[]){ 'l', 'u', 'i', 's', 'l', 'l', 'a', 'm', 'a', 's', '.', 'e', 's', 0, 0, 0 }, sizeof data);
          
          Serial.println("Scrittura dati: luisllamas.es");
          tft.setCursor(5, 85);
          tft.println("Scrittura...");
          
          success = nfc.mifareclassic_WriteDataBlock(4, data);
          
          if (success) {
            Serial.println("✓ SCRITTURA COMPLETATA!");
            tft.setTextColor(ST77XX_GREEN);
            tft.setCursor(5, 105);
            tft.println("SCRITTURA OK!");
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(5, 125);
            tft.print("Dati: luisllamas.es");
          } else {
            Serial.println("✗ ERRORE SCRITTURA!");
            tft.setTextColor(ST77XX_RED);
            tft.setCursor(5, 105);
            tft.println("ERRORE SCRITTURA!");
          }
        } else {
          Serial.println("✗ ERRORE AUTENTICAZIONE!");
          tft.setTextColor(ST77XX_RED);
          tft.setCursor(5, 65);
          tft.println("ERRORE AUTENTICAZIONE!");
        }
      } else {
        tft.setTextColor(ST77XX_RED);
        tft.setCursor(5, 45);
        tft.println("Solo Mifare Classic");
        tft.setCursor(5, 60);
        tft.println("UID 4 bytes");
        Serial.println("Questo tag non e' Mifare Classic (UID length != 4)");
      }
      
      delay(3000);
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.println("WRITE CUSTOM DATA");
      tft.setCursor(5, 20);
      tft.println("Avvicina un tag...");
      tft.setCursor(5, 60);
      tft.println("Scrive in blocco 4:");
      tft.println("'luisllamas.es'");
      tft.setCursor(5, 140);
      tft.setTextColor(ST77XX_YELLOW);
      tft.println("BACK per uscire");
    }
    
    delay(100);
  }
}

// ---------------- NFC READ ----------------
void nfcReadTag() {
  Serial.println("\n=== NFC READ MODE ===");
  
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
    
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUid, &uidLength, 200);
    
    if (success) {
      Serial.println("\n✓ Tag rilevato!");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) Serial.print("0");
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(5, 5);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      // Determina tipo tag e leggi
      if (uidLength == 4) {
        // Mifare Classic
        tft.setCursor(5, 45);
        tft.println("Tipo: Mifare Classic");
        
        uint8_t data[16];
        int yPos = 65;
        tft.setCursor(5, yPos);
        tft.println("Blocco 4:");
        yPos += 12;
        
        if (authenticateBlock(4)) {
          if (nfc.mifareclassic_ReadDataBlock(4, data)) {
            tft.setCursor(5, yPos);
            tft.print("Dati: ");
            for (int i = 0; i < 16; i++) {
              if (data[i] >= 32 && data[i] <= 126) {
                tft.print((char)data[i]);
                Serial.print((char)data[i]);
              } else {
                tft.print(".");
                Serial.print(".");
              }
            }
            Serial.println();
          }
        }
      } else {
        // NTAG
        tft.setCursor(5, 45);
        tft.println("Tipo: NTAG");
        
        uint8_t data[4];
        if (nfc.ntag2xx_ReadPage(4, data)) {
          tft.setCursor(5, 65);
          tft.print("Pagina 4: ");
          for (int i = 0; i < 4; i++) {
            if (data[i] >= 32 && data[i] <= 126) {
              tft.print((char)data[i]);
              Serial.print((char)data[i]);
            } else {
              tft.print(".");
              Serial.print(".");
            }
          }
          Serial.println();
        }
      }
      
      delay(3000);
      
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

// ---------------- NFC WRITE ----------------
void nfcWriteTag() {
  Serial.println("\n=== NFC WRITE MODE ===");
  Serial.print("Dati da scrivere: ");
  Serial.println(dataToWrite);
  
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
      Serial.println("✓ Tag rilevato!");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) Serial.print("0");
        Serial.print(nfcUid[i], HEX);
      }
      Serial.println();
      Serial.print("UID Length: ");
      Serial.println(uidLength);
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("TAG RILEVATO!");
      tft.setTextColor(ST77XX_WHITE);
      
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
      bool writeSuccess = false;
      
      // Scrittura per Mifare Classic (UID 4 byte)
      if (uidLength == 4) {
        tft.setCursor(5, 45);
        tft.println("Tipo: Mifare Classic");
        tft.setCursor(5, 60);
        tft.println("Scrittura blocco 4...");
        
        Serial.println("Tentativo scrittura su Mifare Classic...");
        
        if (authenticateBlock(4)) {
          uint8_t writeData[16] = {0};
          int dataLen = dataToWrite.length();
          if (dataLen > 16) dataLen = 16;
          
          for (int i = 0; i < dataLen; i++) {
            writeData[i] = dataToWrite[i];
          }
          
          writeSuccess = nfc.mifareclassic_WriteDataBlock(4, writeData);
          
          if (writeSuccess) {
            Serial.println("✓ Scrittura Mifare Classic completata!");
          } else {
            Serial.println("✗ Errore scrittura Mifare Classic!");
          }
        } else {
          Serial.println("✗ Autenticazione fallita!");
        }
      }
      // Scrittura per NTAG (UID 7 byte)
      else if (uidLength == 7) {
        tft.setCursor(5, 45);
        tft.println("Tipo: NTAG");
        tft.setCursor(5, 60);
        tft.println("Scrittura pagina 4...");
        
        Serial.println("Tentativo scrittura su NTAG...");
        
        uint8_t writeData[4] = {0};
        int dataLen = dataToWrite.length();
        if (dataLen > 4) dataLen = 4;
        
        for (int i = 0; i < dataLen; i++) {
          writeData[i] = dataToWrite[i];
        }
        
        writeSuccess = nfc.ntag2xx_WritePage(4, writeData);
        
        if (writeSuccess) {
          Serial.println("✓ Scrittura NTAG completata!");
          
          // Verifica
          uint8_t verifyData[4];
          if (nfc.ntag2xx_ReadPage(4, verifyData)) {
            Serial.print("Verifica: ");
            for (int i = 0; i < 4; i++) {
              Serial.print((char)verifyData[i]);
            }
            Serial.println();
          }
        } else {
          Serial.println("✗ Errore scrittura NTAG!");
        }
      }
      else {
        tft.setCursor(5, 45);
        tft.setTextColor(ST77XX_RED);
        tft.println("Tipo non supportato!");
        tft.setTextColor(ST77XX_WHITE);
        Serial.println("Tipo tag non supportato per scrittura!");
      }
      
      tft.setCursor(5, 85);
      if (writeSuccess) {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("SCRITTURA OK!");
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(5, 105);
        tft.print("Scritto: ");
        tft.println(dataToWrite);
        Serial.println("Scrittura completata!");
      } else {
        tft.setTextColor(ST77XX_RED);
        tft.println("ERRORE SCRITTURA!");
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(5, 105);
        tft.println("Tag non scrivibile");
        Serial.println("Errore scrittura!");
      }
      
      delay(3000);
      
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

// ---------------- NFC INFO ----------------
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
      
      tft.setCursor(5, 25);
      tft.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) tft.print("0");
        tft.print(nfcUid[i], HEX);
      }
      
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
      
      tft.setCursor(5, 80);
      tft.print("UID Length: ");
      tft.print(uidLength);
      tft.println(" bytes");
      
      Serial.println("\n=== INFO TAG ===");
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (nfcUid[i] < 0x10) Serial.print("0");
        Serial.print(nfcUid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("UID Length: ");
      Serial.println(uidLength);
      if (uidLength == 4) {
        Serial.println("Tipo: Mifare Classic");
      } else if (uidLength == 7) {
        Serial.println("Tipo: NTAG21x");
      }
      
      delay(3000);
      
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

// ---------------- NFC DUMP ----------------
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
      Serial.println();
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 5);
      tft.setTextColor(ST77XX_GREEN);
      tft.println("DUMP IN CORSO...");
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 25);
      tft.println("Vedi Serial Monitor");
      
      if (uidLength == 4) {
        // Mifare Classic
        tft.setCursor(5, 45);
        tft.println("Dump blocchi 0-15");
        
        uint8_t data[16];
        for (int block = 0; block < 16; block++) {
          if (authenticateBlock(block)) {
            if (nfc.mifareclassic_ReadDataBlock(block, data)) {
              Serial.print("Blocco ");
              Serial.print(block);
              Serial.print(": ");
              for (int i = 0; i < 16; i++) {
                if (data[i] < 0x10) Serial.print("0");
                Serial.print(data[i], HEX);
                Serial.print(" ");
                if ((i + 1) % 4 == 0) Serial.print(" ");
              }
              Serial.print(" | ");
              for (int i = 0; i < 16; i++) {
                char c = (data[i] >= 32 && data[i] <= 126) ? (char)data[i] : '.';
                Serial.print(c);
              }
              Serial.println();
            }
          }
          delay(50);
        }
      } else if (uidLength == 7) {
        // NTAG
        tft.setCursor(5, 45);
        tft.println("Dump pagine 0-15");
        
        uint8_t data[4];
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
              char c = (data[i] >= 32 && data[i] <= 126) ? (char)data[i] : '.';
              Serial.print(c);
            }
            Serial.println();
          }
        }
      }
      
      Serial.println("=== FINE DUMP ===\n");
      
      delay(3000);
      
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
  delay(1000);  // Attesa per seriale ESP32-S3
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("SISTEMA NFC - ESP32-S3");
  Serial.println("========================================");
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  Serial.println("✓ Pulsanti configurati");

  SPI.begin(36, 37, 35);
  Serial.println("✓ SPI inizializzato (SCK=36, MISO=37, MOSI=35)");

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 50);
  tft.println("Inizializzazione...");
  Serial.println("✓ Display TFT inizializzato");

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  
  if (versiondata) {
    nfcInitialized = true;
    nfc.SAMConfig();
    Serial.print("✓ PN532 NFC trovato! Versione: ");
    Serial.print((versiondata >> 16) & 0xFF, HEX);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF, HEX);
    
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.println("NFC: OK");
    tft.setCursor(10, 65);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Versione: ");
    tft.print((versiondata >> 16) & 0xFF, HEX);
    tft.print(".");
    tft.println((versiondata >> 8) & 0xFF, HEX);
  } else {
    Serial.println("✗ PN532 NFC non trovato!");
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(ST77XX_RED);
    tft.println("NFC: Non trovato");
  }
  
  delay(2000);
  drawMenu();
  Serial.println("Menu principale avviato");
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
    Serial.print("OK - Esecuzione: ");
    Serial.println(menu[index].name);
    
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