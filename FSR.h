#ifndef _FSR_H_
#define _FSR_H_

class FSR {
public:
  FSR(const int pin); 
  float getResistance();
  float getForce();
  float Vcc, res;

private:
  const int m_pin;
};

#endif
