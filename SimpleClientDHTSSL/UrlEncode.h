#ifndef URL_ENCODE_HEADER
#define URL_ENCODE_HEADER

#include "Arduino.h"

String urldecode(String str);

String urlencode(String str);

unsigned char h2int(char c);

#endif // #ifndef URL_ENCODE_HEADER
