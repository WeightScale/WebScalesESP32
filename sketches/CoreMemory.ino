#include "CoreMemory.h"
#include "webconfig.h"

CoreMemoryClass CoreMemory;
EEPROM32_Rotate EEPROMr;

void CoreMemoryClass::init(){
	SPIFFS.begin(true);
	/* Разделы памяти для хранения по очереди. Настраиваются с помощью ESP32 Partition manager*/
	EEPROMr.add_by_name("eeprom");
	EEPROMr.add_by_name("eeprom1");
	EEPROMr.offset(4080); /* 4096 - 16 byte  */
	EEPROMr.begin(4096);	
	EEPROMr.get(0, eeprom);
	/*if (percentUsed() >= 0) {
		get(0, eeprom);
	}else{
		doDefault();	
	}*/	
}

bool CoreMemoryClass::save(){
	EEPROMr.put(0, eeprom);
	return EEPROMr.commit();
}

bool CoreMemoryClass::doDefault(){
	String u = "admin";
	String p = "1234";
	String apSsid = "SCALES";
	u.toCharArray(eeprom.scales_value.user, u.length()+1);
	p.toCharArray(eeprom.scales_value.password, p.length()+1);
	u.toCharArray(eeprom.settings.scaleName, u.length()+1);
	p.toCharArray(eeprom.settings.scalePass, p.length()+1);
	apSsid.toCharArray(eeprom.settings.apSSID, apSsid.length() + 1);
	eeprom.battery.bat_max = MAX_CHG;
	eeprom.battery.bat_min = MIN_CHG;
	eeprom.cloud.time_off = 600000;
	eeprom.cloud.hostPin = 0;
	eeprom.settings.autoIp = true;	
	
	eeprom.scales_value.accuracy = 1;
	eeprom.scales_value.average = 1;
	eeprom.scales_value.filter = 100;
	eeprom.scales_value.max = 1000;
	eeprom.scales_value.step = 1;
	return save();
}


