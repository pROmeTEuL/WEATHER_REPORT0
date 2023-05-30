#include "esp32-hal.h"
#include "dht.h"

DHT::DHT(int pin)
{
  dataPin = pin;
}

void DHT::read_dht()
{
  uint8_t recvBuffer[5] = {0, 0, 0, 0, 0};
  uint8_t cnt = 7;
  uint8_t idx = 0;
  pinMode(dataPin, OUTPUT);
  digitalWrite(dataPin, LOW);
  delay(18);
  digitalWrite(dataPin, HIGH);
  delayMicroseconds(40);
  pinMode(dataPin, INPUT);
  pulseIn(dataPin, HIGH);
  for (int i = 0; i < 40; ++i) {
    unsigned int timeout = 10000;
    while (digitalRead(dataPin) == LOW) {
      if (timeout == 0) {
        Serial.println("timeout");
        return;
      }
      --timeout;
    }
    unsigned long t = micros();
    timeout = 10000;
    while (digitalRead(dataPin) == HIGH) {
      if (timeout == 0) {
        Serial.println("timeout");
        return;
      }
      --timeout;
    }
    if (micros() - t > 40)
      recvBuffer[idx] |= (1<<cnt);
    if (cnt == 0) {
      cnt = 7;
      ++idx;
    } else {
      --cnt;
    }
  }
  humidity = recvBuffer[0] + float(recvBuffer[1]) / 100.f;
  temperature = recvBuffer[2] + float(recvBuffer[3]) / 100.f;
  uint8_t sum = recvBuffer[0] + recvBuffer[2];
  if (recvBuffer[4] != sum)
    Serial.println("Checksum fail!");
}