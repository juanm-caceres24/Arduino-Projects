#include <Wire.h>
#include <EEPROM.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// LCD object
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7); // address, E, RW, RS, D4, D5, D6, D7

// I2C address of sound amplifier chips
#define ADR_PT2322 0b01000100 // 68
#define ADR_PT2323 0b01001010 // 74

// menu buttons
#define ROTARY_ENCODER_SW_BTN  12
#define ROTARY_ENCODER_DT_PIN  4
#define ROTARY_ENCODER_CLK_PIN 2
#define INPUT_BTN              11
#define MUTE_BTN               7
#define MIX_BTN                3

// global variables
// auxiliar variable for chips PT2322 & PT2323 send
byte dataAux;

// rotary encoder variable
static unsigned long lastInterruptionTime;
unsigned long interruptionTime;
int rotaryEncoderOutput;

// menu variables
int menu;
int menuResetCounter;

// LCD variables
int displayOnCounter;

// logic variables
int input;
int speakerMode;
int channelMute;
int surround;
int mix;
int mute;
int volume;
int volume10;
int volume01;
int treble;
int mid;
int bass;
int sub;
int FL;
int FR;
int CN;
int SL;
int SR;

void setup() {
  Wire.begin(9600);
  Serial.begin(9600);

  // built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // menu buttons
  pinMode(ROTARY_ENCODER_SW_BTN, INPUT);
  pinMode(ROTARY_ENCODER_DT_PIN, INPUT);
  pinMode(ROTARY_ENCODER_CLK_PIN, INPUT);
  pinMode(INPUT_BTN, INPUT);
  pinMode(MUTE_BTN, INPUT);
  pinMode(MIX_BTN, INPUT);

  // menu buttons pull-up resistors
  pinMode(ROTARY_ENCODER_SW_BTN, INPUT_PULLUP);
  pinMode(INPUT_BTN, INPUT_PULLUP);
  pinMode(MUTE_BTN, INPUT_PULLUP);
  pinMode(MIX_BTN, INPUT_PULLUP);

  // encoder interruption
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK_PIN), rotaryEncoder, LOW);

  // LCD initialization
  lcd.begin(20, 4);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH); // backlinght ON
  lcd.setCursor(0,0);
  lcd.print("----INITIALIZING----");
  for (int i = 0; i < 20; i++) {
    lcd.setCursor(i, 1);
    lcd.print(">");
    lcd.setCursor(i, 2);
    lcd.print(">");
    lcd.setCursor(i, 3);
    lcd.print(">");
    delay(50);
  }

  // chips initialization
  manualInitialization();
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);

  // KEYPAD input control
  if (digitalRead(ROTARY_ENCODER_SW_BTN) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    menuSelection();
    menuResetCounter = 0;
    displayOnCounter = 0;
    delay(250);
  }
  if (rotaryEncoderOutput == -1) {
    digitalWrite(LED_BUILTIN, HIGH);
    menuDown();
    menuResetCounter = 0;
    displayOnCounter = 0;
  }
  if (rotaryEncoderOutput == 1) {
    digitalWrite(LED_BUILTIN, HIGH);
    menuUp();
    menuResetCounter = 0;
    displayOnCounter = 0;
  }
  if (digitalRead(INPUT_BTN) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    inputSelection();
    displayOnCounter = 0;
    delay(250);
  }
  if (digitalRead(MUTE_BTN) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    volMute();
    displayOnCounter = 0;
    delay(250);
  }
  if (digitalRead(MIX_BTN) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    mixSelection();
    displayOnCounter = 0;
    delay(250);
  }
  rotaryEncoderOutput = 0;

  // menu reset
  if (menu != 0)
    menuResetCounter++;
  if (menuResetCounter > 50) {
    menuReset();
    menuResetCounter = 0;
  }

  // LCD keep on / turn off
  if (displayOnCounter >= 0 && displayOnCounter < 50) {
    lcd.setBacklight(HIGH);
    displayOnCounter++;
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("                    ");
    lcd.setCursor(0, 1);
    lcd.print("                    ");
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    lcd.setBacklight(LOW);
  }

  // SERIAL input master control
  if (Serial.available()) {
    digitalWrite(LED_BUILTIN, HIGH);
    switch (Serial.read()) {
      case 48:               // '0' ASCII | initialize()
        manualInitialization();
        break;
      case 99:               // 'c' ASCII | inputSelection()
        inputSelection();
        break;
      case 110:              // 'n' ASCII | speakerModeSelection()
        speakerModeSelection();
        break;
      case 98:               // 'b' ASCII | mixSelection()
        mixSelection();
        break;
      case 109:              // 'm' ASCII | volMute()
        volMute();
        break;
      case 43:               // '+' ASCII | volUp()
        volUp();
        break;
      case 45:               // '-' ASCII | volDown()
        volDown();
        break;
      case 117:              // 'u' ASCII | subUp()
        bassUp();
        break;
      case 106:              // 'j' ASCII | subDown()
        bassDown();
        break;
      case 105:              // 'i' ASCII | midUp()
        midUp();
        break;
      case 107:              // 'k' ASCII | midDown()
        midDown();
        break;
      case 111:              // 'o' ASCII | trebleUp()
        trebleUp();
        break;
      case 108:              // 'l' ASCII | trebleDown()
        trebleDown();
        break;
      case 113:              // 'q' ASCII | subUp()
        subUp();
        break;
      case 97:               // 'a' ASCII | subDown()
        subDown();
        break;
      case 119:              // 'w' ASCII | subUp()
        FLUp();
        break;
      case 115:              // 's' ASCII | subDown()
        FLDown();
        break;
      case 101:              // 'e' ASCII | subUp()
        FRUp();
        break;
      case 100:              // 'd' ASCII | subDown()
        FRDown();
        break;
      case 114:              // 'r' ASCII | subUp()
        CNUp();
        break;
      case 102:              // 'f' ASCII | subDown()
        CNDown();
        break;
      case 116:              // 't' ASCII | subUp()
        SLUp();
        break;
      case 103:              // 'g' ASCII | subDown()
        SLDown();
        break;
      case 121:              // 'y' ASCII | subUp()
        SRUp();
        break;
      case 104:              // 'h' ASCII | subDown()
        SRDown();
        break;
      case 122:              // 'z' ASCII | showValues()
        showValues();
        break;
    }
  }

  // LCD first line data information
  if (displayOnCounter >= 0 && displayOnCounter < 50) {
    lcd.setCursor(0, 0);
    switch (menu) {
      case 0:
        lcd.print("VOLUME:             ");
        lcd.setCursor(8, 0);
        lcd.print(volume);
        break;
      case 1:
        lcd.print("TREBLE:             ");
        lcd.setCursor(8, 0);
        lcd.print(treble);
        break;
      case 2:
        lcd.print("MID:                ");
        lcd.setCursor(5, 0);
        lcd.print(mid);
        break;
      case 3:
        lcd.print("BASS:               ");
        lcd.setCursor(6, 0);
        lcd.print(bass);
        break;
      case 4:
        lcd.print("SUBWOOFER:          ");
        lcd.setCursor(11, 0);
        lcd.print(sub);
        break;
      case 5:
        lcd.print("FRONT LEFT:         ");
        lcd.setCursor(12, 0);
        lcd.print(FL);
        break;
      case 6:
        lcd.print("FRONT RIGHT:        ");
        lcd.setCursor(13, 0);
        lcd.print(FR);
        break;
      case 7:
        lcd.print("CENTER:             ");
        lcd.setCursor(8, 0);
        lcd.print(CN);
        break;
      case 8:
        lcd.print("SURROUND LEFT:      ");
        lcd.setCursor(15, 0);
        lcd.print(SL);
        break;
      case 9:
        lcd.print("SURROUND RIGHT:     ");
        lcd.setCursor(16, 0);
        lcd.print(SR);
        break;
    }

    // LCD second line data information
    lcd.setCursor(0, 1);
    lcd.print("INPUT: ");
    lcd.setCursor(7, 1);
    switch (input) {
      case 0:
        lcd.print("AUXILIAR 1   ");
        break;
      case 1:
        lcd.print("AUXILIAR 2   ");
        break;
      case 2:
        lcd.print("AUXILIAR 3   ");
        break;
      case 3:
        lcd.print("LR SURROUND  ");
        break;
      case 4:
        lcd.print("5.1 SURROUND ");
        break;
    }

    // LCD third line data information
    lcd.setCursor(0, 2);
    lcd.print("MUTE: ");
    lcd.setCursor(6, 2);
    if (mute == 1)
      lcd.print("ON            ");
    else
      lcd.print("OFF           ");

    // LCD fourth line data information
    lcd.setCursor(0, 3);
    lcd.print("MIX: ");
    if (mix == 1)
      lcd.print("ON             ");
    else
      lcd.print("OFF            ");
  }
}

