#pragma once
#include <ESPAsyncWebServer.h>
#include <IPAddress.h>
//#include <ESPAsyncDNSServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Core.h"

#define MAX_WEBSOCKET_CLIENT		4

// DNS server
#define DNS_PORT 53

typedef struct {
	String wwwUsername;
	String wwwPassword;
} strHTTPAuth;

class BrowserServerClass : public AsyncWebServer{
	protected:
		strHTTPAuth _httpAuth;
		static std::function<void(int)> _onComplete;

	public:
	
		BrowserServerClass(uint16_t port);
		~BrowserServerClass();
		void begin();
		void init();
		bool checkAdminAuth(AsyncWebServerRequest * request);
		bool isAuthentified(AsyncWebServerRequest * request);
		String getName(){ return _httpAuth.wwwUsername;};
		String getPass(){ return _httpAuth.wwwPassword;};
		void stop(){_server.end();};
		void scanNetworksAsync(std::function<void(int)> onComplete);
		void scanDone(int);
		
};

class CaptiveRequestHandler : public AsyncWebHandler {
	public:
	CaptiveRequestHandler() {}
	virtual ~CaptiveRequestHandler() {}
	
	bool canHandle(AsyncWebServerRequest *request){
		if (!request->host().equalsIgnoreCase(WiFi.softAPIP().toString())){
			return true;
		}
		return false;
	}

	void handleRequest(AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(302, "text/plain","");
		response->addHeader("Location", String("http://") + WiFi.softAPIP().toString());
		request->send ( response);
	}
};

//extern AsyncDNSServer dnsServer;
extern IPAddress apIP;
extern IPAddress netMsk;
extern IPAddress lanIp;			// Надо сделать настройки ip адреса
extern IPAddress gateway;
extern BrowserServerClass browserServer;
extern AsyncWebSocket ws;

#ifdef HTML_PROGMEM
	void handleBatteryPng(AsyncWebServerRequest*);
	void handleScalesPng(AsyncWebServerRequest*);
#endif

void handleSettings(AsyncWebServerRequest * request);
void handleFileReadAuth(AsyncWebServerRequest*);
void handleScaleProp(AsyncWebServerRequest*);
void handleRSSI(AsyncWebServerRequest*);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
