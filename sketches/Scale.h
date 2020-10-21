#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "HX711.h"
#include "BrowserServer.h"
#include "CoreMemory.h"
#include "Task.h"

//#define DEBUG_WEIGHT_RANDOM
//#define DEBUG_WEIGHT_MILLIS

#define DOUT_PIN		33
#define SCK_PIN			32
#define RATE_PIN		25

#define PAGE_FILE			"/calibr.html"
#define CDATE_FILE			"/cdate.json"

#define WEIGHT_MAX_JSON		"mw_id"
#define ZERO_MAN_RANGE_JSON	"zm_id"
#define ZERO_ON_RANGE_JSON	"zo_id"
#define OFFSET_JSON			"ofs"
#define AVERAGE_JSON		"av_id"
#define STEP_JSON			"st_id"
#define ACCURACY_JSON		"ac_id"
#define SCALE_JSON			"scale"
#define FILTER_JSON			"fl_id"
#define SEAL_JSON			"sl_id"
#define USER_JSON			"us_id"
#define PASS_JSON			"ps_id"
#define RATE_JSON			"rt_id"

typedef struct{
	bool isSave;
	int stabNum;
	float value;
	long int time;
}t_save_value;

class ScaleClass : public HX711{
private:
	float _weight;
	//char _buffer[10];
protected:
	BrowserServerClass *_server;
	//char _buffer[10];
	bool _authenticated;
	bool _stableWeight = true;
	t_save_value _saveWeight;
#ifndef PREFERENCES_MEMORY
	t_scales_value * _scales_value;
#else
	Preferences * _prefs;
	t_scales_value _value;
#endif // !PREFERENCES_MEMORY
	long _offset_local;
	float _round;						/* множитиль для округления */
	float _stable_step;					/* шаг для стабилизации */	
	//float _weight_zero_range;	/* вес диапазона ноля */
	float _weight_zero_auto; /* вес автосброса на ноль */
	float _tape;	/* Значение тары */

public:
	ScaleClass(byte, byte);
	~ScaleClass();
	void setup(BrowserServerClass *server);	
	//bool saveDate();
	void saveValueCalibratedHttp(AsyncWebServerRequest *);
	void fetchWeight();
	void mathScale(float referenceW, long calibrateW);
	void mathRound();
	void offset(long offset = 0){_offset_local = offset;};
	long offset(){return _offset_local;};
	void init();	
	long readAverage();
	long getValue();
	void average(unsigned char);
	unsigned char average(){return _value.average;};
	void seal(int s){_value.seal = s; };
	int seal(){ return _value.seal;};	
	void detectStable(float);
	void detectAutoZero();
		
	float getUnits();
	float getWeight();
	void zero(float range);
		
	void formatValue(float value, char* string);
	float Round(){return _round;};
	bool isSave(){return _saveWeight.isSave;};
	float getSaveValue(){return _saveWeight.value;};
	void isSave(bool s){_saveWeight.isSave = s;};
	size_t doData(JsonObject& json);
	size_t doDataRandom(JsonObject& json);
		
	char *user(){return _value.user;};
	char *password(){return _value.password;};
	int accuracy(){return _value.accuracy;};
};

extern ScaleClass Scale;
extern Task *taskWeight;
extern void takeWeight();
void handleSeal(AsyncWebServerRequest*);
void handleSlave(AsyncWebServerRequest*);