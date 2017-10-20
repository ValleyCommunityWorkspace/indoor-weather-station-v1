#include "SequenceLED.h"

extern "C" {
#include "user_interface.h"
}

os_timer_t myTimer;

// start of timerCallback
void timerCallback(void *pArg) {

      SequenceLED * self = (SequenceLED *) pArg;

      if (self->_running) {
        self->_bar++;
        if ( *(self->_bar) == 0 ) {
            if (self->_repeat) { 
              self->_bar = self->_sequence; 
             } else {
              digitalWrite(self->_pin, self->_usualState);
              self->_running = false;
              os_timer_arm((_ETSTIMER_*)&myTimer, self->_tempo, false);  // run 1 more time and stop.
              return;
            }
         }
         digitalWrite(self->_pin, *(self->_bar) - '0' );
      }

} // End of timerCallback

/* Button Connected from pin to Ground
*/
SequenceLED::SequenceLED(uint8_t pin, int usualState)
{
	_pin=pin;
  _sequence = "H";
  _bar = _sequence;
  _tempo = 100; //ms
  _repeat = false;
  _lastLEDTime = millis();
  _usualState = usualState;
  _running = false;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, _usualState);
  //analogWriteRange(16);
  //analogWrite(_pin, PWMRANGE);

  os_timer_setfn((_ETSTIMER_*)&myTimer, timerCallback, this);
}

void SequenceLED::startSequence(const char * sequence, unsigned long tempo, bool repeat )
{
  _sequence = sequence;  // "1010"
  _bar = _sequence;
  _tempo = tempo; //ms
  _repeat = repeat;

  _running = true;


  digitalWrite(_pin, *_bar - '0' );
  _lastLEDTime = millis();

  os_timer_arm((_ETSTIMER_*)&myTimer, _tempo, true);

}

void SequenceLED::stopSequence() {

      digitalWrite(_pin, _usualState);
      _running = false;
      os_timer_arm((_ETSTIMER_*)&myTimer, _tempo, false);
      return;
}


int hex_digit_to_int(char c) {
    switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
    }
}

