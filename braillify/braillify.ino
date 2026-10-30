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
#include <vector>
#define FIRMWARE_VERSION "1.0.5.1"

Preferences prefs;
uint32_t bootId = 0;

// LCD setup (I2C)
#define I2C_ADDR 0x27  
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

#define LCD_BACKLIGHT_PIN 18 // PWM Backlight Control Pin (Optional Header Hookup)
int lcdBrightness = 100;     // 0 - 100%
int soundVolume = 100;       // 0 - 100%
int ttsMode = 1;             // 0: OFF, 1: CHAR_AND_WORD, 2: WORD_ONLY, 3: FULL_TEXT

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

bool soundActive = false;
unsigned long soundStart = 0;
unsigned int soundDuration = 0;

FeedbackMode feedbackMode = SOUND_ONLY;
bool vibrationEnabled = true;
bool lastButtonState = HIGH;
unsigned long lastBeep = 0;
const unsigned long debounceMillis = 35;

// Storage settings
const int MAX_CHARS = 1024;
int loadingProgress = 0;
unsigned long lastUpdate = 0;

// Braille / editor state & text selection
byte brailleBits = 0;
int cursorPos = 0;
int selectStart = 0;
int selectEnd = 0;
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
void applyBrightness(int percent);
byte charToBraille(char c);

// Text Selection Helpers
bool hasSelection() {
  return selectStart != selectEnd && selectStart >= 0 && selectEnd <= fullBuffer.length() && selectStart < selectEnd;
}

void clearSelection() {
  selectStart = cursorPos;
  selectEnd = cursorPos;
}

void deleteSelection() {
  if (!hasSelection()) return;
  int start = min(selectStart, selectEnd);
  int end = max(selectStart, selectEnd);
  fullBuffer = fullBuffer.substring(0, start) + fullBuffer.substring(end);
  for (int i = start; i < fullBuffer.length(); i++) {
    fullBufferBraille[i] = fullBufferBraille[i + (end - start)];
  }
  cursorPos = start;
  clearSelection();
  scrollWindow();
  redrawLCDLine();
}

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

