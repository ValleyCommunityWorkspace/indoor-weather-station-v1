#include "Button.h"

/* Button Connected from pin to Ground
*/
Button::Button(uint8_t pin)
{
	_pin=pin;
  _lastButtonState = HIGH;
  _lastDebounceTime = millis();
 pinMode(_pin, INPUT_PULLUP);
 
}



bool Button::pushed(unsigned long duration)
{
  
    int reading = digitalRead(_pin);
    if (reading == HIGH ) {
      _lastDebounceTime = millis();
      _lastButtonState = HIGH;
    }

    if ((millis() - _lastDebounceTime) > duration) {
      if (_lastButtonState != reading) {
          _lastButtonState = reading;
          return true;
      }
    } 
      
    return false;
    

}

