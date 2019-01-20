#ifndef _SCALE_h
#define _SCALE_h

/*
#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif*/
#include "HX711.h"
#include "BrowserServer.h"
#include "Core.h"
#include "CoreMemory.h"

//#define DEBUG_WEIGHT_RANDOM
//#define DEBUG_WEIGHT_MILLIS

#define DOUT_PIN		33
#define SCK_PIN			32
#define RATE_PIN		25

#define PAGE_FILE			"/calibr.html"
#define CDATE_FILE			"/cdate.json"

#define WEIGHT_MAX_JSON		"mw_id"
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

class BrowserServerClass;

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
		t_scales_value * _scales_value;
		float _round;						/* множитиль для округления */
		float _stable_step;					/* шаг для стабилизации */	
		//bool _downloadValue();

	public:
		ScaleClass(byte, byte);
		~ScaleClass();
		void setup(BrowserServerClass *server);	
		//bool saveDate();
		void saveValueCalibratedHttp(AsyncWebServerRequest *);
		void fetchWeight();
		void mathScale(float referenceW, long calibrateW);
		void mathRound();
		void setOffset(long offset = 0){_scales_value->offset = offset;};
		void init();	
		long readAverage();
		long getValue();
		void setAverage(unsigned char);	
		void setSeal(int s){_scales_value->seal = s; };
		int getSeal(){ return _scales_value->seal;};	
		void detectStable(float);
		
		float getUnits();
		float getWeight();
		void tare();
		
		void formatValue(float value, char* string);
		
		float getRound(){return _round;};
						
		bool isSave(){return _saveWeight.isSave;};
		float getSaveValue(){return _saveWeight.value;};
		void setIsSave(bool s){_saveWeight.isSave = s;};
		
		size_t doData(JsonObject& json );
		
		char *getUser(){return _scales_value->user;};
		char *getPassword(){return _scales_value->password;};
		int getAccuracy(){return _scales_value->accuracy;};
};

extern ScaleClass Scale;
void handleSeal(AsyncWebServerRequest*);
void handleSlave(AsyncWebServerRequest*);

#endif

