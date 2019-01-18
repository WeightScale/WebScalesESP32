#include "DateTime.h"

RtcDS1307<TwoWire> Rtc(Wire);

DateTimeClass::DateTimeClass(const String& date){
	_dayOfMonth = date.substring(0, 2).toInt();
	_month = date.substring(3, 5).toInt();
	_year = date.substring(6, date.indexOf("-")).toInt();
	_hour = date.substring(11, 13).toInt();
	_minute = date.substring(14, 16).toInt();
	_second = date.substring(17).toInt();
	
}

DateTimeClass::~DateTimeClass(){}


RtcDateTime DateTimeClass::toRtcDateTime(){
	return RtcDateTime(_year, _month, _dayOfMonth, _hour, _minute, _second);
}

String getDateTime(){
	char datestring[20];
	RtcDateTime now = Rtc.GetDateTime();
	snprintf_P(datestring, countof(datestring),PSTR("%04u.%02u.%02u-%02u:%02u:%02u"),
	now.Year(),
	now.Month(),
	now.Day(),
	now.Hour(),
	now.Minute(),
	now.Second() );
	return String(datestring);
}