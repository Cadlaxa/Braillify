#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <cstring>
#include <Wire.h>
#include <BleKeyboard.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

Preferences prefs;

// LCD setup (I2C)
#define I2C_ADDR 0x27  
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

// Web Server & Captive Portal DNS
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

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

// EEPROM settings
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

#define MAX_BUFFER 128
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

BleKeyboard bleKeyboard("Braillify", "Group ano", 100);
bool bluetoothEnabled = false;
bool wasConnected = false;

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
void sendChangesToBLE(const String& oldStr, const String& newStr);
void sound(int freq, int dur);
void vibrate(int durationMs, int strength);
void startupTone();
void saveTone();
void loadTone();
void btTone();
void btdTone();
void enterTone();
void modeTone();
void toggleBluetooth();
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

// Reverse lookup: converts ASCII char to 6-dot bitmask
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

// ---------- Core convert (braille cell -> output string) ----------
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
          if (s != "0") {
              out = String(s);
          }
          else {
              char n = brailleToNumber(pattern);
              if (n != '?') {
                  out = String(n);
              }
              else {
                  out = "?";
              }
          }
      }
  } else if (currentMode == SPECIAL) {
      String s = specialFromBraille(pattern);
      if (s != "0") {
          out = String(s);
      } else {
          out = brailleToText(pattern);
      }
  } else if (currentMode == NUMBER) {
      char n = brailleToNumber(pattern);
      if (n != '?') {
          out = String(n);
      } 
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

// ---------- Temp indicator helpers ----------
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

// ---------- Editor display & buffer helpers ----------
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
  if (bluetoothEnabled && bleKeyboard.isConnected()) {
      bleKeyboard.write(KEY_BACKSPACE);
  }
}

void insertSpaceAtCursor() {
  insertAtCursor(' ', 0);
  if (bluetoothEnabled && bleKeyboard.isConnected()) {
      bleKeyboard.print(' ');
  }
}

void moveCursorLeft() {
  if (cursorPos > 0) {
      cursorPos--;
      scrollWindow();
      redrawLCDLine();
      if (bluetoothEnabled && bleKeyboard.isConnected()) {
          bleKeyboard.write(KEY_LEFT_ARROW);
      }
  }
}
void moveCursorRight() {
  if (cursorPos < fullBuffer.length()) {
      cursorPos++;
      scrollWindow();
      redrawLCDLine();
      if (bluetoothEnabled && bleKeyboard.isConnected()) {
          bleKeyboard.write(KEY_RIGHT_ARROW);
      }
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
    case AUTO:    lcd.print("Mode: AUTO     "); break;
    case TEXT:    lcd.print("Mode: Text     "); break;
    case NUMBER:  lcd.print("Mode: Number   "); break;
    case SPECIAL: lcd.print("Mode: Special  "); break;
  }
  lcd.setCursor(15, 0);
  if (bluetoothEnabled) {
    lcd.write(byte(0));
  } else {
    lcd.print(" ");
  }
}

void saveLineToEEPROM() {
  int len = fullBuffer.length();
  for (int i = 0; i < MAX_CHARS; i++) {
    if (i < len)
        EEPROM.write(EEPROM_ADDR + i, fullBuffer[i]);
    else
        EEPROM.write(EEPROM_ADDR + i, 0);
  }
  EEPROM.write(EEPROM_ADDR + MAX_CHARS, len);
  EEPROM.commit();
  lcd.setCursor(0,0);
  lcd.print("Saved to MEMORY ");
  saveTone();
  int barWidth = 16;
  int speed = 10;
  for (int i = 0; i <= barWidth; i++) {
      lcd.setCursor(0, 0);
      for (int j = 0; j < barWidth; j++) {
          if (j <= i) lcd.print("#");
          else lcd.print(" ");
      }
      delay(speed);
  }
  updateLCDMode();
}

void loadLineFromEEPROM() {
    uint8_t marker = EEPROM.read(EEPROM_ADDR + MAX_CHARS);
    String loaded = "";
    for (int i = 0; i < MAX_CHARS; ++i) {
        char c = EEPROM.read(EEPROM_ADDR + i);
        if (c == 0xFF || c == 0x00) continue; 
        if (c >= 32 && c <= 126) {
            loaded += c;
        }
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

    if (bluetoothEnabled && bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_END);
        for (int i = 0; i < loaded.length(); i++) {
          char c = loaded[i];
          if (c >= 32 && c <= 126) {
              bleKeyboard.write(c);
              delay(35);
          }
      }
    }
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
    String oldStr = fullBuffer;
    char buf[MAX_BUFFER]; 
    int bufLen = fullBuffer.length();
    fullBuffer.toCharArray(buf, sizeof(buf));

    int oldLen = fullBuffer.length();

    setEndContraction(buf, bufLen, fullBufferBraille);
    setFrontContraction(buf, bufLen, contractionPrefix);

    fullBuffer = String(buf);
    if (::hasContraction) {
      cursorPos = fullBuffer.length();
    }
    ::hasContraction = false;

    sendChangesToBLE(oldStr, fullBuffer);
}

