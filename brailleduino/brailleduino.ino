#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

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

// Braille variables
byte brailleBits = 0;
int cursorPos = 0;
bool nextCapital = false;  // single dot6
bool capsLock = false;     // double dot6
bool nextNumber = false;   // single-number trigger
bool numberLock = false;   // double-number trigger
bool dot6PressedOnce = false;
int indicatorLength = 0;
bool numPressedOnce = false;

enum Mode {
  AUTO,
  TEXT,
  NUMBER,
  SPECIAL
};

Mode currentMode = AUTO;

// Reorder bits to standard braille positions
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

// Special characters
char specialFromBraille(byte p) {
  switch(p) {
    // needs context for word placements
    case 0b100110: return '?';
    case 0b10110: return '!';
    case 0b101111: return '&';
    case 0b10: return ',';
    case 0b100100: return '_';
    case 0b110010: return '.';
    case 0b110: return ';';
    case 0b10010: return ':';
    case 0b110100: return '“';
    //case 0b100110: return '”';
    case 0b110110: return '( )';
    case 0b1100: return '/';
    case 0b101100: return '@';
    //case 0b10110: return '+';
    //case 0b10010: return '-';
    case 0b10100: return '*';
    case 0b100: return '\'';
    
    default: return '0';
  }
}

// Braille to TEXT
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
    //case 0b110010: return "ou";
    case 0b101010: return "ow";
    case 0b100010: return "en";
    //case 0b010100: return "in";
    //case 0b001100: return "st";
    //case 0b101100: return "ing";
    case 0b011100: return "ar";
  }
  return "~";
}

// Braille numbers
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

String brailleToChar(byte bits) {
    byte pattern = reorderBraille(bits);
    String out = "";

    // CAPITALIZATION SYSTEM
    if (pattern == 0b100000) { // dot6
        if (dot6PressedOnce) { // second press → caps lock
            capsLock = true;
            nextCapital = false;
            dot6PressedOnce = false;

            lcd.setCursor(cursorPos, 1);
            lcd.print(">");
            indicatorLength = 2;
            cursorPos++;
            if (cursorPos > 15) cursorPos = 15;
            return "";
        } else { // first press → next char uppercase
            dot6PressedOnce = true;
            nextCapital = true;
            lcd.setCursor(cursorPos, 1);
            lcd.print(">");
            indicatorLength = 1;
            cursorPos++;
            if (cursorPos > 15) cursorPos = 15;
            return "";
        }
    }
    if (pattern != 0b100000) dot6PressedOnce = false;

    // NUMBER SYSTEM TRIGGER
    if (pattern == 0b111100) { // dot3+4+5+6
        if (numPressedOnce) { // second press → number lock
            numberLock = true;
            nextNumber = false;
            numPressedOnce = false;

            lcd.setCursor(cursorPos, 1);
            lcd.print("#");
            indicatorLength = 2;
            cursorPos++;
            if (cursorPos > 15) cursorPos = 15;
            return "";
        } else {
            numPressedOnce = true;
            nextNumber = true;
            numberLock = false;
            lcd.setCursor(cursorPos, 1);
            lcd.print("#");
            indicatorLength = 1;
            cursorPos++;
            if (cursorPos > 15) cursorPos = 15;
            return "";
        }
    }
    if (pattern != 0b111100) numPressedOnce = false;

    // ---- TRIGGER NUMBERS ----
    if (nextNumber || numberLock || numPressedOnce) {
        currentMode = NUMBER;
        nextNumber = false;
        clearTempIndicator();
    }

    // DETECT AUTO MODE
    if (currentMode == AUTO && pattern != 0) {
        if (brailleToText(pattern) != "~") currentMode = TEXT;
        else if (specialFromBraille(pattern) != '0') currentMode = SPECIAL;
        else if (brailleToNumber(pattern) != '?') currentMode = NUMBER;
    }

    // PROCESS BASED ON MODE
    switch(currentMode) {
      case TEXT:
          {
              char s = specialFromBraille(pattern);
              if (s != '0') out = String(s);
              else out = brailleToText(pattern);
          }
          break;

      case SPECIAL:
          {
              char s = specialFromBraille(pattern);
              if (s != '0') out = String(s);
              else out = brailleToText(pattern);
          }
          break;

      case NUMBER: {
        char n = brailleToNumber(pattern);
        if (n != '?') {
            out = String(n);
        } else {
            char s = specialFromBraille(pattern);
            if (s != '0') out = String(s);
            else out = brailleToText(pattern);

            currentMode = TEXT;
            numberLock = false;
            nextNumber = false;
            numPressedOnce = false;
        }
      } break;
    }

    // CAPITALIZE
    if (nextCapital || capsLock) {
        out.toUpperCase();
        nextCapital = false;
        clearTempIndicator();
    }

    return out;
}

void clearTempIndicator() {
  if (indicatorLength > 0) {
      lcd.setCursor(cursorPos - indicatorLength, 1);
      for (int i = 0; i < indicatorLength; i++) lcd.print(" ");
      cursorPos -= indicatorLength;
      indicatorLength = 0;
  }
}

void updateLCDMode() {
  lcd.setCursor(0,0);
  switch(currentMode) {
    case AUTO: lcd.print("Mode: AUTO       "); break;
    case TEXT: lcd.print("Mode: Text       "); break;
    case NUMBER: lcd.print("Mode: Number     "); break;
    case SPECIAL: lcd.print("Mode: Special    "); break;
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();

  updateLCDMode();
  lcd.setCursor(0,1);

  lcd.cursor();
  lcd.blink();

  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();
  if (!key) return;

  switch(key) {
    case '0':
      if (currentMode == AUTO) currentMode = TEXT;
      else if (currentMode == TEXT) currentMode = SPECIAL;
      else if (currentMode == SPECIAL) currentMode = NUMBER;
      else if (currentMode == NUMBER) currentMode = AUTO;
      updateLCDMode();
      break;

    case '2': brailleBits |= 1 << 0; break;
    case '5': brailleBits |= 1 << 1; break;
    case '8': brailleBits |= 1 << 2; break;
    case '3': brailleBits |= 1 << 3; break;
    case '6': brailleBits |= 1 << 4; break;
    case '9': brailleBits |= 1 << 5; break;

    case '*':
      lcd.setCursor(cursorPos,1);
      lcd.print(" ");
      if (cursorPos < 15) cursorPos++;

      currentMode = TEXT;
      capsLock = false;
      nextCapital = false;
      nextNumber = false;
      numberLock = false;
      break;

    case '#': {
      String out = brailleToChar(brailleBits);
      Serial.print("Bits: "); Serial.println(brailleBits,BIN);

      if (out == "") {
        brailleBits = 0;
        break;
      }

      lcd.setCursor(cursorPos,1);
      lcd.print(out);
      cursorPos += out.length();
      if (cursorPos > 15) cursorPos = 15;

      brailleBits = 0;
    } break;

    case 'D':
      if (cursorPos > 0) cursorPos--;
      lcd.setCursor(cursorPos,1);
      lcd.print(" ");
      lcd.setCursor(cursorPos,1);
      break;

    case '7': if (cursorPos > 0) cursorPos--; break;
    case 'C': if (cursorPos < 15) cursorPos++; break;
  }

  lcd.setCursor(cursorPos,1);
}