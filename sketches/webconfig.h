/*
 * web_server_config.h
 *
 * Created: 01.04.2018 8:50:55
 *  Author: Kostya
 */ 


#ifndef WEB_SERVER_CONFIG_H_
#define WEB_SERVER_CONFIG_H_

#define HTML_PROGMEM          //Использовать веб страницы из flash памяти

#ifdef HTML_PROGMEM
	#include "Page.h"
#endif

//#define MY_HOST_NAME "scl"
//#define SOFT_AP_SSID "SCALES"
#define SOFT_AP_PASSWORD "12345678"
#define HOST_URL "sdb.net.ua"		/** Адрес базы данных*/

#define TIMEOUT_HTTP 3000			/** Время ожидания ответа HTTP запраса*/
#define STABLE_NUM_MAX 10			/** Количество стабильных измерений*/
#define MAX_EVENTS 100				/** Количество записей событий*/

#define LED  2								/* индикатор работы */


#endif /* WEB_SERVER_CONFIG_H_ */