String escapeJsonString(String src) {
  src.replace("\\", "\\\\");
  src.replace("\"", "\\\"");
  src.replace("\r", "");
  src.replace("\n", "");
  return src;
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

String brailleToChar(byte bits, bool commitState = true) {
  byte pattern = bits;
  String out = "";

  // Dot 6 (Capitalization Indicator)
  if (pattern == 32) {
    if (!commitState) return "^"; 
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
  if (commitState && pattern != 32) dot6PressedOnce = false;

  // Dot 3-4-5-6 (Number Indicator)
  if (pattern == 60) {
    if (!commitState) return "#";
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
  if (commitState && pattern != 60) numPressedOnce = false;

  // Special Indicator
  if (pattern == 56) {
    if (!commitState) return "$";
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
  if (commitState && pattern != 56) specialPressedOnce = false;

  // Text Indicator
  if (pattern == 48) {
    if (!commitState) return ">"; // Pure UI preview only
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
  if (commitState && pattern != 48) textPressedOnce = false;

  Mode evalMode = currentMode;
  if (evalMode == AUTO && pattern != 0) {
    if (brailleToText(pattern) != "~") evalMode = TEXT;
    else if (specialFromBraille(pattern) != "0") evalMode = SPECIAL;
    else if (brailleToNumber(pattern) != '?') evalMode = NUMBER;
  }

  if (commitState) {
    if (nextNumber || numberLock) { currentMode = NUMBER; nextNumber = false; clearTempIndicator(); }
    if (nextSpecial || specialLock) { currentMode = SPECIAL; nextSpecial = false; clearTempIndicator(); }
    if (nextText || textLock) { currentMode = TEXT; nextText = false; clearTempIndicator(); }
  }

  Mode activeMode = (currentMode == AUTO) ? evalMode : currentMode;

  if (activeMode == TEXT) {
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
  } else if (activeMode == SPECIAL) {
      String s = specialFromBraille(pattern);
      if (s != "0") out = String(s);
      else out = brailleToText(pattern);
  } else if (activeMode == NUMBER) {
      char n = brailleToNumber(pattern);
      if (n != '?') out = String(n);
      else {
          String s = specialFromBraille(pattern);
          if (s != "0") out = String(s);
          else out = brailleToText(pattern);
      }
  }

  if ((nextCapital || capsLock) && out.length() > 0) {
    out.toUpperCase();
    if (commitState) {
      nextCapital = false;
      clearTempIndicator();
    }
  }

  if (commitState && (nextCapital || capsLock) && pattern == 4) {
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
  if (hasSelection()) {
    int selLen = selectEnd - selectStart;
    String selSub = fullBuffer.substring(selectStart, selectEnd);
    lcd.setCursor(0, 0);
    String l0 = "Sel(" + String(selLen) + "):" + selSub;
    while (l0.length() < 16) l0 += " ";
    if (l0.length() > 16) l0 = l0.substring(0, 16);
    lcd.print(l0);

    lcd.setCursor(0, 1);
    String visible = "";
    for (int i = windowStart; i < windowStart + 16 && i <= fullBuffer.length(); i++) {
      if (i == selectStart) visible += "[";
      if (i == selectEnd) visible += "]";
      if (i < fullBuffer.length()) visible += fullBuffer[i];
    }
    while (visible.length() < 16) visible += " ";
    if (visible.length() > 16) visible = visible.substring(0, 16);
    lcd.print(visible);
  } else {
    updateLCDMode();
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
}

void insertAtCursor(char c, byte brailleCell) {
    if (hasSelection()) {
      deleteSelection();
    }
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
    clearSelection();
    scrollWindow();
    redrawLCDLine();
}

void backspaceAtCursor() {
  if (hasSelection()) {
    deleteSelection();
    return;
  }
  if (cursorPos == 0) return;
  fullBuffer = fullBuffer.substring(0, cursorPos - 1) + fullBuffer.substring(cursorPos);
  for (int i = cursorPos - 1; i < fullBuffer.length(); i++) {
    fullBufferBraille[i] = fullBufferBraille[i + 1];
  }
  cursorPos--;
  if (windowStart > 0 && cursorPos < windowStart) windowStart--;
  clearSelection();
  scrollWindow();
  redrawLCDLine();
}

void insertSpaceAtCursor() {
  insertAtCursor(' ', 0);
}

void moveCursorLeft() {
  if (cursorPos > 0) {
      cursorPos--;
      clearSelection();
      scrollWindow();
      redrawLCDLine();
  }
}

void moveCursorRight() {
  if (cursorPos < fullBuffer.length()) {
      cursorPos++;
      clearSelection();
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
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed! Attempting force format...");
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
        smartDelay(speed);
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
        smartDelay(1000);
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

    if (fullBuffer.length() > 0 && cursorPos > 0 && fullBuffer[cursorPos - 1] != ' ') {
        insertSpaceAtCursor();
    }

    clearSelection();

    lcd.setCursor(0, 0);
    lcd.print("Appended MEMORY ");
    loadTone();
    smartDelay(200);

    int barWidth = 16;
    int speed = 10;
    for (int i = 0; i <= barWidth; i++) {
        lcd.setCursor(0, 1);
        for (int j = 0; j < barWidth; j++) {
            if (j <= i) lcd.print("#");
            else lcd.print(" ");
        }
        smartDelay(speed);
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

void handleSpaceKeyDirect() {
    if (fullBuffer.length() == 0 || cursorPos == 0) return;

    int wordStart = cursorPos - 1;
    while (wordStart >= 0 && fullBuffer[wordStart] != ' ') {
        wordStart--;
    }
    wordStart++;

    if (cursorPos <= wordStart) return;

    String targetWord = fullBuffer.substring(wordStart, cursorPos);
    String lowerWord = targetWord;
    lowerWord.toLowerCase();

    int bestIndex = -1;

    for (int i = 0; i < contractionCount; i++) {
        String pattern = String(uebContractions[i].letters);
        pattern.trim();
        if (pattern.startsWith(">") || pattern.startsWith("-")) {
            pattern = pattern.substring(1);
        }

        if (lowerWord == pattern) {
            bestIndex = i;
            break;
        }
    }

    if (bestIndex >= 0) {
        String replacement = String(uebContractions[bestIndex].contraction);
        replacement.trim();

        bool allUpper = true;
        bool firstUpper = (targetWord.length() > 0 && targetWord[0] >= 'A' && targetWord[0] <= 'Z');
        for (int k = 0; k < targetWord.length(); k++) {
            if (!(targetWord[k] >= 'A' && targetWord[k] <= 'Z')) { allUpper = false; break; }
        }

        if (allUpper) {
            replacement.toUpperCase();
        } else if (firstUpper && replacement.length() > 0) {
            replacement[0] = toupper(replacement[0]);
        }

        fullBuffer = fullBuffer.substring(0, wordStart) + replacement + fullBuffer.substring(cursorPos);
        cursorPos = wordStart + replacement.length();

        for (int k = 0; k < replacement.length(); k++) {
            fullBufferBraille[wordStart + k] = charToBraille(replacement[k]);
        }

        ::hasContraction = true;
    }
}

void applyBrightness(int percent) {
  lcdBrightness = constrain(percent, 0, 100);
  int duty = map(lcdBrightness, 0, 100, 0, 255);
  ledcWrite(1, duty);
  if (lcdBrightness == 0) lcd.noBacklight();
  else lcd.backlight();
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
      smartDelay(speed);
  }

  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

struct WifiCred {
  String ssid;
  String pass;
};
std::vector<WifiCred> knownWifis;

void loadKnownWifis() {
  knownWifis.clear();
  if (!LittleFS.exists("/wifis.txt")) return;
  File f = LittleFS.open("/wifis.txt", FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int tabIdx = line.indexOf('\t');
    if (tabIdx != -1) {
      WifiCred w;
      w.ssid = line.substring(0, tabIdx);
      w.pass = line.substring(tabIdx + 1);
      knownWifis.push_back(w);
    }
  }
  f.close();
}

void saveKnownWifis() {
  File f = LittleFS.open("/wifis.txt", FILE_WRITE);
  if (!f) return;
  for (const auto& w : knownWifis) {
    f.print(w.ssid);
    f.print('\t');
    f.println(w.pass);
  }
  f.close();
}

void addOrUpdateWifi(String ssid, String pass) {
  bool found = false;
  for (auto& w : knownWifis) {
    if (w.ssid == ssid) {
      w.pass = pass;
      found = true;
      break;
    }
  }
  if (!found) {
    WifiCred w;
    w.ssid = ssid;
    w.pass = pass;
    knownWifis.push_back(w);
  }
  saveKnownWifis();
}

void forgetWifi(String ssid) {
  for (auto it = knownWifis.begin(); it != knownWifis.end(); ++it) {
    if (it->ssid == ssid) {
      knownWifis.erase(it);
      break;
    }
  }
  saveKnownWifis();
}

void autoConnectWifi() {
  loadKnownWifis();
  if (knownWifis.empty()) {
    wifiStatusMsg = "AP Mode Active (192.168.4.1)";
    return;
  }

  wifiSSID = knownWifis.back().ssid;
  wifiPass = knownWifis.back().pass;
  WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
  wifiStatusMsg = "Connecting to " + wifiSSID + "...";
}

// ---------- Web Server Endpoints ----------
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleApiSavedWifis() {
  loadKnownWifis();
  String json = "[";
  for (size_t i = 0; i < knownWifis.size(); ++i) {
    json += "{";
    json += "\"ssid\":\"" + escapeJsonString(knownWifis[i].ssid) + "\",";
    json += "\"pass\":\"" + escapeJsonString(knownWifis[i].pass) + "\"";
    json += "}";
    if (i < knownWifis.size() - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleApiForgetWifi() {
  if (server.hasArg("ssid")) {
    String targetSSID = server.arg("ssid");
    
    forgetWifi(targetSSID);

    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == targetSSID) {
      WiFi.disconnect(false, true);
      wifiStatusMsg = "AP Mode Active (192.168.4.1)";
    }

    server.send(200, "application/json", "{\"status\":\"Network forgotten\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"Missing SSID\"}");
  }
}

void handleApiConnect() {
  if (server.hasArg("ssid")) {
    wifiSSID = server.arg("ssid");
    wifiPass = server.hasArg("pass") ? server.arg("pass") : "";

    addOrUpdateWifi(wifiSSID, wifiPass);

    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    wifiStatusMsg = "Connecting to " + wifiSSID + "...";
  }

  String json = "{\"status\":\"" + wifiStatusMsg + "\"}";
  server.send(200, "application/json", json);
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

    clearSelection();

    for (int i = 0; i < fullBuffer.length() && i < MAX_BUFFER; i++) {
      fullBufferBraille[i] = charToBraille(fullBuffer[i]);
    }

    scrollWindow();
    redrawLCDLine();
  }
  handleApiState();
}

void handleApiSettings() {
  prefs.begin("settings", false);
  bool volumeUpdated = false;
  bool modeChanged = false;

  if (server.hasArg("brightness")) {
    lcdBrightness = server.arg("brightness").toInt();
    lcdBrightness = constrain(lcdBrightness, 0, 100);
    applyBrightness(lcdBrightness);
    prefs.putInt("bright", lcdBrightness);
  }

  if (server.hasArg("volume")) {
    int newVol = server.arg("volume").toInt();
    newVol = constrain(newVol, 0, 100);
    FeedbackMode oldMode = feedbackMode;

    if (newVol > 0) {
      soundVolume = newVol;
      feedbackMode = SOUND_ONLY;
    } else {
      feedbackMode = VIBRATE_ONLY;
    }

    if (oldMode != feedbackMode) {
      modeChanged = true;
      prefs.putUInt("fbmode", feedbackMode);
    }

    prefs.putInt("vol", soundVolume);
    volumeUpdated = true;
  }

  if (server.hasArg("ttsMode")) {
    ttsMode = server.arg("ttsMode").toInt();
    prefs.putInt("ttsMode", ttsMode);
  }

  prefs.end();

  if (modeChanged) {
    showFeedbackMode();
  } 
  else if (volumeUpdated && feedbackMode == SOUND_ONLY && soundVolume > 0) {
    sound(2200, 70);
    smartDelay(90);
    sound(2800, 100);
  }

  handleApiState();
}

void handleApiPower() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "off") {
      applyBrightness(0);
      lcd.noDisplay();
      lcd.noBacklight();
    } else if (state == "on") {
      lcd.display();
      lcd.backlight();
      applyBrightness(lcdBrightness > 0 ? lcdBrightness : 100);
      updateLCDMode();
      redrawLCDLine();
    }
  }
  handleApiState();
}

void handleApiReboot() {
  server.send(200, "application/json", "{\"status\":\"ESP32 is rebooting...\"}");
  smartDelay(1000);
  ESP.restart();
}

void getSystemStateJson(String &jsonOut) {
  jsonOut.reserve(2048);

  String line0 = "";
  if (hasSelection()) {
    int selLen = selectEnd - selectStart;
    String selSub = fullBuffer.substring(selectStart, selectEnd);
    line0 = "Sel(" + String(selLen) + "):" + selSub;
    while (line0.length() < 16) line0 += " ";
    if (line0.length() > 16) line0 = line0.substring(0, 16);
  } else {
    switch(currentMode) {
      case AUTO:    line0 = "Mode: AUTO     "; break;
      case TEXT:    line0 = "Mode: Text     "; break;
      case NUMBER:  line0 = "Mode: Number   "; break;
      case SPECIAL: line0 = "Mode: Special  "; break;
    }
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

  String preview = brailleToChar(brailleBits, false);
  if (preview == "") preview = " ";

  String safeFullText = fullBuffer;
  safeFullText.replace("\\", "\\\\");
  safeFullText.replace("\"", "\\\"");

  String jsonBrailleArray = "[";
  jsonBrailleArray.reserve(1024);
  int len = fullBuffer.length();
  for(int i = 0; i < len; i++) {
    byte b = fullBufferBraille[i];
    if (b == 0 && fullBuffer[i] != ' ') {
      b = charToBraille(fullBuffer[i]);
    }
    jsonBrailleArray += String(b);
    if(i < len - 1) jsonBrailleArray += ",";
  }
  jsonBrailleArray += "]";

  if (WiFi.status() == WL_CONNECTED) {
    String currentSSID = WiFi.SSID();
    if (currentSSID.length() == 0) currentSSID = wifiSSID;
    wifiStatusMsg = "Connected to: " + currentSSID;
  }

  int reportedVolume = (feedbackMode == VIBRATE_ONLY) ? 0 : soundVolume;

  jsonOut = "{";
  jsonOut += "\"bootId\":" + String(bootId) + ",";
  jsonOut += "\"bits\":" + String(brailleBits) + ",";
  jsonOut += "\"mode\":" + String((int)currentMode) + ",";
  jsonOut += "\"cursor\":" + String(cursorPos) + ",";
  jsonOut += "\"selStart\":" + String(selectStart) + ",";
  jsonOut += "\"selEnd\":" + String(selectEnd) + ",";
  jsonOut += "\"ttsMode\":" + String(ttsMode) + ",";
  jsonOut += "\"preview\":\"" + preview + "\",";
  jsonOut += "\"lcd0\":\"" + line0 + "\",";
  jsonOut += "\"lcd1\":\"" + line1 + "\",";
  jsonOut += "\"fullText\":\"" + safeFullText + "\",";
  jsonOut += "\"brailleArray\":" + jsonBrailleArray + ",";
  jsonOut += "\"wifiStatus\":\"" + wifiStatusMsg + "\",";
  jsonOut += "\"brightness\":" + String(lcdBrightness) + ",";
  jsonOut += "\"volume\":" + String(reportedVolume) + ",";
  jsonOut += "\"feedbackMode\":" + String((int)feedbackMode);
  jsonOut += "}";
}

void handleApiState() {
  String json;
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
  if (server.hasArg("start") && server.hasArg("end")) {
    int s = server.arg("start").toInt();
    int e = server.arg("end").toInt();
    s = constrain(s, 0, fullBuffer.length());
    e = constrain(e, 0, fullBuffer.length());
    if (s <= e) {
      selectStart = s;
      selectEnd = e;
      cursorPos = e;
    } else {
      selectStart = e;
      selectEnd = s;
      cursorPos = s;
    }
    scrollWindow();
    redrawLCDLine();
  } else if (server.hasArg("pos")) {
    int newPos = server.arg("pos").toInt();
    if (newPos >= 0 && newPos <= fullBuffer.length()) {
      cursorPos = newPos;
      clearSelection();
      scrollWindow();
      redrawLCDLine();
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
    smartDelay(2000);
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
  client.setInsecure();

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

  if (latestVersion.length() > 0 && latestVersion == FIRMWARE_VERSION) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Already Up-to- ");
    lcd.setCursor(0, 1);
    lcd.print(" Date! (v" FIRMWARE_VERSION ") ");
    sound(2000, 100);
    smartDelay(2500);
    scrollWindow();
    redrawLCDLine();
    updateLCDMode();
    return;
  }

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
      smartDelay(3000);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" No New Version ");
      smartDelay(2000);
      break;

    case HTTP_UPDATE_OK:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Update Success!");
      lcd.setCursor(0, 1);
      lcd.print(" Rebooting...   ");
      saveTone();
      smartDelay(2000);
      ESP.restart();
      break;
  }
  scrollWindow();
  redrawLCDLine();
  updateLCDMode();
}

// ---------- ESP32 Setup & Loop ----------
void setup() {
  bootId = esp_random();
  Serial.begin(9600);
  
  // LEDC Channel 0 for Buzzer PWM Sound / Volume
  ledcSetup(0, 2000, 8);
  ledcAttachPin(buzzerPin, 0);

  // LEDC Channel 1 for LCD PWM Brightness
  ledcSetup(1, 5000, 8);
  ledcAttachPin(LCD_BACKLIGHT_PIN, 1);

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
  lcdBrightness = prefs.getInt("bright", 100);
  soundVolume = prefs.getInt("vol", 100);
  ttsMode = prefs.getInt("ttsMode", 1);
  prefs.end();

  applyBrightness(lcdBrightness);

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
  server.on("/api/wifi/saved", handleApiSavedWifis);
  server.on("/api/wifi/forget", handleApiForgetWifi);
  server.on("/api/connect", handleApiConnect);
  server.on("/api/settext", handleApiSetText);
  server.on("/api/settings", handleApiSettings);
  server.on("/api/power", handleApiPower);
  server.on("/api/reboot", handleApiReboot);
  autoConnectWifi();
  
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
    smartDelay(1000);
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

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    dnsServer.processNextRequest();
    server.handleClient();
    if (soundActive && millis() - soundStart >= soundDuration) {
      ledcWrite(0, 0);
      soundActive = false;
    }
    if (vibActive && millis() - vibStart >= vibDuration) {
      digitalWrite(vibroPin, LOW);
      vibActive = false;
    }
    yield();
  }
}

void sound(int freq, int dur) {
  unsigned long now = millis();
  if (now - lastBeep >= debounceMillis) {
    if (feedbackMode != VIBRATE_ONLY && soundVolume > 0) {
      float normalized = soundVolume / 100.0f;
      int duty = (int)(128.0f * pow(normalized, 2.8f));
      
      if (duty < 1) duty = 1;

      ledcWriteTone(0, freq);
      ledcWrite(0, duty);
      soundStart = now;
      soundDuration = dur;
      soundActive = true;
    }
    if (feedbackMode != SOUND_ONLY) vibrate(200, 230);
    lastBeep = now;
  }
}

void startupTone() {
  sound(1500, 120); smartDelay(150);
  sound(2000, 120); smartDelay(150);
  sound(2500, 160); smartDelay(200);
}
void saveTone() {
  sound(2000, 60); smartDelay(200);
  sound(2400, 60); smartDelay(200);
  sound(2800, 80); smartDelay(300);
}
void loadTone() {
  sound(2800, 60); smartDelay(200);
  sound(2400, 60); smartDelay(200);
  sound(2000, 80); smartDelay(300);
}
void enterTone() {
  sound(1600, 60); smartDelay(100);
  sound(1600, 60); smartDelay(100);
}
void modeTone() {
  sound(2000, 120); smartDelay(150);
  sound(2500, 160); smartDelay(300);
}
void errorTone() {
  sound(400, 150); smartDelay(150);
  sound(300, 250); smartDelay(250);
}
void scanTone() {
  sound(2000, 80); smartDelay(100);
  sound(2600, 80); smartDelay(100);
  sound(3200, 120); smartDelay(150);
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
      smartDelay(speed);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("   Scan using   ");
    lcd.setCursor(0, 1);
    lcd.print("external device!");
    scanTone();
    smartDelay(300);

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
      smartDelay(speed);
    }
  }
  smartDelay(400);
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

  if (soundActive && millis() - soundStart >= soundDuration) {
    ledcWrite(0, 0);
    soundActive = false;
  }

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
  smartDelay(1500);
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