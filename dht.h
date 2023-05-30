#ifndef DHT_H
#define DHT_H
#include <Arduino.h>

class DHT {
public:
  DHT(int pin);
  void read_dht();
  float temperature = 0.f;
  float humidity = 0.f;
private:
  int dataPin;
};

#endif