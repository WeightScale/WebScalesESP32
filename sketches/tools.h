#pragma once
#include <Arduino.h>
#include <IPAddress.h>

/** Is this an IP? */
boolean isIp(String str);
/** IP to String? */
String toStringIp(IPAddress ip);
int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams);