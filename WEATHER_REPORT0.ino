#include <LiquidCrystal.h>

const int 
d0 = 4,
d1 = 16,
d2 = 5,
d3 = 18,
d4 = 19,
d5 = 21,
d6 = 22,
d7 = 23;
const int rs = 15;
const int e = 8;
const int mic_pin = 2;

LiquidCrystal lcd(22, 21, 5, 18, 23, 19);


void setup() {
  Serial.begin(115200);
  pinMode(mic_pin, INPUT);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("db: ");
}

void loop() {
  unsigned int min = 1 << 10;
  unsigned int max = 0;
  unsigned int sensorValue = 0;
  int count = 200;
  while (count--) {
    sensorValue = analogRead(mic_pin);
    if (sensorValue < min) min = sensorValue;
    if (sensorValue > max) max = sensorValue;
    //      delay(1);
  }
  sensorValue = max - min;
  float voltage = sensorValue * (3.3 / 3071.0);
  float dB = 20 * log10(voltage / 0.00632);
  lcd.setCursor(4, 0);
  lcd.print(dB);
  delay(400);
}
