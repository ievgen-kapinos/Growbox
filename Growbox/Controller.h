#ifndef Controller_h
#define Controller_h

#include "Global.h"

class ControllerClass{
  
  int c_freeMemoryLastCheck;
  
public:
  ControllerClass();

  void rebootController();

  // discover memory overflow errors in the arduino C++ code
  void checkFreeMemory();
  
  void updateHardwareInputStatus();
  
  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showControllerMessage(T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("CONTROLLER> "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }
};

extern ControllerClass GB_Controller;

#endif

