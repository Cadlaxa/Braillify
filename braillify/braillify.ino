#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <LittleFS.h>
#include <cstring>
#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "web_pages.h"
#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#define FIRMWARE_VERSION "1.0.3.1"

Preferences prefs;

// LCD setup (I2C)
#define I2C_ADDR 0x27  
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

// Web Server & Captive Portal DNS
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Wi-Fi Configuration State
String wifiSSID = "";
String wifiPass = "";
String wifiStatusMsg = "AP Mode Active (192.168.4.1)";

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
const int buzzerPin = 15;
const int vibroPin = 4;
enum FeedbackMode {
  SOUND_ONLY = 0,
  VIBRATE_ONLY = 1
};
bool vibActive = false;
unsigned long vibStart = 0;
unsigned int vibDuration = 0;

FeedbackMode feedbackMode = SOUND_ONLY;
bool vibrationEnabled = true;
bool lastButtonState = HIGH;
unsigned long lastBeep = 0;
const unsigned long debounceMillis = 35;

// EEPROM / Storage settings
const int EEPROM_ADDR = 0;
const int MAX_CHARS = 1024;
int loadingProgress = 0;
unsigned long lastUpdate = 0;

// Braille / editor state
byte brailleBits = 0;
int cursorPos = 0;
String fullBuffer = "";
int windowStart = 0;
bool hasContraction = false;
bool nextCapital = false;
bool capsLock = false;
bool dot6PressedOnce = false;

bool nextNumber = false;
bool numberLock = false;
bool numPressedOnce = false;

bool nextSpecial = false;
bool specialLock = false;
bool specialPressedOnce = false;

bool nextText = false;
bool textLock = false;
bool textPressedOnce = false;
int indicatorLength = 0;

#define MAX_BUFFER 4096
byte fullBufferBraille[MAX_BUFFER];
byte contractionPrefix = 0;

char lastKey = NO_KEY;
unsigned long keyPressTime = 0;
const unsigned long holdDelay = 800;
const unsigned long repeatRate = 120;
unsigned long nextRepeatTime = 0;
bool hashPending = false;
unsigned long hashPressTime = 0;
const unsigned long hashDebounceTime = 250;

enum Mode { AUTO, TEXT, NUMBER, SPECIAL };
Mode currentMode = AUTO;

// Note Frequencies
#define C 2093
#define D 2349
#define E 2637
#define F 2794
#define G 3136
#define A 3520

// Forward declarations
void handleKeyPress(char key);
void handleHashSinglePress();
void handleHashDoublePress();
void showTempIndicator(char ch, int len);
void clearTempIndicator();
int getLcdCursor();
void scrollWindow();
void redrawLCDLine();
void insertAtCursor(char c, byte brailleCell = 0);
void backspaceAtCursor();
void insertSpaceAtCursor();
void moveCursorLeft();
void moveCursorRight();
void cycleMode();
void updateLCDMode();
void saveLineToEEPROM();
void loadLineFromEEPROM();
void handleSpaceKeyDirect();
void sound(int freq, int dur);
void vibrate(int durationMs, int strength);
void startupTone();
void saveTone();
void loadTone();
void enterTone();
void modeTone();
void errorTone();
void scanTone();
void triggerScanNotice();
void updateFirmwareFromGitHub(const String& binUrl);
byte charToBraille(char c);

// ---------- Special & mapping functions ----------
const char specialOutput[][8] = {
  "?", "!", "&", ",", "_", ".", ";", ":", "\"", "()",
  "/", "@", "*", "\'", "cannot", "had", "many", "spirit", 
  "their", "world"
};

const byte specialDots[] = {
  38, 22, 47, 2, 36, 50, 6, 18, 52, 54,
  12, 44, 20, 4, 9, 19, 13, 14, 46, 58
};

String specialFromBraille(byte p) {
  for (int i = 0; i < 20; i++) {
    if (p == specialDots[i]) {
      return String(specialOutput[i]);
    }
  }
  return "0";
}

const byte brailleDotsTable[] = {
  1,3,9,25,17,11,27,19,10,26,5,7,13,29,21,15,31,23,14,30,
  37,39,58,45,61,53,47,63,55,46,62,33,35,41,57,49,43,59,42,
  34,28,51,2,6,18,50,22,54,38,52,12,44,16,40
};

const char brailleTextTable[][7] = {
  "a","b","c","d","e","f","g","h","i","j",
  "k","l","m","n","o","p","q","r","s","t",
  "u","v","w","x","y","z",
  "and","for","of","the","with",
  "ch","gh","sh","th","wh","ed","er","ow","en","ar",
  "ou","ea","bb","cc","dd","ff","gg","in","by","st","ing",">","-"
};

String brailleToText(byte b) {
  int tableSize = sizeof(brailleDotsTable) / sizeof(brailleDotsTable[0]);
  for (int i = 0; i < tableSize; i++) {
    if (b == brailleDotsTable[i]) {
      return String(brailleTextTable[i]);
    }
  }
  return "~";
}

const byte brailleNumDots[] = {1,3,9,25,17,11,27,19,10,26};
const char brailleNumChars[] = {'1','2','3','4','5','6','7','8','9','0'};

