#include "SequenceLED.h"

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

}

void SequenceLED::update() {

  if (_running) {
      if ( millis() - _lastLEDTime > _tempo ) {
        _bar++;
        if ( *_bar == 0 ) {
          if (_repeat) { 
            _bar = _sequence; 
           } else {
            digitalWrite(_pin, _usualState);
            _running = false;
            return;
          }
        }
        digitalWrite(_pin, *_bar - '0' );
        _lastLEDTime = millis();
      }
  } 
  
  

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

