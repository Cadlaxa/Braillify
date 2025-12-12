#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
//#include <Arduino.h>
#include <cstring>
#include <Wire.h>
#include <BleKeyboard.h>
#include <Preferences.h>
Preferences prefs;

// LCD setup (I2C)
#define I2C_ADDR 0x27  
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

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
const int EEPROM_ADDR = 0;       // starting address
const int MAX_CHARS = 1024;
int loadingProgress = 0;
unsigned long lastUpdate = 0;

// Braille / editor state
byte brailleBits = 0;
int cursorPos = 0;
String fullBuffer = "";      // full text buffer (can be longer than 16)
int windowStart = 0;         // left-most index shown on LCD
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
byte fullBufferBraille[MAX_BUFFER];  // parallel array to store braille cells
byte contractionPrefix = 0;

char lastKey = NO_KEY;
unsigned long keyPressTime = 0; // when key was first pressed
const unsigned long holdDelay = 800; // ms to wait before auto-repeat
const unsigned long repeatRate = 120; // ms between repeats
unsigned long nextRepeatTime = 0;
bool hashPending = false;
unsigned long hashPressTime = 0;
const unsigned long hashDebounceTime = 250;

BleKeyboard bleKeyboard("Braillify", "Group ano", 100);
bool bluetoothEnabled = false;
bool wasConnected = false;

enum Mode { AUTO, TEXT, NUMBER, SPECIAL };
Mode currentMode = AUTO;

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

