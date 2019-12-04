# IoT Devices

This directory contains C/C++ code designed to run on the NodeMCU microcontroller. The code contained in `libiot` simplifies the design of IoT-based sensors by handling connection to WiFi networks and Open Sound Control (OSC) messages sent over UDP or TCP connections. Devices using this library will store wifi credentials, and automatically connect to their most recent network when powered on. If this fails they open an access point with a captive configuration portal where you can enter wifi credentials, device identifiers, and UDP/TCP port numbers.

The example `gate` can be used with a hall effect sensor to detect the presence of a magnetic field, which can be used to sense the opening/closing of doors, boxes, and retrieval of objects placed on top of the sensor when magnets are embedded in the objects. 

A Javascript file `devmanager.js` is designed to be used in Max/MSP with a `[js]` block, which manages connections to the IoT devices. Two Max patches `devmanager_udp.maxpat` and `devmanager_tcp.maxpat` demonstrate its use.

*Note: these devices transmit and store wifi credentials on the device without encryption, so do not use these devices on networks that require any measure of security. It is preferable to use a dedicated router with no internet connection.*

## Arduino Library Installation

To install, move the `libiot` directory to the Arduino libraries directory (`~/Documents/Arduino/libraries`). You will then be able to access the example IoT device within the Arduino IDE by going to `File->Examples->libiot->gate`.

### Dependencies

#### esp8266 boards package

1. Open Arduino Preferences
1. Under 'Additional Boards Manager URLs', enter the following URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
1. Open the Boards Manager under Tools->Board->Boards Manager
1. Search 'esp8266' and install

#### [esp8266 USB Driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)

Note: OS X will block the installation of the driver, which you can enable (after begining the driver installation) in System Preferences->Security & Privacy.

#### Libraries 
* [esp8266-OSC](https://github.com/sandeepmistry/esp8266-OSC)
* [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)

## Example Project

The `gate` example monitors the state of pin digital pin D1 on the NodeMCU, and sends `/gate <node_id> 1` when the pin goes high, and `/gate <node_id> 0` when the pin goes low, where `<node_id>` is the device's configured node ID number, allowing multiple `/gate` messages to be disambiguated.

The following image shows how to connect an [A3144](https://www.amazon.com/A3144E-OH3144E-Effect-Sensor-Three-pin/dp/B01M2WASFL) or similar Hall effect sensor to pin D1 with a 10k pullup resistor. This will sense the presence of a magnet when the field is oriented correctly.

![HallGate](/images/hall_gate.jpg)

### Device Configuration

The node ID number, along with a device ID can be set on the device's configuration portal. To access the portal, program the device and wait for it to attempt to connect to a wifi network (during which the onboard LED will blink slowly). It will fail to connect, as it initially has no wifi credentials. When the LED turns off, check your list of networks for `ap-device-1`, and connect to it with the password `iotconfig`. 

You will then be prompted with a captive portal, where you may enter the name of a wifi network and password, as well as a device identifier, node identifier, and UDP/TCP port number to use. 

*Note: on some machines, the captive portal does not open or disappears quickly, so you may need to attempt to use different devices (including mobile phones).*

## Max/MSP Examples

The examples `devmanager_udp.maxpat` and `devmanager_tcp.maxpat` demonstrate connections needed to interface the `[js devmanager.js]` object with the associated IoT devices. 

### Using UDP Only

The example `devmanager_udp.maxpat` uses Max's `[udpsend]` and `[udpreceive]` objects to send and receive OSC messages directly to and from the IoT devices. To use, make sure the port numbers provided to `[js devmanager.js]` and `[udpsend]` are the same as the port configured on the device. 

Send the `ping` message to `[js devmanager.js]` and check the Max console to verify receipt of the corresponding `/pong` OSC message from the device, which contains its IP address and identifiers. `[js devmanager.js]` should print a statement showing that the device and node IDs and IP address have been added to the list of devices.

The list of devices can be queried from `[js devmanager.js]` by sending it the message `devices`, and any OSC messages can be sent to the devices when they are preceded by the device and node IDs. For example, the `device 1 /test` message in the example patch sends the OSC message `/test` to any device with the device ID `device` and node ID `1`.

*Note: sending the `/config` message to any device causes it to disconnect from the wifi network, and opens the configuration portal.*

### Using UDP and TCP

If `[js devmanager.js]` is provided with a second port number, this second (UDP) port will be used to communicate with a python script `tcp.py` which manages TCP connections with the IoT devices, which guarantees arrival of OSC messages as long as the connection is maintained, unlike UDP, which may drop messages.

#### Python Dependencies

The `tcp.py` script has been designed and tested with Python 3.7.3, and uses [python-osc](https://pypi.org/project/python-osc/), which can be installed using pip with `pip install python-osc`.

#### Running the Example

* Open `devmanager_tcp.maxpat`, and run the `tcp.py` script from the command line with the same two port numbers provided to `[js devmanager.js]`.
* Power on one of the IoT devices. If it has previously received a `/ping` message from the device manager, and no IP addresses or port numbers have changed, it will initiate the TCP connection automatically. If not, sending a ping to the device will cause it to initiate the connection.
	* You should see `-- Routing OSC Message: (TCP) 10.0.1.19:56640 --> (UDP) localhost:7770` when the IoT device connects.
	* You should see the similar printouts any time an OSC message is sent to or from the IoT device.
