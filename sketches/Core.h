
#ifndef _CORE_h
#define _CORE_h
#include "webconfig.h"
#include "TaskController.h"
#include "Task.h"

const char netIndex[] PROGMEM = R"(<html lang='en'><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/><body><form method='POST'><input name='ssid'><br/><input type='password' name='key'><br/><input type='submit' value='СОХРАНИТЬ'></form></body></html>)";

/*
#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif*/
#include <ArduinoJson.h>
#include "Scale.h"
#include "CoreMemory.h"
#include "TaskController.h"
//using namespace ArduinoJson;

#define SETTINGS_FILE "/settings.json"

#define SCALE_JSON		"scale"
#define SERVER_JSON		"server"
//#define DATE_JSON		"date"
#define EVENTS_JSON		"events"

extern TaskController taskController;		/*  */
extern Task taskBlink;								/*  */
extern Task taskConnectWiFi;
extern Task taskWeight;
extern void connectWifi();



class CoreClass : public AsyncWebHandler{
	private:
		settings_t * _settings;
	
		String _hostname;
		String _username;
		String _password;
		bool _authenticated;
	
		//bool saveAuth();
		//bool loadAuth();
		
	public:			
		CoreClass(const String& username, const String& password);
		~CoreClass();
		void begin();
		#if POWER_PLAN
			void powerOff();
		#endif
		char* getNameAdmin(){return _settings->scaleName;};
		char* getPassAdmin(){return _settings->scalePass;};
		char* getSSID(){return _settings->wSSID;};
		char* getLanIp(){return _settings->scaleLanIp;};
		char* getGateway(){return _settings->scaleGateway;};
		char* getPASS(){return _settings->wKey;};
		char* getApSSID() { return _settings->apSSID; };
		bool saveEvent(const String&, const String&);
		bool eventToServer(const String&, const String&, const String&);
		String getHash(const int, const String&, const String&, const String&);
		int getPin(){return _settings->hostPin;};
		String& getHostname() { return _hostname; };
		
		bool isAuto(){return _settings->autoIp;};		
		virtual bool canHandle(AsyncWebServerRequest *request) override final;
		virtual void handleRequest(AsyncWebServerRequest *request) override final;
		virtual bool isRequestHandlerTrivial() override final {return false;}
		
		
};

class BlinkClass : public Task {
	public:
	unsigned int _flash = 500;
	//unsigned int _blink = 500;
	public:
	BlinkClass() : Task(500){
		pinMode(LED, OUTPUT);
		onRun(std::bind(&BlinkClass::blinkAP,this));
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

#endif //_CORE_h