char brailleToNumber(byte b) {
  for (int i = 0; i < 10; i++) {
    if (b == brailleNumDots[i]) {
      return brailleNumChars[i];
    }
  }
  return '?';
}

byte charToBraille(char c) {
  if (c == ' ') return 0;
  char lowerC = c;
  if (lowerC >= 'A' && lowerC <= 'Z') lowerC += 32;

  for (int i = 0; i < 26; i++) {
    if (brailleTextTable[i][0] == lowerC && brailleTextTable[i][1] == '\0') {
      return brailleDotsTable[i];
    }
  }
  for (int i = 0; i < 10; i++) {
    if (brailleNumChars[i] == c) {
      return brailleNumDots[i];
    }
  }
  for (int i = 0; i < 20; i++) {
    if (strlen(specialOutput[i]) == 1 && specialOutput[i][0] == c) {
      return specialDots[i];
    }
  }
  return 0;
}

String brailleToChar(byte bits) {
  byte pattern = bits;
  String out = "";

  if (pattern == 32) {
    if (dot6PressedOnce) {
      capsLock = true;
      nextCapital = false;
      dot6PressedOnce = false;
      showTempIndicator('^', 2);
      return "";
    } else {
      dot6PressedOnce = true;
      nextCapital = true;
      showTempIndicator('^', 1);
      return "";
    }
  }
  if (pattern != 32) dot6PressedOnce = false;

  if (pattern == 60) {
    if (numPressedOnce) {
      numberLock = true;
      nextNumber = false;
      numPressedOnce = false;
      showTempIndicator('#', 2);
      return "";
    } else {
      numPressedOnce = true;
      nextNumber = true;
      numberLock = false;
      showTempIndicator('#', 1);
      return "";
    }
  }
  if (pattern != 60) numPressedOnce = false;

  if (pattern == 56) {
    if (specialPressedOnce) {
      specialLock = true;
      nextSpecial = false;
      specialPressedOnce = false;
      showTempIndicator('$', 2);
      return "";
    } else {
      specialPressedOnce = true;
      nextSpecial = true;
      specialLock = false;
      showTempIndicator('$', 1);
      return "";
    }
  }
  if (pattern != 56) specialPressedOnce = false;

  if (pattern == 48) {
    if (textPressedOnce) {
      textLock = true;
      nextText = false;
      textPressedOnce = false;
      showTempIndicator('>', 2);
      return "";
    } else {
      textPressedOnce = true;
      nextText = true;
      textLock = false;
      showTempIndicator('>', 1);
      return "";
    }
  }
  if (pattern != 48) textPressedOnce = false;

  if (nextNumber || numberLock) {
    currentMode = NUMBER;
    nextNumber = false;
    clearTempIndicator();
  }

  if (nextSpecial || specialLock) {
    currentMode = SPECIAL;
    nextSpecial = false;
    clearTempIndicator();
  }

  if (nextText || textLock) {
    currentMode = TEXT;
    nextText = false;
    clearTempIndicator();
  }

  if (currentMode == AUTO && pattern != 0) {
    if (brailleToText(pattern) != "~") currentMode = TEXT;
    else if (specialFromBraille(pattern) != "0") currentMode = SPECIAL;
    else if (brailleToNumber(pattern) != '?') currentMode = NUMBER;
  }

  if (currentMode == TEXT) {
      out = brailleToText(pattern);
      if (out == "~") {
          String s = specialFromBraille(pattern);
          if (s != "0") out = String(s);
          else {
              char n = brailleToNumber(pattern);
              if (n != '?') out = String(n);
              else out = "?";
          }
      }
  } else if (currentMode == SPECIAL) {
      String s = specialFromBraille(pattern);
      if (s != "0") out = String(s);
      else out = brailleToText(pattern);
  } else if (currentMode == NUMBER) {
      char n = brailleToNumber(pattern);
      if (n != '?') out = String(n);
      else {
          String s = specialFromBraille(pattern);
          if (s != "") out = String(s);
          else out = brailleToText(pattern);
      }
  }

  if ((nextCapital || capsLock) && out.length() > 0) {
    out.toUpperCase();
    nextCapital = false;
    clearTempIndicator();
  }

  if ((nextCapital || capsLock) && pattern == 4) {
    capsLock = false;
    nextCapital = false;
    clearTempIndicator();
    return "";
  }

  return out;
}

void showTempIndicator(char ch, int len) {
  lcd.setCursor(getLcdCursor(), 1);
  for (int i = 0; i < len && getLcdCursor() + i < 16; i++) {
    lcd.print(ch);
  }
  indicatorLength = len;
  int newCursor = getLcdCursor() + len;
  if (newCursor > 15) newCursor = 15;
  lcd.setCursor(newCursor, 1);
}

void clearTempIndicator() {
  if (indicatorLength <= 0) return;
  int lcdCur = getLcdCursor();
  int start = lcdCur - indicatorLength;
  if (start < 0) start = 0;
  lcd.setCursor(start, 1);
  for (int i = 0; i < indicatorLength; i++) lcd.print(' ');
  lcd.setCursor(start, 1);
  indicatorLength = 0;
}

