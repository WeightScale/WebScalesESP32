#include "BrowserServer.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <IPAddress.h>
//#include <Hash.h>
#include <AsyncJson.h>
#include <functional>
#include "webconfig.h"
#include "tools.h"
#include "Core.h"
#include "Version.h"
//#include "DateTime.h"
#include "HttpUpdater.h"
#include "CoreMemory.h"
#include "Battery.h"

/* */
//ESP8266HTTPUpdateServer httpUpdater;
/* Soft AP network parameters */
IPAddress apIP(192,168,4,1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress lanIp;			// Надо сделать настройки ip адреса
IPAddress gateway;

BrowserServerClass browserServer(80);
AsyncWebSocket ws("/ws");
//AsyncEventSource events("/events");  // event source (Server-Sent events)
DNSServer dnsServer;
//holds the current upload
//File fsUploadFile;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */


BrowserServerClass::BrowserServerClass(uint16_t port) : AsyncWebServer(port) {}

BrowserServerClass::~BrowserServerClass(){}	


void BrowserServerClass::begin(){
	/* Setup the DNS server redirecting all the domains to the apIP */	
	_httpAuth.wwwUsername = "sa";
	_httpAuth.wwwPassword = "343434";
	ws.onEvent(onWsEvent);
	addHandler(&ws);
	//addHandler(&events);
	CORE = new CoreClass(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());	
	addHandler(CORE);
	addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
	addHandler(new SPIFFSEditor(SPIFFS,_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str()));	
	addHandler(new HttpUpdaterClass("sa", "654321"));
	init();
}

void BrowserServerClass::init(){
	on("/settings.json",HTTP_ANY, handleSettings);
	on("/rc", reconnectWifi);									/* Пересоединиться по WiFi. */
	on("/sv", handleScaleProp);									/* Получить значения. */	
	on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
		String str = String("Heap: ");
		str += String(ESP.getFreeHeap());
		str += " client: ";
		str += String(ws.count());
		request->send(200, "text/plain", str);
	});
	on("/rst",HTTP_ANY,[this](AsyncWebServerRequest * request){
		if (!isAuthentified(request)){
			return request->requestAuthentication();
		}
		if(CoreMemory.doDefault())
			request->send(200,"text/html", "Установлено!");
		else
			request->send(400);
	});
	on("/rssi", handleRSSI);
	on("/binfo.html", std::bind(&BatteryClass::handleBinfo, BATTERY, std::placeholders::_1));
#ifdef HTML_PROGMEM
	on("/",[](AsyncWebServerRequest * reguest){	reguest->send_P(200,F("text/html"),index_html);});								/* Главная страница. */	 
	on("/global.css",[](AsyncWebServerRequest * reguest){	reguest->send_P(200,F("text/css"),global_css);});					/* Стили */
	/*on("/favicon.png",[](AsyncWebServerRequest * request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", favicon_png, favicon_png_len);
		request->send(response);
	});*/	
	on("/battery.png",handleBatteryPng);
	on("/scales.png",handleScalesPng);	
	on("/und.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200,F("image/png"), und_png, und_png_len) ;
		request->send(response) ;
	});
	on("/set.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), set_png, set_png_len);
		request->send(response);
	});
	on("/zerow.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", zerow_png, zerow_png_len);
		request->send(response);
	});
	serveStatic("/", SPIFFS, "/");
#else
	rewrite("/sn", "/settings.html");
	serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");	
#endif
	if (BATTERY->isDischarged()){
		on("/ds",
			[](AsyncWebServerRequest *request) {
				AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/balert.html");
				response->addHeader("Connection", "close");
				request->onDisconnect([]() {
					ESP.deepSleep(20000);
				});
				request->send(response);	
			});
		rewrite("/", "/index.html");
		rewrite("/index.html", "/ds");
	}
	//serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");					
	onNotFound([](AsyncWebServerRequest *request){
		request->send(404);
	});
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
	AsyncWebServer::begin();   // Web server start
}

bool BrowserServerClass::checkAdminAuth(AsyncWebServerRequest * r) {	
	return r->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
}

/*
void BrowserServerClass::restart_esp() {
	String message = "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/>";
	message += "<meta http-equiv='refresh' content='10; URL=/admin.html'>Please Wait....Configuring and Restarting.";
	send(200, "text/html", message);
	SPIFFS.end(); // SPIFFS.end();
	delay(1000);
	ESP.restart();
}*/

bool BrowserServerClass::isAuthentified(AsyncWebServerRequest * request){
	if (!request->authenticate(CORE->getNameAdmin(), CORE->getPassAdmin())){
		if (!checkAdminAuth(request)){
			return false;
		}
	}
	return true;
}

