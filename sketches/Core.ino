#include <HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <functional>
#include "webconfig.h"
#include "Core.h"
#include "BrowserServer.h"
#include "Battery.h"
#include "CoreMemory.h"

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
	//Rtc.Begin();
	CoreMemory.init();
	_settings = &CoreMemory.eeprom.settings; //ссылка на переменную 
	_cloud = &CoreMemory.eeprom.cloud;  //ссылка на переменную
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
}

bool CoreClass::canHandle(AsyncWebServerRequest *request){	
	if(request->url().equalsIgnoreCase("/settings.html")){
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
	if (!request->authenticate(_settings->scaleName, _settings->scalePass)){
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
		/*if(request->hasArg("data")){
			DateTimeClass DateTime(request->arg("data"));
			Rtc.SetDateTime(DateTime.toRtcDateTime());
			request->send(200, TEXT_HTML, getDateTime());
			return;
		}*/
		if (request->hasArg("host")){
			request->arg("host").toCharArray(_cloud->hostUrl, request->arg("host").length()+1);
			_cloud->hostPin = request->arg("pin").toInt();
			request->arg("n_admin").toCharArray(_settings->scaleName,request->arg("n_admin").length()+1);
			request->arg("p_admin").toCharArray(_settings->scalePass,request->arg("p_admin").length()+1);
			goto save;
		}			
		save:
		if (CoreMemory.save()){
			goto url;
		}
		return request->send(400);
	}
	url:
	#ifdef HTML_PROGMEM
		request->send_P(200,F("text/html"), settings_html);
	#else
		if(request->url().equalsIgnoreCase("/sn"))
			request->send_P(200, F("text/html"), netIndex);
	else
		request->send(SPIFFS, request->url());
	#endif
}


bool CoreClass::saveEvent(const String& event, const String& value) {
	//String date = getDateTime();
	String date = "";
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["cmd"] = "swt";
	root["d"] = date;
	root["v"] = value;
	root["a"] = Scale.getAccuracy();
	size_t len = root.measureLength();
	AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
	if (buffer) {
		root.printTo((char *)buffer->get(), len + 1);
		ws.textAll(buffer);
	}
	return WiFi.status() == WL_CONNECTED?eventToServer(date, event, value):false;
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
	if(_cloud->hostPin == 0)
		return false;
	HTTPClient http;
	String message = "http://";
	message += _cloud->hostUrl;
	String hash = getHash(_cloud->hostPin, date, type, value);	
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
	AsyncWebServerResponse *response = request->beginResponse_P(200, PSTR(TEXT_HTML), "RECONNECT...");
	response->addHeader("Connection", "close");
	request->onDisconnect([](){
		SPIFFS.end();
		ESP.restart();});
	request->send(response);
}








