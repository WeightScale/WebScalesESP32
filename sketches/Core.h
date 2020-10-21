#pragma once
#include <Arduino.h>
#include "TaskController.h"
#include <ESPAsyncWebServer.h>
#include "CoreMemory.h"
#include "webconfig.h"

const char netIndex[] PROGMEM = R"(<html lang='en'><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/><body><form method='POST'><input name='ssid'><br/><input type='password' name='key'><br/><input type='submit' value='СОХРАНИТЬ'></form></body></html>)";

#define SETTINGS_FILE "/settings.json"

#define SCALE_JSON		"scale"
#define SERVER_JSON		"server"
//#define DATE_JSON		"date"
#define EVENTS_JSON		"events"

extern TaskController taskController;/*  */
extern Task taskBlink;/*  */
extern Task taskConnectWiFi;
extern void connectWifi(bool recconect = true);

class CoreClass : public AsyncWebHandler {
private:
#ifndef PREFERENCES_MEMORY
	settings_t * _settings;
#else
	Preferences * _settings;
#endif // !PREFERENCES_MEMORY
	String _hostname;
	String _username;
	String _password;
	bool _authenticated;


public:
	CoreClass(const String& username, const String& password);
	~CoreClass();
	void begin();
#ifndef PREFERENCES_MEMORY
	char* getNameAdmin(){return _settings->scaleName;};
	char* getPassAdmin(){return _settings->scalePass;};
	char* getSSID(){return _settings->wSSID;};
	char* getLanIp(){return _settings->scaleLanIp;};
	char* getGateway(){return _settings->scaleGateway;};
	char* getPASS(){return _settings->wKey;};
	char* getApSSID() { return _settings->apSSID; };
	int getPin(){return _settings->hostPin;};
	bool isAuto(){return _settings->autoIp;};
#else
	String getNameAdmin(){return _settings->getString(KEY_SCALE_NAME);};
	String getPassAdmin(){return _settings->getString(KEY_SCALE_PASS);};
	String getSSID(){return _settings->getString(KEY_W_SSID);};
	String getPASS(){return _settings->getString(KEY_W_KEY);};
	bool isAuto(){return _settings->getBool(KEY_AUTO_IP);};
	String getLanIp(){return _settings->getString(KEY_LAN_IP);};
	String getGateway(){return _settings->getString(KEY_LAN_GATEWAY);};
	String getApSSID() { return _settings->getString(KEY_AP_SSID); };
#endif // !PREFERENCES_MEMORY
	bool saveEvent(const String&, float);	
	bool eventToServer(const String&, const String&, const String&);
	String getHash(const int, const String&, const String&, const String&);	
	String& getHostname() { return _hostname; };	
	virtual bool canHandle(AsyncWebServerRequest *request) override final;
	virtual void handleRequest(AsyncWebServerRequest *request) override final;
	virtual bool isRequestHandlerTrivial() override final {return false;};
};

class BlinkClass : public Task {
public:
	unsigned int _flash = 500;
public:
	BlinkClass() : Task(500){
		pinMode(LED, OUTPUT);
		onRun(std::bind(&BlinkClass::blinkAP, this));
	};
	void blinkSTA(){
		static unsigned char clk;
		bool led = !digitalRead(LED);
		digitalWrite(LED, led);
		if (clk < 6){
			led ? _flash = 70 : _flash = 40;
			clk++;
		}else{
			_flash = 2000;
			digitalWrite(LED, LOW);
			clk = 0;
		}
		setInterval(_flash);
	}
	void blinkAP(){
		bool led = !digitalRead(LED);
		digitalWrite(LED, led);
		setInterval(500);
	}
};


void reconnectWifi(AsyncWebServerRequest*);
extern CoreClass * CORE;
extern BlinkClass * BLINK;