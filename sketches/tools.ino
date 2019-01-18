
#include "tools.h"

/** Is this an IP? */
boolean isIp(String str) {
	for (int i = 0; i < str.length(); i++) {
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
	String res = "";
	for (int i = 0; i < 3; i++) {
		res += String((ip >> (8 * i)) & 0xFF) + ".";
	}
	res += String(((ip >> 8 * 3)) & 0xFF);
	return res;
}
/** sInput - входящая строка
	cDelim - разделитель
	sParams - масив выходных параметров
	iMaxParams - кол параметров */

int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams)
{
	int iParamCount = 0;
	int iPosDelim, iPosStart = 0;

	do {
		// Searching the delimiter using indexOf()
		iPosDelim = sInput.indexOf(cDelim,iPosStart);
		if (iPosDelim > (iPosStart+1)) {
			// Adding a new parameter using substring()
			sParams[iParamCount] = sInput.substring(iPosStart,iPosDelim-1);
			iParamCount++;
			// Checking the number of parameters
			if (iParamCount >= iMaxParams) {
				return (iParamCount);
			}
			iPosStart = iPosDelim + 1;
		}
	} while (iPosDelim >= 0);
	if (iParamCount < iMaxParams) {
		// Adding the last parameter as the end of the line
		sParams[iParamCount] = sInput.substring(iPosStart);
		iParamCount++;
	}

	return (iParamCount);
}






