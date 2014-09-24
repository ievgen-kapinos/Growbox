#include <MemoryFree.h>

#include <Time.h>
// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

#include "Controller.h"
#include "Logger.h"
#include "StorageHelper.h"
#include "Watering.h"

ControllerClass::ControllerClass() :
    c_lastFreeMemory(0),
    c_isAutoCalculatedClockTimeUsed(false),
    c_lastBreezeTimeStamp(0),
    c_isDayInGrowbox(-1),
    c_fan_isOn(false),
    c_fan_numerator(0),
    c_fan_denominator(0) {
  // We set c_isDayInGrowbox == -1 to force log on startup
}

void ControllerClass::rebootController() {
  showControllerMessage(F("Reboot"));
  void (*resetFunc)(void) = 0; // initialize Software Reset function
  resetFunc(); // call zero pointer
}

void ControllerClass::update() {

  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));
  c_lastBreezeTimeStamp = now();

  checkInputPinsStatus();
  checkFreeMemory();

  updateFan();
}

void ControllerClass::updateClockState() {
  // Check hardware
  if (!GB_StorageHelper.isUseRTC()) {
    GB_Logger.stopLogError(ERROR_CLOCK_RTC_DISCONNECTED);
    GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
    GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
    return;
  }

  // Check software
  if (!isRTCPresent()) {
    GB_Logger.logError(ERROR_CLOCK_RTC_DISCONNECTED);

    GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
    GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
  }
  else {
    GB_Logger.stopLogError(ERROR_CLOCK_RTC_DISCONNECTED);

    if (isClockNotSet()) {
      GB_Logger.logError(ERROR_CLOCK_NOT_SET);
    }
    else if (isClockNeedsSync()) {
      GB_Logger.logError(ERROR_CLOCK_NEEDS_SYNC);
    }
    else {
      GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
      GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
    }
  }

}

void ControllerClass::updateAutoAdjustClockTime() {
  int16_t delta = getAutoAdjustClockTimeDelta();
  if (delta == 0) {
    return;
  }
  setClockTime(now() + delta, false);
  GB_Logger.logEvent(EVENT_CLOCK_AUTO_ADJUST);
}

void ControllerClass::checkInputPinsStatus(boolean checkFirmwareReset) {

  boolean newUseSerialMonitor = (digitalRead(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN) == HARDWARE_BUTTON_ON);

  if (newUseSerialMonitor != g_useSerialMonitor) {

    if (newUseSerialMonitor) {
      Serial.begin(9600);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      }
      g_useSerialMonitor = newUseSerialMonitor; // true
      showControllerMessage(F("Serial monitor enabled"));
    }
    else {
      showControllerMessage(F("Serial monitor disabled"));
      Serial.flush();
      Serial.end();
      g_useSerialMonitor = newUseSerialMonitor; // false
    }
  }

  if (checkFirmwareReset && g_useSerialMonitor && digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
    showControllerMessage("Resetting firmware...");
    byte counter;
    for (counter = 5; counter > 0; counter--) {
      Serial.println(counter);
      delay(1000);
      if (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) != HARDWARE_BUTTON_ON) {
        break;
      }
    }
    if (counter == 0) {
      GB_StorageHelper.resetFirmware();
      showControllerMessage("Operation finished successfully. Remove wire from Reset pin. Device will reboot automatically");
      while (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
        delay(1000);
      }
      rebootController();
    }
    else {
      showControllerMessage("Operation aborted");
    }
  }
}

void ControllerClass::checkFreeMemory() {

  int currentFreeMemory = freeMemory();
  if (currentFreeMemory < 2000) {
    GB_Logger.logError(ERROR_MEMORY_LOW);
    rebootController();
  }
  // no sense if reboot
  //  else {
  //    GB_Logger.stopLogError(ERROR_MEMORY_LOW); 
  //  }

  if (c_lastFreeMemory != currentFreeMemory) {
    if (g_useSerialMonitor) {
      showControllerMessage(F("Free memory: ["), false);
      Serial.print(currentFreeMemory);
      Serial.println(']');
    }
    c_lastFreeMemory = currentFreeMemory;
  }
}

boolean ControllerClass::isBreezeFatalError() {
  return ((now() - c_lastBreezeTimeStamp) > 10UL); // No breeze more than 10 seconds
}

/////////////////////////////////////////////////////////////////////
//                              CLOCK                              //
/////////////////////////////////////////////////////////////////////

