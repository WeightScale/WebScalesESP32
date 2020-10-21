#include <WiFi.h>
#include <NetBIOS.h>
#include <HTTPClient.h>
//#include <ESPAsyncDNSServer.h>
//#include <ESPAsyncWebServer.h>
#include <functional>
#include "webconfig.h"
#include "TaskController.h"
#include "Task.h"
#include "Core.h"
#include "BrowserServer.h"
#include "Scale.h"
#include "Battery.h"

extern "C" {
	//#include <esp_wifi.h>
	extern char *netif_list;
	uint8_t etharp_request(char *, char *);    // required for forceArp to work
}

using namespace std;
using namespace std::placeholders;

void forceARP() {
	char *netif = netif_list;
	while (*netif) {
		etharp_request((netif), (netif + 4));
		netif = *((char **) netif);
	}
}

void onStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onScanResultDone(WiFiEvent_t event, WiFiEventInfo_t info);


void takeWeight();
void connectNetwork(int networksFound);
void shutDown();
//
TaskController taskController = TaskController(); /*  */
Task taskConnectWiFi([](){
	connectWifi(true);
}, 30000); /* Пытаемся соедениться с точкой доступа каждые 30 секунд */

//#define USE_SERIAL Serial

void setup() {
	
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
	WiFi.setTxPower(WIFI_POWER_19_5dBm);
	//WiFi.smartConfigDone();
	//WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	WiFi.mode(WIFI_AP_STA);
	browserServer.begin();
	Scale.setup(&browserServer);
	taskController.add(&taskConnectWiFi);
	connectWifi(false);
	taskController.add(BLINK);
	taskController.add(taskWeight);
	/* If battery discharged start timer for shutdown*/
	if (BATTERY->isDischarged()) {
		Scale.power_down();
		taskController.add(new Task(shutDown, 120000));
		taskController.remove(taskWeight);
	}
	taskController.add(new Task(forceARP, 1000));
	//ESP.wdtEnable(5000);
}

void takeWeight() {
	Scale.fetchWeight();
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.createObject();
	json["cmd"] = "wt";
	Scale.doData(json);
	BATTERY->doData(json);
	size_t lengh = json.measureLength();	
	AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(lengh);
	if (buffer) {
		json.printTo((char *)buffer->get(), lengh + 1);
		ws.textAll(buffer);
	}
	if (Scale.isSave()) {
		CORE->saveEvent("weight", Scale.getSaveValue());
		Scale.isSave(false);
	}
	taskWeight->updateCache();
}

void connectWifi(bool reconnect) {
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
		browserServer.scanNetworksAsync(connectNetwork);
	}
	else if (n > 0) {
		connectNetwork(n);
	}
}
void onScanResultDone(WiFiEvent_t event, WiFiEventInfo_t info) {
	browserServer.scanDone(info.scan_done.number);
}


void connectNetwork(int networksFound){
	for (int i = 0; i < networksFound; ++i) {
		if (WiFi.SSID(i).equals(CORE->getSSID())) {
			WiFi.persistent(true);
			WiFi.begin(CORE->getSSID().c_str(), CORE->getPASS().c_str());
			return;
		}
	}
	WiFi.scanDelete();
	taskConnectWiFi.resume();
}

void loop() {
	taskController.run();
}

void onStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
	taskConnectWiFi.pause();
	WiFi.softAP(CORE->getApSSID().c_str(), SOFT_AP_PASSWORD, info.connected.channel);   //Устанавливаем канал как роутера
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