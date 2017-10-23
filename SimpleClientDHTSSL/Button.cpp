#include "Button.h"

/* Button Connected from pin to Ground
*/
Button::Button(uint8_t pin,unsigned long duration)
{
	_pin=pin;
  _duration=duration;
  _lastButtonState = HIGH;
  _lastDebounceTime = millis();
 pinMode(_pin, INPUT_PULLUP);
 
}



bool Button::pushed()
{
  
    int reading = digitalRead(_pin);
    if (reading == HIGH ) {
      _lastDebounceTime = millis();
      _lastButtonState = HIGH;
    }

    if ((millis() - _lastDebounceTime) > _duration) {
      if (_lastButtonState != reading) {
          _lastButtonState = reading;
          return true;
      }
    } 
      
    return false;
    

}

