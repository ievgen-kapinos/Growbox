// Warning! We need to include all used libraries, 
// otherwise Arduino IDE doesn't set correct build 
// params for gcc compilator
#include <MemoryFree.h>

#include <Time.h>
// We use free() method
#define USE_SPECIALIST_METHODS  
#include <TimeAlarms.h>

// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

// Termometer
#include <OneWire.h>
#include <DallasTemperature.h>

// Growbox
#include "Global.h"
#include "Controller.h"
#include "StorageHelper.h"
#include "Thermometer.h"
#include "RAK410_XBeeWifi.h"
#include "WebServer.h"
#include "Watering.h"

/////////////////////////////////////////////////////////////////////
//                              STATUS                             //
/////////////////////////////////////////////////////////////////////

boolean isDayInGrowbox(){
  if(timeStatus() == timeNeedsSync){
    GB_Logger.logError(ERROR_TIMER_NEEDS_SYNC);
  } 
  else {
    GB_Logger.stopLogError(ERROR_TIMER_NEEDS_SYNC);
  }

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  word currentTime = hour() * 60 + minute();

  boolean isDayInGrowbox = false;
  if (upTime < downTime){
    if (upTime < currentTime && currentTime < downTime ){
      isDayInGrowbox = true;
    }
  } 
  else { // upTime > downTime
    if (upTime < currentTime || currentTime < downTime ){
      isDayInGrowbox = true;
    }
  } 
  return isDayInGrowbox;
}


void printStatusOnBoot(const __FlashStringHelper* str){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(F("Checking "));
    Serial.print(str);
    Serial.println(F("..."));
  }
}

void stopOnFatalError(const __FlashStringHelper* str){ //TODO 
  digitalWrite(ERROR_PIN, HIGH);
  if (g_useSerialMonitor){
    Serial.print(F("Fatal error: "));
    Serial.println(str);
  }
  while(true) delay(5000); // Stop boot process
}

/////////////////////////////////////////////////////////////////////
//                                MAIN                             //
/////////////////////////////////////////////////////////////////////

