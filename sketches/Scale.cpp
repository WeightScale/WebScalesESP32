#include "Scale.h"
#include "Battery.h"
#include "webconfig.h"

ScaleClass Scale(DOUT_PIN,SCK_PIN);	
Task* taskWeight = new Task(takeWeight, 200);

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
		BATTERY->doData(json);
		json.printTo(*response);
		request->send(response);
	});
	_server->on("/wtr",
		HTTP_GET,
		[this](AsyncWebServerRequest * request) {
			/* Получить вес и заряд. */
			AsyncResponseStream *response = request->beginResponseStream("text/json");
			DynamicJsonBuffer jsonBuffer;
			JsonObject &json = jsonBuffer.createObject();
			Scale.doDataRandom(json);
			json.printTo(*response);
			request->send(response);
		});
	_server->on(PAGE_FILE, [this](AsyncWebServerRequest * request) {							/* Открыть страницу калибровки.*/
		if(!request->authenticate(_value.user, _value.password))
			if (!_server->checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		_authenticated = true;
		saveValueCalibratedHttp(request);
	});
	_server->on("/av", [this](AsyncWebServerRequest * request){									/* Получить значение АЦП усредненное. */
		request->send(200, F("text/html"), String(readAverage()));
	});
	_server->on("/tp", [this](AsyncWebServerRequest * request){									/* Установить тару. */
		#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
#ifndef PREFERENCES_MEMORY
			zero(CoreMemory.eeprom.scales_value.zero_man_range);
#else
			zero(CoreMemory.getFloat(KEY_ZERO_MAN_RANGE));
#endif // !PREFERENCES_MEMORY
		#endif 					/* Установить тару. */
		request->send(204, F("text/html"), "");
	});
	_server->on("/sl", handleSeal);																/* Опломбировать */
	_server->on("/cdate.json",HTTP_ANY, [this](AsyncWebServerRequest * request){									/* Получить настройки. */
		if(!request->authenticate(_value.user, _value.password))
			if (!browserServer.checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		
		json[RATE_JSON] = _value.rate;
		json[STEP_JSON] = _value.step;
		json[AVERAGE_JSON] = _value.average;
		json[WEIGHT_MAX_JSON] = _value.max;
		json[ZERO_MAN_RANGE_JSON] = _value.zero_man_range;
		json[ZERO_ON_RANGE_JSON] = _value.zero_on_range;
		json["auto_en_id"] = _value.enable_zero_auto;
		json["zd_id"] = _value.zero_auto;
		json[OFFSET_JSON] = _value.offset;
		json[ACCURACY_JSON] = _value.accuracy;
		json[SCALE_JSON] = _value.scale;
		json[FILTER_JSON] = GetFilterWeight();
		json[SEAL_JSON] = _value.seal;
		json[USER_JSON] = _value.user;
		json[PASS_JSON] = _value.password;
		
		json.printTo(*response);
		request->send(response);
	});
}

void ScaleClass::init(){
	pinMode(RATE_PIN, OUTPUT);	
	reset();	
#ifndef PREFERENCES_MEMORY
	_scales_value = &CoreMemory.eeprom.scales_value;
	if (_scales_value->scale != _scales_value->scale){
		_scales_value->scale = 0.0f;	
	}
	_offset_local = _scales_value->offset;
	digitalWrite(RATE_PIN, _scales_value->rate);
	mathRound();
	SetFilterWeight(_scales_value->filter);
#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
	readAverage();
	
	//_weight_zero_range = _scales_value->max * _scales_value->zero_man_range; /* Диапазон нуля */
	//_weight_zero_auto = round(_scales_value->zero_auto * _round) / _round; /* Вес автообнуления */
	
	zero(_scales_value->zero_on_range);			  
#endif // !DEBUG_WEIGHT_RANDOM && DEBUG_WEIGHT_MILLIS
#else
	_prefs = &CoreMemory;
	
	_offset_local = _prefs->getLong(KEY_OFFSET);
	digitalWrite(RATE_PIN, _prefs->getBool(KEY_RATE));
	mathRound();
	_value.filter = _prefs->getUChar(KEY_FILTER, 100);
	SetFilterWeight(_value.filter);
#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
	readAverage();
	
	//_weight_zero_range = _scales_value->max * _scales_value->zero_man_range; /* Диапазон нуля */
	//_weight_zero_auto = round(_scales_value->zero_auto * _round) / _round; /* Вес автообнуления */	
	Scale.zero(CoreMemory.getFloat(KEY_ZERO_MAN_RANGE));			  
#endif // !DEBUG_WEIGHT_RANDOM && DEBUG_WEIGHT_MILLIS
#endif // !PREFERENCES_MEMORY
}

void ScaleClass::mathRound(){
#ifndef PREFERENCES_MEMORY
	_round = pow(10.0, _scales_value->accuracy) / _scales_value->step; // множитель для округления}
	_stable_step = 1/_round;
	_weight_zero_auto = (float)_scales_value->zero_auto / _round;
#else
	
	_round = pow(10.0, _prefs->getInt(KEY_ACCURACY)) / _prefs->getUChar(KEY_STEP);   // множитель для округления}
	_stable_step = 1 / _round;
	_weight_zero_auto = (float)_value.zero_auto / _round;
#endif // !PREFERENCES_MEMORY

	
}

void ScaleClass::saveValueCalibratedHttp(AsyncWebServerRequest * request) {
	if (request->args() > 0) {
		if (request->hasArg("update")){
			_value.accuracy = request->arg(F("weightAccuracy")).toInt();
			_value.step = request->arg(F("weightStep")).toInt();
			average(request->arg(F("weightAverage")).toInt());
			SetFilterWeight(request->arg(F("weightFilter")).toInt());
			_value.filter = GetFilterWeight();
			_value.max = request->arg(F("weightMax")).toFloat();
			_value.zero_man_range = request->arg(F("zm_range")).toFloat();
			_value.zero_on_range = request->arg(F("zo_range")).toFloat();
			_value.zero_auto = request->arg(F("zd_auto")).toInt();
			//_weight_zero_range = _scales_value->max * _scales_value->zero_range; /* Диапазон нуля */
			if (request->hasArg("z_en_auto"))
				_value.enable_zero_auto = true;
			else
				_value.enable_zero_auto = false;
			_weight_zero_auto = round(_value.zero_auto * _round) / _round; /* Вес автообнуления */
			if (request->hasArg("rate"))
				_value.rate = true;
			else
				_value.rate = false;
			digitalWrite(RATE_PIN, _value.rate);
			mathRound();
			if (CoreMemory.save()){
				goto ok;
			}
			goto err;
		}
		
		if (request->hasArg("zero")){
			SetCurrent(read());
			_value.offset = _offset_local = Current();
			//offset(Current());
		}
		
		if (request->hasArg(F("weightCal"))){
			float rw = request->arg(F("weightCal")).toFloat();
			SetCurrent(read());
			long cw = Current();
			//long cw = readAverage();
			mathScale(rw,cw);
		}
		
		if (request->hasArg("user")){
			request->arg("user").toCharArray(_value.user,request->arg("user").length()+1);
			request->arg("pass").toCharArray(_value.password,request->arg("pass").length()+1);
			if (CoreMemory.save()){
				goto url;
			}
			goto err;
		}
		
		ok:
			return request->send(200, F("text/html"), "");
		err:
			return request->send(204);	
	}
	url:
	#ifdef HTML_PROGMEM
		request->send_P(200, F("text/html"), calibr_html);
	#else
		request->send(SPIFFS, request->url());
	#endif
}

void ScaleClass::fetchWeight(){
	_weight = getWeight();
	detectAutoZero();
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
	for (byte i = 0; i < _value.average; i++) {
		sum += read();
	}
	Filter(_value.average == 0?sum / 1:sum / _value.average);
#endif // DEBUG_WEIGHT_RANDOM
	
	return Current();
}

long ScaleClass::getValue() {
	int i = readAverage();
	return i - _offset_local;
}

float ScaleClass::getUnits() {
#ifdef DEBUG_WEIGHT_MILLIS
	static long long time = 0;
	float v = (micros() - time);
	time  = micros();
	return v;
#else
	return getValue() * _value.scale;
#endif // DEBUG_WEIGHT_MILLIS
}

float ScaleClass::getWeight(){
	return round(getUnits() * _round) / _round; 
}

void ScaleClass::zero(float range) {
	_weight = getWeight();
	if (fabs(_weight) > (_value.max * range)) //если текущий вес больше диапазона нуля тогда не сбрасываем
		return;
	SetCurrent(read());
	offset(Current());
}

void ScaleClass::average(unsigned char a){
	_value.average = constrain(a, 1, 5);
}

void ScaleClass::mathScale(float referenceW, long calibrateW){
	_value.scale = referenceW / float(calibrateW - _value.offset);
}

/*! Функция для форматирования значения веса
	value - Форматируемое значение
	digits - Количество знаков после запятой
	accuracy - Точновть шага значений (1, 2, 5, ...)
	string - Выходная строка отфармотронова значение 
*/
void ScaleClass::formatValue(float value, char* string){
	dtostrf(value, 6-_value.accuracy, _value.accuracy, string);
}

/* */
void ScaleClass::detectStable(float w){	
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
	if (fabs(_saveWeight.value) > _weight_zero_auto) {
		_saveWeight.isSave = true;
		_saveWeight.time = millis() + 10000;
	}
}

void ScaleClass::detectAutoZero(){
	static long time;
	static bool flag;
	if (!_value.enable_zero_auto)
		return;
	if (fabs(_weight) > _weight_zero_auto)
		return;
	if (!flag){
		flag = true;
		time = millis() + 60000;
	}
	if (time > millis())
		return;
	zero(_value.zero_man_range);
	flag = false;	
}

size_t ScaleClass::doData(JsonObject& json ){
	//json["w"]= String(_buffer);
	if(_weight > _value.max){
		json["cmd"] = "ovl";	//перегруз
	}else{
		json["w"] = _weight;
		json["a"] = _value.accuracy;
		json["s"] = _saveWeight.stabNum;	
	}		
	return json.measureLength();
}

size_t ScaleClass::doDataRandom(JsonObject& json) {
	long sum = random(500,5010);	
	json["w"] = sum;
	json["t"] = millis();
	json["m"] = ESP.getFreeHeap();
	json["c"] = ws.count();	
	return json.measureLength();
}

void handleSeal(AsyncWebServerRequest * request){
	randomSeed(Scale.readAverage());
	Scale.seal(random(1000));
	
	if (CoreMemory.save()){
		return request->send(200, F("text/html"), String(Scale.seal()));
	}
	return request->send(400, F("text/html"), F("Ошибка!"));
}





