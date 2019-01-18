#include <FS.h>
#include <ArduinoJson.h>
#include "webconfig.h"
#include "Scale.h"

ScaleClass Scale(33,32);		/*  gpio16 gpio0  */

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
	_server->on("/wt",HTTP_GET, [this](AsyncWebServerRequest * request){						/* �������� ��� � �����. */
		AsyncResponseStream *response = request->beginResponseStream("text/json");
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		Scale.doData(json);
		json.printTo(*response);
		#if POWER_PLAN
			POWER.updateCache();
		#endif
		request->send(response);
		//request->send(200, "text/html", String("{\"w\":\""+String(getBuffer())+"\",\"c\":"+String(BATTERY.getCharge())+",\"s\":"+String(getStableWeight())+"}"));
	});
	_server->on(PAGE_FILE, [this](AsyncWebServerRequest * request) {							/* ������� �������� ����������.*/
		if(!request->authenticate(_scales_value->user, _scales_value->password))
			if (!_server->checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		_authenticated = true;
		saveValueCalibratedHttp(request);
	});
	_server->on("/av", [this](AsyncWebServerRequest * request){									/* �������� �������� ��� �����������. */
		request->send(200, TEXT_HTML, String(readAverage()));
	});
	_server->on("/tp", [this](AsyncWebServerRequest * request){									/* ���������� ����. */
		tare();
		request->send(204, TEXT_HTML, "");
	});
	_server->on("/sl", handleSeal);																/* ������������� */
	_server->on("/cdate.json",HTTP_ANY, [this](AsyncWebServerRequest * request){									/* �������� �������� ��� �����������. */
		if(!request->authenticate(_scales_value->user, _scales_value->password))
			if (!browserServer.checkAdminAuth(request)){
				return request->requestAuthentication();
			}
		AsyncResponseStream *response = request->beginResponseStream("application/json");
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		
		json[POWER_5V] = _scales_value->power5v;
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
	
	reset();
	_scales_value = &CoreMemory.eeprom.scales_value;
	/*pinMode(EN_MT, OUTPUT);
	 digitalWrite(EN_MT, _scales_value->power5v);
	 */	
	//_downloadValue();
	mathRound();
	readAverage();
	tare();
	SetFilterWeight(_scales_value->filter);
}

void ScaleClass::mathRound(){
	_round = pow(10.0, _scales_value->accuracy) / _scales_value->step; // ��������� ��� ����������}
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
			/*if (request->hasArg("p5v"))
				_scales_value->power5v = true;
			else
				_scales_value->power5v = false;
			digitalWrite(EN_MT, _scales_value->power5v);
			*/			
			mathRound();
			if (CoreMemory.save()){
				goto ok;
			}
			goto err;
		}
		
		if (request->hasArg("zero")){
			_scales_value->offset = readAverage();
		}
		
		if (request->hasArg("weightCal")){
			float rw = request->arg("weightCal").toFloat();			
			long cw = readAverage();
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
			return request->send(400, TEXT_HTML, "������ ");	
	}
	url:
	#ifdef HTML_PROGMEM
		request->send_P(200,F(TEXT_HTML), calibr_html);
	#else
		request->send(SPIFFS, request->url());
	#endif
}

void ScaleClass::fetchWeight(){
	//float w = getWeight();
	//formatValue(w,_buffer);
	_weight = getWeight();
	detectStable(_weight);
}

/*
bool ScaleClass::_downloadValue(){
	_scales_value->average = 1;
	_scales_value->step = 1;
	_scales_value->accuracy = 0;
	_scales_value->offset = 0;
	_scales_value->max = 1000;
	_scales_value->scale = 1;
	SetFilterWeight(80);
	_scales_value->user = "admin";
	_scales_value->password = "1234";
	File dateFile = SPIFFS.open(CDATE_FILE, "r");
	/ *if (SPIFFS.exists(CDATE_FILE)){
		dateFile = SPIFFS.open(CDATE_FILE, "r");
	}else{
		dateFile = SPIFFS.open(CDATE_FILE, "w+");
	}* /
	if (!dateFile) {
		//dateFile.close();
		return false;
	}
	size_t size = dateFile.size();
		
	std::unique_ptr<char[]> buf(new char[size]);
		
	dateFile.readBytes(buf.get(), size);
	dateFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		return false;
	}
	_scales_value->max = json[WEIGHT_MAX_JSON];
	_scales_value->offset = json[OFFSET_JSON];
	setAverage(json[AVERAGE_JSON]);
	_scales_value->step = json[STEP_JSON];
	_scales_value->accuracy = json[ACCURACY_JSON];
	_scales_value->scale = json[SCALE_JSON];
	SetFilterWeight(json[FILTER_JSON]);
	_scales_value->seal = json[SEAL_JSON];
	if (!json.containsKey(USER_JSON)){
		_scales_value->user = "admin";
		_scales_value->password = "1234";	
	}else{
		_scales_value->user = json[USER_JSON].as<String>();
		_scales_value->password = json[PASS_JSON].as<String>();
	}
	
	return true;
	
}*/

/*
bool ScaleClass::saveDate() {	
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	
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
	
	File cdateFile = SPIFFS.open(CDATE_FILE, "w");
	if (!cdateFile) {
		return false;
	}
	
	json.printTo(cdateFile);
	cdateFile.flush();
	cdateFile.close();
	return true;
}*/

long ScaleClass::readAverage() {
	long long sum = 0;
	for (byte i = 0; i < _scales_value->average; i++) {
		sum += read();
	}
	Filter(_scales_value->average == 0?sum / 1:sum / _scales_value->average);
	return Current();
}

long ScaleClass::getValue() {
	//Filter(readAverage());
	//return Current() - _scales_value->offset;
	return readAverage() - _scales_value->offset;
}

float ScaleClass::getUnits() {
	float v = getValue();
	return (v * _scales_value->scale);
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

/*! ������� ��� �������������� �������� ����
	value - ������������� ��������
	digits - ���������� ������ ����� �������
	accuracy - �������� ���� �������� (1, 2, 5, ...)
	string - �������� ������ �������������� �������� 
*/
void ScaleClass::formatValue(float value, char* string){
	dtostrf(value, 6-_scales_value->accuracy, _scales_value->accuracy, string);
}

/* */
void ScaleClass::detectStable(float w){
	//static unsigned char stable_num;
	if (_saveWeight.value != w){
		_saveWeight.stabNum = STABLE_NUM_MAX;
		//stable_num = STABLE_NUM_MAX;
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

	/*if (_saveWeight.value == w) {
		if (stable_num > STABLE_NUM_MAX) {
			if (!_stableWeight){
				if (millis() > _saveWeight.time ){
					if(fabs(w) > _stable_step){
						_saveWeight.isSave = true;
						_saveWeight.time = millis() + 10000;
					}
				}
				_stableWeight = true;
			}				
			return;
		}
		stable_num++;
	} else {
		stable_num = 0;
		_stableWeight = false;
		_saveWeight.value = w;
	}*/
}

/*
double calculateDistance(double rssi) {
	
	double txPower = -41;   //hard coded power value. Usually ranges between -59 to -65
	
	/ *if(rssi == 0) {
		return -1.0;
	}

	double ratio = rssi * 1.0 / txPower;
	if (ratio < 1.0) {
		return pow(ratio, 10);
	}
	else {
		double distance =  (0.89976)*pow(ratio, 7.7095) + 0.111;
		return distance;
	}* /
	return pow(10, ((double)txPower - rssi) / (10 * 2));
}*/

size_t ScaleClass::doData(JsonObject& json ){
	//json["w"]= String(_buffer);
	json["w"] = _weight;
	json["a"] = _scales_value->accuracy;
	json["c"]= BATTERY.getCharge();
	json["s"]= _saveWeight.stabNum;	
	return json.measureLength();
}

void handleSeal(AsyncWebServerRequest * request){
	randomSeed(Scale.readAverage());
	Scale.setSeal(random(1000));
	
	if (CoreMemory.save()){
		request->send(200, TEXT_HTML, String(Scale.getSeal()));
	}
}





