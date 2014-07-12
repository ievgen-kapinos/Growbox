#ifndef GB_StringUtils_h
#define GB_StringUtils_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#include <Time.h> 

/////////////////////////////////////////////////////////////////////
//                         FALASH STRINGS                          //
/////////////////////////////////////////////////////////////////////

namespace StringUtils {
  int flashStringLength(const char PROGMEM* pstr);

  char flashStringCharAt(const char PROGMEM* pstr, int index, boolean checkOverflow = true);
  boolean flashStringEquals(const String &str, const char PROGMEM* pstr);
  boolean flashStringEquals(const char* cstr, size_t cstr_length, const char PROGMEM* pstr);
  boolean flashStringStartsWith(const String &str, const char PROGMEM* pstr);
  boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const char PROGMEM* pstr);
  boolean flashStringEndsWith(const String &str, const char PROGMEM* pstr);
  String flashStringLoad(const char PROGMEM* pstr);

  String flashStringLoad(const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const String &str, const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr);
  boolean flashStringEquals(const String &str, const __FlashStringHelper* fstr);
  int flashStringLength(const __FlashStringHelper* fstr);
  char flashStringCharAt(const __FlashStringHelper* fstr, int index);
  boolean flashStringEquals(const char* cstr, size_t length, const __FlashStringHelper* fstr);

  String getFixedDigitsString(const int number, const byte numberOfDigits);
  String getHEX(byte number, boolean addPrefix = false);
  String floatToString(float number);
  String getTimeString(time_t time);

}

#endif



