#include <ESPAsyncWebServer.h>
#include "Battery.h"
#include "CoreMemory.h"
#include "BrowserServer.h"
#include "Scale.h"

BatteryClass* BATTERY;

BatteryClass::BatteryClass() : Task(20000) {
	analogSetPinAttenuation(CHANEL, ADC_0db);
	/* 20 Обновляем заряд батареи */
	onRun(std::bind(&BatteryClass::fetchCharge, this));
#ifndef PREFERENCES_MEMORY
	_max = CoreMemory.eeprom.settings.bat_max;
	_min = CoreMemory.eeprom.settings.bat_min;
#else
	_max = CoreMemory.getInt(KEY_BAT_MAX, MAX_CHG);
	_min = CoreMemory.getInt(KEY_BAT_MIN, MIN_CHG);
#endif // !PREFERENCES_MEMORY
	fetchCharge();
#ifdef DEBUG_BATTERY
	_isDischarged = false;
#endif // DEBUG_BATTERY

}

unsigned int BatteryClass::fetchCharge() {
	_charge = _get_adc(1);
	_charge = constrain(_charge, _min, _max);
	_charge = map(_charge, _min, _max, 0, 100);
	_isDischarged = _charge <= 5;
	if (_isDischarged) {
		ws.textAll("{\"cmd\":\"dchg\"}");
	}
	return _charge;
}

unsigned int BatteryClass::_get_adc(byte times) {
	unsigned int sum = 0;
#ifdef DEBUG_BATTERY
	for (byte i = 0; i < times; i++) {
		sum += random(ADC);
	}	
#else
	for (byte i = 0; i < times; i++) {
		sum += analogRead(CHANEL);
	}
#endif // DEBUG_BATTERY	
	return times == 0 ? sum : sum / times;	
}

size_t BatteryClass::doInfo(JsonObject& json){
	json["id_min"] = CALCULATE_VIN(_min);
	json["id_max"] = CALCULATE_VIN(_max);
	json["id_cvl"] = CALCULATE_VIN(_get_adc(1));
	return json.measureLength();	
}

size_t BatteryClass::doData(JsonObject& json) {
	json["c"] = _charge;
	return json.measureLength();
};

void BatteryClass::handleBinfo(AsyncWebServerRequest *request){
	if (!request->authenticate(Scale.user(), Scale.password()))
		if (!browserServer.checkAdminAuth(request)) {
			return request->requestAuthentication();
		}
	if (request->args() > 0) {
		bool flag = false;
		if (request->hasArg("bmax")) {
			float t = request->arg("bmax").toFloat();
#ifndef PREFERENCES_MEMORY
		CoreMemory.eeprom.settings.bat_max = _max = CONVERT_V_TO_ADC(t);
#else
		_max = CONVERT_V_TO_ADC(t);
		CoreMemory.putInt(KEY_BAT_MAX, _max);
#endif // !PREFERENCES_MEMORY
			flag = true;
		}
		if (flag){
			if (request->hasArg("bmin")){
				float t = request->arg("bmin").toFloat();
#ifndef PREFERENCES_MEMORY
		CoreMemory.eeprom.settings.bat_min = _min = CONVERT_V_TO_ADC(t);
#else
		_min = CONVERT_V_TO_ADC(t);
		CoreMemory.putInt(KEY_BAT_MIN, _min);
#endif // !PREFERENCES_MEMORY				
			}else{
				flag = false;
			}				
		}
#ifdef PREFERENCES_MEMORY
		if (flag && CoreMemory.save()) {
			goto url;
		}
		return request->send(400);
#endif // PREFERENCES_MEMORY
	}
	url:
	request->send(SPIFFS, request->url());
}