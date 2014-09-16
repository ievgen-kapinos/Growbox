#ifndef Global_h
#define Global_h

#include <OneWire.h>

#include "ArduinoPatch.h"
#include "StringUtils.h"

/////////////////////////////////////////////////////////////////////
//                     HARDWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// relay pins
const byte LIGHT_PIN = 2;
const byte FAN_PIN = 3;
const byte FAN_SPEED_PIN = 4;

// 1-Wire pins
const byte ONE_WIRE_PIN = 12;

// status pins
const byte ERROR_PIN = 11;
const byte BREEZE_PIN = LED_BUILTIN; //13

// Watering
const byte MAX_WATERING_SYSTEMS_COUNT = 4;
const byte WATERING_WET_SENSOR_IN_PINS[] = {A0, A1, A2, A3};
const byte WATERING_WET_SENSOR_POWER_PINS[] = {22, 24, 26, 28};
const byte WATERING_PUMP_PINS[] = {23, 25, 27, 29};

// hardware buttons
const byte HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN = 53; // pullup used, 0 - enabled, 1 - disabled
const byte HARDWARE_BUTTON_RESET_FIRMWARE_PIN = 52; // pullup used, 0 - enabled, 1 - disabled (works when Serial Monitor enabled)

/////////////////////////////////////////////////////////////////////
//                          PIN CONSTANTCS                         //
/////////////////////////////////////////////////////////////////////

// Relay consts
const byte RELAY_ON = LOW, RELAY_OFF = HIGH;

// Serial consts
const byte HARDWARE_BUTTON_OFF = HIGH, HARDWARE_BUTTON_ON = LOW;

// Fun speed
const byte FAN_SPEED_MIN = RELAY_OFF;
const byte FAN_SPEED_MAX = RELAY_ON;

/////////////////////////////////////////////////////////////////////
//                             DELAY'S                             //
/////////////////////////////////////////////////////////////////////

// Minimum Growbox reaction time
const time_t UPDATE_BREEZE_DELAY = 1;
const time_t UPDATE_GROWBOX_STATE_DELAY = 5 * 60; // 5 min
const time_t UPDATE_CONTROLLER_STATE_DELAY = 1;
const time_t UPDATE_CONTROLLER_CORE_HARDWARE_STATE_DELAY = 60; // 60 sec
const time_t UPDATE_CONTROLLER_AUTO_ADJUST_CLOCK_TIME_DELAY = SECS_PER_DAY; // 1 day
const time_t UPDATE_TERMOMETER_STATISTICS_DELAY = 20; //20 sec 
const time_t UPDATE_WEB_SERVER_STATUS_DELAY = 3 * 60; // 2min

const int UPDATE_WEB_SERVER_AVERAGE_PAGE_LOAD_DELAY = 3; // 3 sec 

const int WATERING_SYSTEM_TURN_ON_DELAY = 3; // 3 sec 

// error blinks in milleseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;

// Watering
const long WATERING_MAX_SCHEDULE_CORRECTION_TIME = 6 * 60 * 60; // hours
const long WATERING_ERROR_DELTA = 5 * 60; // 6 minutes

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
extern OneWire g_oneWirePin;
extern byte g_isDayInGrowbox;
extern boolean g_useSerialMonitor;

/////////////////////////////////////////////////////////////////////
//                         GLOBAL STRINGS                          //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_DEFAULT_SSID[] PROGMEM = "Growbox"; //WPA2 in AP mode
const char S_WIFI_DEFAULT_PASS[] PROGMEM = "ingodwetrust";

const char S_empty[] PROGMEM = "";
const char S___[] PROGMEM = "  ";
const char S_CRLF[] PROGMEM = "\r\n";
const char S_CRLFCRLF[] PROGMEM = "\r\n\r\n";
const char S_Connected[] PROGMEM = "Connected";
const char S_Disconnected[] PROGMEM = "Disconnected";
const char S_Enabled[] PROGMEM = "Enabled";
const char S_Disabled[] PROGMEM = "Disabled";
const char S_Temperature[] PROGMEM = "Temperature";
const char S_Free_memory[] PROGMEM = "Free memory: ";
const char S_bytes[] PROGMEM = " bytes";
const char S_PlusMinus[] PROGMEM = "+/-";
const char S_Next[] PROGMEM = " > ";

/////////////////////////////////////////////////////////////////////
//                            COLLECTIONS                          //
/////////////////////////////////////////////////////////////////////

template<class T>
  class Node{
  public:
    T* data;
    Node* nextNode;
  };

template<class T>
  class Iterator{
    Node<T>* currentNode;

  public:
    Iterator(Node<T>* currentNode) :
        currentNode(currentNode) {
    }
    boolean hasNext() {
      return (currentNode != 0);
    }
    T* getNext() {
      T* rez = currentNode->data;
      currentNode = currentNode->nextNode;
      return rez;
    }
  };

template<class T>
  class LinkedList{
  private:
    Node<T>* lastNode;

  public:
    LinkedList() :
        lastNode(0) {
    }

    ~LinkedList() {

      while (lastNode != 0) {
        Node<T>* currNode = lastNode;
        lastNode = lastNode->nextNode;
        delete currNode;
      }
    }

    void add(T* data) {
      Node<T>* newNode = new Node<T>;
      newNode->data = data;
      newNode->nextNode = lastNode;
      lastNode = newNode;
    }

    Iterator<T> getIterator() {
      return Iterator<T>(lastNode);
    }
  };

#endif

