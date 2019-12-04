/* TCPClient.cpp

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

#include <WifiManager.h>
#include <UDPClient.h>
#include <TCPClient.h>
#include <OSCManager.h>
#include <LEDPin.h>

Stream *debug = NULL;       // Uncomment for deployment
//Stream *debug = &Serial;    // Uncomment for testing/debugging

// Sensor pin
const int PIN_SENSOR = D1;

// WiFi, UDP, and TCP
// ==================
LEDPin wifi_led(LED_BUILTIN, 20);       // WiFi Status and UDP/TCP I/O Indicator LED
WifiManager wifi(LED_BUILTIN, debug);   // WiFi Manager
UDPClient udp_client(debug);            // UDP Client
TCPClient tcp_client(debug);            // TCP Client

// OSC
// ===
OSCManager osc(debug);                // Open Sound Control Manager
OSCMessage outgoing_msg("/gate");     // OSC Message

// EEPROM-Stored Destination IP
// ============================
const int EEPROM_DEST_IP_ADDR = 768;
struct dest_config_t {
  char valid[8];
  char addr[32];
};
dest_config_t dest;

// Main Setup
// ==========
void setup() {

  // I/O setup
  if (debug) 
    Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), sensor_change, CHANGE); 

  // Initialize EEPROM (used by WifiManager)
  EEPROM.begin(1024);
  
  // Set callback function for successful wifi connection
  wifi.set_connect_handler(wifi_connected, NULL);
  
  // Initilize and connect Wifi or open access point
  if (!wifi.init() || !wifi.connect()) 
      wifi.open_access_point(); 

  // Set OSC handlers
  osc.dispatch("/ping", osc_handle_ping);
  osc.dispatch("/config", osc_handle_config);

  // Set UDP client data handler
  udp_client.set_data_handler(udp_handle_data, NULL);
  
  // Set TCP client data and connection handlers
  tcp_client.set_data_handler(tcp_handle_data, NULL);
  tcp_client.set_connect_handler(tcp_handle_connect, NULL);
  // (TCP client connects when we receive /ping via UDP (broadcast)))
}

// Main Loop
// =========
void loop() {
  wifi.loop();
  udp_client.loop();
  wifi_led.loop();
}

// Sensor pin interrupt
// ====================
ICACHE_RAM_ATTR void sensor_change() {
  outgoing_msg.set(1, digitalRead(PIN_SENSOR));
  osc_send(outgoing_msg);
}

// WiFi Connect Handler
// ====================
/* Connection handler; set outgoing node ID and open UDP port after connection.
   also attempt to connect via TCP to a previous destination address */
void wifi_connected(void *userdata) { 
  char node_id[32];
  wifi.get_node_id(node_id);
  outgoing_msg.set(0, atoi(node_id));
  udp_client.open_port(wifi.get_iot_port());
  if (load_dest()) {
    connect_dest(dest.addr, wifi.get_iot_port());
  }
}

// Destination IP save/load/connect:
// =================================
/* Save the destination upon successful TCP connection */
void save_dest(const char *addr, uint16_t port) {
  strcpy(dest.valid, "xyz123");
  strcpy(dest.addr, addr);
  EEPROM.put(EEPROM_DEST_IP_ADDR, dest);
  EEPROM.commit();
}

/* Attempt to load a previous destination IP from EEPROM */
bool load_dest() {
  bool success;
  EEPROM.get(EEPROM_DEST_IP_ADDR, dest);
  success = strcmp(dest.valid, "xyz123") == 0;
  return success;
}

/* Connect UDP/TCP clients to destination address and port */
void connect_dest(const char *addr, uint16_t port) {
  udp_client.connect(addr, port); 
  OSCMessage response = make_pong();
  udp_client.send(response);
  tcp_client.connect(addr, port);
}

// UDP Data Handler
// ================
/* Pass raw data to OSC manager for decoding/dispatching */
void udp_handle_data(uint8_t *data, size_t len, void *userdata) {
  osc.handle_buffer(data, len);
  wifi_led.blink();
}

// TCP Handlers:
// =============
/* Data handler; pass raw data to OSC manager for decoding/dispatching */
void tcp_handle_data(uint8_t *data, size_t len, void *userdata) {
  osc.handle_buffer(data, len);
  wifi_led.blink();
}

/* Connection handler */
void tcp_handle_connect(void *userdata) {
  OSCMessage response = make_pong();
  tcp_client.send(response);
  wifi_led.blink();
}

// OSC Output:
// ===========
/* Send TCP if connected; fall back on UDP */
void osc_send(OSCMessage &msg) {
  if (tcp_client.connected()) 
      tcp_client.send(msg);
  else
      udp_client.send(msg, udp_client.get_remote_addr());
  wifi_led.blink();
}

/* Make the '/ping' response message */
OSCMessage make_pong() {
  char buff[32];
  OSCMessage response("/pong");
  wifi.get_dev_id(buff);
  response.add(buff);
  wifi.get_node_id(buff);
  response.add(atoi(buff));
  response.add(wifi.get_local_address().toString().c_str());
  return response;
}

// OSC Message Handlers:
// =====================
/* 
 * /ping
 *  
 * Connect TCP/UDP clients to remote IP. Wait for TCP connection to send /pong
 */
void osc_handle_ping(OSCMessage &msg) {  
  save_dest(udp_client.get_remote_addr().toString().c_str(), wifi.get_iot_port());
  connect_dest(udp_client.get_remote_addr().toString().c_str(), wifi.get_iot_port());
}

/*
 * /config
 * 
 * Disconnect WiFI and open access point
 */
void osc_handle_config(OSCMessage &msg) {
  wifi.open_access_point();
}
