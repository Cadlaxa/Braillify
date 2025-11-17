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
bool nextNumber = false;
bool numberLock = false;
bool dot6PressedOnce = false;
bool numPressedOnce = false;
int indicatorLength = 0;

enum Mode { AUTO, TEXT, NUMBER, SPECIAL };
Mode currentMode = AUTO;

// contraction arrays (kept for possible future use)
const uint8_t contractionKeys[] = {
  0b110011, 0b000010, 0b000110, 0b010010, 0b110010,
  0b010110, 0b110110, 0b100110, 0b110100, 0b001100
};
const char* contractionValues[] = {
  "ou","ea","bb","cc","dd","ff","gg","in","by","st"
};
const int contractionCount = sizeof(contractionKeys)/sizeof(contractionKeys[0]);

// ---------- Utility: reorder bits (keeps API consistent) ----------
byte reorderBraille(byte bits) {
  byte pattern = 0;
  if (bits & (1 << 0)) pattern |= 1 << 0;
  if (bits & (1 << 1)) pattern |= 1 << 1;
  if (bits & (1 << 2)) pattern |= 1 << 2;
  if (bits & (1 << 3)) pattern |= 1 << 3;
  if (bits & (1 << 4)) pattern |= 1 << 4;
  if (bits & (1 << 5)) pattern |= 1 << 5;
  return pattern;
}

// ---------- Special & mapping functions ----------
char specialFromBraille(byte p) {
  switch(p) {
    case 0b100110: return '?';
    case 0b10110:  return '!';
    case 0b101111: return '&';
    case 0b10:     return ',';
    case 0b100100: return '_';
    case 0b110010: return '.';
    case 0b110:    return ';';
    case 0b10010:  return ':';
    case 0b110100: return '"';
    case 0b110110: return ')';
    case 0b1100:   return '/';
    case 0b101100: return '@';
    case 0b10100:  return '*';
    case 0b100:    return '\''; 
    default: return '0';
  }
}

String brailleToText(byte b) {
  switch(b) {
    case 0b1: return "a";
    case 0b11: return "b";
    case 0b1001: return "c";
    case 0b11001: return "d";
    case 0b10001: return "e";
    case 0b1011: return "f";
    case 0b11011: return "g";
    case 0b10011: return "h";
    case 0b1010: return "i";
    case 0b11010: return "j";
    case 0b101: return "k";
    case 0b111: return "l";
    case 0b1101: return "m";
    case 0b11101: return "n";
    case 0b10101: return "o";
    case 0b1111: return "p";
    case 0b11111: return "q";
    case 0b10111: return "r";
    case 0b1110: return "s";
    case 0b11110: return "t";
    case 0b100101: return "u";
    case 0b100111: return "v";
    case 0b111010: return "w";
    case 0b101101: return "x";
    case 0b111101: return "y";
    case 0b110101: return "z";

    // contractions / common words
    case 0b101111: return "and";
    case 0b111111: return "for";
    case 0b110111: return "of";
    case 0b101110: return "the";
    case 0b111110: return "with";
    case 0b100001: return "ch";
    case 0b100011: return "gh";
    case 0b101001: return "sh";
    case 0b111001: return "th";
    case 0b110001: return "wh";
    case 0b101011: return "ed";
    case 0b111011: return "er";
    case 0b101010: return "ow";
    case 0b100010: return "en";
    case 0b011100: return "ar";
  }
  return "~"; // sentinel for not a text letter
}

char brailleToNumber(byte b) {
  switch(b) {
    case 0b000001: return '1';
    case 0b000011: return '2';
    case 0b001001: return '3';
    case 0b011001: return '4';
    case 0b010001: return '5';
    case 0b001011: return '6';
    case 0b011011: return '7';
    case 0b010011: return '8';
    case 0b001010: return '9';
    case 0b011010: return '0';
  }
  return '?';
}

// ---------- Core convert (braille cell -> output string) ----------
String brailleToChar(byte bits) {
  byte pattern = reorderBraille(bits);
  String out = "";

  // CAPITALIZATION DOT6
  if (pattern == 0b100000) {
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
  if (pattern != 0b100000) dot6PressedOnce = false;

  // NUMBER TRIGGER (dot3+4+5+6) -> 0b111100
  if (pattern == 0b111100) {
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
  if (pattern != 0b111100) numPressedOnce = false;

  if (nextNumber || numberLock) {
    currentMode = NUMBER;
    nextNumber = false;
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
    char s = specialFromBraille(pattern);
    if (s != '0') out = String(s);
    else out = brailleToText(pattern);
  } else if (currentMode == SPECIAL) {
    char s = specialFromBraille(pattern);
    if (s != '0') out = String(s);
    else out = brailleToText(pattern);
  } else if (currentMode == NUMBER) {
    char n = brailleToNumber(pattern);
    if (n != '?') out = String(n);
    /*else {
      // fallback to text or special
      char s = specialFromBraille(pattern);
      if (s != '0') out = String(s);
      else out = brailleToText(pattern);
      // exit number mode
      currentMode = TEXT;
      numberLock = false;
      nextNumber = false;
      numPressedOnce = false;
    }*/
  }

  // Apply capitalization
  if ((nextCapital || capsLock) && out.length() > 0) {
    out.toUpperCase();
    nextCapital = false;
    clearTempIndicator();
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
  delay(1000);
  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

void startUP() {
  lcd.setCursor(0,0);
  lcd.print("  BrailleDuino  ");
  delay(2000);
  updateLCDMode();
  scrollWindow();
  redrawLCDLine();
}

// ---------- Arduino setup/loop ----------
void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();

  startUP();

  updateLCDMode();
  lcd.setCursor(getLcdCursor(), 1);
  lcd.cursor();
  lcd.blink();
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();
  if (!key) return;

  switch(key) {
    case '0': cycleMode(); break;

    // dots -> set bits (these map to keypad numbers you've used)
    case '2': brailleBits |= 1 << 0; break;
    case '5': brailleBits |= 1 << 1; break;
    case '8': brailleBits |= 1 << 2; break;
    case '3': brailleBits |= 1 << 3; break;
    case '6': brailleBits |= 1 << 4; break;
    case '9': brailleBits |= 1 << 5; break;

    case '*': // space
      insertSpaceAtCursor();
      // reset transient modes
      if (currentMode = AUTO) {
        currentMode = TEXT;
      }
      capsLock = false;
      nextCapital = false;
      nextNumber = false;
      numberLock = false;
      break;

    case '#': { // convert braille cell -> output string and insert
      String out = brailleToChar(brailleBits);
      Serial.print("BrailleBits: "); Serial.print(out); Serial.print(" "); Serial.println(brailleBits, BIN);
      if (out == "") { 
          brailleBits = 0;
          break;
      }

      if (out != "") {
        for (int i = 0; i < out.length(); ++i) {
          insertAtCursor(out[i]);
        }
      }
      brailleBits = 0;
      break;
    }

    case 'D': // backspace
      backspaceAtCursor();
      break;

    case '7': moveCursorLeft(); break;
    case 'C': moveCursorRight(); break;

    case 'A': // ENTER / SAVE to EEPROM
      saveLineToEEPROM();
      break;
    
    case 'B': // ENTER / SAVE to EEPROM
      loadLineFromEEPROM();
      break;

    default:
      break;
  }

  lcd.setCursor(getLcdCursor(), 1);
}
