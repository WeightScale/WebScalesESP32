#ifndef _COREMEMORY_h
#define _COREMEMORY_h

#include <Arduino.h>
#include "EEPROM32_Rotate.h"

typedef struct {
	bool autoIp;
	bool power_time_enable;
	char scaleName[16];
	char scalePass[16];
	char scaleLanIp[16];
	char scaleGateway[16];
	char scaleSubnet[16];
	char wSSID[33];
	char wKey[33];
	char apSSID[16];
	char hostUrl[0xff];
	int hostPin;
	int time_off;
	int bat_max;
	int bat_min;
} settings_t;

typedef struct {
	bool rate;
	long offset; /*  */
	unsigned char average; /*  */
	unsigned char step; /*  */
	int accuracy; /*  */
	unsigned int max; /*  */
	float scale;
	unsigned char filter;
	int seal;
	char user[16];
	char password[16];
}t_scales_value;

class CoreMemoryClass{
	public:
	struct MyEEPROMStruct {
		settings_t settings;
		t_scales_value scales_value;
	} eeprom;


	public:
	void init();
	bool save();
	bool doDefault();
	
};

extern CoreMemoryClass CoreMemory;
extern EEPROM32_Rotate EEPROMr;

#endif