int getLcdCursor() {
  int lcdCursor = cursorPos - windowStart;
  if (lcdCursor < 0) lcdCursor = 0;
  if (lcdCursor > 15) lcdCursor = 15;
  return lcdCursor;
}

void scrollWindow() {
  if (cursorPos < windowStart) windowStart = cursorPos;
  if (cursorPos >= windowStart + 16) windowStart = cursorPos - 15;
  if (windowStart < 0) windowStart = 0;
}

void redrawLCDLine() {
  lcd.setCursor(0,1);
  String visible;
  if (fullBuffer.length() <= windowStart) {
    visible = "";
  } else {
    int end = windowStart + 16;
    if (end > fullBuffer.length()) end = fullBuffer.length();
    visible = fullBuffer.substring(windowStart, end);
  }
  lcd.print(visible);
  for (int i = visible.length(); i < 16; i++) lcd.print(' ');
  int lcdCursor = getLcdCursor();
  lcd.setCursor(lcdCursor, 1);
}

void insertAtCursor(char c, byte brailleCell) {
    if (fullBuffer.length() >= MAX_BUFFER) return;
    fullBuffer = fullBuffer.substring(0, cursorPos) + c + fullBuffer.substring(cursorPos);
    for (int i = fullBuffer.length() - 1; i > cursorPos; i--) {
        fullBufferBraille[i] = fullBufferBraille[i - 1];
    }
    if (brailleCell == 0 && c != ' ') {
      brailleCell = charToBraille(c);
    }
    fullBufferBraille[cursorPos] = brailleCell;
    cursorPos++;
    scrollWindow();
    redrawLCDLine();
}

void backspaceAtCursor() {
  if (cursorPos == 0) return;
  fullBuffer = fullBuffer.substring(0, cursorPos - 1) + fullBuffer.substring(cursorPos);
  for (int i = cursorPos - 1; i < fullBuffer.length(); i++) {
    fullBufferBraille[i] = fullBufferBraille[i + 1];
  }
  cursorPos--;
  if (windowStart > 0 && cursorPos < windowStart) windowStart--;
  scrollWindow();
  redrawLCDLine();
}

void insertSpaceAtCursor() {
  insertAtCursor(' ', 0);
}

void moveCursorLeft() {
  if (cursorPos > 0) {
      cursorPos--;
      scrollWindow();
      redrawLCDLine();
  }
}

void moveCursorRight() {
  if (cursorPos < fullBuffer.length()) {
      cursorPos++;
      scrollWindow();
      redrawLCDLine();
  }
}

void cycleMode() {
  if (currentMode == AUTO) currentMode = TEXT;
  else if (currentMode == TEXT) currentMode = SPECIAL;
  else if (currentMode == SPECIAL) currentMode = NUMBER;
  else if (currentMode == NUMBER) currentMode = AUTO;
  else currentMode = AUTO;
  updateLCDMode();
}

void updateLCDMode() {
  lcd.setCursor(0,0);
  switch(currentMode) {
    case AUTO:    lcd.print("Mode: AUTO      "); break;
    case TEXT:    lcd.print("Mode: Text      "); break;
    case NUMBER:  lcd.print("Mode: Number    "); break;
    case SPECIAL: lcd.print("Mode: Special   "); break;
  }
}

void setupLittleFS() {
    // Attempt normal mount with auto-format enabled
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed! Attempting force format...");
        
        // Force format if corrupt directory pairs exist
        if (LittleFS.format()) {
            Serial.println("LittleFS formatted successfully.");
            if (LittleFS.begin(true)) {
                Serial.println("LittleFS mounted after formatting!");
                return;
            }
        }
        Serial.println("Error: Failed to initialize LittleFS!");
    } else {
        Serial.println("LittleFS mounted successfully.");
    }
}

void saveLineToEEPROM() {
    File file = LittleFS.open("/buffer.txt", FILE_WRITE);
    if (file) {
        file.print(fullBuffer);
        file.close();
    } else {
        Serial.println("Failed to open file for writing");
        return;
    }

    lcd.setCursor(0, 0);
    lcd.print("Saved to MEMORY ");
    saveTone();

    int barWidth = 16;
    int speed = 10;
    for (int i = 0; i <= barWidth; i++) {
        lcd.setCursor(0, 1);
        for (int j = 0; j < barWidth; j++) {
            if (j <= i) lcd.print("#");
            else lcd.print(" ");
        }
        delay(speed);
    }
    insertSpaceAtCursor(); 
    scrollWindow();
    redrawLCDLine();
    updateLCDMode();
}

void loadLineFromEEPROM() {
    if (!LittleFS.exists("/buffer.txt")) {
        lcd.setCursor(0, 0);
        lcd.print("No Save File!   ");
        delay(1000);
        updateLCDMode();
        return;
    }

    String loaded = "";
    File file = LittleFS.open("/buffer.txt", FILE_READ);
    if (file) {
        loaded = file.readString();
        file.close();
    }

    while (loaded.endsWith(" ")) loaded.remove(loaded.length() - 1);
    if (loaded.length() == 0) return;
    lcd.setCursor(0, 0);
    lcd.print("Loaded from MEM  ");
    loadTone();
    delay(200);

    int barWidth = 16;
    int speed = 10;
    for (int i = 0; i <= barWidth; i++) {
        lcd.setCursor(0, 1);
        for (int j = 0; j < barWidth; j++) {
            if (j <= i) lcd.print("#");
            else lcd.print(" ");
        }
        delay(speed);
    }

    for (int i = 0; i < loaded.length(); i++) {
        insertAtCursor(loaded[i], charToBraille(loaded[i])); 
    }
    insertSpaceAtCursor(); 
    scrollWindow();
    redrawLCDLine();
    updateLCDMode();
}

