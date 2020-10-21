#pragma once
#include <Arduino.h>

#define PREFERENCES_MEMORY

#define KEY_BAT_MAX "b_max"				/* namespace bat max */
#define KEY_BAT_MIN "b_min"				/* namespace bat min */
#define KEY_AUTO_IP "id_auto"			/* namespace auto ip */
#define KEY_AP_SSID "id_assid"			/* ap ssid */
#define KEY_SCALE_NAME "id_n_admin"		/* scale name */
#define KEY_SCALE_PASS "id_p_admin"		/* scale password */
#define KEY_LAN_IP "id_lan_ip"			/* lan ip */
#define KEY_LAN_GATEWAY "id_gateway"	/* lan gateway */
#define KEY_LAN_SUBNET "id_subnet"		/* lan subnet */ 
#define KEY_W_SSID "id_ssid"			/* ssid */ 
#define KEY_W_KEY "id_key"				/* key */ 
#define KEY_HOST "id_host"				/* host */ 
#define KEY_PIN "id_pin"				/* pi */
#define KEY_ZERO_MAN_RANGE "z_man"		/* zero man */
#define KEY_OFFSET "offset"				/* offset */
#define KEY_RATE "rate"					/* rate */
#define KEY_FILTER "filter"				/* filter */
#define KEY_ACCURACY "accur"			/* accuracy */
#define KEY_STEP "step"					/* step */
#define KEY_ZERO_AUTO_RANGE "step"					/* step */

#ifdef PREFERENCES_MEMORY
	#include <Preferences.h>
#else
	#include "EEPROM32_Rotate.h"
#endif // EEPROM_MEMORY

typedef struct {
	bool autoIp;
	char scaleName[16];
	char scalePass[16];
	char scaleLanIp[16];
	char scaleGateway[16];
	char scaleSubnet[16];
	char wSSID[33];
	char wKey[33];
	char apSSID[16];
	char hostUrl[0xff];
	unsigned int hostPin;
	unsigned int time_off;
	unsigned int bat_max;
	unsigned int bat_min;
} settings_t;

typedef struct {
	bool rate;
	bool enable_zero_auto;
	long offset; /*  */
	unsigned char average; /*  */
	unsigned char step; /*  */
	int accuracy; /*  */
	float max; /*  */
	float scale;
	unsigned char filter;
	unsigned int seal;
	float zero_man_range;			// 0,2,4,10,20,50,100% диапазон обнуления
	float zero_on_range; 			// 0,2,4,10,20,50,100% диапазон обнуления
	unsigned char zero_auto;			// 0~4 дискрет авто обнуление
	char user[16];
	char password[16];
}t_scales_value;

template <typename T>class MemoryClass : protected Preferences {
private:
	String _name;
	T _data;
protected:
	size_t get() {
		return getBytes(_name.c_str(), &_data, sizeof(T));
	};	
	bool init() {
		if (begin(_name.c_str())){
			get();			
			return true;
		}		
		return false;
	};
public:
	MemoryClass(String name) : _name(name) {		
		init();
	};
	~MemoryClass() {
		delete _data;
		close(); 
	};	
	
	size_t save() {		
		return putBytes(_name.c_str(), &_data, sizeof(T));
	};
	void close() {
		end();
	};
	
	T &data() {return _data;};
	bool doDefault();
};

#ifndef PREFERENCES_MEMORY
	class CoreMemoryClass{
#else
	class CoreMemoryClass : public Preferences{
#endif // !PREFERENCES_MEMORY
	struct MyEEPROMStruct {
		settings_t settings;
		t_scales_value scales_value;
	};	
public:
	MyEEPROMStruct *_eeprom;	
#ifdef PREFERENCES_MEMORY
	MemoryClass<MyEEPROMStruct> *_memory;
#endif // !PREFERENCES_MEMORY
	CoreMemoryClass(){};
	~CoreMemoryClass() {
		//delete _memory;
	};
	public:
	void init();
	bool save();
	bool doDefault();
};

extern CoreMemoryClass CoreMemory;
#ifndef PREFERENCES_MEMORY
	extern EEPROM32_Rotate EEPROMr;
#endif // EEPROM_MEMORY

