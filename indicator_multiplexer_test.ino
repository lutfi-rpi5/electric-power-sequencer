#include <Wire.h>
#include "LiquidCrystal_I2C.h"

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// MUX
#define S0 16
#define S1 17
#define S2 18
#define S3 19
#define SIG 23

// Relay state (logika)
bool relayState[8] = {0};

// ===== BYTE CHAR =====
// ===== BYTE CHAR =====
byte ch1[] = {
  B00000,
  B00000,
  B01100,
  B00100,
  B00100,
  B00100,
  B01110,
  B00000
};

byte ch1inv[] = {
  B11111,
  B11111,
  B10011,
  B11011,
  B11011,
  B11011,
  B10001,
  B11111
};

byte ch2[] = {
  B00000,
  B00000,
  B01110,
  B00010,
  B01110,
  B01000,
  B01110,
  B00000
};

byte ch2inv[] = {
  B11111,
  B11111,
  B10001,
  B11101,
  B10001,
  B10111,
  B10001,
  B11111
};

byte ch3[] = {
  B00000,
  B00000,
  B01110,
  B00010,
  B01110,
  B00010,
  B01110,
  B00000
};

byte ch3inv[] = {
  B11111,
  B11111,
  B10001,
  B11101,
  B10001,
  B11101,
  B10001,
  B11111
};

byte ch4[] = {
  B00000,
  B00000,
  B01010,
  B01010,
  B01110,
  B00010,
  B00010,
  B00000
};

byte ch4inv[] = {
  B11111,
  B11111,
  B10101,
  B10101,
  B10001,
  B11101,
  B11101,
  B11111
};

byte ch5[] = {
  B00000,
  B00000,
  B01110,
  B01000,
  B01110,
  B00010,
  B01110,
  B00000
};

byte ch5inv[] = {
  B11111,
  B11111,
  B10001,
  B10111,
  B10001,
  B11101,
  B10001,
  B11111
};

byte ch6[] = {
  B00000,
  B00000,
  B01110,
  B01000,
  B01110,
  B01010,
  B01110,
  B00000
};

byte ch6inv[] = {
  B11111,
  B11111,
  B10001,
  B10111,
  B10001,
  B10101,
  B10001,
  B11111
};

byte ch7[] = {
  B00000,
  B00000,
  B01110,
  B00010,
  B00010,
  B00010,
  B00010,
  B00000
};

byte ch7inv[] = {
  B11111,
  B11111,
  B10001,
  B11101,
  B11101,
  B11101,
  B11101,
  B11111
};

byte ch8[] = {
  B00000,
  B00000,
  B01110,
  B01010,
  B01110,
  B01010,
  B01110,
  B00000
};

byte ch8inv[] = {
  B11111,
  B11111,
  B10001,
  B10101,
  B10001,
  B10101,
  B10001,
  B11111
};

// ===== CHAR POINTER =====
byte *charOff[8] = {ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8};
byte *charOn[8]  = {ch1inv, ch2inv, ch3inv, ch4inv, ch5inv, ch6inv, ch7inv, ch8inv};

// ===== MUX SELECT =====
void selectChannel(int ch) {
  digitalWrite(S0, ch & 0x01);
  digitalWrite(S1, (ch >> 1) & 0x01);
  digitalWrite(S2, (ch >> 2) & 0x01);
  digitalWrite(S3, (ch >> 3) & 0x01);
}

// ===== UPDATE CHAR =====
void updateChar(int i) {
  if (relayState[i]) {
    lcd.createChar(i, charOn[i]);
  } else {
    lcd.createChar(i, charOff[i]);
  }
  delay(2); // fix LCD timing
}

// ===== UPDATE LCD =====
void updateLCD() {
  delay(2);

  lcd.setCursor(4, 1);
  for (int i = 0; i < 8; i++) {
    lcd.write(byte(i));
  }
}

// ===== SET STATE ONLY =====
void setRelayState(int ch, bool state) {
  if (ch < 0 || ch > 7) return;

  relayState[ch] = state;
  updateChar(ch);
  updateLCD();
}

// ===== SET ALL =====
void setAll(bool state) {
  for (int i = 0; i < 8; i++) {
    relayState[i] = state;
    updateChar(i);
  }
  updateLCD();
}

// ===== SERIAL =====
String input = "";

void handleCommand(String cmd) {
  cmd.toLowerCase();
  cmd.trim();

  if (cmd == "allon") {
    setAll(HIGH);
  }
  else if (cmd == "alloff") {
    setAll(LOW);
  }
  else {
    int ch = cmd.charAt(0) - '1';

    if (cmd.endsWith("on")) {
      setRelayState(ch, HIGH);
    }
    else if (cmd.endsWith("off")) {
      setRelayState(ch, LOW);
    }
  }
}

// ===== MUX SCANNING =====
// ini yang bikin semua relay “terasa” tetap ON
void refreshMux() {
  static int ch = 0;

  selectChannel(ch);
  digitalWrite(SIG, relayState[ch]);

  ch++;
  if (ch >= 8) ch = 0;
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  Wire.begin();
  lcd.begin();
  lcd.backlight();

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SIG, OUTPUT);

  lcd.setCursor(1, 0);
  // lcd.print("RelayControl");
  lcd.print("PowerSequencer");

  // init char
  for (int i = 0; i < 8; i++) {
    updateChar(i);
  }

  updateLCD();
}

// ===== LOOP =====
void loop() {
  // serial non-blocking
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (input.length() > 0) {
        handleCommand(input);
        input = "";
      }
    } else {
      input += c;
    }
  }

  // refresh MUX terus menerus
  refreshMux();
}