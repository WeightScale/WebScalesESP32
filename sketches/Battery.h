#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Task.h"

//#define DEBUG_BATTERY		/*Для теста*/

#define BATTERY_6V				6
#define BATTERY_4V				4
#define R1_KOM					470.0f
#define R2_KOM					75.0f
#define VREF					1.1f
#define ADC						4095.0f

#define CHANEL					35

/*План питания от батареи*/
#define PLAN_BATTERY			BATTERY_6V

#define CONVERT_V_TO_ADC(v)		(((v * (R2_KOM /(R1_KOM + R2_KOM)))*ADC)/VREF)

/*ADC = (Vin * 1024)/Vref  Vref = 1V*/
#if PLAN_BATTERY == BATTERY_6V		
	#define MAX_CHG					CONVERT_V_TO_ADC(6.4)		/*Vin 6.4V*/
	#define MIN_CHG					CONVERT_V_TO_ADC(5.5)		/*Vin 5.5V*/				
#elif PLAN_BATTERY == BATTERY_4V
	#define MAX_CHG					CONVERT_V_TO_ADC(4.2)		/*Vin 4.2V*/
	#define MIN_CHG					CONVERT_V_TO_ADC(3.3)		/*Vin 3.3V*/
#endif // PLAN_BATTERY == BATTERY_6V

#define CALCULATE_VIN(v)		(((v/ADC)/(R2_KOM /(R1_KOM + R2_KOM)))*VREF)


class BatteryClass : public Task {	
protected:
	bool _isDischarged = false;
	unsigned int _charge;
	unsigned int _max; /*Значение ацп максимального заряд*/
	unsigned int _min; /*Значение ацп минимального заряд*/
	unsigned int _get_adc(byte times = 1);	
public:
	BatteryClass();
	~BatteryClass() {};
	unsigned int fetchCharge();		
	void charge(unsigned int ch){_charge = ch; };
	unsigned int charge(){return _charge;};
	void max(unsigned int m){_max = m; };	
	void min(unsigned int m){_min = m; };	
	unsigned int max(){return _max;};
	unsigned int min(){return _min;};
	size_t doInfo(JsonObject& json);
	size_t doData(JsonObject& json);
	void handleBinfo(AsyncWebServerRequest *request);
	bool isDischarged(){return _isDischarged;};
};

extern BatteryClass* BATTERY;