// ================ LOGIC METHODS ================ //

void rotaryEncoder() {
  interruptionTime = millis();
  if (interruptionTime - lastInterruptionTime > 5) {
    if (digitalRead(ROTARY_ENCODER_DT_PIN) == LOW)
      rotaryEncoderOutput = 1;
    else
      rotaryEncoderOutput = -1;
    lastInterruptionTime = interruptionTime;
  }
}

void manualInitialization() {

  // desactivate PT2322
  Wire.beginTransmission(ADR_PT2322);
  Wire.write(0b11111111);
  Wire.endTransmission();
  // activate PT2322
  Wire.beginTransmission(ADR_PT2322);
  Wire.write(0b11000111);
  Wire.endTransmission();

  lastInterruptionTime = 0;      // 0 - inf
  
  menu = 0;                      // 0->volume | 1->treble | 2->mid | 3->bass | 4->sub | 5->FL | 6->FR | 7->CN | 8->SL | 9->SR
  menuResetCounter = 0;          // 0 - 50
  displayOnCounter = 0;          // 0 - 50
  
  input = EEPROM.read(0);
  if (input < 0 || input > 4)
    input = 3;                   // 0->aux_1 | 1->aux_2 | 2->aux_3 | 3->LR_surround | 4->5.1_surround
    
  volume = EEPROM.read(1);
  if (volume < 0 || volume > 79)
    volume = 30;                 // 0 - 79
    
  mute = EEPROM.read(2);
  if (mute < 0 || mute > 1)
    mute = 0;                    // 0->unmuted | 1->muted
    
  mix = EEPROM.read(3);
  if (mix < 0 || mix > 1)
    mix = 1;                     // 0->0dB | 1->+6dB
    
  treble = EEPROM.read(4);
  if (treble < 0 || treble > 15) 
    treble = 7;                  // 0 - 15

  mid = EEPROM.read(5);
  if (mid < 0 || mid > 15)
    mid = 7;                     // 0 - 15

  bass = EEPROM.read(6);
  if (bass < 0 || bass > 15)
    bass = 7;                    // 0 - 15
    
  sub = EEPROM.read(7);
  if (sub < 0 || sub > 15)
    sub = 7;                     // 0 - 15

  FL = EEPROM.read(8);
  if (FL < 0 || FL > 15)
    FL = 7;                      // 0 - 15

  FR = EEPROM.read(9);
  if (FR < 0 || FR > 15)
    FR = 7;                      // 0 - 15

  CN = EEPROM.read(10);
  if (CN < 0 || CN > 15)
    CN = 7;                      // 0 - 15

  SL = EEPROM.read(11);
  if (SL < 0 || SL > 15)
    SL = 7;                      // 0 - 15

  SR = EEPROM.read(12);
  if (SR < 0 || SR > 15)
    SR = 7;                      // 0 - 15
    
  speakerMode = 0;               // 0->5.1 | 1->2.1

  // set the chips with the actual values
  setInput();
  setSpeakerMode();
  setMix();
  setMute();
  setVolume();
  setBass();
  setMid();
  setTreble();
  setFL();
  setFR();
  setCN();
  setSL();
  setSR();
  functionPT2322(0, 1, 0);
  
  // DEBUG
  //Serial.println("<< manual initialization >>");
}

