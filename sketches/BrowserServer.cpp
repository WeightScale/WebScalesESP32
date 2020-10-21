#include "BrowserServer.h"
#include <ESPAsyncWebServer.h>
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
#include "HttpUpdater.h"
#include "CoreMemory.h"
#include "Battery.h"
#include "Scale.h"

/* Soft AP network parameters */
IPAddress apIP(192,168,4,1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress lanIp;			// Надо сделать настройки ip адреса
IPAddress gateway;

BrowserServerClass browserServer(80);
AsyncWebSocket ws("/ws");
//AsyncEventSource events("/events");
//AsyncDNSServer dnsServer;

BrowserServerClass::BrowserServerClass(uint16_t port) : AsyncWebServer(port) {}

BrowserServerClass::~BrowserServerClass(){}

std::function<void(int)> BrowserServerClass::_onComplete;

void BrowserServerClass::begin(){
	/* Setup the DNS server redirecting all the domains to the apIP */
	//dnsServer.setTTL(300);
	//dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
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
	//dnsServer.start(DNS_PORT, "*", apIP);
	AsyncWebServer::begin(); // Web server start
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
		request->send(200, F("text/plain"), str);
	});
	on("/rst",HTTP_ANY,[this](AsyncWebServerRequest * request){
		if (!isAuthentified(request)){
			return request->requestAuthentication();
		}
		if(CoreMemory.doDefault())
			request->send(200,F("text/html"), F("Установлено!"));
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
	on("/bat.png",handleBatteryPng);
	on("/scales.png",handleScalesPng);	
	on("/und.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), und_png, und_png_len) ;
		request->send(response) ;
	});
	on("/set.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), set_png, set_png_len);
		request->send(response);
	});
	on("/zerow.png",[](AsyncWebServerRequest * request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), zerow_png, zerow_png_len);
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
				response->addHeader(F("Connection"), F("close"));
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
}

bool BrowserServerClass::checkAdminAuth(AsyncWebServerRequest * r) {	
	return r->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
}

bool BrowserServerClass::isAuthentified(AsyncWebServerRequest * request){
	if (!request->authenticate(CORE->getNameAdmin().c_str(), CORE->getPassAdmin().c_str())){
		if (!checkAdminAuth(request)){
			return false;
		}
	}
	return true;
}

void BrowserServerClass::scanNetworksAsync(std::function<void(int)> onComplete) {
	_onComplete = onComplete;
	WiFi.scanNetworks(true);	
};

void BrowserServerClass::scanDone(int scanCount){
	_onComplete(scanCount);
	_onComplete = nullptr;
}


void handleSettings(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request))
		return request->requestAuthentication();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	JsonObject& scale = root.createNestedObject(SCALE_JSON);
#ifndef PREFERENCES_MEMORY
	scale["id_auto"] = CoreMemory.eeprom.settings.autoIp;
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
#else
	scale[KEY_AUTO_IP] = CoreMemory.getBool(KEY_AUTO_IP);
	scale[KEY_AP_SSID] = CoreMemory.getString(KEY_AP_SSID);
	scale[KEY_SCALE_NAME] = CoreMemory.getString(KEY_SCALE_NAME);   //CoreMemory.eeprom.settings.scaleName;
	scale[KEY_SCALE_PASS] = CoreMemory.getString(KEY_SCALE_PASS);   //CoreMemory.eeprom.settings.scalePass;
	scale[KEY_LAN_IP] = CoreMemory.getString(KEY_LAN_IP);   //CoreMemory.eeprom.settings.scaleLanIp;
	scale[KEY_LAN_GATEWAY] = CoreMemory.getString(KEY_LAN_GATEWAY);   //CoreMemory.eeprom.settings.scaleGateway;
	scale[KEY_LAN_SUBNET] = CoreMemory.getString(KEY_LAN_SUBNET);   //CoreMemory.eeprom.settings.scaleSubnet;
	scale[KEY_W_SSID] = CoreMemory.getString(KEY_W_SSID);   //String(CoreMemory.eeprom.settings.wSSID);
	scale[KEY_W_KEY] = CoreMemory.getString(KEY_W_KEY);   //String(CoreMemory.eeprom.settings.wKey);
	
	JsonObject& server = root.createNestedObject(SERVER_JSON);
	server[KEY_HOST] = CoreMemory.getString(KEY_HOST);   //String(CoreMemory.eeprom.settings.hostUrl);
	server[KEY_PIN] = CoreMemory.getInt(KEY_PIN, 1);   //CoreMemory.eeprom.settings.hostPin;
#endif // !PREFERENCES_MEMORY

	
	
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
	root["id_local_host"] = WiFi.getHostname();
	root["id_ap_ssid"] = String(CORE->getApSSID());
	root["id_ap_ip"] = toStringIp(WiFi.softAPIP());
	root["id_ip"] = toStringIp(WiFi.localIP());
	root["sl_id"] = String(Scale.seal());
	root["chip_v"] = String(ESP.getCpuFreqMHz());
	root["id_mac"] = WiFi.macAddress();
	root["id_vr"] = SKETCH_VERSION;
	response->setLength();
	request->send(response);
}

#ifdef HTML_PROGMEM
	void handleBatteryPng(AsyncWebServerRequest * request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), bat_png, bat_png_len);
		request->send(response);
	}

	void handleScalesPng(AsyncWebServerRequest * request){	
		AsyncWebServerResponse *response = request->beginResponse_P(200, F("image/png"), scales_png, scales_png_len);
		request->send(response);
	}
#endif // HTML_PROGMEM

void handleRSSI(AsyncWebServerRequest * request){
	request->send(200, F("text/html"), String(WiFi.RSSI()));
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
		//String msg = "";
		//for(size_t i=0; i < len; i++) {
		//	msg += (char) data[i];
		//}
		DynamicJsonBuffer jsonBuffer(len);
		JsonObject &root = jsonBuffer.parseObject(data);
		if (!root.success()) {
			return;
		}
		const char *command = root["cmd"];			/* Получить показания датчика*/
		JsonObject& json = jsonBuffer.createObject();
		json["cmd"] = command;
		if (strcmp(command, "wt") == 0){
			Scale.doData(json);
			BATTERY->doData(json);
		}else if (strcmp(command, "tp") == 0){
			#if !defined(DEBUG_WEIGHT_RANDOM)  && !defined(DEBUG_WEIGHT_MILLIS)
#ifndef PREFERENCES_MEMORY
			Scale.zero(CoreMemory.eeprom.scales_value.zero_man_range);
#else
			Scale.zero(CoreMemory.getFloat(KEY_ZERO_MAN_RANGE));
#endif // !PREFERENCES_MEMORY

				
			#endif 
		}else if (strcmp(command, "scan") == 0) {
			browserServer.scanNetworksAsync(printScanResult);
			return;
		}else if (strcmp(command, "binfo") == 0) {
			BATTERY->doInfo(json);
		}else if (strcmp(command, "stopnet") == 0) {		/* останавливаем попвтки подключения к сети */
			taskConnectWiFi.pause();
		}else{
			return;
		}
		size_t lengh = json.measureLength();
		AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(lengh);
		if (buffer) {
			json.printTo((char *)buffer->get(), lengh + 1);
			if (client) {
				client->text(buffer);
			}else{
				delete buffer;
			}
		}
	}
}