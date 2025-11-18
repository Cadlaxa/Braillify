#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

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
const int MAX_CHARS = 16;       // we save a single 16-char line

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

char lastKey = NO_KEY;
unsigned long keyPressTime = 0; // when key was first pressed
const unsigned long holdDelay = 800; // ms to wait before auto-repeat
const unsigned long repeatRate = 150; // ms between repeats
unsigned long nextRepeatTime = 0;

enum Mode { AUTO, TEXT, NUMBER, SPECIAL };
Mode currentMode = AUTO;


// ---------- Special & mapping functions ----------
char specialFromBraille(byte p) {
  switch(p) {
    case 38: return '?';
    case 22: return '!';
    case 47: return '&';
    case 2:  return ',';
    case 36: return '_';
    case 50: return '.';
    case 6:  return ';';
    case 18: return ':';
    case 52: return '"';
    case 54: return ')'; 
    case 12: return '/';
    case 44: return '@';
    case 20: return '*';
    case 4:  return '\'';

    default: return '0';
  }
}

String brailleToText(byte b) {
  switch(b) {
    case 1: return "a";
    case 3: return "b";
    case 9: return "c";
    case 25: return "d";
    case 17: return "e";
    case 11: return "f";
    case 27: return "g";
    case 19: return "h";
    case 10: return "i";
    case 26: return "j";
    case 5: return "k"; 
    case 7: return "l";
    case 13: return "m";
    case 29: return "n";
    case 21: return "o";
    case 15: return "p";
    case 31: return "q";
    case 23: return "r";
    case 14: return "s";
    case 30: return "t";
    case 37: return "u";
    case 39: return "v";
    case 58: return "w";
    case 45: return "x";
    case 61: return "y";
    case 53: return "z";

    // contractions / common words
    case 47: return "and";
    case 63: return "for";
    case 55: return "of";
    case 46: return "the";
    case 62: return "with";
    case 33: return "ch";
    case 35: return "gh";
    case 41: return "sh";
    case 57: return "th";
    case 49: return "wh";
    case 43: return "ed";
    case 59: return "er";
    case 42: return "ow";
    case 34: return "en";
    case 28: return "ar";

    case 51: return "ou";
    case 2: return "ea";
    case 6: return "bb";
    case 18: return "cc";
    case 50: return "dd";
    case 22: return "ff";
    case 54: return "gg";
    case 38: return "in";
    case 52: return "by";
    case 12: return "st";
    case 44: return "ing";
  }
  return "~";
}

char brailleToNumber(byte b) {
  switch(b) {
    case 1: return '1';
    case 3: return '2';
    case 9: return '3';
    case 25: return '4';
    case 17: return '5';
    case 11: return '6';
    case 27: return '7';
    case 19: return '8';
    case 10: return '9'; 
    case 26: return '0';
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
          char s = specialFromBraille(pattern);
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
      char s = specialFromBraille(pattern);
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
          char s = specialFromBraille(pattern);
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
void insertAtCursor(char c) {
  fullBuffer = fullBuffer.substring(0, cursorPos) + String(c) + fullBuffer.substring(cursorPos);
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
            insertSpaceAtCursor();
            if (currentMode = AUTO) currentMode = TEXT;
            capsLock = false;
            nextCapital = false;
            nextNumber = false;
            numberLock = false;
            nextSpecial = false;
            specialLock = false;
            break;

        case '#': { // braille -> char
            String out = brailleToChar(brailleBits);
            Serial.print("BrailleBits: "); Serial.print(out); Serial.print(" "); Serial.print(brailleBits, DEC); Serial.print(" "); Serial.println(brailleBits, BIN);
            if (out != "") {
                for (int i = 0; i < out.length(); ++i) insertAtCursor(out[i]);
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