void inputSelection() {
  input++;
  if (input > 4)
    input = 0;
  setInput();
  
  // DEBUG
  /*
  Serial.print("<< input selection: ");
  Serial.print(input);
  Serial.println(" >>");
  */
}

void speakerModeSelection() {
  speakerMode++;
  setSpeakerMode();

  // DEBUG
  /*
  Serial.print("<< speaker mode: ");
  Serial.print(speakerMode);
  Serial.println(" >>");
  */
}

void mixSelection() {
  mix = !mix;
  setMix();

  // DEBUG
  /*
  Serial.print("<< mix: ");
  Serial.print(mix);
  Serial.println(" >>");
  */
}

void volMute() {
  mute = !mute;
  setMute();
  
  // DEBUG
  /*
  Serial.print("<< mute: ");
  Serial.print(mute);
  Serial.println(" >>");
  */
}

void volUp() {
  volume++;
  setVolume();
  
  // DEBUG
  /*
  Serial.print("<< volume up | volume: ");
  Serial.print(volume);
  Serial.println(" >>");
  */
}

void volDown() {
  volume--;
  setVolume();
  
  // DEBUG
  /*
  Serial.print("<< volume down | volume: ");
  Serial.print(volume);
  Serial.println(" >>");
  */
}

void bassUp() {
  bass++;
  setBass();

  // DEBUG
  /*
  Serial.print("<< bass up | volume: ");
  Serial.print(bass);
  Serial.println(" >>");
  */
}