struct UEBContraction {
    byte prefixDots;
    const char letters[8];
    const char contraction[16];
};

const UEBContraction uebContractions[] = {
  {0, " ab", " about"}, {0, "abv", "above"}, {0, "acc", "according"}, {0, "acr", "across"},
  {0, "adm", "administration"}, {0, " af", " after"}, {0, "afn", "afternoon"}, {0, "afw", "afterward"},
  {0, " ag", " again"}, {12, "agst", "against"}, {0, " alm", " almost"}, {0, "alr", "already"},
  {57, "alth", "although"}, {0, "altg", "altogether"}, {0, "alw", "always"}, {0, " z", " as"},

  {6, "bb", "be"}, {6, "bbc", "because"}, {6, "bbf", "before"}, {6, "bbh", "behind"},
  {6, "bbl", "below"}, {6, "bbn", "beneath"}, {6, "bbs", "beside"}, {6, "bbt", "between"},
  {6, "bby", "beyond"}, {0, " bl", " blind"}, {0, "brl", "braille"}, {0, " b", " but"},

  {0, " c", " can"}, {16, ">ch", "character"}, {33, " ch", " child"}, {33, "chn", "children"},
  {18, "cccv", "conceive"}, {18, "cccvg", "conceiving"}, {0, "cd", "could"},

  {16, ">d", "day"}, {0, "dcv", "deceive"}, {0, "dcvg", "deceiving"}, {0, "dcl", "declare"},
  {0, "dclg", "declaring"}, {0, " d", " do"},

  {0, "ei", "either"}, {34, " en", " enough"}, {16, ">e", "ever"},

  {16, ">f", "father"}, {12, "fst", "first"}, {0, "fr", "friend"}, {0, " f", " from"},

  {0, " g", " go"}, {0, " gd", " good"}, {0, "grt", "great"},

  {0, " h", " have"}, {16, ">h", "here"}, {59, "herf", "herself"}, {0, "hmf", "himself"},
  {38, " in", " his"},

  {0, "imm", "immediate"}, {0, " x", " it"}, {0, " xs", " its"}, {0, "xf", "itself"},

  {0, " j", "just"}, {16, ">k", "know"}, {0, " k", " knowledge"},

  {0, "lr", "letter"}, {0, " l", " like"}, {0, " ll", " little"}, {16, ">l", "lord"},

  {0, " m", " more"}, {16, ">m", "mother"}, {33, "mch", "much"}, {12, "mst", "must"},
  {0, "myf", "myself"},

  {16, ">n", "name"}, {0, "nec", "necessary"}, {0, "nei", "neither"}, {0, " n", " not"},

  {16, ">o", "one"}, {16, ">of", "oneself"}, {0, " m", " more"}, {51, " >ou", " ought"},
  {40, "-d", "ound"}, {40, "-t", "ount"}, {51, "ourvs", "ourselves"}, {51, " ou", " out"},

  {0, "pd", "paid"}, {0, " p", " people"}, {59, "percv", "perceive"}, {59, "percvg", "perceiving"},
  {59, "perh", "perhaps"},

  {16, ">q", "question"}, {0, "qk", "quick"}, {0, " q", " quite"},

  {0, " r", " rather"}, {0, "rcv", "receive"}, {0, "rcvg", "receiving"}, {0, "rjc", "rejoice"},
  {0, "rjcg", "rejoicing"}, {16, ">r", "right"},

  {0, "sd", "said"}, {41, " sh", " shall"}, {0, " s", " so"}, {16, ">s", "some"},
  {12, " st", " still"}, {33, "sch", "such"},

  {0, " t", " that"}, {46, "themvs", "themselves"}, {16, ">the", "there"}, {57, " th", " this"},
  {24, "-th", "those"}, {16, ">th", "through"}, {57, "thyf", "thyself"}, {16, ">t", "time"},
  {0, "td", "today"}, {0, "tgr", "together"}, {0, "tm", "tomorrow"}, {0, "tn", "tonight"},

  {16, ">u", "under"}, {0, " u", " us"},

  {0, " v", " very"},

  {52, " by", " was"}, {54, " gg", " were"}, {16, ">wh", "where"}, {49, " wh", " which"},
  {0, " w", " will"}, {16, ">w", "work"}, {0, "wd", "would"},

  {0, " y", " you"}, {16, ">y", "young"}, {0, "yr", "your"}, {0, "yrf", "yourself"},
  {0, "yrvs", "yourselves"},
};

const int contractionCount = sizeof(uebContractions)/sizeof(uebContractions[0]);

