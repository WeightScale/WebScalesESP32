#include "Core.h"
#include "BrowserServer.h"
#include <ArduinoJson.h>
#include "Scale.h"
#include <HTTPClient.h>

using namespace std;
using namespace std::placeholders;

CoreClass * CORE;
BlinkClass * BLINK;

CoreClass::CoreClass(const String& username, const String& password):
_username(username),
_password(password),
_authenticated(false){
	begin();
}

CoreClass::~CoreClass(){}

void CoreClass::begin(){
#ifndef PREFERENCES_MEMORY
	_settings = &CoreMemory.eeprom.settings; //ссылка на переменную
	_hostname = String(_settings->apSSID);
	_hostname.toLowerCase();
	if (!isAuto()) {
		if (lanIp.fromString(getLanIp()) && gateway.fromString(getGateway())) {
			WiFi.config(lanIp, gateway, netMsk);   									// Надо сделать настройки ip адреса
		}
	}
	WiFi.setHostname(_hostname.c_str());
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(getApSSID(), SOFT_AP_PASSWORD);
#else
	_settings = &CoreMemory;
	_hostname = _settings->getString(KEY_AP_SSID,"SCALES");
	_hostname.toLowerCase();
	if (!isAuto()) {
		if (lanIp.fromString(getLanIp()) && gateway.fromString(getGateway())) {
			WiFi.config(lanIp, gateway, netMsk);    									// Надо сделать настройки ip адреса
		}
	}
	WiFi.setHostname(_hostname.c_str());
	WiFi.softAPConfig(apIP, apIP, netMsk);
	WiFi.softAP(getApSSID().c_str(), SOFT_AP_PASSWORD);
#endif // !PREFERENCES_MEMORY
			
}

bool CoreClass::canHandle(AsyncWebServerRequest *request){	
	if(request->url().equalsIgnoreCase(F("/settings.html"))){
		goto auth;
	}
#ifndef HTML_PROGMEM
	else if(request->url().equalsIgnoreCase("/sn")){
		goto auth;
	}
#endif
	else
		return false;
	auth:
	if (!request->authenticate(getNameAdmin().c_str(), getPassAdmin().c_str())) {
		if(!request->authenticate(_username.c_str(), _password.c_str())){
			request->requestAuthentication();
			return false;
		}
	}
	return true;
}

void CoreClass::handleRequest(AsyncWebServerRequest *request){
	if (request->args() > 0){
		String message = " ";
		
#ifndef PREFERENCES_MEMORY
		if (request->hasArg("ssid")){
			if (request->hasArg("auto"))
				_settings->autoIp = true;
			else
				_settings->autoIp = false;
			
			request->arg("lan_ip").toCharArray(_settings->scaleLanIp,request->arg("lan_ip").length()+1);
			request->arg("gateway").toCharArray(_settings->scaleGateway,request->arg("gateway").length()+1);
			request->arg("subnet").toCharArray(_settings->scaleSubnet,request->arg("subnet").length()+1);
			request->arg("ssid").toCharArray(_settings->wSSID,request->arg("ssid").length()+1);
			if(String(_settings->wSSID).length()>0){
				taskConnectWiFi.resume();
			}else{
				taskConnectWiFi.pause();
			}
			request->arg("key").toCharArray(_settings->wKey, request->arg("key").length()+1);
			request->arg("assid").toCharArray(_settings->apSSID, request->arg("assid").length() + 1);
			goto save;
		}
		if (request->hasArg("host")){
			request->arg("host").toCharArray(_settings->hostUrl, request->arg("host").length()+1);
			_settings->hostPin = request->arg("pin").toInt();
			request->arg("n_admin").toCharArray(_settings->scaleName,request->arg("n_admin").length()+1);
			request->arg("p_admin").toCharArray(_settings->scalePass,request->arg("p_admin").length()+1);
			goto save;
		}
		save:
		if (CoreMemory.save()){
			goto url;
		}	
		return request->send(400);
#else
		if (request->hasArg("ssid")) {
			if (request->hasArg("auto"))
				_settings->putBool(KEY_AUTO_IP, true);
			else
				_settings->putBool(KEY_AUTO_IP, false);
			
			_settings->putString(KEY_LAN_IP, request->arg("lan_ip"));
			_settings->putString(KEY_LAN_GATEWAY, request->arg("gateway"));
			_settings->putString(KEY_LAN_SUBNET, request->arg("subnet"));
			_settings->putString(KEY_W_SSID, request->arg("ssid"));			
			
			if (String(_settings->getString(KEY_W_SSID)).length() > 0) {
				taskConnectWiFi.resume();
			}else {
				taskConnectWiFi.pause();
			}
			
			_settings->putString(KEY_W_KEY, request->arg("key"));
			_settings->putString(KEY_AP_SSID, request->arg("assid"));
		}
		if (request->hasArg("host")) {
			_settings->putString(KEY_HOST, request->arg("host"));
			_settings->putInt(KEY_PIN, request->arg("pin").toInt());			
			_settings->putString(KEY_SCALE_NAME, request->arg("n_admin"));
			_settings->putString(KEY_SCALE_PASS, request->arg("p_admin"));
		}
#endif // !PREFERENCES_MEMORY
	}
	url:
	#ifdef HTML_PROGMEM
		request->send_P(200, F("text/html"), settings_html);
	#else
		if(request->url().equalsIgnoreCase("/sn"))
			request->send_P(200, F("text/html"), netIndex);
	else
		request->send(SPIFFS, request->url());
	#endif
}