void bassDown() {
  bass--;
  setBass();

  // DEBUG
  /*
  Serial.print("<< bass down | volume: ");
  Serial.print(bass);
  Serial.println(" >>");
  */
}

void midUp() {
  mid++;
  setMid();

  // DEBUG
  /*
  Serial.print("<< mid up | volume: ");
  Serial.print(mid);
  Serial.println(" >>");
  */
}

void midDown() {
  mid--;
  setMid();

  // DEBUG
  /*
  Serial.print("<< mid down | volume: ");
  Serial.print(mid);
  Serial.println(" >>");
  */
}

void trebleUp() {
  treble++;
  setTreble();

  // DEBUG
  /*
  Serial.print("<< treble up | volume: ");
  Serial.print(treble);
  Serial.println(" >>");
  */
}

void trebleDown() {
  treble--;
  setTreble();

  // DEBUG
  /*
  Serial.print("<< treble down | volume: ");
  Serial.print(treble);
  Serial.println(" >>");
  */
}

void subUp() {
  sub++;
  setSub();

  // DEBUG
  /*
  Serial.print("<< sub up | volume: ");
  Serial.print(sub);
  Serial.println(" >>");
  */
}

void subDown() {
  sub--;
  setSub();

  // DEBUG
  /*
  Serial.print("<< sub down | volume: ");
  Serial.print(sub);
  Serial.println(" >>");
  */
}

void FLUp() {
  FL++;
  setFL();

  // DEBUG
  /*
  Serial.print("<< FL up | volume: ");
  Serial.print(FL);
  Serial.println(" >>");
  */
}

void FLDown() {
  FL--;
  setFL();

  // DEBUG
  /*
  Serial.print("<< FL down | volume: ");
  Serial.print(FL);
  Serial.println(" >>");
  */
}

void FRUp() {
  FR++;
  setFR();

  // DEBUG
  /*
  Serial.print("<< FR up | volume: ");
  Serial.print(FR);
  Serial.println(" >>");
  */
}

void FRDown() {
  FR--;
  setFR();

  // DEBUG
  /*
  Serial.print("<< FR down | volume: ");
  Serial.print(FR);
  Serial.println(" >>");
  */
}

void CNUp() {
  CN++;
  setCN();

  // DEBUG
  /*
  Serial.print("<< CN up | volume: ");
  Serial.print(CN);
  Serial.println(" >>");
  */
}

void CNDown() {
  CN--;
  setCN();

  // DEBUG
  /*
  Serial.print("<< CN down | volume: ");
  Serial.print(CN);
  Serial.println(" >>");
  */
}

void SLUp() {
  SL++;
  setSL();

  // DEBUG
  /*
  Serial.print("<< SL up | volume: ");
  Serial.print(SL);
  Serial.println(" >>");
  */
}

void SLDown() {
  SL--;
  setSL();

  // DEBUG
  /*
  Serial.print("<< SL down | volume: ");
  Serial.print(SL);
  Serial.println(" >>");
  */
}

void SRUp() {
  SR++;
  setSR();

  // DEBUG
  /*
  Serial.print("<< SR up | volume: ");
  Serial.print(SR);
  Serial.println(" >>");
  */
}

void SRDown() {
  SR--;
  setSR();

  // DEBUG
  /*
  Serial.print("<< SR down | volume: ");
  Serial.print(SR);
  Serial.println(" >>");
  */
}