byte getLastNonCapitalBrailleCell(const byte *arr, int len) {
    for (int i = len - 1; i >= 0; --i) {
        if (arr[i] != 32) return arr[i];
    }
    return 0;
}

void applyCaseStyle(const char* original, char* output) {
    int n = strlen(original);
    bool allUpper = true;
    bool firstUpper = false;

    if (n > 0 && original[0] >= 'A' && original[0] <= 'Z') firstUpper = true;
    for (int i = 0; i < n; i++)
        if (!(original[i] >= 'A' && original[i] <= 'Z')) { allUpper = false; break; }

    if (allUpper) { for (int i = 0; output[i]; i++) if (output[i] >= 'a' && output[i] <= 'z') output[i] -= 32; return; }
    if (firstUpper && output[0] >= 'a' && output[0] <= 'z') output[0] -= 32;
}

void applyContraction(char* buffer, int& bufLen, byte* brailleArr = nullptr, byte prefixPassed = 0, bool front = true) {
    int bestLen = 0, bestIndex = -1;
    char letters[8];
    byte lastBraille = brailleArr ? getLastNonCapitalBrailleCell(brailleArr, bufLen) : 0;

    for (int i = 0; i < contractionCount; i++) {
        byte prefix = uebContractions[i].prefixDots;
        strncpy(letters, uebContractions[i].letters, 7);
        letters[7] = '\0';

        int lettersLen = strlen(letters);
        if (lettersLen == 0 || lettersLen > bufLen) continue;

        bool match = true;
        for (int k = 0; k < lettersLen; k++) {
            char c1 = buffer[bufLen - lettersLen + k];
            if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
            if (c1 != letters[k]) { match = false; break; }
        }
        if (!match) continue;

        if (front) {
            int startPos = bufLen - lettersLen;
            if (startPos > 0 && buffer[startPos - 1] != ' ') continue;
            if (prefix != 0 && prefix != prefixPassed) continue;
        } else if (brailleArr && prefix != 0 && prefix != lastBraille) continue;

        if (lettersLen > bestLen) { bestLen = lettersLen; bestIndex = i; }
    }

    if (bestIndex < 0) return;

    ::hasContraction = true;
    char contraction[16];
    strncpy(contraction, uebContractions[bestIndex].contraction, 15);
    contraction[15] = '\0';
    int contractionLen = strlen(contraction);

    char original[16];
    strncpy(original, buffer + bufLen - bestLen, bestLen);
    original[bestLen] = '\0';

    applyCaseStyle(original, contraction);

    if (!front && brailleArr)
        for (int k = bufLen - bestLen; k < bufLen; k++) brailleArr[k] = 0;

    memcpy(buffer + bufLen - bestLen, contraction, contractionLen);
    bufLen = bufLen - bestLen + contractionLen;
    buffer[bufLen] = '\0';
}

void setFrontContraction(char* buffer, int& bufLen, byte prefixPassedToFunction) {
    applyContraction(buffer, bufLen, nullptr, prefixPassedToFunction, true);
}

void setEndContraction(char* buffer, int& bufLen, byte* fullBufferBraille) {
    applyContraction(buffer, bufLen, fullBufferBraille, 0, false);
}

void handleSpaceKeyDirect() {
    if (fullBuffer.length() > 0) {
        int bufLen = fullBuffer.length();
        char* buf = new char[MAX_BUFFER];
        fullBuffer.toCharArray(buf, MAX_BUFFER);

        setEndContraction(buf, bufLen, fullBufferBraille);
        setFrontContraction(buf, bufLen, contractionPrefix);

        fullBuffer = String(buf);
        delete[] buf;

        if (::hasContraction) {
            cursorPos = fullBuffer.length();
        }
        ::hasContraction = false;
    }
}

void startUP() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Braillify   ");
  startupTone();

  int barWidth = 16;
  int speed = 80;
  for (int i = 0; i <= barWidth; i++) {
      lcd.setCursor(0, 1);
      for (int j = 0; j < barWidth; j++) {
          if (j <= i) lcd.print("#");
          else lcd.print(" ");
      }
      delay(speed);
  }

  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

