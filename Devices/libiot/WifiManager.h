/* WifiManager.h

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

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

#define DEFAULT_SSID ""
#define DEFAULT_PASS ""
#define DEFAULT_DEVICE_ID "device"
#define DEFAULT_NODE_ID "1"
#define DEFAULT_IOT_PORT "8000"
#define CONFIG_PORTAL_PASS "iotconfig"

const int WIFI_CONNECT_NUM_ATTEMPTS = 50;
const int SSID_MAX_LENGTH = 32;
const int PASS_MAX_LENGTH = 32;
const int DEV_ID_MAX_LENGTH = 32;
const int NODE_ID_MAX_LENGTH = 32;
const int IOT_PORT_MAX_LENGTH = 8;
const int USER_PARAMS_MAX_NUM = 8;
const int USER_PARAM_MAX_LENGTH = 32;
const int CONFIG_PORTAL_HTML_LENGTH = 4096;
const byte DNS_PORT = 53;
const unsigned int EEPROM_ADDRESS = 0;
const char VALIDATION_STRING[8] = "xyz123";

struct WifiConfig {
    char valid[8];                   
    char ssid[SSID_MAX_LENGTH];       
    char pass[PASS_MAX_LENGTH];       
    char dev_id[DEV_ID_MAX_LENGTH];   
    char node_id[NODE_ID_MAX_LENGTH]; 
    char iot_port[8];
};

enum class WifiStatus {
    Idle = 0,
    Connected,
    AccessPoint
};

class WifiManager {

public:

    // Create with status indicator LED and debug serial port
    WifiManager();
    WifiManager(int status_led_pin);
    WifiManager(int status_led_pin, Stream *debug_serial);

    // Load configuration from EEPROM; return false if no valid configuration found
    bool init();

    // Connect to network with credentials specified in configuration
    bool connect();

    // Set callback function for successful connect
    void set_connect_handler(void (*handler)(void *), void *userdata) {
        connect_handler = handler;
        connect_userdata = userdata;
    }

    // Open access point for configuration
    bool open_access_point();

    // Main loop; return false if disconnected
    bool loop() {
        if (this->status == WifiStatus::Connected)
            return true;
        else if (this->status == WifiStatus::AccessPoint) {
            dns_server.processNextRequest();
            web_server.handleClient();
            return true;
        }
        digitalWrite(status_led_pin, HIGH);
        return false;
    }
    
    WifiStatus get_status()       { return this->status; }
    IPAddress get_local_address() { return this->local_address; }

    // Retrieve configuration parameter by name, includes:
    // - Defaults: "SSID", "Pass", "DevID", "NodeID"
    void get_config(const char *p_name, char *p_value);
    void get_dev_id(char *buff);
    void get_node_id(char *buff);
    uint16_t get_iot_port();

protected:

    void make_configuration_portal();
    void serve_configuration_portal();
    void handle_root();
    void eeprom_save();
    bool eeprom_load();
    void print_config();
    void blinks(int status_led_pin, int n, int dt_ms);

    bool initialized;
    int status_led_pin;
    Stream *debug_serial;

    WifiConfig config;
    IPAddress local_address;

    // Access point for configuration portal
    DNSServer dns_server;
    IPAddress ap_address;
    ESP8266WebServer web_server;

    WifiStatus status;
    char portal_html[CONFIG_PORTAL_HTML_LENGTH];

    // Callback for successful connect
    void (*connect_handler)(void *);
    void *connect_userdata;
};
#endif
