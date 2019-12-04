/* UDPClient.h

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

#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <WiFiUdp.h>
#include <OSCMessage.h>

class UDPClient {

public:

	UDPClient();
	UDPClient(Stream *debug_serial);
	~UDPClient();

	// Set handler callback function for incoming data
	void set_data_handler(void (*handler)(uint8_t *, size_t, void *), void *userdata) {
		data_handler = handler;
		this->user_data = userdata;
	}

	// Start listening for incoming messages
	bool open_port(uint16_t port);

	// Set default destination for outgoing messages
	void connect(IPAddress addr, uint16_t port);
	void connect(const char *addr, uint16_t port);

	// OSC Message senders
	void send(OSCMessage &msg);
	void send(OSCMessage &msg, IPAddress dest);
	void send(OSCMessage &msg, IPAddress dest, uint16_t port);

	// Raw data senders
	void send(char *data, size_t len);
	void send(char *data, size_t len, IPAddress dest);
	void send(char *data, size_t len, IPAddress dest, uint16_t port);

	// Loop (polls UDP port for incoming data) 
	bool loop();

	// Getters
	bool connected() 			{ return remote_addr.isSet(); 	 }
	IPAddress get_remote_addr() { return udp_local.remoteIP();   }
	uint16_t get_remote_port()  { return udp_local.remotePort(); }

protected:

	// Print utilities
	void print_udp(char *description, const char *addr, uint16_t port);
	void print_udp_data(char *description, char *data, size_t len);

	WiFiUDP udp_local;
	uint16_t local_port;

	IPAddress remote_addr;
    uint16_t remote_port;

	void (*data_handler)(uint8_t *, size_t, void *);
	void *user_data;

	Stream *debug_serial;
};

#endif