void handleSettings(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request))
		return request->requestAuthentication();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	JsonObject& scale = root.createNestedObject(SCALE_JSON);
	scale["id_auto"] = CoreMemory.eeprom.settings.autoIp;
	scale["bat_max"] = CoreMemory.eeprom.settings.bat_max;
	scale["id_pe"] = CoreMemory.eeprom.settings.power_time_enable;
	scale["id_pt"] = CoreMemory.eeprom.settings.time_off;
	scale["id_assid"] = CoreMemory.eeprom.settings.apSSID;
	scale["id_n_admin"] = CoreMemory.eeprom.settings.scaleName;
	scale["id_p_admin"] = CoreMemory.eeprom.settings.scalePass;
	scale["id_lan_ip"] = CoreMemory.eeprom.settings.scaleLanIp;
	scale["id_gateway"] = CoreMemory.eeprom.settings.scaleGateway;
	scale["id_subnet"] = CoreMemory.eeprom.settings.scaleSubnet;
	scale["id_ssid"] = String(CoreMemory.eeprom.settings.wSSID);
	scale["id_key"] = String(CoreMemory.eeprom.settings.wKey);
	
	JsonObject& server = root.createNestedObject(SERVER_JSON);
	server["id_host"] = String(CoreMemory.eeprom.settings.hostUrl);
	server["id_pin"] = CoreMemory.eeprom.settings.hostPin;
	
	root.printTo(*response);
	request->send(response);
}

void handleFileReadAuth(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request)){
		return request->requestAuthentication();
	}
	request->send(SPIFFS, request->url());
}

void handleScaleProp(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request))
	return request->requestAuthentication();
	AsyncJsonResponse * response = new AsyncJsonResponse();
	JsonObject& root = response->getRoot();
	//root["id_date"] = getDateTime();
	root["id_local_host"] = WiFi.getHostname();
	root["id_ap_ssid"] = String(CORE->getApSSID());
	root["id_ap_ip"] = toStringIp(WiFi.softAPIP());
	root["id_ip"] = toStringIp(WiFi.localIP());
	root["sl_id"] = String(Scale.getSeal());					
	root["chip_v"] = String(ESP.getCpuFreqMHz());
	root["id_mac"] = WiFi.macAddress();
	root["id_vr"] = SKETCH_VERSION;
	response->setLength();
	request->send(response);
}

#ifdef HTML_PROGMEM
	void handleBatteryPng(AsyncWebServerRequest * request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", battery_png, battery_png_len);
		request->send(response);
	}

	void handleScalesPng(AsyncWebServerRequest * request){	
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", scales_png, scales_png_len);
		request->send(response);
	}
#endif // HTML_PROGMEM

void handleRSSI(AsyncWebServerRequest * request){
	request->send(200, TEXT_HTML, String(WiFi.RSSI()));
}

void ICACHE_FLASH_ATTR printScanResult(int networksFound) {
	// sort by RSSI
	int n = networksFound;
	int indices[n];
	int skip[n];
	for (int i = 0; i < networksFound; i++) {
		indices[i] = i;
	}
	for (int i = 0; i < networksFound; i++) {
		for (int j = i + 1; j < networksFound; j++) {
			if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
				std::swap(indices[i], indices[j]);
				std::swap(skip[i], skip[j]);
			}
		}
	}
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["cmd"] = "ssl";
	JsonArray &scan = root.createNestedArray("list");
	for (int i = 0; i < 5 && i < networksFound; ++i) {
		JsonObject &item = scan.createNestedObject();
		item["ssid"] = WiFi.SSID(indices[i]);
		item["rssi"] = WiFi.RSSI(indices[i]);
	}
	size_t len = root.measureLength();
	AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);  //  creates a buffer (len + 1) for you.
	if(buffer) {
		root.printTo((char *)buffer->get(), len + 1);
		ws.textAll(buffer);
	}
	WiFi.scanDelete();
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){	
	if(type == WS_EVT_CONNECT){	
		if (server->count() > MAX_WEBSOCKET_CLIENT) {
			client->text("{\"cmd\":\"cls\",\"code\":1111}");
		}
	}else if(type == WS_EVT_DISCONNECT) {
		//client->close(true);
	}else if(type == WS_EVT_ERROR) {
		client->close(true);
	}else if (type == WS_EVT_DATA) {
		String msg = "";
		for(size_t i=0; i < len; i++) {
			msg += (char) data[i];
		}
		DynamicJsonBuffer jsonBuffer;
		JsonObject &root = jsonBuffer.parseObject(msg);
		if (!root.success()) {
			return;
		}		
		const char *command = root["cmd"];			/* Получить показания датчика*/
		JsonObject& json = jsonBuffer.createObject();
		json["cmd"] = command;
		if (strcmp(command, "wt") == 0){			
			Scale.doData(json);
		}else if (strcmp(command, "tp") == 0){
			#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
				Scale.tare();
			#endif 
		}/*else if (strcmp(command, "scan") == 0) {
			WiFi.scanNetworksAsync(printScanResult, true);
			return;
		}*/else if (strcmp(command, "binfo") == 0) {
			BATTERY->doInfo(json);
		}else {
			return;
		}
		size_t lengh = json.measureLength();
		AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(lengh);
		if (buffer) {
			json.printTo((char *)buffer->get(), lengh + 1);
			if (client){
				client->text(buffer);
			}else{
				delete buffer;
			}
		}
	}
}