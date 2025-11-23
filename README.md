# Brailleduino
Braille to Text device for deciphering braille system to readable text format

## Group Members
- @prettyvhal
- @markclarence13
- joven angeles

## Todo: Software
- [x] Braille to English System
- [x] Mode functions `(AUTO, TEXT, SPECIAL, NUMBER)`
- [x] Capitalization Marker `dot6`
- [x] Number sign `dot3 + dot4 + dot5 + dot6`
- [x] Special character sign `dot4 + dot5 + dot6`
- [x] Text sign `dot5 + dot6`
- [ ] Italic sign/double italic sign
- [x] Termination sign
- [x] Sliding text
- [x] Uncontracted braille
- [x] Contracted braille (semi and full words)
- [ ] Send data to device (via blueetooth)
- [x] Insert/delete function
- [ ] Accent markers
- [ ] Context based punctuation markers (semi working)
- [x] 5th sequence and other contractions 
- [x] Save string to memory
- [x] Load string from memory
- [x] Hold button to trigger hold
- [ ] Way to know the battery status and indicate it

## Todo: Prototype Hardware/Design
- [x] Arduino Uno
- [x] LCD I2C
- [x] Keypad membrane
- [x] Wire connectors
- [x] Passive buzzer
- [ ] Battery
- [ ] Blueetooth module
- [ ] Casing
- [ ] Prototype design
- [ ] Type-C connector for charging
- [ ] On/off button

## Genral Functions
- **`Circular symbols`** = main braille pattern input
- **`Left and Right arrows`** = move the cursor in the screen
- **`Star symbol`** = mode function
- **`Backspace symbol`** = delete character
- **`Space symbol`** = add space
- **`Enter symbol`** = inputs the braille pattern into readable symbol `(text, number, special symbols)`
- **`Yellow S`** = Save string to memory
- **`Yellow L`** = Load string from memory
- **`Blue buttons`** = no functions yet
<img width="640" height="480" alt="BrailleDuino beta keypad design" src="https://github.com/user-attachments/assets/5c2b1de3-8c59-4dbd-ac14-2a38999336c0" />