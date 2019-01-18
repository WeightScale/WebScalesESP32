#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Update.h>
#include <ESP32httpUpdate.h>
#include "StreamString.h"
#include "Core.h"
#include "HttpUpdater.h"
#include "Version.h"

HttpUpdaterClass::HttpUpdaterClass(const String& username, const String& password)
:_username(username),_password(password),_authenticated(false)
{}

bool HttpUpdaterClass::canHandle(AsyncWebServerRequest *request){
	if(request->url().equalsIgnoreCase("/update")){
		return true;
	}
	return false;
}

void HttpUpdaterClass::handleRequest(AsyncWebServerRequest *request){
	if(_username.length() && _password.length() && !request->authenticate(_username.c_str(), _password.c_str()))
	return request->requestAuthentication();
	_authenticated = true;
	//Serial.println(String(Update.hasError()));
	if(request->method() == HTTP_GET){
		request->send_P(200, "text/html", serverIndex);		
	}else if (request->method()==HTTP_POST){
		digitalWrite(2, LOW); //led off
		taskWeight.resume();
		BATTERY->resume();
		if (_command == U_SPIFFS){
			CoreMemory.save();				
			request->redirect("/");
			return;
		}
		if(_updaterError && _updaterError[0] != 0x00){
			AsyncWebServerResponse * response = request->beginResponse(200, "text/html", _updaterError);
			request->send(response);
		}else{
			AsyncWebServerResponse * response = request->beginResponse_P(200, "text/html", successResponse);
			response->addHeader("Connection", "close");
			request->send(response);
			request->onDisconnect([](){ESP.restart();});
		}
	}
}

void HttpUpdaterClass::handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
	if(!index){
		taskWeight.pause();
		BATTERY->pause();
		_updaterError = String();
		if(!_authenticated){
			return;
		}
		size_t size;
		if(filename.indexOf("spiffs.bin",0) != -1) {
			_command = U_SPIFFS;
			//size = ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);
		}else if(filename.indexOf("ino.bin",0) != -1) {
			_command = U_FLASH;
			//size = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
		}else{
			request->client()->close(true);
			return;
		}
		
		//Update.runAsync(true);
		//if(!Update.begin(size, _command)){
		if(!Update.begin(UPDATE_SIZE_UNKNOWN,_command)) {
			setUpdaterError();
		}
	}
	if(!Update.hasError()){
		digitalWrite(LED, !digitalRead(LED));	//led on
		if(Update.write(data, len) != len){
			setUpdaterError();
		}
	}
	if(final){
		if(!Update.end(true)){
			setUpdaterError();
		}
	}
}

void HttpUpdaterClass::setUpdaterError(){
	StreamString str;
	Update.printError(str);
	_updaterError = str.c_str();
}

void HttpUpdaterClass::handleHttpStartUpdate(AsyncWebServerRequest * request){										/* Обновление чере интернет address/hu?host=sdb.net.ua/update.php */
	if(!request->authenticate(_username.c_str(), _password.c_str()))
		return request->requestAuthentication();
	if(request->hasArg("host")){
		String host = request->arg("host");
		//_server->send(200, "text/plain", host);
		ESPhttpUpdate.rebootOnUpdate(false);
		digitalWrite(LED, LOW);
		String url = String("http://");
		url += host;
		t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url,SPIFFS_VERSION);
		if (ret == HTTP_UPDATE_OK){
			CoreMemory.save();
			//CORE->saveSettings();
			//Scale.saveDate();									
			ret = ESPhttpUpdate.update(url, SKETCH_VERSION);
		}
		switch(ret) {
			case HTTP_UPDATE_FAILED:
				request->send(404, "text/plain", ESPhttpUpdate.getLastErrorString());
			break;
			case HTTP_UPDATE_NO_UPDATES:
				request->send(304, "text/plain", "Обновление не требуется");
			break;
			case HTTP_UPDATE_OK:
				request->client()->stop();
				ESP.restart();
			break;
		}
		
	}
	digitalWrite(LED, HIGH);
};
	