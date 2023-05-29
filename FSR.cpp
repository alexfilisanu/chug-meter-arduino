#include "FSR.h"
#include <Arduino.h>

FSR::FSR(const int pin) : m_pin(pin), Vcc(5), res(3300) {
  pinMode(pin, INPUT);
}

float FSR::getResistance() {
  float senVoltage = analogRead(m_pin) * Vcc / 1023;
  return res * (Vcc / senVoltage - 1);
}

float FSR::getForce() {
  float resistance = getResistance();
  // calculate force using curve broken into two parts of different slope
  if (resistance <= 600)
    return (1.0 / resistance - 0.00075) / 0.00000032639;
  else
    return (1.0 / resistance)  / 0.000000642857;
}
