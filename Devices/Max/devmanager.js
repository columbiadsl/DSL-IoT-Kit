/* ------------------------------------------------------------------------
 * devmanager.js
 *
 * Interface for managing communication with custom IoT devices.
 * 
 * -------- Jeff Gregorio, 2019 -------------------------------------------
 */

inlets = 1;
outlets = 2;

var use_tcp = false;

var dev_dict = {};

var addr_broadcast = '255.255.255.255';
var iotport = 7771;
var pyport = 9000;			// UDP port number of UDP->TCP bridge (Python script)
if (jsarguments.length > 1)
	iotport = Number(jsarguments[1]);
if (jsarguments.length > 2) {
	pyport = Number(jsarguments[2]);
	use_tcp = true;
}

// Put all IoT devices on local network into configuration mode (acess portal)
function config() {
	outlet(0, 'host', addr_broadcast);
	outlet(0, 'port', iotport);
	outlet(0, '/config');
}

// Main message handler
function anything() {
	// Treat any string start with a slash as an OSC message
	if (messagename.charAt(0) == '/') 
		incoming_osc(messagename, arguments);
	else if (dev_dict.hasOwnProperty(messagename)) {
		osc_to_device(messagename, arguments);
	}
}

// OSC handler for messages from IoT devices
function incoming_osc(oscpath, args) {

	// Handle creation of new devices on response to /marco
	if (oscpath == '/pong' && args.length == 3) {
		
		var devID = args[0];
		var nodeID = args[1];
		var nodeAddr = args[2];

		// Create main dict entry for this device type if not seen yet
		if (!dev_dict.hasOwnProperty(devID)) 
			dev_dict[devID] = {};

		// Create a sub-dict entry for this node ID
		if (!dev_dict[devID].hasOwnProperty(nodeID)) {
			
			dev_dict[devID][nodeID] = nodeAddr;

			post("================================\n");
			post('New device ID \'' + devID + ' ' + nodeID + '\'\n');
			post('at address ' + nodeAddr + '\n');
			post("================================\n");
		}
	}
}

function osc_to_device(devID, args) {

	if (arguments.length < 2) 
		return;

	// Get destination (specified nodeID or 0 for broadcast)
	var destaddr;
	var nodeID = args[0];
	if (nodeID == 0)
		destaddr = addr_broadcast;
	else
		destaddr = dev_dict[devID][nodeID];

	// Get OSC paths and arguments
	var oscpath = args[1];
	var outargs = [];
	for (var i = 2; i < args.length; i++) 
		outargs.push(args[i]);

	// Send using TCP or UDP
	if (use_tcp) 
		send_tcp(destaddr, oscpath, outargs);
	else
		send_udp(destaddr, oscpath, outargs);
}

function devices() {
	post("Devices:\n");
	for (var dev_id in dev_dict) {
		if (dev_dict.hasOwnProperty(dev_id)) {
			for (var node_id in dev_dict[dev_id]) {
				if (dev_dict[dev_id].hasOwnProperty(node_id)) {
					post(dev_id + ' ' + node_id + '\n');
				}
			}
		}
	}
}

// Send device query (OSC)
function ping() {
	send_udp(addr_broadcast, '/ping');
}

// Send UDP
function send_udp(addr, path, args) {
	outlet(0, 'host', addr);
	outlet(0, 'port', iotport);
	outlet(0, path, args)
}

// Send TCP (actually sent to udp-tcp-bridge.py via UDP, which relays the
// message to the IoT device via reliable TCP connection)
function send_tcp(addr, path, args) {
	var toks = addr.split('.');
	if (toks[toks.length-1] == '255') {
		for (var dev_id in dev_dict) {
    		if (dev_dict.hasOwnProperty(dev_id)) {
	          	for (var node_id in dev_dict[dev_id]) {
					if (dev_dict[dev_id].hasOwnProperty(node_id))
						send_tcp(dev_dict[dev_id][node_id], path, args);
				}
			}
		}
	}
	else {
		outlet(0, 'host', '127.0.0.1');
		outlet(0, 'port', pyport);
		outlet(0, '/tcp', addr, iotport, path, args)
	}
}
