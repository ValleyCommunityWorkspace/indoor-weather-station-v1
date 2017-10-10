#ifndef __TIMB_SEQUENCELED_H
#define __TIMB_SEQUENCELED_H


#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

extern int hex_digit_to_int(char c);

class SequenceLED {
public:
	SequenceLED(uint8_t pin=D4, int usualState=HIGH);
	void startSequence(const char * sequence, unsigned long tempo, bool repeat );
  void update();
  
private:
	uint8_t _pin;
  const char * _sequence;
  const char * _bar;
  unsigned long _tempo;
  bool _repeat;
  bool _running;
  int _usualState;
  
  int _lastLEDState;
  unsigned long _lastLEDTime;

};


#endif