void showValues() {
  Serial.print("menu: ");
  Serial.println(menu);
  Serial.print("input: ");
  Serial.println(input);
  Serial.print("volume: ");
  Serial.println(volume);
  Serial.print("mute: ");
  Serial.println(mute);
  Serial.print("mix: ");
  Serial.println(mix);
  Serial.print("bass: ");
  Serial.println(bass);
  Serial.print("mid: ");
  Serial.println(mid);
  Serial.print("treble: ");
  Serial.println(treble);
  Serial.print("sub: ");
  Serial.println(sub);
  Serial.print("FL: ");
  Serial.println(FL);
  Serial.print("FR: ");
  Serial.println(FR);
  Serial.print("CN: ");
  Serial.println(CN);
  Serial.print("SL: ");
  Serial.println(SL);
  Serial.print("SR: ");
  Serial.println(SR);
  Serial.print("speakerMode: ");
  Serial.println(speakerMode);
  Serial.print("channelMute: ");
  Serial.println(channelMute);
  Serial.print("surround: ");
  Serial.println(surround);
}

// ================ MENU METHODS ================ //

void menuSelection() {
  menu++;
  if (menu > 9)
    menu = 0;

  // DEBUG
  /*
  Serial.print("<< menu: ");
  Serial.print(menu);
  Serial.println(" >>");
  */
}

void menuReset() {
  menu = 0;

  // DEBUG
  /*
  Serial.print("<< menu (reset): ");
  Serial.print(menu);
  Serial.println(" >>");
  */
}

void menuUp() {
  switch (menu) {
    case 0:
      volUp();
      break;
    case 1:
      trebleUp();
      break;
    case 2:
      midUp();
      break;
    case 3:
      bassUp();
      break;
    case 4:
      subUp();
      break;
    case 5:
      FLUp();
      break;
    case 6:
      FRUp();
      break;
    case 7:
      CNUp();
      break;
    case 8:
      SLUp();
      break;
    case 9:
      SRUp();
      break;
  }
}

void menuDown() {
  switch (menu) {
    case 0:
      volDown();
      break;
    case 1:
      trebleDown();
      break;
    case 2:
      midDown();
      break;
    case 3:
      bassDown();
      break;
    case 4:
      subDown();
      break;
    case 5:
      FLDown();
      break;
    case 6:
      FRDown();
      break;
    case 7:
      CNDown();
      break;
    case 8:
      SLDown();
      break;
    case 9:
      SRDown();
      break;
  }
}

// ================ CHIPS LOGIC & COMMUNICATION METHODS ================ //

void setInput() {
  if (input > 4)
    input = 0;
  switch (input) {
    case 0:        // 1 input
      sendToPT2323(0b11001011);
      break; 
    case 1:        // 2 input
      sendToPT2323(0b11001010);
      break;
    case 2:        // 3 input
      sendToPT2323(0b11001001);
      break;
    case 3:        // 4 input
      sendToPT2323(0b11001000);
      break;
    case 4:        // 6 channels input
      sendToPT2323(0b11000111);
      break;
  }
  EEPROM.update(0, input);
}

void setSurround() {
  if (surround > 1)
    surround = 0;
  switch (surround) {
    case 0:           // surround ON
      sendToPT2323(0b11010000);
      break;
    case 1:           // surround OFF
      sendToPT2323(0b11010001);
      break;
  }
}

void setSpeakerMode() {
  if (speakerMode > 1)
    speakerMode = 0;
  switch (speakerMode) {
    case 0:              // 5.1 mode
      channelMute = 0;
      surround = 0;
      break;
    case 1:              // 2.1 mode
      channelMute = 1;
      surround = 1;
      break;
  }
  setCN();
  setSL();
  setSR();
  setSurround();
}

void setMix() {
  if (mix > 1)
    mix = 0;
  switch (mix) {
    case 0:      // 0dB setup
      sendToPT2323(0b10010000);
      break;
    case 1:      // +6dB setup
      sendToPT2323(0b10010001);
      break;
  }
  EEPROM.update(3, mix);
}

void setMute() {
  if (mute > 1)
    mute = 0;
  switch (mute) {
    case 0:       // all channels unmuted
      sendToPT2323(0b11111110);
      break;
    case 1:       // all channels muted
      sendToPT2323(0b11111111);
      break;
  }
  EEPROM.update(2, mute);
}

