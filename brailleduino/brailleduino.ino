#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <Arduino.h>

// LCD setup (I2C)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9,8,7,6};
byte colPins[COLS] = {5,4,3,2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
const int buzzerPin = 10;
unsigned long lastBeep = 0;
const unsigned long debounceMillis = 35;

// EEPROM settings
const int EEPROM_ADDR = 0;       // starting address
const int MAX_CHARS = 128;       // we save a single 128-char line

// Braille / editor state
byte brailleBits = 0;
int cursorPos = 0;           // index inside fullBuffer (0..fullBuffer.length)
String fullBuffer = "";      // full text buffer (can be longer than 16)
int windowStart = 0;         // left-most index shown on LCD
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
const unsigned long repeatRate = 150; // ms between repeats
unsigned long nextRepeatTime = 0;

enum Mode { AUTO, TEXT, NUMBER, SPECIAL };
Mode currentMode = AUTO;


// ---------- Special & mapping functions ----------
const char specialOutput[][8] PROGMEM = {
  {"?"}, {"!"}, {"&"}, {","}, {"_"}, {"."}, {";"}, {":"}, {"\""}, {"()"},
  {"/"}, {"@"}, {"*"}, {"\'"}, {"cannot"}, {"had"}, {"many"}, {"spirit"}, 
  {"their"}, {"world"}
};
const byte specialDots[] PROGMEM = {
  38,22,47,2,36,50,6,18,52,54,12,44,20,4,9,19,13,14,46,58
};
String specialFromBraille(byte p) {
  for (int i=0;i<20;i++){
    if (p == pgm_read_byte(&specialDots[i])) {
      char buf[7];
      strcpy_P(buf, specialOutput[i]);
      return String(buf);
    }
  }
  return "0";
}


const byte brailleDotsTable[] PROGMEM = {
  1,3,9,25,17,11,27,19,10,26,5,7,13,29,21,15,31,23,14,30,37,39,58,45,61,53,
  47,63,55,46,62,33,35,41,57,49,43,59,42,34,28,51,2,6,18,50,22,54,38,52,12,44,16
};

const char brailleTextTable[][7] PROGMEM = {
  "a","b","c","d","e","f","g","h","i","j",
  "k","l","m","n","o","p","q","r","s","t",
  "u","v","w","x","y","z",
  "and","for","of","the","with",
  "ch","gh","sh","th","wh","ed","er","ow","en","ar",
  "ou","ea","bb","cc","dd","ff","gg","in","by","st","ing",">"
};
String brailleToText(byte b) {
  for (int i = 0; i < sizeof(brailleDotsTable); i++) {
    if (b == pgm_read_byte(&brailleDotsTable[i])) {
      char buf[8];
      strcpy_P(buf, brailleTextTable[i]);
      return String(buf);
    }
  }
  return "~";
}

const byte brailleNumDots[] PROGMEM = {1,3,9,25,17,11,27,19,10,26};
const char brailleNumChars[] PROGMEM = {'1','2','3','4','5','6','7','8','9','0'};
char brailleToNumber(byte b) {
  for (int i=0;i<10;i++){
    if (b == pgm_read_byte(&brailleNumDots[i]))
      return pgm_read_byte(&brailleNumChars[i]);
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
    else if (specialFromBraille(pattern) != '0') currentMode = SPECIAL;
    else if (brailleToNumber(pattern) != '?') currentMode = NUMBER;
  }

  // Choose output based on mode
  if (currentMode == TEXT) {
      out = brailleToText(pattern);
      if (out == "~") {
          String s = specialFromBraille(pattern);
          if (s != '0') {
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
      if (s != '0') {
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
          if (s != '0') out = String(s);
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
}

// insert space at cursor
void insertSpaceAtCursor() {
  insertAtCursor(' ');
}

// move cursor left/right
void moveCursorLeft() {
  if (cursorPos > 0) cursorPos--;
  scrollWindow();
  redrawLCDLine();
}
void moveCursorRight() {
  if (cursorPos < fullBuffer.length()) cursorPos++;
  scrollWindow();
  redrawLCDLine();
}

// Cycle modes (0 key)
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
    case AUTO:    lcd.print("Mode: AUTO       "); break;
    case TEXT:    lcd.print("Mode: Text       "); break;
    case NUMBER:  lcd.print("Mode: Number     "); break;
    case SPECIAL: lcd.print("Mode: Special    "); break;
  }
}

// ---------- EEPROM save/load ----------
void saveLineToEEPROM() {
  for (int i = 0; i < MAX_CHARS; ++i) {
    char c = (i < fullBuffer.length()) ? fullBuffer[i] : ' ';
    EEPROM.write(EEPROM_ADDR + i, (uint8_t)c);
  }
  // write a marker byte after the data to indicate saved data exists (optional)
  EEPROM.write(EEPROM_ADDR + MAX_CHARS, 0xA5);
  lcd.setCursor(0,0);
  lcd.print("Saved to MEMORY ");
  saveTone();
  delay(1000);
  updateLCDMode();
}

void loadLineFromEEPROM() {
  uint8_t marker = EEPROM.read(EEPROM_ADDR + MAX_CHARS);
  if (marker != 0xA5) {
    fullBuffer = "";
    return;
  }
  fullBuffer = "";
  for (int i = 0; i < MAX_CHARS; ++i) {
    char c = (char)EEPROM.read(EEPROM_ADDR + i);
    fullBuffer += c;
  }
  while (fullBuffer.length() > 0 && fullBuffer[fullBuffer.length()-1] == ' ') fullBuffer.remove(fullBuffer.length()-1);
  cursorPos = fullBuffer.length();
  windowStart = 0;
  lcd.setCursor(0,0);
  lcd.print("Load from MEMORY");
  loadTone();
  delay(1000);
  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

struct UEBContraction {
    byte prefixDots;
    const char letters[8];
    const char contraction[16];
};

const UEBContraction uebContractions[] PROGMEM = {
  {0, "ab", "about"},
  {0, "abv", "above"},
  {0, "acc", "according"},
  {0, "acr", "across"},
  {0, "adm", "administration"},
  {0, "af", "after"},
  {0, "afn", "afternoon"},
  {0, "afw", "afterward"},
  {0, "ag", "again"},
  {12, "agst", "against"},
  {0, "alm", "almost"},
  {0, "alr", "already"},
  {57, "alth", "although"},
  {0, "altg", "altogether"},
  {0, "alw", "always"},
  {0, " z", " as"},

  {6, "bb", "be"},
  {6, "bbc", "because"},
  {6, "bbf", "before"},
  {6, "bbh", "behind"},
  {6, "bbl", "below"},
  {6, "bbn", "beneath"},
  {6, "bbs", "beside"},
  {6, "bbt", "between"},
  {6, "bby", "beyond"},
  {0, "bl", "blind"},
  {0, "brl", "braille"},
  {0, " b", " but"},

  {0, " c", " can"},
  {16, ">ch", "character"},
  {33, " ch", " child"},
  {33, "chn", "children"},
  {18, "cccv", "conceive"},
  {18, "cccvg", "conceiving"},
  {0, "cd", "could"},

  {16, ">d", "day"},
  {0, "dcv", "deceive"},
  {0, "dcvg", "deceiving"},
  {0, "dcl", "declare"},
  {0, "dclg", "declaring"},
  {0, " d", " do"},

  {0, "ei", "either"},
  //{16, ">e", "ence"},
  {34, " en", " enough"},
  {16, ">e", "ever"},

  {16, ">f", "father"},
  {12, "fst", "first"},
  {0, "fr", "friend"},
  {0, " f", " from"},

  {0, " g", " go"},
  {0, "gd", "good"},
  {0, "grt", "great"},

  {0, " h", " have"},
  {16, ">h", "here"},
  {59, "herf", "herself"},
  {0, "hmf", "himself"},
  {38, " in", " his"}, // hmm

  {0, "imm", "immediate"},
  {0, " x", " it"},
  {0, " xs", " its"},
  {0, "xf", "itself"},

  {0, " j", "just"},

  {16, ">k", "know"},
  {0, " k", " knowledge"},

  {0, "lr", "letter"},
  {0, " l", " like"},
  {0, "ll", "little"},
  {16, ">l", "lord"},

  {0, " m", " more"},
  {16, ">m", "mother"},
  {33, "mch", "much"},
  {12, "mst", "must"},
  {0, "myf", "myself"},

  {16, ">n", "name"},
  {0, "nec", "necessary"},
  {0, "nei", "neither"},
  {0, " n", " not"},

  {16, ">o", "one"},
  {16, ">of", "oneself"},
  {0, " m", " more"},
  {51, " >ou", " ought"},
  {40, "0d", "ound"},
  {40, "0t", "ount"},
  {51, "ourvs", "ourselves"},
  {51, " ou", " out"},

  {0, "pd", "paid"},
  {0, " p", " people"},
  {59, "percv", "perceive"},
  {59, "percvg", "perceiving"},
  {59, "perh", "perhaps"},

  {16, ">q", "question"},
  {0, "qk", "quick"},
  {0, " q", " quite"},

  {0, " r", " rather"},
  {0, "rcv", "receive"},
  {0, "rcvg", "receiving"},
  {0, "rjc", "rejoice"},
  {0, "rjcg", "rejoicing"},
  {16, ">r", "right"},

  {0, "sd", "said"},
  {41, " sh", " shall"},
  {0, " s", " so"},
  {16, ">s", "some"},
  {12, " st", " still"},
  {33, "sch", "such"},

  {0, " t", " that"},
  {46, "themvs", "themselves"},
  {16, ">the", "there"},
  {57, " th", "this"},
  {24, "0th", "those"},
  {16, ">th", "through"},
  {57, "thyf", "thyself"},
  {16, ">t", "time"},
  {0, "td", "today"},
  {0, "tgr", "together"},
  {0, "tm", "tomorrow"},
  {0, "tn", "tonight"},

  {16, ">u", "under"},
  {0, " u", " us"},

  {0, " v", " very"},

  {52, " by", " was"},
  {54, " gg", " were"},
  {16, ">wh", "where"},
  {49, " wh", " which"},
  {0, " w", " will"},
  {16, ">w", "work"},
  {0, "wd", "would"},

  {0, " y", " you"},
  {16, ">y", "young"},
  {0, "yr", "your"},
  {0, "yrf", "yourself"},
  {0, "yrvs", "yourselves"},
};

const int contractionCount = sizeof(uebContractions)/sizeof(uebContractions[0]);

bool checkUEBContraction(const char* buffer, byte lastDots, char* bufferOut) {
    int bufLen = strlen(buffer);
    char letters[8];
    char contraction[16];

    for (int i = 0; i < contractionCount; i++) {
        byte prefix = pgm_read_byte(&(uebContractions[i].prefixDots));
        memcpy_P(letters, uebContractions[i].letters, 8);
        letters[7] = '\0';
        memcpy_P(contraction, uebContractions[i].contraction, 16);
        contraction[15] = '\0';

        int lettersLen = strlen(letters);
        if (lettersLen <= bufLen) {
            if (strncmp(buffer + bufLen - lettersLen, letters, lettersLen) == 0) {
                if (prefix == 0 || prefix == lastDots) {
                    strcpy(bufferOut, contraction);
                    return true; // found
                }
            }
        }
    }
    bufferOut[0] = '\0';
    return false; // not found
}

int getContractionLength(const char* buffer, byte lastDots) {
    int bufLen = strlen(buffer);
    int longest = 0;

    char letters[8];
    for (int i = 0; i < contractionCount; i++) {
        byte prefix = pgm_read_byte(&(uebContractions[i].prefixDots));
        memcpy_P(letters, uebContractions[i].letters, 8);
        letters[7] = '\0';

        int lettersLen = strlen(letters);
        if (lettersLen > bufLen) continue;

        if (strncmp(buffer + bufLen - lettersLen, letters, lettersLen) == 0) {
            if (prefix == 0 || prefix == lastDots) {
                if (lettersLen > longest) longest = lettersLen;
            }
        }
    }
    return longest;
}

void applyContraction(char* buffer, int& bufLen, byte* brailleArr = nullptr, byte prefixPassed = 0, bool front = true) {
    int bestLen = 0;
    int bestIndex = -1;
    char letters[8];

    for (int i = 0; i < contractionCount; i++) {
        byte prefix = pgm_read_byte(&(uebContractions[i].prefixDots));
        memcpy_P(letters, uebContractions[i].letters, 8);
        letters[7] = '\0';

        int lettersLen = strlen(letters);
        if (lettersLen == 0 || lettersLen > bufLen) continue;

        // Check if the last letters match this contraction
        if (strncmp(buffer + bufLen - lettersLen, letters, lettersLen) != 0) continue;

        // Front contraction: check prefix passed
        if (front) {
            if (prefix != 0 && prefix != prefixPassed) continue;
        } 
        // End contraction: check last braille cell
        else if (brailleArr) {
            byte lastBrailleCell = brailleArr[bufLen - 1];
            if (prefix != 0 && prefix != lastBrailleCell) continue;
        }

        if (lettersLen > bestLen) {
            bestLen = lettersLen;
            bestIndex = i;
        }
    }

    if (bestIndex >= 0) {
        char contraction[16];
        memcpy_P(contraction, uebContractions[bestIndex].contraction, 16);
        contraction[15] = '\0';
        int contractionLen = strlen(contraction);

        // If end contraction, clear braille cells of replaced letters
        if (!front && brailleArr) {
            for (int k = bufLen - bestLen; k < bufLen; k++) brailleArr[k] = 0;
        }

        // Replace letters with contraction
        memcpy(buffer + bufLen - bestLen, contraction, contractionLen);
        bufLen = bufLen - bestLen + contractionLen;
        buffer[bufLen] = '\0';
    }
}

void setFrontContraction(char* buffer, int& bufLen, byte prefixPassedToFunction) {
    applyContraction(buffer, bufLen, nullptr, prefixPassedToFunction, true);
}

void setEndContraction(char* buffer, int& bufLen, byte* fullBufferBraille) {
    applyContraction(buffer, bufLen, fullBufferBraille, 0, false);
}

void handleSpaceKeyDirect() {
    char buf[128]; 
    int bufLen = fullBuffer.length();
    fullBuffer.toCharArray(buf, sizeof(buf));

    setEndContraction(buf, bufLen, fullBufferBraille);
    setFrontContraction(buf, bufLen, contractionPrefix);
    
    fullBuffer = String(buf);
    scrollWindow();
    redrawLCDLine();
    cursorPos = fullBuffer.length();
}

void startUP() {
  lcd.setCursor(0,0);
  lcd.print("  BrailleDuino  ");
  startupTone();
  delay(2000);
  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

// ---------- Arduino setup/loop ----------
void setup() {
  pinMode(buzzerPin, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  keypad.setHoldTime(holdDelay);

  startUP();

  updateLCDMode();
  lcd.setCursor(getLcdCursor(), 1);
  lcd.cursor();
  lcd.blink();
  Serial.begin(9600);
}

void sound(int freq) {
  unsigned long now = millis();
  if (now - lastBeep >= debounceMillis) {
    tone(buzzerPin, freq, 60);
    lastBeep = now;
  }
}

void startupTone() {
  tone(buzzerPin, 1500, 120);
  delay(150);
  tone(buzzerPin, 2000, 120);
  delay(150);
  tone(buzzerPin, 2500, 160);
  delay(200);
}
void saveTone() {
  tone(buzzerPin, 2000, 60);
  delay(200);
  tone(buzzerPin, 2400, 60);
  delay(200);
  tone(buzzerPin, 2800, 80);
  delay(300);
}
void loadTone() {
  tone(buzzerPin, 2800, 60); 
  delay(200);
  tone(buzzerPin, 2400, 60);
  delay(200);
  tone(buzzerPin, 2000, 80);
  delay(300);
}
void enterTone() {
  sound(1600); 
  delay(100);
  sound(1600);
  delay(100);
}
void modeTone() {
  tone(buzzerPin, 2000, 120);
  delay(150);
  tone(buzzerPin, 2500, 160);
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

  if (key != NO_KEY) {
      if (key != lastKey) {
          lastKey = key;
          keyPressTime = now;
          nextRepeatTime = now + holdDelay;
          handleKeyPress(key);
            switch (key) {
              case '2':
                sound(C);
                break;
              case '5':
                sound(D);
                break;
              case '8':
                sound(E);
                break;
              case '3':
                sound(F);
                break;
              case '6':
                sound(G);
                break;
              case '9':
                sound(A);
                break;
            }
          if (key == '#') {
            enterTone();
          } else if (key == '*' || key == '7' || key == 'C') {
            sound(1800);
          } else if (key == 'D') {
            sound(2100);
          } else if (key == '0') {
            modeTone();
          }
      } else if (now >= nextRepeatTime) {
          // only repeat these keys
          if (key == '*' || key == 'D' || key == '7' || key == 'C') {
              handleKeyPress(key);
              nextRepeatTime = now + repeatRate;
          }
      }
  } else {
      lastKey = NO_KEY; // key released
  }
}

void handleKeyPress(char key) {
    switch(key) {
        case '0': cycleMode(); break;

        // Braille dots - single press only
        case '2': brailleBits |= 1;   break;   // dot 1
        case '5': brailleBits |= 2;   break;   // dot 2
        case '8': brailleBits |= 4;   break;   // dot 3
        case '3': brailleBits |= 8;   break;   // dot 4
        case '6': brailleBits |= 16;  break;   // dot 5
        case '9': brailleBits |= 32;  break;   // dot 6

        case '*': // space
            handleSpaceKeyDirect();
            insertSpaceAtCursor();
            if (currentMode = AUTO) currentMode = TEXT;
            capsLock = false;
            nextCapital = false;
            nextNumber = false;
            numberLock = false;
            nextSpecial = false;
            specialLock = false;
            contractionPrefix = 0;
            break;

        case '#': { // braille -> char
            String out = brailleToChar(brailleBits);
            Serial.print("BrailleBits: "); Serial.print(out); Serial.print(" "); Serial.print(brailleBits, DEC); Serial.print(" "); Serial.println(brailleBits, BIN);
            if (out != "") {
                if (fullBuffer.length() + out.length() >= MAX_BUFFER) {
                    // Handle error or just ignore input
                    brailleBits = 0;
                    break;
                }
                if (fullBuffer.length() == 0 || fullBuffer.endsWith(" ")) {
                    contractionPrefix = brailleBits;
                }
                for (int i = 0; i < out.length(); ++i) {
                    insertAtCursor(out[i], brailleBits);
                }
            }
            brailleBits = 0;
            break;
        }

        case 'D': backspaceAtCursor(); break;
        case '7': moveCursorLeft(); break;
        case 'C': moveCursorRight(); break;

        case 'A': saveLineToEEPROM(); break;
        case 'B': loadLineFromEEPROM(); break;

        default: break;
    }

    lcd.setCursor(getLcdCursor(), 1);
}