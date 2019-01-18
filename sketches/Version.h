#ifndef _VERSION_h
#define _VERSION_h
#include "webconfig.h"

#define SKETCH_VERSION "web_scales_esp32.001"
#define SPIFFS_VERSION SKETCH_VERSION



/*
Формат файла

[имя].[версия].[тип].[расширение]


имя			- имя файла или мак адресс
версия		- версия программы число
тип			- тип файла ino или spiff
расширение	- bin

пример: weight_scale.001.spiff.bin

алгоритм проверки приблизительно такой

делаем split
если не bin false
если тип spiff или ino выбираем соответствуюшую папку типа spiffs sketch 
в папке ищем папку по имени и проверяем версию

хидеры которые отправляет код

    http.useHTTP10(true);
    http.setTimeout(8000);
    http.setUserAgent(F("ESP8266-http-Update"));
    http.addHeader(F("x-ESP8266-STA-MAC"), WiFi.macAddress());
    http.addHeader(F("x-ESP8266-AP-MAC"), WiFi.softAPmacAddress());
    http.addHeader(F("x-ESP8266-free-space"), String(ESP.getFreeSketchSpace()));
    http.addHeader(F("x-ESP8266-sketch-size"), String(ESP.getSketchSize()));
    http.addHeader(F("x-ESP8266-sketch-md5"), String(ESP.getSketchMD5()));
    http.addHeader(F("x-ESP8266-chip-size"), String(ESP.getFlashChipRealSize()));
    http.addHeader(F("x-ESP8266-sdk-version"), ESP.getSdkVersion());

    if(spiffs) {
	    http.addHeader(F("x-ESP8266-mode"), F("spiffs"));
	    } else {
	    http.addHeader(F("x-ESP8266-mode"), F("sketch"));
    }

    if(currentVersion && currentVersion[0] != 0x00) {
	    http.addHeader(F("x-ESP8266-version"), currentVersion);
    }

    const char * headerkeys[] = { "x-MD5" };
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

    // track these headers
    http.collectHeaders(headerkeys, headerkeyssize);

*/

#endif //_VERSION_h