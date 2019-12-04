/* UDPClient.cpp

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

#include "UDPClient.h"
#include "Arduino.h"

// Public:
// ============================================================================
UDPClient::UDPClient() {
	UDPClient(NULL);
}

UDPClient::UDPClient(Stream *debug_serial) : 
local_port(0), remote_port(0), data_handler(NULL), user_data(NULL), 
debug_serial(debug_serial) {
	remote_addr = IPAddress();
}

UDPClient::~UDPClient() {
	udp_local.stop();
}

bool UDPClient::open_port(uint16_t port) {
	local_port = remote_port = port;
	if (debug_serial)
		debug_serial->printf("Opening UDP Port %d\n", local_port);
	return udp_local.begin(local_port) == 1;
}

void UDPClient::connect(IPAddress addr, uint16_t port) {
	remote_addr = addr;
	remote_port = port;
}

void UDPClient::connect(const char *addr, uint16_t port) {
	remote_addr.fromString(addr);
	remote_port = port;
}

void UDPClient::send(OSCMessage &msg) {
	send(msg, remote_addr, remote_port);
}

void UDPClient::send(OSCMessage &msg, IPAddress dest) {
	send(msg, dest, remote_port);
}

void UDPClient::send(OSCMessage &msg, IPAddress dest, uint16_t port) {
	print_udp("OSC to UDP Client", dest.toString().c_str(), port);
	udp_local.beginPacket(dest, port);
	msg.send(udp_local);
	udp_local.endPacket();
}

void UDPClient::send(char *data, size_t len) {
	send(data, len, remote_addr);
}

void UDPClient::send(char *data, size_t len, IPAddress dest) {
	send(data, len, dest, remote_port);
}

void UDPClient::send(char *data, size_t len, IPAddress dest, uint16_t port) {
	print_udp("Data to UDP Client", dest.toString().c_str(), port);
	print_udp_data("Data", data, len);
	udp_local.beginPacket(dest, port);
	udp_local.write((uint8_t *)data, len);
	udp_local.endPacket();
}

bool UDPClient::loop() {

	bool success = false;
	int n_bytes = udp_local.parsePacket();

	if (n_bytes) {

		char *data = (char *)malloc(n_bytes * sizeof(char));
		udp_local.read(data, n_bytes);

		print_udp("Data from UDP Client", 
			udp_local.remoteIP().toString().c_str(), 
			udp_local.remotePort());
		print_udp_data("Data", data, n_bytes);
		
		if (data_handler)
			data_handler((uint8_t *)data, n_bytes, user_data);
		free(data);
	}
	return success;
}

// Print utilities:
// ============================================================================
void UDPClient::print_udp(char *description, const char *addr, uint16_t port) {
	if (debug_serial) 
		debug_serial->printf("\n%24s: %s:%d", description, addr, port);
}

void UDPClient::print_udp_data(char *description, char *data, size_t len) {
	if (debug_serial)
		debug_serial->printf("\n%24s: %s\n", description, data);
}
