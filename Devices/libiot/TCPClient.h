/* TCPClient.h

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

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <ESPAsyncTCP.h>
#include <OSCMessage.h>
#include "Print.h"
#include "Arduino.h"

class TCPClient : public Print {

public:

	TCPClient();
	TCPClient(Stream *debug_serial);
	~TCPClient();

	void set_data_handler(void (*handler)(uint8_t *, size_t, void *), void *user_data) {
		data_handler = handler;
		this->user_data_d = user_data;
	}

	void set_connect_handler(void (*handler)(void *), void *user_data) {
		connect_handler = handler;
		this->user_data_c = user_data;
	}

	void connect(const char *address, uint16_t port_num);

	void send(OSCMessage &msg);
	void send(char *data, size_t len);

	void disconnect();
	void stop();

	// Getters
	bool connected() 			{ return client->connected(); 	}
	IPAddress remote_addr() 	{ return client->remoteIP();	}
	uint16_t remote_port()		{ return client->remotePort();	}

	// Print overrides (so OSCMessages can print directly to a TCPClient)
	virtual size_t write(uint8_t);
	virtual size_t write(const char *str);
	virtual size_t write(uint8_t *buffer, size_t size);
	virtual size_t write(const uint8_t *buffer, size_t size);
	using Print::write;

	// TCP event handlers; should not be called externally, but must be public
	// for accessibility from static event handlers 
	void handle_connect(AsyncClient *client);
	void handle_data(AsyncClient *client, void *data, size_t len);
	void handle_error(AsyncClient *client, int8_t error);
	void handle_disconnect(AsyncClient *client);
	void handle_timeout(AsyncClient *client, uint32_t time);

protected:

	// Print utilities
	void print_tcp(char *description, const char *addr, uint16_t port);
	void print_tcp_data(char *description, char *data, size_t len);

	AsyncClient *client;
	Stream *debug_serial;

	void (*connect_handler)(void *);
	void *user_data_c;

	void (*data_handler)(uint8_t *, size_t, void *);
	void *user_data_d;
};

#endif