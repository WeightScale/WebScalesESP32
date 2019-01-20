#include "Battery.h"
#include "BrowserServer.h"

BatteryClass* BATTERY;

BatteryClass::BatteryClass() : Task(20000) {
	//pinMode(CHANEL, ANALOG);
	analogSetPinAttenuation(CHANEL, ADC_0db);
	//analogReadResolution(10);
	/* 20 Обновляем заряд батареи */
	onRun(std::bind(&BatteryClass::fetchCharge, this));
	//_max = CoreMemory.eeprom.settings.bat_max;
	//_min = CoreMemory.eeprom.settings.bat_min;
	_battery = &CoreMemory.eeprom.battery;
	//if(_battery->bat_max < CONVERT_V_TO_ADC((float)PLAN_BATTERY)) {
	//	CoreMemory.doDefault();
	//}
	_battery->bat_max = constrain(_battery->bat_max, 0, 4095);
	_battery->bat_min = constrain(_battery->bat_min, 0, 4095);
	fetchCharge();
#ifdef DEBUG_BATTERY
	_isDischarged = false;
#endif // DEBUG_BATTERY

}

int BatteryClass::fetchCharge() {
	//_charge = _get_adc(1);
	_charge = constrain(_get_adc(1), _battery->bat_min, _battery->bat_max);
	_charge = map(_charge, _battery->bat_min, _battery->bat_max, 0, 100);
	_isDischarged = _charge <= 5;
	if (_isDischarged) {
		ws.textAll("{\"cmd\":\"dchg\"}");
	}
	return _charge;
}

int BatteryClass::_get_adc(byte times) {
	long sum = 0;
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
	json["id_min"] = CALCULATE_VIN(_battery->bat_min);
	json["id_max"] = CALCULATE_VIN(_battery->bat_max);
	json["id_cvl"] = CALCULATE_VIN(_get_adc(1));
	return json.measureLength();	
}

void BatteryClass::handleBinfo(AsyncWebServerRequest *request) {
	if (!request->authenticate(Scale.getUser(), Scale.getPassword()))
		if (!browserServer.checkAdminAuth(request)) {
			return request->requestAuthentication();
		}
	if (request->args() > 0) {
		bool flag = false;
		if (request->hasArg("bmax")) {
			float t = request->arg("bmax").toFloat();
			_battery->bat_max = CONVERT_V_TO_ADC(t);
			flag = true;
		}
		if (flag){
			if (request->hasArg("bmin")){
				float t = request->arg("bmin").toFloat();
				_battery->bat_min = CONVERT_V_TO_ADC(t);
			}else{
				flag = false;
			}				
		}
		if (flag && CoreMemory.save()) {
			goto url;
		}
		return request->send(400);
	}
	url:
//#ifdef HTML_PROGMEM
//	request->send_P(200, F("text/html"), binfo_html);
//#else
	request->send(SPIFFS, request->url());
//#endif
}