#include <WiFi.h>
#include <NetBIOS.h>
//#include <ESPAsyncWebServer.h>
#include <functional>
#include "webconfig.h"
#include "TaskController.h"
#include "Task.h"
#include "Core.h"
#include "BrowserServer.h"
#include "Scale.h"
#include "Battery.h"


using namespace std;
using namespace std::placeholders;


void onStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onScanResultDone(WiFiEvent_t event, WiFiEventInfo_t info);


void takeWeight();
void connectWifi();
void connectNetwork(int networksFound);
void shutDown();

//
TaskController taskController = TaskController(); /*  */
Task taskConnectWiFi(connectWifi, 30000); /* Пытаемся соедениться с точкой доступа каждые 30 секунд */
Task taskWeight(takeWeight, 200);

//#define USE_SERIAL Serial

void setup(){
/*#if POWER_PLAN
	pinMode(EN_NCP, OUTPUT);
	digitalWrite(EN_NCP, HIGH);
#endif*/ 
	Serial.begin(115200);
	/*while (digitalRead(PWR_SW) == HIGH){
		delay(100);
	};*/
	CoreMemory.init();
	BLINK = new BlinkClass();
	BATTERY = new BatteryClass();	
	taskController.add(BATTERY);
	
	WiFi.onEvent(onStationModeConnected, SYSTEM_EVENT_STA_CONNECTED);
	WiFi.onEvent(onStationModeDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
	WiFi.onEvent(onScanResultDone, SYSTEM_EVENT_SCAN_DONE);
	//ESP.eraseConfig();
	WiFi.persistent(false);	
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	//WiFi.smartConfigDone();
	WiFi.mode(WIFI_AP_STA);
	browserServer.begin();
	Scale.setup(&browserServer);
	taskController.add(&taskConnectWiFi);
	connectWifi();
	taskController.add(BLINK);
	taskController.add(&taskWeight);
	/* If battery discharged start timer for shutdown*/
	if (BATTERY->isDischarged()) {
		Scale.power_down();
		taskController.add(new Task(shutDown, 120000));
		taskController.remove(&taskWeight);
	}
	//ESP.wdtEnable(5000);
}

void takeWeight() {
	Scale.fetchWeight();
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.createObject();
	json["cmd"] = "wt";
	Scale.doData(json);
	size_t lengh = json.measureLength();	
	AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(lengh);
	if (buffer) {
		json.printTo((char *)buffer->get(), lengh + 1);
		ws.textAll(buffer);
	}
	if (Scale.isSave()) {
		CORE->saveEvent("weight", String(Scale.getSaveValue()));
		Scale.setIsSave(false);
	}
	taskWeight.updateCache();
}

void connectWifi() {
	taskConnectWiFi.pause();
	if (String(CORE->getSSID()).length() == 0) {
		WiFi.setAutoConnect(false);
		WiFi.setAutoReconnect(false);
		return;
	}
	if (WiFi.SSID().equals(CORE->getSSID())) {
		WiFi.begin();
		return;
	}
	WiFi.disconnect(false);
	/*!  */
	int n = WiFi.scanComplete();
	if (n == -2) {
		WiFi.scanNetworks(true);
	}
	else if (n > 0) {
		connectNetwork(n);
	}
}
void onScanResultDone(WiFiEvent_t event, WiFiEventInfo_t info) {
	connectNetwork(info.scan_done.number);	
}


void connectNetwork(int networksFound){
	for (int i = 0; i < networksFound; ++i) {
		if (WiFi.SSID(i).equals(CORE->getSSID())) {
			WiFi.persistent(true);
			WiFi.begin(CORE->getSSID(), CORE->getPASS());
			return;
		}
	}
	WiFi.scanDelete();
	taskConnectWiFi.resume();
}

void loop() {
	//ESP.wdtFeed();
	taskController.run();	
	//DNS
	//dnsServer.processNextRequest();
	//HTTP
		
}

void onStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
	//int ch = info.connected.channel;
	taskConnectWiFi.pause();
	WiFi.softAP(CORE->getApSSID(), SOFT_AP_PASSWORD, info.connected.channel);   //Устанавливаем канал как роутера
	//WiFi.softAP("SCALES", SOFT_AP_PASSWORD, info.connected.channel);
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	NBNS.begin(CORE->getHostname().c_str());
	BLINK->onRun(bind(&BlinkClass::blinkSTA, BLINK));
}

void onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
	//WiFi.scanDelete();
	//WiFi.scanNetworks(true);
	taskConnectWiFi.resume();
	BLINK->onRun(bind(&BlinkClass::blinkAP, BLINK));
}

void shutDown() {
	ESP.deepSleep(10);
	//ESP.deepSleep(0, WAKE_RF_DISABLED);
}