void setVolume() {
  if (volume > 79)
    volume = 79;
  if (volume < 0)
    volume = 0;
  int aux = 79 - volume;
  volume10 = aux/10;
  volume01 = aux - volume10*10;
  Wire.beginTransmission(ADR_PT2322);
  Wire.write(volume10 + 0b11100000);
  Wire.write(volume01 + 0b11010000);
  Wire.endTransmission();
  EEPROM.update(1, volume);
}

void setBass() {
  if (bass > 15)
    bass = 15;
  if (bass < 0)
    bass = 0;
  dataAux = bass; 
  if(bass > 7)
    dataAux = 23 - bass;
  sendToPT2322(0b10010000 + dataAux);
  EEPROM.update(6, bass);
}

void setMid() {
  if (mid > 15)
    mid = 15;
  if (mid < 0)
    mid = 0;
  dataAux = mid; 
  if(mid > 7)
    dataAux = 23 - mid;
  sendToPT2322(0b10100000 + dataAux);
  EEPROM.update(5, mid);
}

void setTreble() {
  if (treble > 15)
    treble = 15;
  if (treble < 0)
    treble = 0;
  dataAux = treble; 
  if(treble > 7)
    dataAux = 23 - treble;
  sendToPT2322(0b10110000 + dataAux);
  EEPROM.update(4, treble);
}

void setSub() {
  if (sub > 15)
    sub = 15;
  if (sub < 0)
    sub = 0;
  sendToPT2322(0b01100000 + 15 - sub);
  EEPROM.update(7, sub);
}

void setFL() {
  if (FL > 15)
    FL = 15;
  if (FL < 0)
    FL = 0;
  sendToPT2322(0b00010000 + 15 - FL);
  EEPROM.update(8, FL);
}

void setFR() {
  if (FR > 15)
    FR = 15;
  if (FR < 0)
    FR = 0;
  sendToPT2322(0b00100000 + 15 - FR);
  EEPROM.update(9, FR);
}

void setCN() {
  if (CN > 15)
    CN = 15;
  if (CN < 0)
    CN = 0;
  switch (channelMute) {
    case 0:              // CN mute disabled
      sendToPT2323(0b11110100);
      break;
    case 1:              // CN mute
      sendToPT2323(0b11110101);
      break;
  }
  sendToPT2322(0b00110000 + 15 - CN);
  EEPROM.update(10, CN);
}

void setSL() {
  if (SL > 15)
    SL = 15;
  if (SL < 0)
    SL = 0;
  switch(channelMute) {
    case 0:             // SL mute disabled
      sendToPT2323(0b11111000);
      break;
    case 1:             // SL mute
      sendToPT2323(0b11111001);
      break;
  }
  sendToPT2322(0b01000000 + 15 - SL);
  EEPROM.update(11, SL);
}

void setSR() {
  if (SR > 15)
    SR = 15;
  if (SR < 0)
    SR = 0;
  switch(channelMute) {
    case 0:             // SR mute disabled
      sendToPT2323(0b11111010);
      break;
    case 1:             // SR mute
      sendToPT2323(0b11111011);
      break;
  }
  sendToPT2322(0b01010000 + 15 - SR);
  EEPROM.update(12, SR);
}

void functionPT2322(int mutePT2322, int effect, int toneControl) {
  byte muteSelection;
  byte effectSelection;
  byte toneSelection;
  switch (mutePT2322) {
    case 0:
      muteSelection = 0b00000000;
      break;
    case 1:
      muteSelection = 0b00001000;
      break;
  }
  switch (effect) {
    case 0:
      effectSelection = 0b00000000;
      break;
    case 1:
      effectSelection = 0b00000100;
      break;
  }
  switch (toneControl) {
    case 0:
      toneSelection = 0b00000000;
      break;
    case 1:
      toneSelection = 0b00000010;
      break;
  }
  sendToPT2322(0b01110000 + muteSelection + effectSelection + toneSelection);
}

// ================ PT2322 & PT2323 SEND ================ //

void inputSwitchPT2322() {
  Wire.beginTransmission(ADR_PT2322);
  Wire.write(0b11000111);
  Wire.endTransmission();
}

void sendToPT2322(char c) {
  Wire.beginTransmission(ADR_PT2322);
  Wire.write(c);
  Wire.endTransmission();
}

void sendToPT2323(char c) {
  Wire.beginTransmission(ADR_PT2323);
  Wire.write(c);
  Wire.endTransmission();
}