void sendChangesToBLE(const String& oldStr, const String& newStr) {
    if (!bluetoothEnabled || !bleKeyboard.isConnected()) return;

    int a = 0;
    while (a < oldStr.length() && a < newStr.length() && oldStr[a] == newStr[a]) {
        a++;
    }

    if (a == oldStr.length() && a == newStr.length()) return;

    int bOld = oldStr.length() - 1;
    int bNew = newStr.length() - 1;

    while (bOld >= a && bNew >= a && oldStr[bOld] == newStr[bNew]) {
        bOld--;
        bNew--;
    }

    int toDelete = (bOld >= a) ? (bOld - a + 1) : 0;
    String replacement = (bNew >= a) ? newStr.substring(a, bNew + 1) : "";

    for (int i = 0; i < toDelete; i++) {
        bleKeyboard.write(KEY_BACKSPACE);
        delay(3);
    }

    if (replacement.length() > 0) {
        bleKeyboard.print(replacement);
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

// -------------------------------------------------------------
// HTML UI WITH SEPARATE MODAL FOR FLASHCARD LEARNING MODE
// -------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>BRAILLIFY WEB CONTROLLER</title>
<style>
:root {
  --bg: #f0f4f8;
  --panel: #ffffff;
  --text: #111827;
  --border: #111827;
  --orange: #ff9800;
  --orange-dark: #e65100;
  --blue-light: #e3f2fd;
  --blue: #0288d1;
  --green: #4caf50;
  --red: #f44336;
  --lcd-bg: #e8f5e9;
  --lcd-text: #1b4332;
}
*{box-sizing:border-box;margin:0;padding:0;font-family:system-ui,-apple-system,sans-serif}
body{background:var(--bg);color:var(--text);display:flex;justify-content:center;align-items:center;min-height:100vh;padding:15px}
.container{width:100%;max-width:480px;background:var(--panel);border:3px solid var(--border);box-shadow:6px 6px 0px var(--border);border-radius:12px;padding:20px;position:relative}
.header{display:flex;justify-content:space-between;align-items:center;border-bottom:2px dashed var(--border);padding-bottom:12px;margin-bottom:16px}
.header-title h1{font-size:1.5rem;letter-spacing:1px;color:var(--text);font-weight:900;font-family:'Courier New',monospace}
.header-title .sub{font-size:.7rem;color:var(--orange-dark);font-weight:bold}

.modal-trigger-btn{background:#e8f5e9;border:2px solid var(--border);color:#2e7d32;font-size:0.75rem;font-weight:bold;padding:6px 10px;border-radius:8px;cursor:pointer;display:flex;align-items:center;gap:4px;box-shadow:2px 2px 0px var(--border);transition:all .05s}
.modal-trigger-btn:active{transform:translate(1px,1px);box-shadow:0px 0px 0px var(--border)}

/* Modal Backdrop & Outer Container */
.modal-overlay{position:fixed;top:0;left:0;width:100vw;height:100vh;background:rgba(17,24,39,0.5);backdrop-filter:blur(3px);display:none;justify-content:center;align-items:center;z-index:999;padding:15px}
.modal-overlay.active{display:flex}

.modal-content{width:100%;max-width:420px;background:#ffffff;border:3px solid var(--border);border-radius:12px;box-shadow:8px 8px 0px var(--border);padding:20px;position:relative;animation:popIn 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275)}
@keyframes popIn{from{transform:scale(0.8);opacity:0}to{transform:scale(1);opacity:1}}

.modal-header{display:flex;justify-content:space-between;align-items:center;border-bottom:2px solid var(--border);padding-bottom:10px;margin-bottom:14px}
.modal-title{font-size:1.1rem;font-weight:900;color:var(--blue);display:flex;align-items:center;gap:6px}
.modal-close-btn{background:none;border:none;font-size:1.5rem;font-weight:900;color:var(--text);cursor:pointer;line-height:1;padding:0 4px}

/* Flashcard Content inside Modal */
.fc-scores{display:flex;gap:10px;font-size:0.8rem;font-weight:800;font-family:'Courier New',monospace;margin-bottom:14px;justify-content:center}
.fc-score-badge{background:var(--blue-light);border:2px solid var(--border);padding:4px 10px;border-radius:6px}

.fc-card-display{display:flex;flex-direction:column;align-items:center;background:var(--bg);border:2px solid var(--border);border-radius:10px;padding:16px;margin-bottom:14px}
.fc-target-label{font-size:0.75rem;font-weight:800;color:#57606a;margin-bottom:4px}
.fc-target-char{font-size:3.5rem;font-weight:900;color:var(--orange-dark);font-family:'Courier New',monospace;line-height:1}

.fc-hint-box{margin-top:8px;font-size:0.9rem;font-weight:bold;color:var(--text);display:none}
.fc-hint-braille{font-size:2rem;line-height:1;color:var(--blue);vertical-align:middle}

.fc-status-msg{font-size:0.95rem;font-weight:900;text-align:center;min-height:1.4em;margin-bottom:14px}
.fc-status-msg.correct{color:var(--green)}
.fc-status-msg.wrong{color:var(--red)}

.fc-actions{display:flex;gap:8px}
.fc-btn{flex:1;background:#fff3e0;border:2px solid var(--border);color:var(--orange-dark);font-size:0.8rem;font-weight:800;padding:8px;border-radius:6px;cursor:pointer;box-shadow:2px 2px 0px var(--border);text-align:center;transition:all .05s}
.fc-btn:active{transform:translate(1px,1px);box-shadow:0px 0px 0px var(--border)}

/* Full Text Buffer Section */
.full-text-section{margin-bottom:16px}
.section-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:6px}
.section-title{font-size:.8rem;color:var(--orange-dark);font-weight:bold;text-transform:uppercase}
.action-btn-group{display:flex;gap:6px}
.action-btn{background:#fff3e0;border:2px solid var(--border);color:var(--orange-dark);font-size:0.75rem;font-weight:bold;padding:4px 10px;border-radius:6px;cursor:pointer;display:flex;align-items:center;gap:4px;box-shadow:1px 2px 0px var(--border);transition:all .05s}
.action-btn:active{transform:translate(1px,1px);box-shadow:0px 0px 0px var(--border)}
.action-btn svg{width:14px;height:14px;fill:currentColor}

/* Scrollable Container for Stacked Pairs */
.full-text-box{background:#ffffff;border:2px solid var(--border);border-radius:8px;padding:10px;min-height:64px;max-height:120px;overflow-y:auto;overflow-x:hidden;box-shadow:inset 0 0 4px rgba(0,0,0,0.06);display:flex;flex-wrap:wrap;align-content:flex-start;gap:2px}
.char-pair{display:inline-flex;flex-direction:column;align-items:center;justify-content:center;cursor:pointer;padding:2px 4px;border-radius:4px;user-select:none;min-width:18px;transition:background 0.1s}
.char-pair:hover{background:var(--blue-light)}
.char-pair.cursor-here{background:var(--orange);color:#ffffff;font-weight:bold;animation:blink 1s infinite}
@keyframes blink{0%,100%{opacity:1}50%{opacity:0.4}}

.char-pair .top-char{font-size:1.1rem;font-weight:900;font-family:'Courier New',monospace;line-height:1}
.char-pair .sub-braille{font-size:0.85rem;line-height:1;margin-top:2px;opacity:0.85;font-family:sans-serif}

/* LCD Display Box */
.lcd-frame{background:var(--lcd-bg);border:3px solid var(--border);border-radius:8px;padding:12px 16px;margin-bottom:16px;box-shadow:inset 0 0 5px rgba(0,0,0,0.08)}
.lcd-line{font-size:1.2rem;color:var(--lcd-text);letter-spacing:2px;white-space:pre;font-weight:bold;font-family:'Courier New',monospace;min-height:1.4em}

/* Visual Cell & Transformed Text Banner */
.status-card{display:flex;align-items:center;justify-content:space-between;background:var(--blue-light);border:2px solid var(--border);padding:14px;border-radius:8px;margin-bottom:16px}
.cell-preview-box{display:flex;align-items:center;gap:16px}
.braille-pattern-cell{display:grid;grid-template-columns:repeat(2,16px);grid-gap:8px;background:#ffffff;border:2px solid var(--border);padding:8px;border-radius:6px}
.v-dot{width:16px;height:16px;border-radius:50%;background:#d0d7de;border:1.5px solid var(--border);transition:all .15s}
.v-dot.active{background:var(--green);box-shadow:0 0 6px rgba(76,175,80,0.6)}

.char-output-box{display:flex;flex-direction:column;align-items:flex-start}
.char-label{font-size:0.7rem;font-weight:bold;color:var(--text);text-transform:uppercase;letter-spacing:1px}
.char-val{font-size:1.8rem;font-weight:900;color:var(--orange-dark);font-family:'Courier New',monospace}

.bit-badge{display:flex;flex-direction:column;align-items:flex-end;font-size:0.75rem;font-weight:bold;font-family:'Courier New',monospace}
.bit-badge span{color:var(--blue)}

/* Keypad Grid with SVG Icons */
.keypad-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}
.key-btn{background:#ffffff;border:2px solid var(--border);color:var(--text);height:58px;display:flex;flex-direction:column;align-items:center;justify-content:center;border-radius:8px;cursor:pointer;box-shadow:2px 3px 0px var(--border);transition:all .05s;user-select:none;-webkit-touch-callout:none}
.key-btn:active{transform:translate(2px,2px);box-shadow:0px 0px 0px var(--border);background:var(--blue-light)}
.key-btn.cmd{background:#fff3e0;border-color:var(--border);color:var(--orange-dark)}
.key-btn.cmd:active{background:var(--orange);color:#ffffff}
.key-btn svg{width:22px;height:22px;fill:currentColor}
.key-label{font-size:0.65rem;font-weight:800;margin-top:2px;font-family:'Courier New',monospace}
.dot-key-num{font-size:1.1rem;font-weight:900;font-family:'Courier New',monospace}
.dot-key-sub{font-size:0.6rem;color:#57606a;font-weight:700}
.footer{text-align:center;font-size:.7rem;font-weight:bold;color:#57606a;margin-top:14px;border-top:2px solid var(--border);padding-top:8px}
</style></head>
<body>

<div class="container">
  <div class="header">
    <div class="header-title">
      <h1>BRAILLIFY</h1>
      <div class="sub">HARDWARE TRANSLATOR</div>
    </div>
    <button class="modal-trigger-btn" onclick="openFlashcardModal()">
      <span>🎓 Flashcards</span>
    </button>
  </div>

  <!-- Full Text Buffer Section with Subscript Braille & Buttons -->
  <div class="full-text-section">
    <div class="section-header">
      <div class="section-title">> Transformed Full Text</div>
      <div class="action-btn-group">
        <button class="action-btn" id="tts-btn" onclick="speakFullText()" title="Read Text Aloud">
          <svg viewBox="0 0 24 24"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>
          <span id="tts-btn-text">Speak</span>
        </button>
        <button class="action-btn" id="copy-btn" onclick="copyFullText()" title="Copy to Clipboard">
          <svg viewBox="0 0 24 24"><path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z"/></svg>
          <span id="copy-btn-text">Copy Text</span>
        </button>
      </div>
    </div>
    <div class="full-text-box" id="full-text-box"></div>
  </div>

  <!-- Physical 16x2 LCD Mirror -->
  <div class="lcd-frame">
    <div class="lcd-line" id="lcd-line0">Mode: AUTO     </div>
    <div class="lcd-line" id="lcd-line1">_               </div>
  </div>

  <!-- Visual Cell & Transformed Text -->
  <div class="status-card">
    <div class="cell-preview-box">
      <div class="braille-pattern-cell">
        <div class="v-dot" id="vdot1"></div>
        <div class="v-dot" id="vdot4"></div>
        <div class="v-dot" id="vdot2"></div>
        <div class="v-dot" id="vdot5"></div>
        <div class="v-dot" id="vdot3"></div>
        <div class="v-dot" id="vdot6"></div>
      </div>
      <div class="char-output-box">
        <div class="char-label">Cell Preview</div>
        <div class="char-val" id="char-preview">~</div>
      </div>
    </div>
    <div class="bit-badge">
      <div>BITS: <span id="bit-val">0b000000</span></div>
      <div>DEC: <span id="dec-val">0</span></div>
    </div>
  </div>

  <!-- Keypad Grid -->
  <div class="keypad-grid">
    <!-- Row 1 -->
    <button class="key-btn cmd" data-key="1" title="Bluetooth">
      <svg viewBox="0 0 24 24"><path d="M17.71 7.71L12 2h-1v7.59L6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 11 14.41V22h1l5.71-5.71-4.3-4.29 4.3-4.29zM13 5.83l1.88 1.88L13 9.59V5.83zm1.88 12.34L13 20.05v-3.76l1.88 1.88z"/></svg>
      <span class="key-label">BT</span>
    </button>
    <button class="key-btn" data-key="2">
      <span class="dot-key-num">2</span>
      <span class="dot-key-sub">• Dot 1</span>
    </button>
    <button class="key-btn" data-key="3">
      <span class="dot-key-num">3</span>
      <span class="dot-key-sub">• Dot 4</span>
    </button>
    <button class="key-btn cmd" data-key="A" title="Save Memory">
      <svg viewBox="0 0 24 24"><path d="M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z"/></svg>
      <span class="key-label">SAVE</span>
    </button>

    <!-- Row 2 -->
    <button class="key-btn cmd" data-key="4" title="Mute Toggle">
      <svg viewBox="0 0 24 24"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>
      <span class="key-label">MUTE</span>
    </button>
    <button class="key-btn" data-key="5">
      <span class="dot-key-num">5</span>
      <span class="dot-key-sub">• Dot 2</span>
    </button>
    <button class="key-btn" data-key="6">
      <span class="dot-key-num">6</span>
      <span class="dot-key-sub">• Dot 5</span>
    </button>
    <button class="key-btn cmd" data-key="B" title="Load Memory">
      <svg viewBox="0 0 24 24"><path d="M20 6h-8l-2-2H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2zm0 12H4V8h16v10z"/></svg>
      <span class="key-label">LOAD</span>
    </button>

    <!-- Row 3 -->
    <button class="key-btn cmd" data-key="7" title="Cursor Left">
      <svg viewBox="0 0 24 24"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"/></svg>
      <span class="key-label">LEFT</span>
    </button>
    <button class="key-btn" data-key="8">
      <span class="dot-key-num">8</span>
      <span class="dot-key-sub">• Dot 3</span>
    </button>
    <button class="key-btn" data-key="9">
      <span class="dot-key-num">9</span>
      <span class="dot-key-sub">• Dot 6</span>
    </button>
    <button class="key-btn cmd" data-key="C" title="Cursor Right">
      <svg viewBox="0 0 24 24"><path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z"/></svg>
      <span class="key-label">RIGHT</span>
    </button>

    <!-- Row 4 -->
    <button class="key-btn cmd" data-key="*" title="Cycle Mode">
      <svg viewBox="0 0 24 24"><path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-.25 1.97-.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01.25-1.97.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z"/></svg>
      <span class="key-label">MODE</span>
    </button>
    <button class="key-btn" data-key="0" title="Space">
      <svg viewBox="0 0 24 24"><path d="M18 9v4H6V9H4v6h16V9h-2z"/></svg>
      <span class="key-label">SPACE</span>
    </button>
    <button class="key-btn cmd" data-key="#" title="Enter / Confirm Cell">
      <svg viewBox="0 0 24 24"><path d="M19 7v4H5.83l3.58-3.59L8 6l-6 6 6 6 1.41-1.41L5.83 13H21V7h-2z"/></svg>
      <span class="key-label">ENTER</span>
    </button>
    <button class="key-btn cmd" data-key="D" title="Backspace / Delete">
      <svg viewBox="0 0 24 24"><path d="M22 3H7c-.69 0-1.23.35-1.59.88L0 12l5.41 8.11c.36.53.9.89 1.59.89h15c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H7.07L2.4 12l4.66-7H22v14zm-11.59-2L14 13.41 17.59 17 19 15.59 15.41 12 19 8.41 17.59 7 14 10.59 10.41 7 9 8.41 12.59 12 9 15.59z"/></svg>
      <span class="key-label">DEL</span>
    </button>
  </div>

  <div class="footer">ESP32 BRAILLIFY // AP: Braillify // 192.168.4.1</div>
</div>

<!-- Interactive Flashcard Learning Modal -->
<div class="modal-overlay" id="flashcard-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">🎓 Flashcard Practice</div>
      <button class="modal-close-btn" onclick="closeFlashcardModal()">&times;</button>
    </div>
    
    <div class="fc-scores">
      <div class="fc-score-badge">SCORE: <span id="fc-score">0</span></div>
      <div class="fc-score-badge">STREAK: <span id="fc-streak">0</span></div>
    </div>

    <div class="fc-card-display">
      <div class="fc-target-label">TYPE THIS CHARACTER</div>
      <div class="fc-target-char" id="fc-target">A</div>
      <div class="fc-hint-box" id="fc-hint">Hint: <span class="fc-hint-braille" id="fc-hint-braille">⠁</span></div>
    </div>

    <div class="fc-status-msg" id="fc-status">Set dots on keypad & press ENTER (#)</div>

    <div class="fc-actions">
      <button class="fc-btn" onclick="toggleFlashcardHint()">Toggle Hint</button>
      <button class="fc-btn" onclick="nextFlashcardTarget()">Skip / Next</button>
    </div>
  </div>
</div>

<script>
let currentBits=0;
let currentFullText="";
let currentCursorPos=0;
let holdTimer=null;
let repeatInterval=null;

// Flashcard State
const fcAlphabet = [
  {ch:'a', bits:1},  {ch:'b', bits:3},  {ch:'c', bits:9},  {ch:'d', bits:25}, {ch:'e', bits:17},
  {ch:'f', bits:11}, {ch:'g', bits:27}, {ch:'h', bits:19}, {ch:'i', bits:10}, {ch:'j', bits:26},
  {ch:'k', bits:5},  {ch:'l', bits:7},  {ch:'m', bits:13}, {ch:'n', bits:29}, {ch:'o', bits:21},
  {ch:'p', bits:15}, {ch:'q', bits:31}, {ch:'r', bits:23}, {ch:'s', bits:14}, {ch:'t', bits:30},
  {ch:'u', bits:37}, {ch:'v', bits:39}, {ch:'w', bits:58}, {ch:'x', bits:45}, {ch:'y', bits:61}, {ch:'z', bits:53}
];

let fcTargetIndex = 0;
let fcScore = 0;
let fcStreak = 0;
let fcShowHint = false;
let lastEvaluatedTextLen = 0;

function openFlashcardModal(){
  document.getElementById('flashcard-modal').classList.add('active');
}

function closeFlashcardModal(){
  document.getElementById('flashcard-modal').classList.remove('active');
}

function handleModalBackdropClick(e){
  if(e.target.id === 'flashcard-modal'){
    closeFlashcardModal();
  }
}

function nextFlashcardTarget(){
  let nextIdx;
  do {
    nextIdx = Math.floor(Math.random() * fcAlphabet.length);
  } while (nextIdx === fcTargetIndex);
  
  fcTargetIndex = nextIdx;
  const targetObj = fcAlphabet[fcTargetIndex];
  
  document.getElementById('fc-target').innerText = targetObj.ch.toUpperCase();
  document.getElementById('fc-hint-braille').innerText = String.fromCharCode(0x2800 + targetObj.bits);
  
  const statusEl = document.getElementById('fc-status');
  statusEl.className = 'fc-status-msg';
  statusEl.innerText = 'Set dots on keypad & press ENTER (#)';
}

function toggleFlashcardHint(){
  fcShowHint = !fcShowHint;
  document.getElementById('fc-hint').style.display = fcShowHint ? 'block' : 'none';
}

function evaluateFlashcardAnswer(producedChar){
  if(!producedChar || producedChar.trim() === '') return;
  const targetObj = fcAlphabet[fcTargetIndex];
  const statusEl = document.getElementById('fc-status');
  
  if(producedChar.toLowerCase() === targetObj.ch){
    fcScore += 10;
    fcStreak += 1;
    statusEl.className = 'fc-status-msg correct';
    statusEl.innerText = '🎉 Correct! (+10 pts)';
    document.getElementById('fc-score').innerText = fcScore;
    document.getElementById('fc-streak').innerText = fcStreak;
    setTimeout(nextFlashcardTarget, 1000);
  } else {
    fcStreak = 0;
    statusEl.className = 'fc-status-msg wrong';
    statusEl.innerText = '❌ Incorrect! Try again.';
    document.getElementById('fc-streak').innerText = fcStreak;
  }
}

function updateDotsUI(){
  const dots=[1,2,4,8,16,32];
  const btnMap=[1,2,3,4,5,6];
  dots.forEach((bit,idx)=>{
    let dotNum=btnMap[idx];
    let el=document.getElementById('vdot'+dotNum);
    if(el){
      (bit & currentBits)?el.classList.add('active'):el.classList.remove('active');
    }
  });
  document.getElementById('bit-val').innerText='0b'+currentBits.toString(2).padStart(6,'0');
  document.getElementById('dec-val').innerText=currentBits;
}

function renderFullText(text, pos, brailleArr){
  currentFullText = text;
  currentCursorPos = pos;
  const container = document.getElementById('full-text-box');
  container.innerHTML = '';

  const len = text.length;
  for(let i = 0; i <= len; i++){
    const pairDiv = document.createElement('div');
    pairDiv.className = 'char-pair';
    if(i === pos){
      pairDiv.classList.add('cursor-here');
    }

    const topChar = document.createElement('div');
    topChar.className = 'top-char';
    
    const subBraille = document.createElement('div');
    subBraille.className = 'sub-braille';

    if(i < len){
      let ch = text[i];
      topChar.innerText = (ch === ' ') ? '\u00A0' : ch;
      
      let bits = (brailleArr && brailleArr[i] !== undefined) ? brailleArr[i] : 0;
      subBraille.innerText = bits > 0 ? String.fromCharCode(0x2800 + bits) : '\u2800';
    } else {
      topChar.innerText = '\u00A0';
      subBraille.innerText = '\u2800';
    }

    pairDiv.appendChild(topChar);
    pairDiv.appendChild(subBraille);

    pairDiv.onclick = () => setCursorPos(i);
    container.appendChild(pairDiv);
  }
}

function setCursorPos(idx){
  fetch('/api/setcursor?pos=' + idx)
    .then(r => r.json())
    .then(d => updateState(d));
}

function sendKey(k){
  fetch('/api/key?k=' + encodeURIComponent(k))
    .then(r => r.json())
    .then(d => updateState(d));
}

function startKeyHold(k){
  sendKey(k);
  if(['0','D','7','C'].includes(k)){
    holdTimer = setTimeout(() => {
      repeatInterval = setInterval(() => sendKey(k), 120);
    }, 600);
  }
}

function stopKeyHold(){
  if(holdTimer){ clearTimeout(holdTimer); holdTimer = null; }
  if(repeatInterval){ clearInterval(repeatInterval); repeatInterval = null; }
}

document.querySelectorAll('.key-btn').forEach(btn => {
  const k = btn.getAttribute('data-key');
  if(!k) return;

  btn.addEventListener('mousedown', (e) => { e.preventDefault(); startKeyHold(k); });
  btn.addEventListener('mouseup', stopKeyHold);
  btn.addEventListener('mouseleave', stopKeyHold);

  btn.addEventListener('touchstart', (e) => { e.preventDefault(); startKeyHold(k); });
  btn.addEventListener('touchend', stopKeyHold);
  btn.addEventListener('touchcancel', stopKeyHold);
});

function openInExternalBrowser(){
  const url = 'http://192.168.4.1';
  const isAndroid = /Android/i.test(navigator.userAgent);
  if (isAndroid) {
    window.location.href = 'intent://192.168.4.1#Intent;scheme=http;action=android.intent.action.VIEW;end;';
  } else {
    window.open(url, '_system') || (window.location.href = url);
  }
}

function speakFullText(){
  if(!currentFullText || currentFullText.trim() === '') return;
  if('speechSynthesis' in window){
    window.speechSynthesis.cancel();
    const utterance = new SpeechSynthesisUtterance(currentFullText);
    utterance.rate = 0.95;
    
    const btnLabel = document.getElementById('tts-btn-text');
    btnLabel.innerText = 'Speaking...';
    utterance.onend = () => { btnLabel.innerText = 'Speak'; };
    utterance.onerror = () => { btnLabel.innerText = 'Speak'; };
    
    window.speechSynthesis.speak(utterance);
  } else {
    openInExternalBrowser();
  }
}

function copyFullText(){
  const text = currentFullText;
  if(navigator.clipboard && navigator.clipboard.writeText){
    navigator.clipboard.writeText(text).then(() => flashCopyStatus());
  } else {
    const ta = document.createElement('textarea');
    ta.value = text;
    document.body.appendChild(ta);
    ta.select();
    document.execCommand('copy');
    document.body.removeChild(ta);
    flashCopyStatus();
  }
}

function flashCopyStatus(){
  const label = document.getElementById('copy-btn-text');
  label.innerText = 'Copied!';
  setTimeout(() => label.innerText = 'Copy Text', 1500);
}

function updateState(d){
  currentBits = d.bits;
  updateDotsUI();
  document.getElementById('lcd-line0').innerText = d.lcd0;
  document.getElementById('lcd-line1').innerText = d.lcd1;
  document.getElementById('char-preview').innerText = d.preview || '~';
  renderFullText(d.fullText || "", d.cursor || 0, d.brailleArray || []);

  if (d.fullText.length > lastEvaluatedTextLen) {
    let lastChar = d.fullText.charAt(d.fullText.length - 1);
    evaluateFlashcardAnswer(lastChar);
    lastEvaluatedTextLen = d.fullText.length;
  } else if (d.fullText.length < lastEvaluatedTextLen) {
    lastEvaluatedTextLen = d.fullText.length;
  }
}

function pollState(){
  fetch('/api/state')
    .then(r => r.json())
    .then(d => updateState(d))
    .catch(e => 0);
}

nextFlashcardTarget();
setInterval(pollState, 400);
pollState();
</script></body></html>
)rawliteral";

// ---------- Web Server Endpoints ----------
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void getSystemStateJson(String &jsonOut) {
  String line0 = "";
  switch(currentMode) {
    case AUTO:    line0 = "Mode: AUTO     "; break;
    case TEXT:    line0 = "Mode: Text     "; break;
    case NUMBER:  line0 = "Mode: Number   "; break;
    case SPECIAL: line0 = "Mode: Special  "; break;
  }
  if (bluetoothEnabled) line0.setCharAt(15, '*');

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

  jsonOut = "{";
  jsonOut += "\"bits\":" + String(brailleBits) + ",";
  jsonOut += "\"mode\":" + String((int)currentMode) + ",";
  jsonOut += "\"cursor\":" + String(cursorPos) + ",";
  jsonOut += "\"preview\":\"" + preview + "\",";
  jsonOut += "\"lcd0\":\"" + line0 + "\",";
  jsonOut += "\"lcd1\":\"" + line1 + "\",";
  jsonOut += "\"fullText\":\"" + safeFullText + "\",";
  jsonOut += "\"brailleArray\":" + jsonBrailleArray;
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
  EEPROM.begin(512);
  prefs.begin("settings", false);
  feedbackMode = (FeedbackMode)prefs.getUInt("fbmode", 0);
  prefs.end();

  WiFi.softAP("Braillify", "");
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/api/state", handleApiState);
  server.on("/api/key", handleApiKey);
  server.on("/api/setcursor", handleApiSetCursor);
  server.onNotFound(handleRoot);
  server.begin();

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
void btTone() {
  sound(2400, 60); delay(200);
  sound(2800, 60); delay(200);
  sound(3200, 80); delay(300);
}
void btdTone() {
  sound(3200, 60); delay(200);
  sound(2800, 60); delay(200);
  sound(2400, 60); delay(300);
}
void enterTone() {
  sound(1600, 60); delay(100);
  sound(1600, 60); delay(100);
}
void modeTone() {
  sound(2000, 120); delay(150);
  sound(2500, 160); delay(300);
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
 
  if (bluetoothEnabled) {
      if (bleKeyboard.isConnected()) {
          if (!wasConnected) {
              lcd.setCursor(0,0);
              lcd.print("Host Connected! ");
              btTone();
              delay(2000);
              scrollWindow();
              redrawLCDLine();
              updateLCDMode();
          }
          wasConnected = true;
      } else {
          if (wasConnected) {
              lcd.setCursor(0,0);
              lcd.print("Disconnected!    "); 
              btdTone();
              delay(2000);
              scrollWindow();
              redrawLCDLine();
              updateLCDMode();
          }
          wasConnected = false;
      }
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

        if (bluetoothEnabled && bleKeyboard.isConnected()) {
            bleKeyboard.print(out);
        }
    }
}

void handleHashDoublePress() {
    insertAtCursor(' ', 0);
    if (bluetoothEnabled && bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_RETURN);
    }
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

void toggleBluetooth() {
    if (!bluetoothEnabled) {
        bleKeyboard.begin();
        bluetoothEnabled = true;
        lcd.setCursor(0,0);
        lcd.print("BT HID enabled!");
        startupTone();
        int barWidth = 16;
        int speed = 15;
        for (int i = 0; i <= barWidth; i++) {
            lcd.setCursor(0, 1);
            for (int j = 0; j < barWidth; j++) {
                if (j <= i) lcd.print("#");
                else lcd.print(" ");
            }
            delay(speed);
        }
        scrollWindow();
        redrawLCDLine();

        unsigned long startTime = millis();
        lcd.setCursor(0,0);
        lcd.print("Waiting for host");

        while (!bleKeyboard.isConnected()) {
            dnsServer.processNextRequest();
            server.handleClient();
            unsigned long elapsed = (millis() - startTime) / 1000;

            lcd.setCursor(0,1);
            lcd.print(elapsed);
            lcd.print(" sec ");

            if (elapsed >= 45) {
                bleKeyboard.end();
                bluetoothEnabled = false;

                lcd.setCursor(0,0);
                lcd.print("BT Timeout       ");
                lcd.setCursor(0,1);
                lcd.print("Turning off...   ");
                btdTone();
                delay(2000);
                btStop();

                scrollWindow();
                redrawLCDLine();
                updateLCDMode();
                return;
            }
            delay(50);
        }
        scrollWindow();
        redrawLCDLine();
        updateLCDMode();
    } else {
        bluetoothEnabled = false;
        btStop();
        lcd.setCursor(0,0);
        lcd.print("BT HID disabled!");
        btdTone();
        int barWidth = 16;
        int speed = 15;
        for (int i = 0; i <= barWidth; i++) {
            lcd.setCursor(0, 1);
            for (int j = 0; j < barWidth; j++) {
                if (j <= i) lcd.print("#");
                else lcd.print(" ");
            }
            delay(speed);
        }
        scrollWindow();
        redrawLCDLine();
        updateLCDMode();
        delay(100);
        BleKeyboard bleKeyboard("Braillify", "Maker", 100);
    }
}

// Keypress handling: Works for physical keypad AND web API clicks
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
        case '1': sound(1800, 70); toggleBluetooth(); break;

        default: break;
    }
    lcd.setCursor(getLcdCursor(), 1);
}