#include "Global.h"

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire g_oneWirePin(ONE_WIRE_PIN);

boolean g_isGrowboxStarted = false;
byte g_isDayInGrowbox = -1;
