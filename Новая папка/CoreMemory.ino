#include "CoreMemory.h"
#include "webconfig.h"

void CoreMemoryClass::init(){
	
	//eeprom.scales_value.password = "1234";
	/*begin(sizeof(MyEEPROMStruct));
	if (percentUsed() >= 0) {
		get(0, eeprom);
	}else{
		doDefault();	
	}*/
}

bool CoreMemoryClass::save(){
	/*put(0, eeprom);
	return commit();*/
	return false;
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
	eeprom.settings.bat_max = MIN_CHG + 1;
	eeprom.settings.time_off = 600000;
	eeprom.settings.hostPin = 0;
	eeprom.settings.autoIp = true;	
	return save();
}

CoreMemoryClass CoreMemory;

