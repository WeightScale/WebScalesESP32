#include <FS.h>
#include <ArduinoJson.h>
#include "webconfig.h"
#include "Scale.h"

ScaleClass Scale(DOUT_PIN,SCK_PIN);		

ScaleClass::ScaleClass(byte dout, byte pd_sck) : HX711(dout, pd_sck) /*, ExponentialFilter<long>()*/{
	_server = NULL;	
	_authenticated = false;	
	_saveWeight.isSave = false;
	_saveWeight.value = 0.0;
}

ScaleClass::~ScaleClass(){}
	
void ScaleClass::setup(BrowserServerClass *server){
	init();
	_server = server;	
	_server->on("/wt",HTTP_GET, [this](AsyncWebServerRequest * request){						/* Получить вес и заряд. */
		AsyncResponseStream *response = request->beginResponseStream("text/json");
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		Scale.doData(json);
		json.printTo(*response);
		request->send(response);
		//request->send(200, "text/html", String("{\"w\":\""+String(getBuffer())+"\",\"c\":"+String(BATTERY.getCharge())+",\"s\":"+String(getStableWeight())+"}"));
	});
	_server->on(PAGE_FILE, [this](AsyncWebServerRequest * request) {							/* Открыть страницу калибровки.*/
		if(!request->authenticate(_scales_value->user, _scales_value->password))
			if (!_server->checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		_authenticated = true;
		saveValueCalibratedHttp(request);
	});
	_server->on("/av", [this](AsyncWebServerRequest * request){									/* Получить значение АЦП усредненное. */
		request->send(200, TEXT_HTML, String(readAverage()));
	});
	_server->on("/tp", [this](AsyncWebServerRequest * request){									/* Установить тару. */
		#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
			Scale.tare();
		#endif 
		request->send(204, TEXT_HTML, "");
	});
	_server->on("/sl", handleSeal);																/* Опломбировать */
	_server->on("/cdate.json",HTTP_ANY, [this](AsyncWebServerRequest * request){									/* Получить настройки. */
		if(!request->authenticate(_scales_value->user, _scales_value->password))
			if (!browserServer.checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		AsyncResponseStream *response = request->beginResponseStream("application/json");
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		
		json[RATE_JSON] = _scales_value->rate;
		json[STEP_JSON] = _scales_value->step;
		json[AVERAGE_JSON] = _scales_value->average;
		json[WEIGHT_MAX_JSON] = _scales_value->max;
		json[OFFSET_JSON] = _scales_value->offset;
		json[ACCURACY_JSON] = _scales_value->accuracy;
		json[SCALE_JSON] = _scales_value->scale;
		json[FILTER_JSON] = GetFilterWeight();
		json[SEAL_JSON] = _scales_value->seal;
		json[USER_JSON] = _scales_value->user;
		json[PASS_JSON] = _scales_value->password;
		
		json.printTo(*response);
		request->send(response);	
	});	
}

void ScaleClass::init(){
	pinMode(RATE_PIN, OUTPUT);
	
	reset();
	_scales_value = &CoreMemory.eeprom.scales_value;
	if (_scales_value->scale != _scales_value->scale){
		_scales_value->scale = 0.0f;	
	}
	digitalWrite(RATE_PIN, _scales_value->rate);
	//_downloadValue();
	mathRound();
#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
	readAverage();
	tare();			  
#endif // !DEBUG_WEIGHT_RANDOM && DEBUG_WEIGHT_MILLIS
	SetFilterWeight(_scales_value->filter);
}

void ScaleClass::mathRound(){
	_round = pow(10.0, _scales_value->accuracy) / _scales_value->step; // множитель для округления}
	_stable_step = 1/_round;
}

void ScaleClass::saveValueCalibratedHttp(AsyncWebServerRequest * request) {
	if (request->args() > 0) {
		if (request->hasArg("update")){
			_scales_value->accuracy = request->arg("weightAccuracy").toInt();
			_scales_value->step = request->arg("weightStep").toInt();
			setAverage(request->arg("weightAverage").toInt());
			SetFilterWeight(request->arg("weightFilter").toInt());
			_scales_value->filter = GetFilterWeight();
			_scales_value->max = request->arg("weightMax").toInt();
			if (request->hasArg("rate"))
				_scales_value->rate = true;
			else
				_scales_value->rate = false;
			digitalWrite(RATE_PIN, _scales_value->rate);
			mathRound();
			if (CoreMemory.save()){
				goto ok;
			}
			goto err;
		}
		
		if (request->hasArg("zero")){
			tare();
			//_scales_value->offset = readAverage();
		}
		
		if (request->hasArg("weightCal")){
			float rw = request->arg("weightCal").toFloat();	
			SetCurrent(read());
			long cw = Current();
			//long cw = readAverage();
			mathScale(rw,cw);
		}
		
		if (request->hasArg("user")){
			request->arg("user").toCharArray(_scales_value->user,request->arg("user").length()+1);
			request->arg("pass").toCharArray(_scales_value->password,request->arg("pass").length()+1);
			if (CoreMemory.save()){
				goto url;
			}
			goto err;
		}
		
		ok:
			return request->send(200, TEXT_HTML, "");
		err:
			return request->send(204);	
	}
	url:
	#ifdef HTML_PROGMEM
		request->send_P(200,F(TEXT_HTML), calibr_html);
	#else
		request->send(SPIFFS, request->url());
	#endif
}

void ScaleClass::fetchWeight(){
	_weight = getWeight();
	detectStable(_weight);
}

long ScaleClass::readAverage() {
	long long sum = 0;
#ifdef DEBUG_WEIGHT_RANDOM	
	for (byte i = 0; i < _scales_value->average; i++) {
		sum += analogRead(A0);
	}
	//randomSeed(1000);
	sum = random(8000000);
	Filter(_scales_value->average == 0 ? sum / 1 : sum / _scales_value->average);
#else
	for (byte i = 0; i < _scales_value->average; i++) {
		sum += read();
	}
	Filter(_scales_value->average == 0?sum / 1:sum / _scales_value->average);
#endif // DEBUG_WEIGHT_RANDOM
	
	return Current();
}

long ScaleClass::getValue() {
	int i = readAverage();
	return i - _scales_value->offset;
}

float ScaleClass::getUnits() {
#ifdef DEBUG_WEIGHT_MILLIS
	static long long time = 0;
	float v = (micros() - time);
	time  = micros();
	return v;
#else
	float v = getValue();
	return (v * _scales_value->scale);
#endif // DEBUG_WEIGHT_MILLIS
}

float ScaleClass::getWeight(){
	return round(getUnits() * _round) / _round; 
}

void ScaleClass::tare() {
	SetCurrent(read());
	setOffset(Current());
}

void ScaleClass::setAverage(unsigned char a){
	_scales_value->average = constrain(a, 1, 5);
}

void ScaleClass::mathScale(float referenceW, long calibrateW){
	_scales_value->scale = referenceW / float(calibrateW - _scales_value->offset);
}

/*! Функция для форматирования значения веса
	value - Форматируемое значение
	digits - Количество знаков после запятой
	accuracy - Точновть шага значений (1, 2, 5, ...)
	string - Выходная строка отфармотронова значение 
*/
void ScaleClass::formatValue(float value, char* string){
	dtostrf(value, 6-_scales_value->accuracy, _scales_value->accuracy, string);
}

/* */
void ScaleClass::detectStable(float w){
	//static unsigned char stable_num;
	if (_saveWeight.value != w){
		_saveWeight.stabNum = STABLE_NUM_MAX;
		_stableWeight = false;
		_saveWeight.value = w;
		return;
	}
	
	if (_saveWeight.stabNum) {
		_saveWeight.stabNum--;
		return;
	}	
	if (_stableWeight) {
		return;
	}
	_stableWeight = true;
	if (millis() < _saveWeight.time)
		return;
	if (fabs(_saveWeight.value) > _stable_step) {
		_saveWeight.isSave = true;
		_saveWeight.time = millis() + 10000;
	}
}

size_t ScaleClass::doData(JsonObject& json ){
	//json["w"]= String(_buffer);
	json["w"] = _weight;
	json["a"] = _scales_value->accuracy;
	json["c"]= BATTERY->getCharge();
	json["s"]= _saveWeight.stabNum;	
	return json.measureLength();
}

void handleSeal(AsyncWebServerRequest * request){
	randomSeed(Scale.readAverage());
	Scale.setSeal(random(1000));
	
	if (CoreMemory.save()){
		return request->send(200, TEXT_HTML, String(Scale.getSeal()));
	}
	return request->send(400, TEXT_HTML, "Ошибка!");
}





