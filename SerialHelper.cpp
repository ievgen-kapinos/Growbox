#include "SerialHelper.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

/*volatile*/ boolean g_UseSerialMonitor = false;
/*volatile*/ boolean g_UseSerialWifi = false;

boolean GB_SerialHelper::autoStartWifiOnReset = false;