boolean ControllerClass::initClock(time_t lastStoredTimeStamp) {

  // TODO use only if GB_StorageHelper.isUseRTC() 
  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  if (isClockNotSet() || lastStoredTimeStamp > now()) {
    // second expression can appear, if something wrong with RTC
    if (lastStoredTimeStamp != 0) {
      lastStoredTimeStamp += SECS_PER_MIN; // if not first start, we set +minute as default time
    }
    setRTCandClockTimeStamp(lastStoredTimeStamp);
    c_isAutoCalculatedClockTimeUsed = true;
  }
  else {
    c_isAutoCalculatedClockTimeUsed = false;
  }

  return c_isAutoCalculatedClockTimeUsed;
}

void ControllerClass::initClock_afterLoadConfiguration() {

  if (c_isAutoCalculatedClockTimeUsed) {
    GB_StorageHelper.setAutoCalculatedClockTimeUsed(true);
  }
  c_isAutoCalculatedClockTimeUsed = false; // we will not use this variable. Call GB_StorageHelper.getAutoCalculatedClockTimeUsed(); instead
  updateClockState();
}

boolean ControllerClass::isClockNotSet() {
  return (timeStatus() == timeNotSet);
}

boolean ControllerClass::isClockNeedsSync() {
  return (timeStatus() == timeNeedsSync);
}

// WEB command
void ControllerClass::setClockTime(time_t newTimeStamp) {
  setClockTime(newTimeStamp, true);
}

//private:

void ControllerClass::setClockTime(time_t newTimeStamp, boolean checkStartupTime) {
  time_t oldTimeStamp = now();
  long delta =
      (newTimeStamp > oldTimeStamp) ? (newTimeStamp - oldTimeStamp) : -((long)(oldTimeStamp - newTimeStamp));

  setRTCandClockTimeStamp(newTimeStamp);

  if (GB_StorageHelper.isAutoCalculatedClockTimeUsed()) {

    GB_StorageHelper.setAutoCalculatedClockTimeUsed(false);

  }

  c_lastBreezeTimeStamp += delta;
  GB_Watering.adjustLastWatringTimeOnClockSet(delta);
  // TODO what about wi-fi last active time stamp?

  // If we started with clean RTC we should update time stamps
  if (!checkStartupTime) {
    return;
  }
  if (GB_StorageHelper.getStartupTimeStamp() == 0) {
    GB_StorageHelper.adjustStartupTimeStamp(delta);
  }
  if (GB_StorageHelper.getFirstStartupTimeStamp() == 0) {
    GB_StorageHelper.adjustFirstStartupTimeStamp(delta);
  }
}
//public: 

// WEB command
void ControllerClass::setAutoAdjustClockTimeDelta(int16_t delta) {
  GB_StorageHelper.setAutoAdjustClockTimeDelta(delta);
}

int16_t ControllerClass::getAutoAdjustClockTimeDelta() {
  return GB_StorageHelper.getAutoAdjustClockTimeDelta();
}

// WEB command
void ControllerClass::setUseRTC(boolean flag) {
  GB_StorageHelper.setUseRTC(flag);

  setSyncProvider(RTC.get);   // Force resync Clock with RTC 
  updateClockState();
}

boolean ControllerClass::isUseRTC() {
  return GB_StorageHelper.isUseRTC();
}

boolean ControllerClass::isRTCPresent() {
  RTC.get(); // update status
  return RTC.chipPresent();
}

// private:

void ControllerClass::setRTCandClockTimeStamp(time_t newTimeStamp) {

  RTC.set(newTimeStamp);
  setTime(newTimeStamp);

  if (g_useSerialMonitor) {
    showControllerMessage(F("Set new Clock time ["), false);
    Serial.print(StringUtils::timeStampToString(newTimeStamp));
    Serial.println(F("] "));
  }
}

/////////////////////////////////////////////////////////////////////
//                              DEVICES                            //
/////////////////////////////////////////////////////////////////////

// public:

// Light

