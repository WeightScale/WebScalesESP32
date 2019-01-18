/*
 * web_server_config.h
 *
 * Created: 01.04.2018 8:50:55
 *  Author: Kostya
 */ 


#ifndef WEB_SERVER_CONFIG_H_
#define WEB_SERVER_CONFIG_H_

#define EXTERNAL_POWER 0
#define INTERNAL_POWER 1

//#define POWER_PLAN				INTERNAL_POWER
#define POWER_PLAN				EXTERNAL_POWER
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

#define EN_NCP  12							/* сигнал включения питания  */
#define PWR_SW  13							/* сигнал от кнопки питания */
#define EN_MT	15							/*Сигнал включения питание датчика*/
#define LED  2								/* индикатор работы */

//#define MAX_CHG 1013//980	//делитель U2=U*(R2/(R1+R2)) 0.25
//#define MIN_CHG 880			//ADC = (Vin * 1024)/Vref  Vref = 1V
//#define MIN_CHG 670			//ADC = (Vin * 1024)/Vref  Vref = 1V	Vin = 0.75 5.5v-6.5v
//#define MIN_CHG 500			//ADC = (Vin * 1024)/Vref  Vref = 1V  Vin = 0.49v  3.5v-4.3v
#if POWER_PLAN == INTERNAL_POWER
	#define MIN_CHG 500			//ADC = (Vin * 1024)/Vref  Vref = 1V  Vin = 0.49v  3.5v-4.3v
#else if POWER_PLAN == EXTERNAL_POWER
	#define MIN_CHG 780			//ADC = (Vin * 1024)/Vref  Vref = 1V	Vin = 0.75 5.5v-6.5v
#endif


#endif /* WEB_SERVER_CONFIG_H_ */