// the setup routine runs once when you press reset:
void setup() {                

  // Initialize the info pins
  pinMode(LED_BUILTIN, OUTPUT); // brease
  pinMode(BREEZE_PIN, OUTPUT);  // brease
  pinMode(ERROR_PIN, OUTPUT);

  // Hardware buttons
  pinMode(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN, INPUT_PULLUP);
  pinMode(HARDWARE_BUTTON_RESET_FIRMWARE_PIN, INPUT_PULLUP);

  // Reley pins
  pinMode(LIGHT_PIN, OUTPUT);   
  pinMode(FAN_PIN, OUTPUT);  
  pinMode(FAN_SPEED_PIN, OUTPUT); 

  // Configure relay
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);


  // Watering
  for (int i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
    pinMode(WATERING_WET_SENSOR_IN_PINS[i], INPUT_PULLUP);

    pinMode(WATERING_WET_SENSOR_POWER_PINS[i], OUTPUT);
    pinMode(WATERING_PUMP_PINS[i], OUTPUT);

    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[i], HIGH);
    digitalWrite(WATERING_PUMP_PINS[i], RELAY_OFF);
  }
  unsigned long pinConfiguredMillis = millis();

  // Configure inputs
  //attachInterrupt(0, interrapton0handler, CHANGE); // PIN 2

  GB_Controller.checkInputPins(); // Check for Serial monitor
  GB_Controller.checkFreeMemory();
  
  printStatusOnBoot(F("software configuration"));
  initLoggerModel();
  if (!Event::isInitialized()){
    stopOnFatalError(F("not all Events initialized"));
  }  
  if (!WateringEvent::isInitialized()){
    stopOnFatalError(F("not all Watering Events initialized"));
  }
  if (!Error::isInitialized()){
    stopOnFatalError(F("not all Errors initialized"));
  }
  if (BOOT_RECORD_SIZE != sizeof(BootRecord)){
    if (g_useSerialMonitor){
      Serial.print(F("Expected "));
      Serial.print(BOOT_RECORD_SIZE);  
      Serial.print(F(" bytes, used ")); 
      Serial.println(sizeof(BootRecord));
      Serial.print(F(" bytes")); 
    }
    stopOnFatalError(F("wrong BootRecord size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_WET_SENSOR_IN_PINS)){
    stopOnFatalError(F("wrong WATERING_WET_SENSOR_IN_PINS size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_WET_SENSOR_POWER_PINS)){
    stopOnFatalError(F("wrong WATERING_WET_SENSOR_POWER_PINS size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_PUMP_PINS)){
    stopOnFatalError(F("wrong WATERING_PUMP_PINS size"));
  }
  GB_Controller.checkFreeMemory();
  
  printStatusOnBoot(F("storage"));
  if (!GB_StorageHelper.init()){
    stopOnFatalError(F("Not all required starage hardware present"));
  }
  GB_Controller.checkFreeMemory();
  
  printStatusOnBoot(F("termometer"));
  GB_Thermometer.init();
  while(!GB_Thermometer.updateStatistics()) { // Load temperature on startup
    delay(1000);
  }
  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    Serial.println(F("Growbox successfully booted up"));
  }
  
  printStatusOnBoot(F("clock")); // used in GB_StorageHelper.loadConfiguration.. needs log
  GB_Controller.startupClock(); // may be disconnected - it is OK
  GB_Controller.checkFreeMemory();

  printStatusOnBoot(F("stored configuration"));
  g_isGrowboxStarted = true;
  GB_StorageHelper.loadConfiguration();  // after we a ready for logging
  GB_Controller.checkFreeMemory();

  //printStatusOnBoot(F("watering systems")); // used time
  time_t pinConfiguredTime = now() - (millis() - pinConfiguredMillis)/1000;
  GB_Watering.init(pinConfiguredTime); // call before updateGrowboxState();
  GB_Controller.checkFreeMemory();

  // Max 6 Alarms in instance
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(UPDATE_CONTROLLER_STATUS_DELAY, updateController);
  Alarm.timerRepeat(UPDATE_WIFI_STATUS_DELAY, updateWiFiStatus); 
  Alarm.timerRepeat(UPDATE_BREEZE_DELAY, updateBreezeStatus); 
  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);  // repeat every N seconds

  // TODO update if configuration changed
  // Create suolemental rare switching
  //Alarm.alarmRepeat(upHour, upMinute, 00, switchToDayMode);      // repeat once every day
  //Alarm.alarmRepeat(downHour, downMinute, 00, switchToNightMode);  // repeat once every day
  GB_Controller.checkFreeMemory();

  updateGrowboxState();
  GB_Controller.checkFreeMemory();

  printStatusOnBoot(F("Wi-Fi"));
  RAK410_XBeeWifi.init(); // check is Present - once
  GB_Controller.checkFreeMemory();
 

  if (RAK410_XBeeWifi.isPresent()){ 
    RAK410_XBeeWifi.updateWiFiStatus(); // start Web server
  }
  GB_Controller.checkFreeMemory();
  
  GB_Watering.updateWateringSchedule(); // Run watering after init

  if(g_useSerialMonitor){ 
    Serial.println(F("Growbox successfully started"));
  }

}

// the loop routine runs over and over again forever:
void loop() {
  //WARNING! We need quick response for Serial events, thay handled afer each loop. So, we decreese delay to zero 
  Alarm.delay(0); 
  
  // We use another instance of Alarm object to increase MAX alarms count (6 by default, look dtNBR_ALARMS in TimeAlarms.h
  GB_Watering.updateAlarms();
}

void serialEvent(){

  if(!g_isGrowboxStarted){
    // We will not handle external events during startup
    return;
  }

  boolean forceUpdate = GB_WebServer.handleSerialMonitorEvent();
  if (forceUpdate){
    updateGrowboxState();
  }
}

// Wi-Fi is connected to Serial1
void serialEvent1(){

  if(!g_isGrowboxStarted){
    // We will not handle external events during startup
    return;
  }
  //  String input;
  //  Serial_readString(input); // at first we should read, after manipulate  
  //  Serial.println(input);
  boolean forceUpdate = GB_WebServer.handleSerialEvent();
  if (forceUpdate){
    updateGrowboxState();
  }
}


/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {
 
  GB_Watering.turnOnWetSensors();
  
  // Init/Restore growbox state
  if (isDayInGrowbox()){
    if (g_isDayInGrowbox != true){
      g_isDayInGrowbox = true;
      GB_Logger.logEvent(EVENT_MODE_DAY);
    }
  } 
  else {
    if (g_isDayInGrowbox != false){
      g_isDayInGrowbox = false;
      GB_Logger.logEvent(EVENT_MODE_NIGHT);
    }
  }

  byte normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
  GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

  float temperature = GB_Thermometer.getTemperature();

  if (temperature >= criticalTemperatue){
    turnOffLight();
    turnOnFan(FAN_SPEED_MAX);
    GB_Logger.logError(ERROR_TERMOMETER_CRITICAL_VALUE);
  } 
  else if (g_isDayInGrowbox) {
    // Day mode
    turnOnLight();
    if (temperature < normalTemperatueDayMin) {
      // Too cold, no heater
      turnOnFan(FAN_SPEED_MIN); // no wind, no grow
    } 
    else if (temperature > normalTemperatueDayMax){
      // Too hot
      turnOnFan(FAN_SPEED_MAX);    
    } 
    else {
      // Normal, day wind
      turnOnFan(FAN_SPEED_MIN); 
    }
  } 
  else { 
    // Night mode
    turnOffLight(); 
    if (temperature < normalTemperatueNightMin) {
      // Too cold, Nothig to do, no heater
      turnOffFan(); 
    } 
    else if (temperature > normalTemperatueNightMax){
      // Too hot
      turnOnFan(FAN_SPEED_MIN);    
    } 
    else {
      // Normal, all devices are turned off
      turnOffFan(); 
    }
  }

  GB_Watering.turnOffWetSensorsAndUpdateWetStatus();

}


/////////////////////////////////////////////////////////////////////
//                              SCHEDULE                           //
/////////////////////////////////////////////////////////////////////

void updateThermometerStatistics(){ // should return void
  GB_Thermometer.updateStatistics(); 
}

void updateWiFiStatus(){ // should return void
  RAK410_XBeeWifi.updateWiFiStatus(); 
}

void updateController(){ // should return void
  GB_Controller.update(); 
}

void updateBreezeStatus() {
  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));
}

/////////////////////////////////////////////////////////////////////
//                              DEVICES                            //
/////////////////////////////////////////////////////////////////////


void turnOnLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_ON){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_LIGHT_ON);
}

void turnOffLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);  
  GB_Logger.logEvent(EVENT_LIGHT_OFF);
}

void turnOnFan(int speed){
  if (digitalRead(FAN_PIN) == RELAY_ON && digitalRead(FAN_SPEED_PIN) == speed){
    return;
  }
  digitalWrite(FAN_SPEED_PIN, speed);
  digitalWrite(FAN_PIN, RELAY_ON);

  if (speed == FAN_SPEED_MIN){
    GB_Logger.logEvent(EVENT_FAN_ON_MIN);
  } 
  else {
    GB_Logger.logEvent(EVENT_FAN_ON_MAX);
  }
}

void turnOffFan(){
  if (digitalRead(FAN_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_FAN_OFF);
}



