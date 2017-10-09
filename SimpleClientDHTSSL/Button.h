#ifndef __TIMB_BUTTON_H
#define __TIMB_BUTTON_H


#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif


class Button {
public:
	Button(uint8_t pin=D3);
	bool pushed(unsigned long duration=1000);
	
private:
	uint8_t _pin;
  int _lastButtonState;
  unsigned long _lastDebounceTime;

};


#endif