// ---------- Web Server Endpoints ----------
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleApiScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; ++i) {
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i));
    json += "}";
    if (i < n - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleApiConnect() {
  if (server.hasArg("ssid")) {
    wifiSSID = server.arg("ssid");
    wifiPass = server.hasArg("pass") ? server.arg("pass") : "";

    prefs.begin("wifi", false);
    prefs.putString("ssid", wifiSSID);
    prefs.putString("pass", wifiPass);
    prefs.end();

    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    wifiStatusMsg = "Connecting to " + wifiSSID + "...";
  }

  String json = "{\"status\":\"" + wifiStatusMsg + "\"}";
  server.send(200, "application/json", json);
}

void handleApiSetText() {
  if (server.hasArg("text")) {
    String incoming = server.arg("text");

    if (incoming.length() >= MAX_BUFFER) {
      incoming = incoming.substring(0, MAX_BUFFER - 1);
    }
    fullBuffer = incoming;

    if (server.hasArg("pos")) {
      cursorPos = server.arg("pos").toInt();
      if (cursorPos < 0) cursorPos = 0;
      if (cursorPos > fullBuffer.length()) cursorPos = fullBuffer.length();
    } else {
      cursorPos = fullBuffer.length();
    }

    for (int i = 0; i < fullBuffer.length() && i < MAX_BUFFER; i++) {
      fullBufferBraille[i] = charToBraille(fullBuffer[i]);
    }

    scrollWindow();
    redrawLCDLine();
  }
  handleApiState();
}

void getSystemStateJson(String &jsonOut) {
  String line0 = "";
  switch(currentMode) {
    case AUTO:    line0 = "Mode: AUTO     "; break;
    case TEXT:    line0 = "Mode: Text     "; break;
    case NUMBER:  line0 = "Mode: Number   "; break;
    case SPECIAL: line0 = "Mode: Special  "; break;
  }

  String line1 = "";
  if (fullBuffer.length() <= windowStart) {
    line1 = "";
  } else {
    int end = windowStart + 16;
    if (end > fullBuffer.length()) end = fullBuffer.length();
    line1 = fullBuffer.substring(windowStart, end);
  }
  while (line1.length() < 16) line1 += " ";

  String preview = brailleToChar(brailleBits);
  if (preview == "") preview = " ";

  String safeFullText = fullBuffer;
  safeFullText.replace("\\", "\\\\");
  safeFullText.replace("\"", "\\\"");

  String jsonBrailleArray = "[";
  for(int i = 0; i < fullBuffer.length(); i++) {
    byte b = fullBufferBraille[i];
    if (b == 0 && fullBuffer[i] != ' ') {
      b = charToBraille(fullBuffer[i]);
    }
    jsonBrailleArray += String(b);
    if(i < fullBuffer.length() - 1) jsonBrailleArray += ",";
  }
  jsonBrailleArray += "]";

  if (WiFi.status() == WL_CONNECTED) {
    wifiStatusMsg = "Connected: " + WiFi.localIP().toString();
  }

  jsonOut = "{";
  jsonOut += "\"bits\":" + String(brailleBits) + ",";
  jsonOut += "\"mode\":" + String((int)currentMode) + ",";
  jsonOut += "\"cursor\":" + String(cursorPos) + ",";
  jsonOut += "\"preview\":\"" + preview + "\",";
  jsonOut += "\"lcd0\":\"" + line0 + "\",";
  jsonOut += "\"lcd1\":\"" + line1 + "\",";
  jsonOut += "\"fullText\":\"" + safeFullText + "\",";
  jsonOut += "\"brailleArray\":" + jsonBrailleArray + ",";
  jsonOut += "\"wifiStatus\":\"" + wifiStatusMsg + "\"";
  jsonOut += "}";
}

void handleApiState() {
  String json;
  json.reserve(5000);
  getSystemStateJson(json);
  server.send(200, "application/json", json);
}

void handleApiKey() {
  if (server.hasArg("k")) {
    String kArg = server.arg("k");
    if (kArg.length() > 0) {
      char key = kArg.charAt(0);
      handleKeyPress(key);
    }
  }
  handleApiState();
}

void handleApiSetCursor() {
  if (server.hasArg("pos")) {
    int newPos = server.arg("pos").toInt();
    if (newPos >= 0 && newPos <= fullBuffer.length()) {
      cursorPos = newPos;
      scrollWindow();
      redrawLCDLine();
      sound(1800, 40);
    }
  }
  handleApiState();
}

void updateFirmwareFromGitHub(const String& binUrl) {
  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Connect to     ");
    lcd.setCursor(0, 1);
    lcd.print(" internet first ");
    errorTone();
    delay(2000);
    updateLCDMode();
    redrawLCDLine();
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Checking update");
  lcd.setCursor(0, 1);
  lcd.print(" v" FIRMWARE_VERSION " ... ");
  scanTone();

  WiFiClientSecure client;
  client.setInsecure(); // Bypass SSL verification for GitHub and AWS S3 redirect hosts

  String manifestUrl = "https://github.com/Cadlaxa/Braillify/releases/latest/download/manifest.json";
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, manifestUrl);

  int httpCode = http.GET();
  String latestVersion = "";

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    int verIndex = payload.indexOf("\"version\": \"");
    if (verIndex != -1) {
      int start = verIndex + 12;
      int end = payload.indexOf("\"", start);
      if (end != -1) {
        latestVersion = payload.substring(start, end);
      }
    }
  }
  http.end();

  if (latestVersion.startsWith("v")) {
    latestVersion = latestVersion.substring(1);
  }

  // 2. Compare latest release version against current FIRMWARE_VERSION
  if (latestVersion.length() > 0 && latestVersion == FIRMWARE_VERSION) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Already Up-to- ");
    lcd.setCursor(0, 1);
    lcd.print(" Date! (v" FIRMWARE_VERSION ") ");
    sound(2000, 100);
    delay(2500);
    scrollWindow();
    redrawLCDLine();
    updateLCDMode();
    return;
  }

  // 3. Newer version detected or manifest bypass, proceed to flash firmware payload
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Downloading... ");

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(false);

  httpUpdate.onProgress([](int current, int total) {
    int percentage = (current * 100) / total;
    lcd.setCursor(0, 1);
    lcd.print(" Progress: ");
    lcd.print(percentage);
    lcd.print("%   ");
  });

  String targetPayloadUrl = (binUrl.length() > 0) ? binUrl : "https://github.com/Cadlaxa/Braillify/releases/latest/download/firmware.bin";
  t_httpUpdate_return ret = httpUpdate.update(client, targetPayloadUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Update Failed! ");
      lcd.setCursor(0, 1);
      lcd.print(" Err: ");
      lcd.print(httpUpdate.getLastError());
      lcd.print(" ");
      errorTone();
      delay(3000);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" No New Version ");
      delay(2000);
      break;

    case HTTP_UPDATE_OK:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Update Success!");
      lcd.setCursor(0, 1);
      lcd.print(" Rebooting...   ");
      saveTone();
      delay(2000);
      ESP.restart();
      break;
  }
  scrollWindow();
  redrawLCDLine();
  updateLCDMode();
}

