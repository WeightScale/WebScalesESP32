#include "CoreMemory.h"
#include <SPIFFS.h>
#include "Battery.h"
#include "webconfig.h"

CoreMemoryClass CoreMemory;
#ifndef PREFERENCES_MEMORY
	EEPROM32_Rotate EEPROMr;
#endif // !PREFERENCES_MEMORY



void CoreMemoryClass::init(){
	SPIFFS.begin();
#ifndef PREFERENCES_MEMORY
	/* Разделы памяти для хранения по очереди. Настраиваются с помощью ESP32 Partition manager*/
	EEPROMr.add_by_name("eeprom");
	EEPROMr.add_by_name("eeprom1");
	EEPROMr.offset(4080); /* 4096 - 16 byte  */
	EEPROMr.begin(4096);	
	EEPROMr.get(0, eeprom);
#else	
	_memory = new MemoryClass<CoreMemoryClass::MyEEPROMStruct>("memory");
	//_memory->init();
	CoreMemoryClass::MyEEPROMStruct &test = _memory->data();
	_eeprom = &_memory->data();
	_eeprom->scales_value.accuracy++;
	_memory->save();
	_memory->close();
	_eeprom->scales_value.accuracy += test.scales_value.accuracy;
	begin("memory");
#endif // !PREFERENCES_MEMORY
}

bool CoreMemoryClass::save(){
#ifndef PREFERENCES_MEMORY
	EEPROMr.put(0, eeprom);
	return EEPROMr.commit();
#endif // !PREFERENCES_MEMORY
}

bool CoreMemoryClass::doDefault(){
#ifndef PREFERENCES_MEMORY
	String u = F("admin");
	String p = F("1234");
	String apSsid = F("SCALES");
	u.toCharArray(eeprom.scales_value.user, u.length()+1);
	p.toCharArray(eeprom.scales_value.password, p.length()+1);
	u.toCharArray(eeprom.settings.scaleName, u.length()+1);
	p.toCharArray(eeprom.settings.scalePass, p.length()+1);
	apSsid.toCharArray(eeprom.settings.apSSID, apSsid.length() + 1);
	eeprom.settings.bat_max = MAX_CHG;
	eeprom.settings.bat_min = MIN_CHG;
	eeprom.settings.time_off = 600000;
	eeprom.settings.hostPin = 0;
	eeprom.settings.autoIp = true;	
	
	eeprom.scales_value.accuracy = 1;
	eeprom.scales_value.average = 1;
	eeprom.scales_value.filter = 100;
	eeprom.scales_value.max = 1000;
	eeprom.scales_value.step = 1;
	eeprom.scales_value.zero_man_range = 0.02;
	eeprom.scales_value.zero_on_range = 0.02;
	eeprom.scales_value.zero_auto = 4;
	eeprom.scales_value.enable_zero_auto = false;
	return save();
#else
	return true;
#endif // !PREFERENCES_MEMORY

	
}


