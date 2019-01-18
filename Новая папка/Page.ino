// 
// 
// 

#include "Page.h"

PageClass::PageClass() : _style(""),_script(""),_body(""),_page(""){	
}

PageClass::PageClass(String onLoad ) : _style(""),_script(""),_body(""),_page(""),_onload(onLoad){
}

PageClass::~PageClass(){}

void PageClass::appendStyle(String stl){
	_style += stl;	
}

void PageClass::appendScript(String scr){
	_script += scr;	
}

void PageClass::appendBody(String body){
	_body += body;	
}

String PageClass::go(){
	_page="<html><head>";
	_page+="<meta charset='utf-8'>";
	_page+="<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1' />";
	_page+="<style>" + _style + "</style>";
	_page+="<script>" + _script + "</script></head>";
	_page+="<body onload='"+_onload+";'>";
	_page+=_body + "</body></html>";
	
	return _page;	
}