// ---------- ESP32 Setup & Loop ----------
void setup() {
  Serial.begin(9600);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(buzzerPin, 0);

  pinMode(vibroPin, OUTPUT);
  digitalWrite(vibroPin, LOW);
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  keypad.setHoldTime(holdDelay);
  setupLittleFS();

  prefs.begin("settings", false);
  feedbackMode = (FeedbackMode)prefs.getUInt("fbmode", 0);
  prefs.end();

  // Dual AP + STA Wi-Fi Mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Braillify", "");
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Restore Saved Wi-Fi Credentials
  prefs.begin("wifi", false);
  wifiSSID = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  prefs.end();

  if (wifiSSID.length() > 0) {
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    wifiStatusMsg = "Connecting to " + wifiSSID + "...";
  }

  // Register Web Routes
  server.on("/", handleRoot);
  server.on("/api/state", handleApiState);
  server.on("/api/key", handleApiKey);
  server.on("/api/setcursor", handleApiSetCursor);
  server.on("/api/scan", handleApiScan);
  server.on("/api/connect", handleApiConnect);
  server.on("/api/settext", handleApiSetText);
  
  // Register GitHub Remote OTA Route
  server.on("/api/ota_update", HTTP_GET, []() {
    String defaultUrl = "https://github.com/Cadlaxa/Braillify/releases/latest/download/firmware.bin";
    String targetUrl = server.hasArg("url") ? server.arg("url") : defaultUrl;
    
    server.send(200, "application/json", "{\"status\":\"OTA Update Started! Watch ESP32 LCD display.\"}");
    updateFirmwareFromGitHub(targetUrl);
  });

  // Manual Local File Upload OTA
  server.on("/update", HTTP_GET, []() {
    String html = "<html><body>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<h2>Braillify OTA Firmware Update</h2>";
    html += "<input type='file' name='update' accept='.bin'>";
    html += "<input type='submit' value='Upload & Flash'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "Update Failed!" : "Update Success! Rebooting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update Firmware: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.onNotFound(handleRoot);
  server.begin();

  // Initialize mDNS Responder (http://braillify.local)
  if (MDNS.begin("braillify")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started: http://braillify.local");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  startUP();

  updateLCDMode();
  lcd.setCursor(getLcdCursor(), 1);
  lcd.cursor();
  lcd.blink();
}

void vibrate(int durationMs, int strength) {
  digitalWrite(vibroPin, HIGH);
  vibStart = millis();
  vibDuration = durationMs;
  vibActive = true;
}

void sound(int freq, int dur) {
  unsigned long now = millis();
  if (now - lastBeep >= debounceMillis) {
    if (feedbackMode != VIBRATE_ONLY) tone(buzzerPin, freq, dur);
    if (feedbackMode != SOUND_ONLY) vibrate(200, 230);
    lastBeep = now;
  }
}

void startupTone() {
  sound(1500, 120); delay(150);
  sound(2000, 120); delay(150);
  sound(2500, 160); delay(200);
}
void saveTone() {
  sound(2000, 60); delay(200);
  sound(2400, 60); delay(200);
  sound(2800, 80); delay(300);
}
void loadTone() {
  sound(2800, 60); delay(200);
  sound(2400, 60); delay(200);
  sound(2000, 80); delay(300);
}
void enterTone() {
  sound(1600, 60); delay(100);
  sound(1600, 60); delay(100);
}
void modeTone() {
  sound(2000, 120); delay(150);
  sound(2500, 160); delay(300);
}
void errorTone() {
  sound(400, 150); delay(150);
  sound(300, 250); delay(250);
}
void scanTone() {
  sound(2000, 80); delay(100);
  sound(2600, 80); delay(100);
  sound(3200, 120); delay(150);
}

void triggerScanNotice() {
  lcd.clear();
  if (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print(" Connect to     ");
    lcd.setCursor(0, 1);
    lcd.print(" internet first ");
    errorTone();

    int barWidth = 16;
    int speed = 25;
    for (int i = 0; i <= barWidth; i++) {
      lcd.setCursor(0, 1);
      for (int j = 0; j < barWidth; j++) {
        if (j <= i) lcd.print("!");
        else lcd.print(" ");
      }
      delay(speed);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("   Scan using   ");
    lcd.setCursor(0, 1);
    lcd.print("external device!");
    scanTone();
    delay(300);

    int barWidth = 16;
    int speed = 30;
    for (int i = 0; i <= barWidth; i++) {
      lcd.setCursor(0, 0);
      for (int j = 0; j < barWidth; j++) {
        if (j <= i) lcd.print("#");
        else lcd.print(" ");
      }
      lcd.setCursor(0, 1);
      for (int j = 0; j < barWidth; j++) {
        if (j <= i) lcd.print("#");
        else lcd.print(" ");
      }
      delay(speed);
    }
  }
  delay(400);
  scrollWindow();
  redrawLCDLine();
  updateLCDMode();
}

char getHeldKey() {
  for (byte r = 0; r < ROWS; r++) {
      pinMode(rowPins[r], OUTPUT);
      digitalWrite(rowPins[r], LOW); 

      for (byte c = 0; c < COLS; c++) {
          pinMode(colPins[c], INPUT_PULLUP);
          if (digitalRead(colPins[c]) == LOW) {
              pinMode(rowPins[r], INPUT);
              return keys[r][c];
          }
      }
      pinMode(rowPins[r], INPUT);
  }
  return NO_KEY;
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  char key = getHeldKey();  
  unsigned long now = millis();

  if (vibActive && millis() - vibStart >= vibDuration) {
    digitalWrite(vibroPin, LOW);
    vibActive = false;
  }

  if (hashPending && (millis() - hashPressTime >= hashDebounceTime)) {
      hashPending = false;
      handleHashSinglePress();
      brailleBits = 0;
  }

  if (key != NO_KEY) {
      if (key != lastKey) {
          lastKey = key;
          keyPressTime = now;
          nextRepeatTime = now + holdDelay;
          handleKeyPress(key);

          if (key == '#') lastKey = NO_KEY;
      } else if (now >= nextRepeatTime) {
          if (key == '0' || key == 'D' || key == '7' || key == 'C') {
              handleKeyPress(key);
              nextRepeatTime = now + repeatRate;
          }
      }
  } else {
      lastKey = NO_KEY;
  }
}

void handleHashSinglePress() {
    String out = brailleToChar(brailleBits);

    if (out != "") {
        if (fullBuffer.length() + out.length() >= MAX_BUFFER) return;

        if (fullBuffer.length() == 0 || fullBuffer.endsWith(" ")) {
            contractionPrefix = brailleBits;
        }

        for (int i = 0; i < out.length(); ++i) {
            insertAtCursor(out[i], brailleBits);
        }
    }
}

void handleHashDoublePress() {
    insertAtCursor(' ', 0);
}

void showFeedbackMode() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (feedbackMode == VIBRATE_ONLY) {
    lcd.print("===   MUTE   ===");
  } 
  else if (feedbackMode == SOUND_ONLY) {
    lcd.print("===  SOUND   ===");
  }
  delay(2000);
  updateLCDMode();
  redrawLCDLine();
}

void handleKeyPress(char key) {
    switch(key) {
        case '*': 
          cycleMode(); 
          modeTone();
          break;

        case '2': brailleBits |= 1;  sound(C, 70); break; // dot 1
        case '5': brailleBits |= 2;  sound(D, 70); break; // dot 2
        case '8': brailleBits |= 4;  sound(E, 70); break; // dot 3
        case '3': brailleBits |= 8;  sound(F, 70); break; // dot 4
        case '6': brailleBits |= 16; sound(G, 70); break; // dot 5
        case '9': brailleBits |= 32; sound(A, 70); break; // dot 6

        case '4':
          feedbackMode = (FeedbackMode)((feedbackMode + 1) % 2);
          prefs.begin("settings", false);
          prefs.putUInt("fbmode", feedbackMode);
          prefs.end();

          if (feedbackMode == VIBRATE_ONLY) {
            vibrate(300, 200);
          } 
          else if (feedbackMode == SOUND_ONLY) {
            modeTone();
          }
          showFeedbackMode();
          break;

        case '0':
            sound(1800, 70);
            handleSpaceKeyDirect();
            insertSpaceAtCursor();
            if (currentMode == AUTO) currentMode = TEXT;
            capsLock = false;
            nextCapital = false;
            nextNumber = false;
            numberLock = false;
            nextSpecial = false;
            specialLock = false;
            contractionPrefix = 0;
            break;

        case '#': {
            enterTone();
            unsigned long now = millis();
            if (hashPending && (now - hashPressTime < hashDebounceTime)) {
                hashPending = false;
                handleHashDoublePress();
                brailleBits = 0;
                break;
            }
            hashPending = true;
            hashPressTime = now;
            break;
        }

        case 'D': sound(2100, 70); backspaceAtCursor(); break;
        case '7': sound(1800, 70); moveCursorLeft(); break;
        case 'C': sound(1800, 70); moveCursorRight(); break;

        case 'A': saveLineToEEPROM(); break;
        case 'B': loadLineFromEEPROM(); break;
        
        case '1': 
          triggerScanNotice();
          break;

        default: break;
    }
    lcd.setCursor(getLcdCursor(), 1);
}