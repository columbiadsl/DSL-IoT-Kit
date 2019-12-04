/* WifiManager.cpp

 	Copyright (c) 2019 Jeff Gregorio. All rights reserved.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "WifiManager.h"
#include "Arduino.h"

WifiManager::WifiManager() {
	WifiManager(NULL, NULL);
}

WifiManager::WifiManager(int status_led_pin) {
	WifiManager(status_led_pin, NULL);
}

WifiManager::WifiManager(int status_led_pin, Stream *debug_serial) 
: initialized(false),
  status_led_pin(status_led_pin), 
  debug_serial(debug_serial), 
  status(WifiStatus::Idle), 
  web_server(80), 
  ap_address(192, 168, 4, 1) {

}

bool WifiManager::init() {
    if (initialized) 
        return true;
    bool success;
    // EEPROM.begin(sizeof(config));
    success = eeprom_load();
    make_configuration_portal();
    initialized = true;
    return success;
}

bool WifiManager::connect() {

	// (Re-)Initialize
	WiFi.disconnect();
	WiFi.mode(WIFI_STA);
	WiFi.begin(config.ssid, config.pass);

	if (debug_serial) {
		debug_serial->println();
		debug_serial->print("Connecting... SSID: ");
		debug_serial->print(config.ssid);
		debug_serial->print(", Pass: ");
		debug_serial->println(config.pass);
	}

	// Attempt to connect to the specified netowrk up to N times
	int tries = 0;
	bool led_on = true;
	while (WiFi.status() != WL_CONNECTED) {
		tries++;
		if (tries > WIFI_CONNECT_NUM_ATTEMPTS) {
			this->status = WifiStatus::Idle;
		  	return false;
		}
		if (status_led_pin) {
			digitalWrite(status_led_pin, led_on ? LOW : HIGH);
			led_on = !led_on;
		}
		delay(500);
	}

	// Get IP address
	local_address = WiFi.localIP();     
	if (debug_serial) {
		debug_serial->print(" success\n Local IP: ");
		debug_serial->println(local_address.toString().c_str());
	}

  	// Victory dance
	this->status = WifiStatus::Connected;
  	if (status_led_pin) {
  		blinks(status_led_pin, 8, 400);
    	digitalWrite(status_led_pin, LOW);
  	}

  	// Pass to user callback
  	if (connect_handler)
  		connect_handler(connect_userdata);

	return true;
}

bool WifiManager::open_access_point() {

	// Access point name
	char ap_name[64];
	sprintf(ap_name, "ap-%s-%s", config.dev_id, config.node_id);

	// Configure WiFi for soft AP 
	WiFi.disconnect();
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(ap_address, ap_address, IPAddress(255, 255, 255, 0));
	WiFi.softAP(ap_name, CONFIG_PORTAL_PASS);
	delay(500);

	if (debug_serial) {
		debug_serial->print("Starting DNS Server \"");
		debug_serial->print(ap_name);
		debug_serial->print("\" at ");
		debug_serial->println(WiFi.softAPIP());
	}

	dns_server.setErrorReplyCode(DNSReplyCode::NoError);
	dns_server.start(DNS_PORT, "*", WiFi.softAPIP());

	// Web Server
	web_server.on("/", std::bind(&WifiManager::handle_root, this));
	web_server.onNotFound([this]() {
		web_server.send(200, "text/html", portal_html);
	});
	web_server.begin();
	this->status = WifiStatus::AccessPoint;

  if (status_led_pin)
    digitalWrite(status_led_pin, HIGH);
}

void WifiManager::get_config(const char *param_name, char *param_value) {
    if (!strcmp(param_name, "SSID")) 
        strcpy(param_value, config.ssid);
    else if (!strcmp(param_name, "Pass")) 
        strcpy(param_value, config.pass);
    else if (!strcmp(param_name, "DevID")) 
        strcpy(param_value, config.dev_id);
    else if (!strcmp(param_name, "NodeID"))
        strcpy(param_value, config.node_id);
    else if (!strcmp(param_name, "IoTPort"))
        strcpy(param_value, config.iot_port);
}  

void WifiManager::get_dev_id(char *buff) {
	strcpy(buff, config.dev_id);
}

void WifiManager::get_node_id(char *buff) {
	strcpy(buff, config.node_id);
}

uint16_t WifiManager::get_iot_port() {
	return atoi(config.iot_port);
}

void WifiManager::make_configuration_portal() {
	
	int idx = 0;
	idx += snprintf(portal_html, CONFIG_PORTAL_HTML_LENGTH,
		"<html>\
		<head>\
    <meta name='description' content='IoT Device Configuration Portal'>\
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\
    <title>IoT Device Configuration Portal</title>\
    <style>\
      body {\
          background-color: #333333;\
          font-family: Arial, Helvetica, Sans-Serif;\
          Color: #FFFFFF;\
        }\
      div {\
        margin: 0 auto;\
        padding-top: 10px;\
        padding-right: 20px;\
        padding-left: 20px;\
        text-align: left;\
        width:350px;\
      }\
      input {\
        float: right;\
      }\
    </style>\
		</head>\
		<body>\
		  <h1>IoT Device Configuration Portal</h1>\
		  <form action='/' method='post'>"
	);

  	// Format HTML elements for default configuration parameters
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div>SSID: <input type='text' name='SSID' size='%d' value='%s'><p></div>",
		SSID_MAX_LENGTH, config.ssid);
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div>Pass: <input type='password' name='Pass' size='%d' value='%s'><p></div>",
		PASS_MAX_LENGTH, config.pass);
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div>Device ID: <input type='text' name='DevID' size='%d' value='%s'><p></div>",
		DEV_ID_MAX_LENGTH, config.dev_id);
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div>Node ID: <input type='text' name='NodeID' size='%d' value='%s'><p></div>",
		NODE_ID_MAX_LENGTH, config.node_id);
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div>UDP/TCP Port: <input type='text' name='IoTPort' size='%d' value='%s'><p></div>",
		IOT_PORT_MAX_LENGTH, config.iot_port);

	// Submit button and end tags
	idx += snprintf(portal_html+idx, CONFIG_PORTAL_HTML_LENGTH-idx,
		"<div><input type='submit' name='Update' value='Submit'></div>\
	  </form>\
	</body>\
	</html>"
	);
}

void WifiManager::handle_root() {

	if (web_server.hasArg("Update")) {
		if (web_server.hasArg("SSID")) {
		  sprintf(config.ssid, "%s", web_server.arg("SSID").c_str());
		}
		if (web_server.hasArg("Pass")) {
		  sprintf(config.pass, "%s", web_server.arg("Pass").c_str());
		}
		if (web_server.hasArg("DevID")) {
		  sprintf(config.dev_id, "%s", web_server.arg("DevID").c_str());
		}
		if (web_server.hasArg("NodeID")) {
		  sprintf(config.node_id, "%s", web_server.arg("NodeID").c_str());
		}
		if (web_server.hasArg("IoTPort")) {
		  sprintf(config.iot_port, "%s", web_server.arg("IoTPort").c_str());
		} 
	}
	eeprom_save();
  	print_config();
	make_configuration_portal();
	web_server.send(200, "text/html", portal_html);
	WiFi.softAPdisconnect();
	if (!connect())
		open_access_point();
}

void WifiManager::eeprom_save() {
    strcpy(config.valid, VALIDATION_STRING);
    // EEPROM.begin(sizeof(config));
    EEPROM.put(EEPROM_ADDRESS, config);
    EEPROM.commit();
    // EEPROM.end();
}

bool WifiManager::eeprom_load() {

    bool success = true;
    // EEPROM.begin(sizeof(config));
    EEPROM.get(EEPROM_ADDRESS, config);

    // Use defaults if we haven't yet written a valid config struct
    if (!(strcmp(config.valid, VALIDATION_STRING) == 0)) {
        
        if (debug_serial) 
          	debug_serial->println("Using default configuration:");
          
        // Set defaults
        strcpy(config.ssid, DEFAULT_SSID);
        strcpy(config.pass, DEFAULT_PASS);
        strcpy(config.dev_id, DEFAULT_DEVICE_ID);
        strcpy(config.node_id, DEFAULT_NODE_ID);
        strcpy(config.iot_port, DEFAULT_IOT_PORT);
        success = false;
    }
    else if (debug_serial) 
        debug_serial->println("Configuration loaded:");
        
    print_config();
    // EEPROM.end();
    return success;
}

void WifiManager::print_config() {
    if (debug_serial) {
        debug_serial->print("SSID: ");
        debug_serial->println(config.ssid);
        debug_serial->print("Pass: ");
        debug_serial->println(config.pass);
        debug_serial->print("DevID: ");
        debug_serial->println(config.dev_id);
        debug_serial->print("NodeID: ");
        debug_serial->println(config.node_id);
        debug_serial->print("IoTPort: ");
        debug_serial->println(config.iot_port);
    }
}
  
void WifiManager::blinks(int status_led_pin, int n, int dt_ms) {
	digitalWrite(status_led_pin, HIGH);
	for (int i = 0; i < n; i++) {
		digitalWrite(status_led_pin, LOW);
		delay(dt_ms/2);
		digitalWrite(status_led_pin, HIGH);
		delay(dt_ms/2);
	}
}
