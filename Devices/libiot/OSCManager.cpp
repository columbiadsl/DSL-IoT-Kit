/* OSCManager.cpp

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

#include "OSCManager.h"
#include "Arduino.h"

static int pad_len(int bytes) { 
	return (4 - (bytes & 03)) & 3; 
}

// ============================================================================
OSCManager::OSCManager() {
	OSCManager(NULL);
}

OSCManager::OSCManager(Stream *debug_serial) : debug_serial(debug_serial), 
local_port(NULL), dest_port(NULL), dest_address(NULL), num_handlers(0) {
	
}

OSCManager::~OSCManager() {

}

bool OSCManager::open_port(uint16_t port) {
	local_port = dest_port = port;
	if (udp_local.begin(local_port) == 1) {
		if (debug_serial)
			debug_serial->printf("Listening for OSC on port %d\n", local_port);
		return true;
	}
	return false;
}

void OSCManager::set_dest(IPAddress addr, uint16_t port) {
	dest_address = addr;
	dest_port = port;
}

void OSCManager::dispatch(char *path, void (*handler)(OSCMessage &)) {
	strcpy(paths[num_handlers], path);
	handlers[num_handlers] = handler;
	num_handlers++;
}

bool OSCManager::loop() {

	bool success = false;
	int n_bytes = udp_local.parsePacket();

	if (n_bytes) {
		char *data = (char *)malloc(n_bytes * sizeof(char));
		udp_local.read(data, n_bytes);		

		// Debug printing
		print_udp("Data from UDP client", 
			udp_local.remoteIP().toString().c_str(), 
			udp_local.remotePort());

		success = handle_buffer((uint8_t *)data, n_bytes);
		free(data);
	}
	return success;
}

void OSCManager::send(OSCMessage &msg) {
	send(msg, dest_address);
}

void OSCManager::send(OSCMessage &msg, IPAddress dest) {
	if (!dest_port)
		return;

	// Debug printing
	print_udp("Sending UDP to client", 
		dest.toString().c_str(), 
		dest_port);
	print_osc_msg("OSC Message", msg);

	udp_local.beginPacket(dest, dest_port);
	msg.send(udp_local);
	udp_local.endPacket();
}

bool OSCManager::handle_message(OSCMessage &msg) {
	
	if (!msg.hasError())  { 

		// Debug printing
		print_osc_msg("OSC Message", msg);

		// Dispatch to handler
		for (int i = 0; i < num_handlers; i++) {
			if (msg.dispatch(paths[i], handlers[i]))
				break; 
		}
	}
	else {
		OSCErrorCode error = msg.getError();
		return false;
	}
	return true;
}

bool OSCManager::handle_buffer(uint8_t *bytes, size_t len) {
	OSCMessage msg; 
	msg.fill(bytes, len);
	return handle_message(msg);	
}

// Print utilities:
// ============================================================================
void OSCManager::print_udp(char *description, const char *addr, uint16_t port) {
	if (debug_serial) 
		debug_serial->printf("\n%24s: %s:%d", description, addr, port);
}

void OSCManager::print_osc_msg(char *description, OSCMessage &msg) {
	if (debug_serial) {
		char oscpath[OSC_MAX_PATH_LENGTH];
		msg.getAddress(oscpath);
		debug_serial->printf("\n%24s: %s\n", description, oscpath);
	}
}






