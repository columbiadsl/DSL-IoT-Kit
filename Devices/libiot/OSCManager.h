/* OSCManager.h

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

#ifndef OSCMANAGER_H
#define OSCMANAGER_H

#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <stdarg.h>
#include "Arduino.h"

#ifndef OSC_MAX_NUM_HANDLERS
#define OSC_MAX_NUM_HANDLERS 32
#endif
#ifndef OSC_MAX_PATH_LENGTH
#define OSC_MAX_PATH_LENGTH 64
#endif

class OSCManager {

public:

    // Constructor/destructor
    OSCManager();
    OSCManager(Stream *debug_serial);
    ~OSCManager();

    // Start listening on the specified port
    bool open_port(uint16_t port);

    // Set a default destination for outgoing messages
    void set_dest(IPAddress addr, uint16_t port);
    
    // Set OSC handlers for the specified path
    void dispatch(char *path, void (*handler)(OSCMessage &));

    // OSC Message senders
    void send(OSCMessage &msg);                     // OSC --> default dest
    void send(OSCMessage &msg, IPAddress dest);     // OSC --> specified dest

    // Loop 
    bool loop(); 

    // OSC
    bool handle_message(OSCMessage &msg);
    bool handle_buffer(uint8_t *bytes, size_t len);

    // Established via UDP only (should be a /ping)
    IPAddress remote_addr() { return udp_local.remoteIP(); }
    uint16_t remote_port() { return udp_local.remotePort(); }

protected:

    // Print utilities
    void print_udp(char *description, const char *addr, uint16_t port);
    void print_osc_msg(char *description, OSCMessage &msg);

    Stream *debug_serial;

    WiFiUDP udp_local;

    uint16_t local_port;
    uint16_t dest_port;
    IPAddress dest_address;    

    int num_handlers;
    char paths[OSC_MAX_NUM_HANDLERS][OSC_MAX_PATH_LENGTH];
    void (*handlers[OSC_MAX_NUM_HANDLERS])(OSCMessage &);
};

#endif
