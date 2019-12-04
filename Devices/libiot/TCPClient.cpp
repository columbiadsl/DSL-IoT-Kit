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

#include "TCPClient.h"
#include <Print.h>

// Static event handlers:
// ============================================================================
static void _s_tcpc_handle_connect(void *arg, AsyncClient *client) {
	TCPClient *client_instance = (TCPClient *)arg;
	client_instance->handle_connect(client);
}

static void _s_tcpc_handle_data(void *arg, AsyncClient *client, void *data, size_t len) {
	TCPClient *client_instance = (TCPClient *)arg;
	client_instance->handle_data(client, data, len);
}

static void _s_tcpc_handle_disconnect(void *arg, AsyncClient *client) {
	TCPClient *client_instance = (TCPClient *)arg;
	client_instance->handle_disconnect(client);
}

static void _s_tcpc_handle_error(void *arg, AsyncClient *client, int8_t error) {
	TCPClient *client_instance = (TCPClient *)arg;
	client_instance->handle_error(client, error);
}

static void _s_tcpc_handle_timeout(void *arg, AsyncClient *client, uint32_t time) {
	TCPClient *client_instance = (TCPClient *)arg;
	client_instance->handle_timeout(client, time);
}

// Print utilities:
// ============================================================================
void TCPClient::print_tcp(char *description, const char *addr, uint16_t port) {
	if (debug_serial)
		debug_serial->printf("\n%24s: %s:%d", description, addr, port);
}

void TCPClient::print_tcp_data(char *description, char *data, size_t len) {
	if (debug_serial)
		debug_serial->printf("\n%24s: %s\n", description, data);
}

// Public:
// ============================================================================
TCPClient::TCPClient() {
	TCPClient(NULL);
}

TCPClient::TCPClient(Stream *debug_serial) : 
debug_serial(debug_serial), 
data_handler(NULL), user_data_d(NULL),
connect_handler(NULL), user_data_c(NULL) {
	client = new AsyncClient;
	client->onConnect(&_s_tcpc_handle_connect, (void *)this);
	client->onData(&_s_tcpc_handle_data, (void *)this);
	client->onDisconnect(&_s_tcpc_handle_disconnect, (void *)this);
	client->onError(&_s_tcpc_handle_error, (void *)this);
	client->onTimeout(&_s_tcpc_handle_timeout, (void *)this);
}

TCPClient::~TCPClient() {
	if (client) {
		disconnect();
		delete(client);
	}
}

void TCPClient::connect(const char *addr, uint16_t port) {
	if (client->connected())
		client->close(true);
	print_tcp("Connecting to", addr, port);
	client->connect(addr, port);
}

void TCPClient::send(OSCMessage &msg) {
	if (client->space() > msg.bytes()) {
		print_tcp("OSC to TCP client", 
			(char *)client->remoteIP().toString().c_str(), 
			client->remotePort());
		msg.send(*this);
		client->send();
	}
}

void TCPClient::send(char *data, size_t len) {
	if (client->space() > len) {
		print_tcp("Data to TCP client", 
			(char *)client->remoteIP().toString().c_str(), 
			client->remotePort());
		print_tcp_data("Data", data, len);
		client->add(data, len);
		client->send();
	}
	else 
		print_tcp_data("Failed to send data", data, len);
}

void TCPClient::disconnect() {
	if (client->connected())
		client->close(true);
}

void TCPClient::stop() {
	if (client->connected())
		client->stop();
}

// Print overrides so we can send by passing this instance to OSCMessage.send()
// ============================================================================
inline size_t TCPClient::write(uint8_t byte) { 
	client->add((char *)(&byte), (size_t)1);
	return 1;
}

size_t TCPClient::write(const char *str) {
	client->add(str, strlen(str));
	return strlen(str);
}

size_t TCPClient::write(uint8_t *buffer, size_t size) {
	client->add((char *)buffer, size);
	return size;
}

size_t TCPClient::write(const uint8_t *buffer, size_t size) { 
	client->add((const char *)buffer, size);
	return size;
}

// Instance event handlers:
// ============================================================================
void TCPClient::handle_connect(AsyncClient *client) {
	print_tcp("Connected to", 
		(char *)client->remoteIP().toString().c_str(), 
		client->remotePort());
	if (connect_handler)
		connect_handler(user_data_c);
}
 
void TCPClient::handle_data(AsyncClient *client, void *data, size_t len) {
	print_tcp("Data from TCP client", 
		(char *)client->remoteIP().toString().c_str(), 
		client->remotePort());
	print_tcp_data("Data", (char *)data, len);
	if (data_handler)
		data_handler((uint8_t *)data, len, user_data_d);
}

void TCPClient::handle_error(AsyncClient *client, int8_t error) {
	print_tcp("Connection error", 
		(char *)client->remoteIP().toString().c_str(), 
		client->remotePort());
}

void TCPClient::handle_disconnect(AsyncClient *client) {
	print_tcp("Disconnected", 
		(char *)client->remoteIP().toString().c_str(), 
		client->remotePort());
}

void TCPClient::handle_timeout(AsyncClient *client, uint32_t time) {
	print_tcp("ACK timeout", 
		(char *)client->remoteIP().toString().c_str(), 
		client->remotePort());
}