// ---------- Core convert (braille cell -> output string) ----------
String brailleToChar(byte bits) {
  byte pattern = bits;
  String out = "";

  // CAPITALIZATION DOT6
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

  // NUMBER TRIGGER (dot3+4+5+6) -> 0b111100
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

  // SPECIAL CHARACTER TRIGGER (dot4+5+6) -> 111000
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

  // TEXT TRIGGER (dot5+6) -> 110000
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

  // AUTO detection
  if (currentMode == AUTO && pattern != 0) {
    if (brailleToText(pattern) != "~") currentMode = TEXT;
    else if (specialFromBraille(pattern) != "0") currentMode = SPECIAL;
    else if (brailleToNumber(pattern) != '?') currentMode = NUMBER;
  }

  // Choose output based on mode
  if (currentMode == TEXT) {
      out = brailleToText(pattern);
      if (out == "~") {
          String s = specialFromBraille(pattern);
          if (s != "0") {
              out = String(s);
              currentMode = SPECIAL;  // switch to special
          }
          else {
              // Not special either → number mode
              char n = brailleToNumber(pattern);
              if (n != '?') {
                  out = String(n);
                  currentMode = NUMBER;
              }
              else {
                  out = "?"; // nothing matched at all
              }
          }
      }
  } else if (currentMode == SPECIAL) {
      String s = specialFromBraille(pattern);
      if (s != "0") {
          out = String(s);
      } else {
          out = brailleToText(pattern);
          currentMode = TEXT;  // special failed → back to text
      }
  } else if (currentMode == NUMBER) {
      char n = brailleToNumber(pattern);
      if (n != '?') {
          out = String(n);
      } 
      else {
          // Fallback to special
          String s = specialFromBraille(pattern);
          if (s != "") out = String(s);
          else out = brailleToText(pattern);
          // exit number mode after fallback
          currentMode = TEXT;
          numberLock = false;
          nextNumber = false;
          numPressedOnce = false;
      }
  }

  // Apply capitalization
  if ((nextCapital || capsLock) && out.length() > 0) {
    out.toUpperCase();
    nextCapital = false;
    clearTempIndicator();
  }

  if ((nextCapital || capsLock) && pattern == 4) {
    capsLock = false;
    nextCapital = false;
    clearTempIndicator();
    return "";   // termination sign
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

// Clear the temporary indicator (erase spaces where indicator was)
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
  // keep windowStart >= 0
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

// insert a char at cursorPos
void insertAtCursor(char c, byte brailleCell = 0) {
    if (fullBuffer.length() >= MAX_BUFFER) return;

    // Insert char into string
    fullBuffer = fullBuffer.substring(0, cursorPos) + c + fullBuffer.substring(cursorPos);

    // Shift braille tracking array safely
    for (int i = fullBuffer.length() - 1; i > cursorPos; i--) {
        fullBufferBraille[i] = fullBufferBraille[i - 1];
    }

    fullBufferBraille[cursorPos] = brailleCell;

    cursorPos++;

    scrollWindow();
    redrawLCDLine();
}

// backspace (remove char before cursorPos)
void backspaceAtCursor() {
  if (cursorPos == 0) return;
  fullBuffer = fullBuffer.substring(0, cursorPos - 1) + fullBuffer.substring(cursorPos);
  cursorPos--;
  if (windowStart > 0 && cursorPos < windowStart) windowStart--;
  scrollWindow();
  redrawLCDLine();
  if (bluetoothEnabled && bleKeyboard.isConnected()) {
      bleKeyboard.write(KEY_BACKSPACE);
  }
}

// insert space at cursor
void insertSpaceAtCursor() {
  insertAtCursor(' ');
  if (bluetoothEnabled && bleKeyboard.isConnected()) {
      bleKeyboard.print(' ');
  }
}

// move cursor left/right
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

// Cycle modes (8 key)
void cycleMode() {
  if (currentMode == AUTO) currentMode = TEXT;
  else if (currentMode == TEXT) currentMode = SPECIAL;
  else if (currentMode == SPECIAL) currentMode = NUMBER;
  else currentMode = AUTO;
  updateLCDMode();
}

// update top-line mode display
void updateLCDMode() {
  lcd.setCursor(0,0);
  switch(currentMode) {
    case AUTO:    lcd.print("Mode: AUTO     "); Serial.println("Mode: AUTO"); break;
    case TEXT:    lcd.print("Mode: Text     "); Serial.println("Mode: Text"); break;
    case NUMBER:  lcd.print("Mode: Number   "); Serial.println("Mode: Number"); break;
    case SPECIAL: lcd.print("Mode: Special  "); Serial.println("Mode: Special"); break;
  }
  lcd.setCursor(15, 0);
  if (bluetoothEnabled) {
    lcd.write(byte(0));
  } else {
    lcd.print(" ");
  }
}

// ---------- EEPROM save/load ----------
void saveLineToEEPROM() {
  int len = fullBuffer.length();
  for (int i = 0; i < MAX_CHARS; i++) {
    if (i < len)
        EEPROM.write(EEPROM_ADDR + i, fullBuffer[i]);
    else
        EEPROM.write(EEPROM_ADDR + i, 0); // Null padding
  }

  EEPROM.write(EEPROM_ADDR + MAX_CHARS, len);
  EEPROM.commit();
  lcd.setCursor(0,0);
  lcd.print("Saved to MEMORY ");
  Serial.println("Text saved to EEPROM");;
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

    // Load saved string from EEPROM
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

    // Show loading message
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

    // Insert loaded text at cursor
    String oldBuffer = fullBuffer;
    int startPos = cursorPos; 
    for (int i = 0; i < loaded.length(); i++) {
        insertAtCursor(loaded[i]); 
    }
    insertAtCursor(' '); 
    scrollWindow();
    redrawLCDLine();
    updateLCDMode();

    if (bluetoothEnabled && bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_END);
        for (int i = 0; i < loaded.length(); i++) {
          char c = loaded[i];
          if (c >= 32 && c <= 126) {
              bleKeyboard.write(c);
              delay(35); // small delay to let BLE send
          }
      }
    }
    Serial.print("Loaded text: "); Serial.println(loaded);
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
    //Serial.print("ContractionBits: "); Serial.println(fullBuffer);

    if (::hasContraction) {
      cursorPos = fullBuffer.length();
    }
    ::hasContraction = false;

    int newChars = fullBuffer.length() - oldLen;

    sendChangesToBLE(oldStr, fullBuffer);
}

void sendChangesToBLE(const String& oldStr, const String& newStr) {
    if (!bluetoothEnabled || !bleKeyboard.isConnected()) return;

    int a = 0;
    while (a < oldStr.length() && a < newStr.length() && oldStr[a] == newStr[a]) {
        a++;
    }

    // If nothing changed, exit
    if (a == oldStr.length() && a == newStr.length()) {
        return;
    }

    int bOld = oldStr.length() - 1;
    int bNew = newStr.length() - 1;

    while (bOld >= a && bNew >= a && oldStr[bOld] == newStr[bNew]) {
        bOld--;
        bNew--;
    }

    // Compute how many characters were removed
    int toDelete = (bOld >= a) ? (bOld - a + 1) : 0;

    // replacement text
    String replacement = (bNew >= a) ? newStr.substring(a, bNew + 1) : "";

    // deletion ---
    for (int i = 0; i < toDelete; i++) {
        bleKeyboard.write(KEY_BACKSPACE);
        delay(3);  // important for BLE HID stability
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
  int speed = 80; // milliseconds per step
  for (int i = 0; i <= barWidth; i++) {
      lcd.setCursor(0, 1); // bottom row
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

// ---------- ESP32 setup/loop ----------
void setup() {
  Serial.begin(9600);
  while (!Serial);
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

    if (feedbackMode != VIBRATE_ONLY) {
      tone(buzzerPin, freq, dur);
    }

    if (feedbackMode != SOUND_ONLY) {
      vibrate(200, 230);
    }

    lastBeep = now;
  }
}

void startupTone() {
  sound(1500, 120);
  delay(150);
  sound(2000, 120);
  delay(150);
  sound(2500, 160);
  delay(200);
}
void saveTone() {
  sound(2000, 60);
  delay(200);
  sound(2400, 60);
  delay(200);
  sound(2800, 80);
  delay(300);
}
void loadTone() {
  sound(2800, 60); 
  delay(200);
  sound(2400, 60);
  delay(200);
  sound(2000, 80);
  delay(300);
}
void btTone() {
  sound(2400, 60);
  delay(200);
  sound(2800, 60);
  delay(200);
  sound(3200, 80);
  delay(300);
}
void btdTone() {
  sound(3200, 60); 
  delay(200);
  sound(2800, 60);
  delay(200);
  sound(2400, 60);
  delay(300);
}
void enterTone() {
  sound(1600, 60); 
  delay(100);
  sound(1600, 60);
  delay(100);
}
void modeTone() {
  sound(2000, 120);
  delay(150);
  sound(2500, 160);
  delay(300);
}
#define C 2093
#define D 2349
#define E 2637
#define F 2794
#define G 3136
#define A 3520

char getHeldKey() {
  for (byte r = 0; r < ROWS; r++) {
      pinMode(rowPins[r], OUTPUT);
      digitalWrite(rowPins[r], LOW); 

      for (byte c = 0; c < COLS; c++) {
          pinMode(colPins[c], INPUT_PULLUP);
          if (digitalRead(colPins[c]) == LOW) {
              // found pressed key
              pinMode(rowPins[r], INPUT);
              return keys[r][c];
          }
      }

      pinMode(rowPins[r], INPUT);
  }
  return NO_KEY;
}

void loop() {
  char key = getHeldKey();  
  unsigned long now = millis();

  if (vibActive && millis() - vibStart >= vibDuration) {
    digitalWrite(vibroPin, LOW);
    vibActive = false;
  }
 
  if (bluetoothEnabled) {
      if (bleKeyboard.isConnected()) {
          if (!wasConnected) {
              // Host just connected
              Serial.println("Host Connected!");
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
              // Host just disconnected
              Serial.println("Host Disconnected!");
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
            switch (key) {
              case '2':
              case '5':
              case '8':
              case '3':
              case '6':
              case '9':
                  if (hashPending) {
                      hashPending = false;
                      brailleBits = 0;
                  }
                  if (key == '2') sound(C, 70);
                  else if (key == '5') sound(D, 70);
                  else if (key == '8') sound(E, 70);
                  else if (key == '3') sound(F, 70);
                  else if (key == '6') sound(G, 70);
                  else if (key == '9') sound(A, 70);
                  break;
            }

          if (key == '#') lastKey = NO_KEY; 
          if (key == '#') {
            enterTone();
          } else if (key == '0' || key == '7' || key == 'C'|| key == '1'|| key == '4') {
            sound(1800, 70);
          } else if (key == 'D') {
            sound(2100, 70);
          } else if (key == '*') {
            modeTone();
          }
      } else if (now >= nextRepeatTime) {
          // only repeat these keys
          if (key == '0' || key == 'D' || key == '7' || key == 'C') {
              handleKeyPress(key);
              nextRepeatTime = now + repeatRate;
          }
      }
  } else {
      lastKey = NO_KEY; // key released
  }
}

void handleHashSinglePress() {
    String out = brailleToChar(brailleBits);
    Serial.print("BrailleBits: "); Serial.print(out); Serial.print(" ");
    Serial.print(brailleBits, DEC); Serial.print(" "); Serial.println(brailleBits, BIN);

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
    insertAtCursor(' ');
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

void handleKeyPress(char key) {
    switch(key) {
        case '*': cycleMode(); break;

        // Braille dots - single press only
        case '2': brailleBits |= 1;   break;   // dot 1
        case '5': brailleBits |= 2;   break;   // dot 2
        case '8': brailleBits |= 4;   break;   // dot 3
        case '3': brailleBits |= 8;   break;   // dot 4
        case '6': brailleBits |= 16;  break;   // dot 5
        case '9': brailleBits |= 32;  break;   // dot 6

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

        case '0': // space
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

        case '#': { // braille -> char
            unsigned long now = millis();
            if (hashPending && (now - hashPressTime < hashDebounceTime)) {
                hashPending = false;  // cancel single
                handleHashDoublePress();
                brailleBits = 0;
                break;
            }
            hashPending = true;
            hashPressTime = now;
            break;
        }

        case 'D': backspaceAtCursor(); break;
        case '7': moveCursorLeft(); break;
        case 'C': moveCursorRight(); break;

        case 'A': saveLineToEEPROM(); break;
        case 'B': loadLineFromEEPROM(); break;

        case '1':
            if (!bluetoothEnabled) {
                bleKeyboard.begin();
                bluetoothEnabled = true;
                lcd.setCursor(0,0);
                lcd.print("BT HID enabled!");
                Serial.println("BT HID enabled!");
                startupTone();
                int barWidth = 16;
                int speed = 15; // milliseconds per step
                for (int i = 0; i <= barWidth; i++) {
                    lcd.setCursor(0, 1); // bottom row
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
                    unsigned long elapsed = (millis() - startTime) / 1000;

                    // show seconds
                    lcd.setCursor(0,1);
                    lcd.print(elapsed);
                    lcd.print(" sec ");

                    // check for timeout (45 seconds)
                    if (elapsed >= 45) {
                        // Disable BT
                        bleKeyboard.end();
                        bluetoothEnabled = false;

                        // Show timeout message
                        lcd.setCursor(0,0);
                        lcd.print("BT Timeout       ");
                        lcd.setCursor(0,1);
                        lcd.print("Turning off...   ");
                        btdTone();
                        delay(2000);
                        bluetoothEnabled = false;
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
                //bleKeyboard.end();
                bluetoothEnabled = false;
                btStop();
                lcd.setCursor(0,0);
                lcd.print("BT HID disabled!");
                Serial.println("BT HID disabled!");
                btdTone();
                int barWidth = 16;
                int speed = 15; // milliseconds per step
                for (int i = 0; i <= barWidth; i++) {
                    lcd.setCursor(0, 1); // bottom row
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
            break;

        default: break;
    }
    
    lcd.setCursor(getLcdCursor(), 1);
}