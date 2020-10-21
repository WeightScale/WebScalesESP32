/*
 * web_server_config.h
 *
 * Created: 01.04.2018 8:50:55
 *  Author: Kostya
 */ 


#pragma once

#define HTML_PROGMEM          //Использовать веб страницы из flash памяти

#ifdef HTML_PROGMEM
	#include "Page.h"
#endif

#define SOFT_AP_PASSWORD "12345678"
#define HOST_URL "sdb.net.ua"		/** Адрес базы данных*/

#define TIMEOUT_HTTP 3000			/** Время ожидания ответа HTTP запраса*/
#define STABLE_NUM_MAX 10			/** Количество стабильных измерений*/
#define MAX_EVENTS 100				/** Количество записей событий*/

#define LED  2								/** индикатор работы */