bool CoreClass::saveEvent(const String& event, float value) {
	//String date = getDateTime();
	String date = "";
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["cmd"] = "swt";
	root["d"] = date;
	root["v"] = value;
	root["a"] = Scale.accuracy();
	size_t len = root.measureLength();
	AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
	if (buffer) {
		root.printTo((char *)buffer->get(), len + 1);
		ws.textAll(buffer);
	}
	return WiFi.status() == WL_CONNECTED?eventToServer(date, event, String(value)):false;
	/*File readFile;
	readFile = SPIFFS.open("/events.json", "r");
    if (!readFile) {  
		return false;
    }
	
    size_t size = readFile.size(); 	
    std::unique_ptr<char[]> buf(new char[size]);
    readFile.readBytes(buf.get(), size);	
    readFile.close();
		
    DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(110));
	//DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.containsKey(EVENTS_JSON)) {	
		json["cur_num"] = 0;
		json["max_events"] = MAX_EVENTS;
		JsonArray& events = json.createNestedArray(EVENTS_JSON);
		for(int i = 0; i < MAX_EVENTS; i++){
			JsonObject& ev = jsonBuffer.createObject();
			ev["d"] = "";
			ev["v"] = "";
			ev["s"] = false;
			events.add(ev);	
		}		
		/ *if (!json.success())
			return false;* /
    }
	
	long n = json["cur_num"];
	
	json[EVENTS_JSON][n]["d"] = date;
	json[EVENTS_JSON][n]["v"] = value;	
	json[EVENTS_JSON][n]["s"] = flag;
		
	if ( ++n == MAX_EVENTS){
		n = 0;
	}
	json["max_events"] = MAX_EVENTS;
	json["cur_num"] = n;
	//TODO add AP data to html
	File saveFile = SPIFFS.open("/events.json", "w");
	if (!saveFile) {
		//SPIFFS.remove("/events.json");
		//saveFile.close();
		return false;
	}

	json.printTo(saveFile);
	//saveFile.flush();
	saveFile.close();
	return true;*/
}



/*
String CoreClass::getIp(){	
	HTTPClient http;	
	http.begin("http://sdb.net.ua/ip.php");
	http.setTimeout(_settings->timeout);	
	int httpCode = http.GET();
	String ip = http.getString();
	http.end();	
	if(httpCode == HTTP_CODE_OK){		
		return ip;
	}	
	return String(httpCode);
}*/

/* */	
bool CoreClass::eventToServer(const String& date, const String& type, const String& value){
#ifndef PREFERENCES_MEMORY
	if(_settings->hostPin == 0)
#else
	if (_settings->getInt(KEY_PIN) == 0)
#endif // !PREFERENCES_MEMORY
	return false;
	
	HTTPClient http;
	String message = "http://";
#ifndef PREFERENCES_MEMORY
	message += _settings->hostUrl;
	String hash = getHash(_settings->hostPin, date, type, value);
#else
	message += _settings->getString(KEY_HOST);
	String hash = getHash(_settings->getInt(KEY_PIN), date, type, value);
#endif // !PREFERENCES_MEMORY		
	message += "/scales.php?hash=" + hash;
	http.begin(message);
	http.setTimeout(TIMEOUT_HTTP);
	int httpCode = http.GET();
	http.end();
	if(httpCode == HTTP_CODE_OK) {
		return true;
	}
	return false;
}

String CoreClass::getHash(const int code, const String& date, const String& type, const String& value){
	
	String event = String(code);
	event += "\t" + date + "\t" + type + "\t" + value;
	int s = 0;
	for (int i = 0; i < event.length(); i++)
		s += event[i];
	event += (char) (s % 256);
	String hash = "";
	for (int i = 0; i < event.length(); i++){
		int c = (event[i] - (i == 0? 0 : event[i - 1]) + 256) % 256;
		int c1 = c / 16; int c2 = c % 16;
		char d1 = c1 < 10? '0' + c1 : 'a' + c1 - 10;
		char d2 = c2 < 10? '0' + c2 : 'a' + c2 - 10;
		hash += "%"; hash += d1; hash += d2;
	} 
	return hash;
}

void reconnectWifi(AsyncWebServerRequest * request){
	AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), "<meta http-equiv='refresh' content='10;URL=/'>RECONNECT...");
	response->addHeader(F("Connection"), F("close"));
	request->onDisconnect([](){
		SPIFFS.end();
		ESP.restart(); });
	request->send(response);
}