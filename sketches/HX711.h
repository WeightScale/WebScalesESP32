#ifndef _HX711_h
#define _HX711_h

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#pragma once
/*
* Implements a simple linear recursive exponential filter.
* See: http://www.statistics.com/glossary&term_id=756 */
template<class T> class ExponentialFilter{
	protected:
	// Weight for new values, as a percentage ([0..100])
	T m_WeightNew = 100;

	// Current filtered value.
	T m_Current;

	public:
	ExponentialFilter() : m_WeightNew(100), m_Current(0){}
	ExponentialFilter(T WeightNew, T Initial) : m_WeightNew(WeightNew), m_Current(Initial){}

	void Filter(T New){
		m_Current = (m_WeightNew * New + (100 - m_WeightNew) * m_Current) / 100;
	}

	void SetFilterWeight(T NewWeight){
		m_WeightNew = NewWeight;
	}

	T GetFilterWeight() const { return m_WeightNew; }

	T Current() const { return m_Current; }

	void SetCurrent(T NewValue){
		m_Current = NewValue;
	}
};

class HX711 : public ExponentialFilter<long> {
	private:
	
		byte PD_SCK;	// Power Down and Serial Clock Input Pin
		byte DOUT;		// Serial Data Output Pin
		byte GAIN;		// amplification factor
		bool pinsConfigured;

	public:
		// define clock and data pin, channel, and gain factor
		// channel selection is made by passing the appropriate gain: 128 or 64 for channel A, 32 for channel B
		// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
		HX711(byte dout, byte pd_sck, byte gain = 128) : ExponentialFilter<long>(){
			PD_SCK 	= pd_sck;
			DOUT 	= dout;

			GAIN = 1;
			pinsConfigured = false;
		};
		~HX711(){};
		bool is_ready() {
			if (!pinsConfigured) {
				// We need to set the pin mode once, but not in the constructor
				pinMode(PD_SCK, OUTPUT);
				pinMode(DOUT, INPUT);
				pinsConfigured = true;
			}
			return digitalRead(DOUT) == LOW;
		};
		long read() {
			// wait for the chip to become ready
			while (!is_ready());

			unsigned long value = 0;
			byte data[3] = { 0 };
			byte filler = 0x00;

			// pulse the clock pin 24 times to read the data
			data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
			data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
			data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);

			// set the channel and the gain factor for the next reading using the clock pin
			for (unsigned int i = 0; i < GAIN; i++) {
				digitalWrite(PD_SCK, HIGH);
				digitalWrite(PD_SCK, LOW);
			}
		
			data[2] ^= 0x80;
			return ((uint32_t) data[2] << 16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
		};
		void power_down(){digitalWrite(PD_SCK, HIGH);};
		void power_up(){digitalWrite(PD_SCK, LOW);};
		void reset(){
			power_down();
			delayMicroseconds(100);
			power_up();
		};
};

#endif /* HX711_h */