boolean ControllerClass::isDayInGrowbox(boolean update){

  if (!update) {
    return c_isDayInGrowbox;
  }
  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  word currentTime = hour() * 60 + minute();

  boolean isDayInGrowbox = false;
  if (upTime == downTime) {
    isDayInGrowbox = true; // Always day
  }
  else if (upTime < downTime) {
    if (upTime < currentTime && currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  else { // upTime > downTime
    if (upTime < currentTime || currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  // Log on change
  if (c_isDayInGrowbox != isDayInGrowbox) {
    if (isDayInGrowbox) {
      GB_Logger.logEvent(EVENT_MODE_DAY);
    } else {
      GB_Logger.logEvent(EVENT_MODE_NIGHT);
    }
  }
  c_isDayInGrowbox = isDayInGrowbox;
  return c_isDayInGrowbox;
}

void ControllerClass::setUseLight(boolean flag) {
  if (isUseLight() == flag) {
    return;
  }
  turnOffLight();
  GB_StorageHelper.setUseLight(flag);
  GB_Logger.logEvent(flag ? EVENT_LIGHT_ENABLED : EVENT_LIGHT_DISABLED);
}

boolean ControllerClass::isUseLight() {
  return GB_StorageHelper.isUseLight();
}

void ControllerClass::turnOnLight() {
  if (!isUseLight()) {
    return;
  }
  if (digitalRead(LIGHT_PIN) == RELAY_ON) {
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_LIGHT_ON);
}

void ControllerClass::turnOffLight() {
  if (!isUseLight()) {
    return;
  }
  if (digitalRead(LIGHT_PIN) == RELAY_OFF) {
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_LIGHT_OFF);
}

boolean ControllerClass::isLightTurnedOn() {
  return (digitalRead(LIGHT_PIN) == RELAY_ON);
}

// Fan

void ControllerClass::setUseFan(boolean flag) {
  if (isUseFan() == flag) {
    return;
  }
  turnOffFan();
  GB_StorageHelper.setUseFan(flag);
  GB_Logger.logEvent(flag ? EVENT_FAN_ENABLED : EVENT_FAN_DISABLED);
}
boolean ControllerClass::isUseFan() {
  return GB_StorageHelper.isUseFan();
}

void ControllerClass::turnOnFan(byte speed, byte numerator, byte denominator) {
  if (!isUseFan()) {
    return;
  }
  if (c_fan_isOn && c_fan_speed == speed && numerator == c_fan_numerator && denominator == c_fan_denominator) {
    return;
  }
  if (numerator > B1111 || denominator > B1111 || numerator > denominator ){ // Error argument
    numerator = 0;
    denominator = 0;
  }
  c_fan_isOn = true;
  c_fan_speed = speed;
  c_fan_numerator = numerator;
  c_fan_denominator = denominator;

  byte logData = (c_fan_numerator << 4) | denominator;
  if (speed == FAN_SPEED_MIN) {
    GB_Logger.logEvent(EVENT_FAN_ON_MIN, logData);
  }
  else {
    GB_Logger.logEvent(EVENT_FAN_ON_MAX, logData);
  }

  updateFan();
}

void ControllerClass::turnOffFan() {
  if (!isUseFan()) {
    return;
  }
  if (!c_fan_isOn) {
    return;
  }
  c_fan_isOn = false;

  GB_Logger.logEvent(EVENT_FAN_OFF);

  updateFan();
}

boolean ControllerClass::isFanTurnedOn() {
  return c_fan_isOn;
}
byte ControllerClass::getFanSpeed() {
  return c_fan_speed;
}
byte ControllerClass::getFanNumerator() {
  return c_fan_numerator;
}
byte ControllerClass::getFanDenominator() {
  return c_fan_denominator;
}
void ControllerClass::updateFan() {
  boolean isFanOnNow = c_fan_isOn;
  if (c_fan_isOn && c_fan_numerator != 0 && c_fan_denominator != 0){
    int secondsOnClockNow = minute() * SECS_PER_MIN + second(); // seconds now
    secondsOnClockNow %= c_fan_denominator * UPDATE_GROWBOX_STATE_DELAY_SEC;
    isFanOnNow = (secondsOnClockNow < (c_fan_numerator * UPDATE_GROWBOX_STATE_DELAY_SEC));
  }
  if (isFanOnNow){
    digitalWrite(FAN_SPEED_PIN, c_fan_speed);
    digitalWrite(FAN_PIN, RELAY_ON);
  }
  else {
    digitalWrite(FAN_PIN, RELAY_OFF);
    digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  }
}

// Heater

void ControllerClass::setUseHeater(boolean flag) {
  if (isUseHeater() == flag) {
    return;
  }
  turnOffHeater();
  GB_StorageHelper.setUseHeater(flag);
  GB_Logger.logEvent(flag ? EVENT_HEATER_ENABLED : EVENT_HEATER_DISABLED);
}

boolean ControllerClass::isUseHeater() {
  return GB_StorageHelper.isUseHeater();
}

void ControllerClass::turnOnHeater() {
  if (!isUseHeater()) {
    return;
  }
  if (digitalRead(HEATER_PIN) == RELAY_ON) {
    return;
  }
  digitalWrite(HEATER_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_HEATER_ON);
}

void ControllerClass::turnOffHeater() {
  if (!isUseHeater()) {
    return;
  }
  if (digitalRead(HEATER_PIN) == RELAY_OFF) {
    return;
  }
  digitalWrite(HEATER_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_HEATER_OFF);
}

boolean ControllerClass::isHeaterTurnedOn() {
  return (digitalRead(HEATER_PIN) == RELAY_ON);
}

ControllerClass GB_Controller;

