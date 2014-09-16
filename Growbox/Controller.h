#ifndef Controller_h
#define Controller_h

#include "Global.h"

class ControllerClass{

  int c_lastFreeMemory;
  boolean c_isAutoCalculatedClockTimeUsed;
  time_t c_lastBreezeTimeStamp;
public:
  ControllerClass();

  void rebootController();

  void update();
  void updateClockState();
  void updateAutoAdjustClockTime();

  // discover memory overflow errors in the arduino C++ code
  void checkInputPinsStatus(boolean checkFirmwareReset = false);
  void checkFreeMemory();
  boolean isBreezeFatalError();

  /////////////////////////////////////////////////////////////////////
  //                              CLOCK                              //
  /////////////////////////////////////////////////////////////////////

  boolean initClock(time_t defaultTimeStamp);
  void initClock_afterLoadConfiguration();

  boolean isClockNotSet();
  boolean isClockNeedsSync();

  void setClockTime(time_t newTimeStamp);
private:
  void setClockTime(time_t newTimeStamp, boolean checkStartupTime);
public:
  void setAutoAdjustClockTimeDelta(int16_t delta);
  int16_t getAutoAdjustClockTimeDelta();

  void setUseRTC(boolean flag);
  boolean isUseRTC();
  boolean isRTCPresent();

private:
  void setRTCandClockTimeStamp(time_t newTimeStamp);

  /////////////////////////////////////////////////////////////////////
  //                              DEVICES                            //
  /////////////////////////////////////////////////////////////////////
public:
  void setUseLight(boolean flag);
  boolean isUseLight();
  void setUseFan(boolean flag);
  boolean isUseFan();

  void turnOnLight();
  void turnOffLight();
  boolean isLightTurnedOn();

  void turnOnFan(int speed);
  void turnOffFan();
  boolean isFanTurnedOn();
  byte getFanSpeed();

  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////
private:
  template<class T>
    void showControllerMessage(T str, boolean newLine = true) {
      if (g_useSerialMonitor) {
        Serial.print(F("CONTROLLER> "));
        Serial.print(str);
        if (newLine) {
          Serial.println();
        }
      }
    }
};

extern ControllerClass GB_Controller;

#endif

