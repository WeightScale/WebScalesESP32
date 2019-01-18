#ifndef _DATETIME_h
#define _DATETIME_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <RtcDS1307.h>
#include <Wire.h>

#define countof(a) (sizeof(a) / sizeof(a[0]))

class RtcDateTime;

class DateTimeClass {
	protected:
		uint16_t _year;
		uint8_t _month;
		uint8_t _dayOfMonth;
		uint8_t _hour;
		uint8_t _minute;
		uint8_t _second;

	public:
		DateTimeClass(const String&);
		~DateTimeClass();
		RtcDateTime toRtcDateTime();
		void init();
};

String getDateTime();

extern RtcDS1307<TwoWire> Rtc;

#endif

