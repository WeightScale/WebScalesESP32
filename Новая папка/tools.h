// tools1.h

#ifndef _TOOLS_h
#define _TOOLS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <IPAddress.h>

/** Is this an IP? */
boolean isIp(String str);
/** IP to String? */
String toStringIp(IPAddress ip);
int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams);

#endif //_TOOLS_h







